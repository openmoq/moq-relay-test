#!/usr/bin/env python3
"""
moq-version-check: MoQT (Media over QUIC Transport) Server Checker

Probes a given endpoint to determine if it is a MoQT server and which
protocol versions it supports. Tests both raw QUIC (over UDP) and
WebTransport (over HTTP/3) transports.

Based on: draft-ietf-moq-transport-16
Reference: https://datatracker.ietf.org/doc/draft-ietf-moq-transport/16/
"""

import argparse
import asyncio
import logging
import ssl
import struct
import sys
import time
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Dict, List, Optional, Tuple
from urllib.parse import urlparse

try:
    from aioquic.asyncio.client import connect
    from aioquic.asyncio.protocol import QuicConnectionProtocol
    from aioquic.quic.configuration import QuicConfiguration
    from aioquic.quic.events import (
        ConnectionTerminated,
        HandshakeCompleted,
        StreamDataReceived,
        StreamReset,
    )

    AIOQUIC_AVAILABLE = True
except ImportError:
    AIOQUIC_AVAILABLE = False

try:
    from aioquic.h3.connection import H3_ALPN, H3Connection
    from aioquic.h3.events import HeadersReceived

    H3_AVAILABLE = True
except ImportError:
    H3_AVAILABLE = False


# ============================================================================
# Constants
# ============================================================================

# MoQT ALPN values to probe (draft-15+ use per-draft ALPNs)
MOQT_ALPNS_NEW = ["moqt"] + [f"moqt-{v}" for v in range(16, 5, -1)]
# Legacy ALPN used by all drafts prior to -15
MOQT_ALPN_LEGACY = "moq-00"

# WebTransport protocol identifiers for WT-Available-Protocols header.
# Servers may use short (e.g. "moqt-16") or full (e.g. "moqt-16-meta-00")
# protocol identifiers.  We try the short ALPN name first; if the server
# doesn't negotiate, the orchestrator can try known alternatives.
WT_PROTOCOL_ALTERNATIVES = {
    "moqt-16": ["moqt-16", "moqt-16-meta-00"],
    "moqt-15": ["moqt-15", "moqt-15-meta-05"],
    "moq-00": ["moq-00"],
}

# Message types (draft-15+ format)
MSG_CLIENT_SETUP = 0x20
MSG_SERVER_SETUP = 0x21

# Legacy message types (drafts 2-14)
MSG_CLIENT_SETUP_OLD = 0x40
MSG_SERVER_SETUP_OLD = 0x41

# Setup parameter types (shared namespace across all MOQT versions)
PARAM_PATH = 0x01
PARAM_MAX_REQUEST_ID = 0x02
PARAM_AUTH_TOKEN = 0x03
PARAM_MAX_AUTH_TOKEN_CACHE_SIZE = 0x04
PARAM_AUTHORITY = 0x05
PARAM_MOQT_IMPLEMENTATION = 0x07

# Legacy parameter (pre-draft-15)
PARAM_ROLE = 0x00

# Default port for moqt:// URIs
DEFAULT_MOQT_PORT = 443

# Timeouts (seconds)
DEFAULT_TIMEOUT = 10.0

# Human-readable setup parameter names
SETUP_PARAM_NAMES = {
    0x00: "ROLE",
    0x01: "PATH",
    0x02: "MAX_REQUEST_ID",
    0x03: "AUTHORIZATION_TOKEN",
    0x04: "MAX_AUTH_TOKEN_CACHE_SIZE",
    0x05: "AUTHORITY",
    0x07: "MOQT_IMPLEMENTATION",
}

# Session termination error code names
SESSION_ERROR_NAMES = {
    0x00: "NO_ERROR",
    0x01: "INTERNAL_ERROR",
    0x02: "UNAUTHORIZED",
    0x03: "PROTOCOL_VIOLATION",
    0x04: "INVALID_REQUEST_ID",
    0x05: "DUPLICATE_TRACK_ALIAS",
    0x06: "KEY_VALUE_FORMATTING_ERROR",
    0x07: "TOO_MANY_REQUESTS",
    0x08: "INVALID_PATH",
    0x09: "MALFORMED_PATH",
    0x10: "GOAWAY_TIMEOUT",
    0x11: "CONTROL_MESSAGE_TIMEOUT",
    0x12: "DATA_STREAM_TIMEOUT",
    0x13: "AUTH_TOKEN_CACHE_OVERFLOW",
    0x14: "DUPLICATE_AUTH_TOKEN_ALIAS",
    0x15: "VERSION_NEGOTIATION_FAILED",
    0x16: "MALFORMED_AUTH_TOKEN",
    0x17: "UNKNOWN_AUTH_TOKEN_ALIAS",
    0x18: "EXPIRED_AUTH_TOKEN",
    0x19: "INVALID_AUTHORITY",
    0x1A: "MALFORMED_AUTHORITY",
}


# ============================================================================
# QUIC Variable-Length Integer Encoding (RFC 9000, Section 16)
# ============================================================================


def encode_varint(value: int) -> bytes:
    """Encode an integer as a QUIC variable-length integer."""
    if value < 0:
        raise ValueError("Varint value must be non-negative")
    if value < 0x40:
        return struct.pack("!B", value)
    elif value < 0x4000:
        return struct.pack("!H", 0x4000 | value)
    elif value < 0x40000000:
        return struct.pack("!I", 0x80000000 | value)
    elif value < 0x4000000000000000:
        return struct.pack("!Q", 0xC000000000000000 | value)
    else:
        raise ValueError(f"Value {value} too large for varint encoding")


def decode_varint(data: bytes, offset: int) -> Tuple[int, int]:
    """Decode a QUIC variable-length integer. Returns (value, new_offset)."""
    if offset >= len(data):
        raise ValueError("Insufficient data for varint")
    first_byte = data[offset]
    prefix = first_byte >> 6
    if prefix == 0:
        return first_byte, offset + 1
    elif prefix == 1:
        if offset + 2 > len(data):
            raise ValueError("Insufficient data for 2-byte varint")
        val = struct.unpack("!H", data[offset : offset + 2])[0] & 0x3FFF
        return val, offset + 2
    elif prefix == 2:
        if offset + 4 > len(data):
            raise ValueError("Insufficient data for 4-byte varint")
        val = struct.unpack("!I", data[offset : offset + 4])[0] & 0x3FFFFFFF
        return val, offset + 4
    else:
        if offset + 8 > len(data):
            raise ValueError("Insufficient data for 8-byte varint")
        val = struct.unpack("!Q", data[offset : offset + 8])[0] & 0x3FFFFFFFFFFFFFFF
        return val, offset + 8


# ============================================================================
# MoQT Message Building (draft-15+ new format)
# ============================================================================


def _build_setup_param(
    param_type: int, value: Any, prev_type: int,
    delta_encode: bool = True,
) -> Tuple[bytes, int]:
    """Build a single setup parameter as a Key-Value-Pair.

    When delta_encode=True (draft-16+): key is delta from prev_type.
    When delta_encode=False (draft-15): key is absolute.

    Even types: value is a varint (value must be int).
    Odd types:  length-prefixed byte value.
    Returns (encoded_bytes, current_type).
    """
    key = (param_type - prev_type) if delta_encode else param_type
    buf = bytearray()
    buf.extend(encode_varint(key))

    if param_type % 2 == 1:
        # Odd type: Length + Value bytes
        if isinstance(value, str):
            value = value.encode("utf-8")
        buf.extend(encode_varint(len(value)))
        buf.extend(value)
    else:
        # Even type: Value is a varint
        buf.extend(encode_varint(int(value)))

    return bytes(buf), param_type


def build_client_setup(
    path: str = "/",
    authority: Optional[str] = None,
    for_webtransport: bool = False,
    draft_version: int = 16,
) -> bytes:
    """Build a CLIENT_SETUP message (draft-15+ format, type 0x20).

    For raw QUIC: includes PATH and AUTHORITY parameters.
    For WebTransport: omits PATH and AUTHORITY (per spec Section 9.3.1).

    draft_version controls parameter encoding:
      - 16+: delta-encoded keys (moxygen draft-16 format)
      - 15:  absolute keys (same even/odd value format, no delta)

    Wire format:
        CLIENT_SETUP {
          Type (i) = 0x20,
          Length (16),
          Number of Parameters (i),
          Setup Parameters (..) ...,
        }
    """
    use_delta = draft_version >= 16
    params_data = bytearray()
    num_params = 0
    prev_type = 0

    # Parameters MUST be in ascending type order (for delta encoding,
    # and for consistency in non-delta mode).

    if not for_webtransport and path is not None:
        # PATH (0x01) — required for raw QUIC
        param_bytes, prev_type = _build_setup_param(
            PARAM_PATH, path, prev_type, delta_encode=use_delta
        )
        params_data.extend(param_bytes)
        num_params += 1

    # MAX_REQUEST_ID (0x02) — default 0
    param_bytes, prev_type = _build_setup_param(
        PARAM_MAX_REQUEST_ID, 0, prev_type, delta_encode=use_delta
    )
    params_data.extend(param_bytes)
    num_params += 1

    if not for_webtransport and authority:
        # AUTHORITY (0x05) — required for raw QUIC
        param_bytes, prev_type = _build_setup_param(
            PARAM_AUTHORITY, authority, prev_type, delta_encode=use_delta
        )
        params_data.extend(param_bytes)
        num_params += 1

    # Build payload: Number‑of‑Parameters + parameters
    payload = bytearray()
    payload.extend(encode_varint(num_params))
    payload.extend(params_data)

    # Complete message: Type(varint) + Length(16-bit BE) + Payload
    message = bytearray()
    message.extend(encode_varint(MSG_CLIENT_SETUP))
    message.extend(struct.pack("!H", len(payload)))
    message.extend(payload)

    return bytes(message)


def build_client_setup_for_wt(
    max_request_id: int = 100,
) -> bytes:
    """Build a CLIENT_SETUP for WebTransport (type 0x20 + version array).

    Matches the moxygen WT client behavior exactly:
    - Type 0x20 (CLIENT_SETUP) with 16-bit length
    - Version array with draft-14 (the only version negotiable
      in-band without ALPN on moxygen servers)
    - Parameters in draft-14 format: key + value_length + value

    Wire format:
        CLIENT_SETUP {
          Type (i) = 0x20,
          Length (16),
          Number of Supported Versions (i),
          Supported Versions (i) ...,
          Number of Parameters (i),
          Setup Parameters (..) ...,
        }
    """
    # Only offer draft-14: the sole version moxygen negotiates
    # without ALPN.  drafts >= 15 REQUIRE ALPN pre-negotiation;
    # drafts <= 13 are no longer in kSupportedVersions.
    versions = [0xFF00000E]  # draft-14

    payload = bytearray()

    # Number of versions + versions
    payload.extend(encode_varint(len(versions)))
    for v in versions:
        payload.extend(encode_varint(v))

    # Parameters (draft-14 format):
    #   Even keys: key(varint) + value(varint)  [NO value_length!]
    #   Odd keys:  key(varint) + length(varint) + value(bytes)
    # This matches moxygen's writeServerSetup / writeClientSetup.
    params = bytearray()
    num_params = 0

    # MAX_REQUEST_ID (0x02) — even key → key + varint value
    params.extend(encode_varint(PARAM_MAX_REQUEST_ID))
    params.extend(encode_varint(max_request_id))
    num_params += 1

    # MAX_AUTH_TOKEN_CACHE_SIZE (0x04) — even key → key + varint value
    params.extend(encode_varint(PARAM_MAX_AUTH_TOKEN_CACHE_SIZE))
    params.extend(encode_varint(1024))
    num_params += 1

    payload.extend(encode_varint(num_params))
    payload.extend(params)

    # Frame: Type 0x20 + 16-bit length + Payload
    message = bytearray()
    message.extend(encode_varint(MSG_CLIENT_SETUP))
    message.extend(struct.pack("!H", len(payload)))
    message.extend(payload)

    return bytes(message)


def build_client_setup_legacy(
    path: str = "/",
    max_request_id: int = 0,
) -> bytes:
    """Build a CLIENT_SETUP message for the moq-00 ALPN (type 0x20).

    The moxygen server's MoQControlCodec::parseFrame only handles
    FrameType::CLIENT_SETUP (0x20) — type 0x40 hits the default case
    and causes PROTOCOL_VIOLATION.  So even for moq-00 we use type
    0x20 with 16-bit length.

    When no ALPN version is pre-negotiated, the codec parses a
    version array from the CLIENT_SETUP payload.  Parameters use
    absolute keys with even→varint, odd→length+bytes encoding
    (no delta encoding — that's draft-16+ only).

    Wire format:
        CLIENT_SETUP {
          Type (i) = 0x20,
          Length (16),
          Number of Supported Versions (i),
          Supported Versions (i) ...,
          Number of Parameters (i),
          Setup Parameters (..) ...,
        }
    """
    # Offer draft-14 only — the sole version negotiable in-band
    # on moxygen (drafts >= 15 REQUIRE ALPN pre-negotiation).
    versions = [0xFF00000E]

    payload = bytearray()

    # Supported versions
    payload.extend(encode_varint(len(versions)))
    for v in versions:
        payload.extend(encode_varint(v))

    # Parameters: absolute key + even/odd value encoding
    params = bytearray()
    num_params = 0

    # PATH (0x01) — odd key → length + bytes
    path_bytes = path.encode("utf-8")
    params.extend(encode_varint(PARAM_PATH))
    params.extend(encode_varint(len(path_bytes)))
    params.extend(path_bytes)
    num_params += 1

    # MAX_REQUEST_ID (0x02) — even key → varint value
    params.extend(encode_varint(PARAM_MAX_REQUEST_ID))
    params.extend(encode_varint(max_request_id))
    num_params += 1

    payload.extend(encode_varint(num_params))
    payload.extend(params)

    # Frame: Type 0x20 + 16-bit length
    message = bytearray()
    message.extend(encode_varint(MSG_CLIENT_SETUP))
    message.extend(struct.pack("!H", len(payload)))
    message.extend(payload)

    return bytes(message)


# ============================================================================
# MoQT Message Parsing
# ============================================================================


def parse_server_setup(
    data: bytes, delta_decode: bool = True,
) -> Tuple[Optional[Dict], Optional[str]]:
    """Parse a SERVER_SETUP message (draft-15+ format, type 0x21).

    delta_decode=True (draft-16+): keys are delta-encoded.
    delta_decode=False (draft-15): keys are absolute.

    Returns (params_dict, error_string).
    """
    try:
        offset = 0
        msg_type, offset = decode_varint(data, offset)
        if msg_type != MSG_SERVER_SETUP:
            return None, f"Expected SERVER_SETUP (0x21), got 0x{msg_type:x}"

        if offset + 2 > len(data):
            return None, "Insufficient data for message length"
        length = struct.unpack("!H", data[offset : offset + 2])[0]
        offset += 2

        payload_end = offset + length
        if payload_end > len(data):
            return None, (
                f"Message length {length} exceeds available data "
                f"({len(data) - offset} bytes)"
            )

        num_params, offset = decode_varint(data, offset)

        params: Dict[int, Any] = {}
        prev_type = 0
        for _ in range(num_params):
            if offset >= payload_end:
                break
            raw_key, offset = decode_varint(data, offset)
            param_type = (prev_type + raw_key) if delta_decode else raw_key

            if param_type % 2 == 1:
                # Odd type: Length + Value bytes
                param_len, offset = decode_varint(data, offset)
                param_value = bytes(data[offset : offset + param_len])
                offset += param_len
            else:
                # Even type: Value is a varint
                param_value, offset = decode_varint(data, offset)

            params[param_type] = param_value
            prev_type = param_type

        return params, None
    except Exception as e:
        return None, f"Parse error: {e}"


def parse_server_setup_draft14(
    data: bytes,
) -> Tuple[Optional[Dict], Optional[str]]:
    """Parse a SERVER_SETUP message (type 0x21, 16-bit len, with version).

    Used when version was negotiated in-band (draft-14 format).
    The server writes selectedVersion before the params.
    Parameters use draft-14 encoding: key + value_length + value.

    Returns (result_dict, error_string).
    result_dict has 'version' (int) and 'params' (dict).
    """
    try:
        offset = 0
        msg_type, offset = decode_varint(data, offset)
        if msg_type != MSG_SERVER_SETUP:
            return None, f"Expected SERVER_SETUP (0x21), got 0x{msg_type:x}"

        if offset + 2 > len(data):
            return None, "Insufficient data for message length"
        length = struct.unpack("!H", data[offset : offset + 2])[0]
        offset += 2

        payload_end = offset + length
        if payload_end > len(data):
            return None, (
                f"Message length {length} exceeds available data "
                f"({len(data) - offset} bytes)"
            )

        # Selected Version (present for draft < 15)
        selected_version, offset = decode_varint(data, offset)

        # Parameters (draft-14 format):
        #   Even keys: key(varint) + value(varint)
        #   Odd keys:  key(varint) + length(varint) + value(bytes)
        num_params, offset = decode_varint(data, offset)
        params: Dict[int, Any] = {}
        for _ in range(num_params):
            if offset >= payload_end:
                break
            key, offset = decode_varint(data, offset)
            if key % 2 == 0:
                # Even key: value is a varint
                val, offset = decode_varint(data, offset)
                params[key] = val
            else:
                # Odd key: length-prefixed bytes
                val_len, offset = decode_varint(data, offset)
                val = bytes(data[offset : offset + val_len])
                offset += val_len
                params[key] = val

        return {"version": selected_version, "params": params}, None
    except Exception as e:
        return None, f"Parse error: {e}"


def parse_server_setup_legacy(
    data: bytes,
) -> Tuple[Optional[Dict], Optional[str]]:
    """Parse a SERVER_SETUP message (pre-draft-15 format, type 0x41).

    Returns (result_dict, error_string).
    result_dict has 'version' (int) and 'params' (dict).
    """
    try:
        offset = 0
        msg_type, offset = decode_varint(data, offset)
        if msg_type != MSG_SERVER_SETUP_OLD:
            return None, f"Expected old SERVER_SETUP (0x41), got 0x{msg_type:x}"

        length, offset = decode_varint(data, offset)
        payload_end = offset + length

        # Selected Version
        selected_version, offset = decode_varint(data, offset)

        # Parameters (old format: key + value_length + value)
        num_params, offset = decode_varint(data, offset)
        params: Dict[int, Any] = {}
        for _ in range(num_params):
            if offset >= payload_end:
                break
            key, offset = decode_varint(data, offset)
            val_len, offset = decode_varint(data, offset)
            val = bytes(data[offset : offset + val_len])
            offset += val_len
            params[key] = val

        return {"version": selected_version, "params": params}, None
    except Exception as e:
        return None, f"Parse error: {e}"


def _try_parse_response(data: bytes, is_legacy: bool) -> Tuple[bool, Any, str]:
    """Attempt to parse whatever the server sent back.

    Returns (complete, result_or_none, error_or_empty).
    complete=True if a full message was parsed (or identified as error).
    """
    if len(data) < 1:
        return False, None, ""

    try:
        offset = 0
        msg_type, offset = decode_varint(data, offset)
    except ValueError:
        return False, None, ""

    if msg_type == MSG_SERVER_SETUP:
        # Type 0x21 — need 16-bit length
        if offset + 2 > len(data):
            return False, None, ""
        length = struct.unpack("!H", data[offset : offset + 2])[0]
        if len(data) >= offset + 2 + length:
            if is_legacy:
                # draft-14 format: 0x21 + 16-bit len + version + kv params
                result, err = parse_server_setup_draft14(data)
            else:
                # draft-15+ format: 0x21 + 16-bit len + delta params
                result, err = parse_server_setup(data)
            return True, result, err or ""
        return False, None, ""
    elif msg_type == MSG_SERVER_SETUP_OLD:
        # Type 0x41 — need varint length
        try:
            length, off2 = decode_varint(data, offset)
            if len(data) >= off2 + length:
                result, err = parse_server_setup_legacy(data)
                return True, result, err or ""
        except ValueError:
            pass
        return False, None, ""
    else:
        expected = "0x41 or 0x21" if is_legacy else "0x21"
        return (
            True,
            None,
            f"Unexpected message type 0x{msg_type:x} (expected {expected})",
        )


# ============================================================================
# Probe Result Types
# ============================================================================


class ProbeStatus(Enum):
    SUCCESS = "success"
    TLS_FAILED = "tls_failed"
    CONNECTION_REFUSED = "connection_refused"
    TIMEOUT = "timeout"
    HANDSHAKE_ERROR = "handshake_error"
    ERROR = "error"
    NO_RESPONSE = "no_response"


@dataclass
class ProbeResult:
    transport: str  # "quic" or "webtransport"
    alpn: str
    status: ProbeStatus
    message: str = ""
    server_params: Optional[Dict] = None
    selected_version: Optional[int] = None  # for legacy probes
    duration_ms: float = 0.0


# ============================================================================
# Raw QUIC Probe
# ============================================================================


class MoQTQuicProtocol(QuicConnectionProtocol):
    """QUIC client protocol for MoQT probing over raw QUIC."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._setup_complete = asyncio.Event()
        self._response_data = bytearray()
        self._control_stream_id: Optional[int] = None
        self._error: Optional[str] = None
        self._is_legacy = False

    def quic_event_received(self, event):
        if isinstance(event, HandshakeCompleted):
            pass  # We'll open the control stream from the caller
        elif isinstance(event, StreamDataReceived):
            if event.stream_id == self._control_stream_id:
                self._response_data.extend(event.data)
                complete, _, _ = _try_parse_response(
                    bytes(self._response_data), self._is_legacy
                )
                if complete:
                    self._setup_complete.set()
        elif isinstance(event, StreamReset):
            if event.stream_id == self._control_stream_id:
                self._error = (
                    f"Control stream reset: error_code=0x{event.error_code:x}"
                )
                self._setup_complete.set()
        elif isinstance(event, ConnectionTerminated):
            err_name = SESSION_ERROR_NAMES.get(
                event.error_code, f"0x{event.error_code:x}"
            )
            reason = f", reason='{event.reason_phrase}'" if event.reason_phrase else ""
            self._error = f"Connection closed: {err_name}{reason}"
            self._setup_complete.set()


async def probe_quic_alpn(
    host: str,
    port: int,
    alpn: str,
    path: str,
    authority: str,
    verify_cert: bool,
    timeout: float,
) -> ProbeResult:
    """Probe a single ALPN over raw QUIC.

    1. Establish QUIC connection with the given ALPN.
    2. Open a bidirectional control stream.
    3. Send CLIENT_SETUP.
    4. Wait for SERVER_SETUP.
    """
    is_legacy = alpn == MOQT_ALPN_LEGACY
    # Extract draft version from ALPN for format selection:
    #   "moqt-16" → 16 (delta-encoded params)
    #   "moqt-15" → 15 (absolute-key params)
    #   "moq-00"  → 14 (legacy, in-band version negotiation)
    draft_version = 14  # default for moq-00
    if alpn.startswith("moqt-"):
        try:
            draft_version = int(alpn.split("-")[1])
        except (IndexError, ValueError):
            draft_version = 16
    start = time.monotonic()

    configuration = QuicConfiguration(is_client=True, alpn_protocols=[alpn])
    if not verify_cert:
        configuration.verify_mode = ssl.CERT_NONE

    try:
        async with asyncio.timeout(timeout):
            async with connect(
                host,
                port,
                configuration=configuration,
                create_protocol=MoQTQuicProtocol,
            ) as client:
                client: MoQTQuicProtocol
                client._is_legacy = is_legacy

                # Open a client-initiated bidirectional stream (= control stream)
                stream_id = client._quic.get_next_available_stream_id(
                    is_unidirectional=False
                )
                client._control_stream_id = stream_id

                # Build CLIENT_SETUP
                if is_legacy:
                    setup_msg = build_client_setup_legacy(path=path)
                else:
                    setup_msg = build_client_setup(
                        path=path, authority=authority,
                        draft_version=draft_version,
                    )

                client._quic.send_stream_data(stream_id, setup_msg)
                client.transmit()

                # Wait for SERVER_SETUP
                try:
                    await asyncio.wait_for(
                        client._setup_complete.wait(), timeout=timeout
                    )
                except asyncio.TimeoutError:
                    elapsed = (time.monotonic() - start) * 1000
                    return ProbeResult(
                        transport="quic",
                        alpn=alpn,
                        status=ProbeStatus.NO_RESPONSE,
                        message="ALPN accepted; timeout waiting for SERVER_SETUP",
                        duration_ms=elapsed,
                    )

                elapsed = (time.monotonic() - start) * 1000

                if client._error:
                    return ProbeResult(
                        transport="quic",
                        alpn=alpn,
                        status=ProbeStatus.HANDSHAKE_ERROR,
                        message=client._error,
                        duration_ms=elapsed,
                    )

                # Parse the response
                resp_data = bytes(client._response_data)
                if is_legacy:
                    # moq-00: server sends type 0x21 with selectedVersion
                    result, err = parse_server_setup_draft14(resp_data)
                    if err:
                        return ProbeResult(
                            transport="quic",
                            alpn=alpn,
                            status=ProbeStatus.HANDSHAKE_ERROR,
                            message=f"Invalid SERVER_SETUP: {err}",
                            duration_ms=elapsed,
                        )
                    return ProbeResult(
                        transport="quic",
                        alpn=alpn,
                        status=ProbeStatus.SUCCESS,
                        message="Legacy SERVER_SETUP received",
                        server_params=result.get("params", {}),
                        selected_version=result.get("version"),
                        duration_ms=elapsed,
                    )
                else:
                    use_delta = draft_version >= 16
                    params, err = parse_server_setup(
                        resp_data, delta_decode=use_delta,
                    )
                    if err:
                        return ProbeResult(
                            transport="quic",
                            alpn=alpn,
                            status=ProbeStatus.HANDSHAKE_ERROR,
                            message=f"Invalid SERVER_SETUP: {err}",
                            duration_ms=elapsed,
                        )
                    return ProbeResult(
                        transport="quic",
                        alpn=alpn,
                        status=ProbeStatus.SUCCESS,
                        message="SERVER_SETUP received",
                        server_params=params,
                        duration_ms=elapsed,
                    )

    except ConnectionRefusedError:
        elapsed = (time.monotonic() - start) * 1000
        return ProbeResult(
            transport="quic",
            alpn=alpn,
            status=ProbeStatus.CONNECTION_REFUSED,
            message="Connection refused (port not open or ICMP unreachable)",
            duration_ms=elapsed,
        )
    except asyncio.TimeoutError:
        elapsed = (time.monotonic() - start) * 1000
        return ProbeResult(
            transport="quic",
            alpn=alpn,
            status=ProbeStatus.TIMEOUT,
            message="Connection timeout (no QUIC response)",
            duration_ms=elapsed,
        )
    except OSError as e:
        elapsed = (time.monotonic() - start) * 1000
        return ProbeResult(
            transport="quic",
            alpn=alpn,
            status=ProbeStatus.ERROR,
            message=f"OS error: {e}",
            duration_ms=elapsed,
        )
    except Exception as e:
        elapsed = (time.monotonic() - start) * 1000
        error_str = str(e)
        # Classify TLS/ALPN failures
        lower = error_str.lower()
        if any(
            kw in lower
            for kw in [
                "no application protocol",
                "alert",
                "tls",
                "ssl",
                "handshake",
                "certificate",
                "no_application_protocol",
            ]
        ):
            status = ProbeStatus.TLS_FAILED
        else:
            status = ProbeStatus.ERROR
        return ProbeResult(
            transport="quic",
            alpn=alpn,
            status=status,
            message=error_str,
            duration_ms=elapsed,
        )


# ============================================================================
# WebTransport Probe (HTTP/3 + extended CONNECT)
# ============================================================================

# WebTransport error-code base (RFC 9297 §5)
_WT_ERROR_BASE = 0x52E4A40FA8DB


def _decode_wt_error(code: int) -> str:
    """Decode a QUIC/H3 error code into a human-readable form.

    If the code falls in the WebTransport application-error range,
    extract and display the inner application error.
    """
    if code >= _WT_ERROR_BASE and code < _WT_ERROR_BASE + (1 << 53):
        app_err = code - _WT_ERROR_BASE
        return (
            f"WebTransport app error 0x{app_err:x} "
            f"(raw 0x{code:x})"
        )
    return f"error_code=0x{code:x}"


def _log_wt_capsule(data: bytes) -> None:
    """Best-effort parse and log a WebTransport capsule."""
    if not data:
        return
    try:
        # Capsule format: Type (varint) | Length (varint) | Value
        cap_type, offset = decode_varint(data, 0)
        cap_len, offset2 = decode_varint(data, offset)
        payload = data[offset2: offset2 + cap_len]

        # 0x2843 = CLOSE_WEBTRANSPORT_SESSION (RFC 9297 §4.6)
        if cap_type == 0x2843:
            err_code = int.from_bytes(payload[:4], "big") if len(payload) >= 4 else None
            reason = payload[4:].decode(errors="replace") if len(payload) > 4 else ""
            logging.debug(
                "WT capsule CLOSE_WEBTRANSPORT_SESSION: "
                "error_code=%s reason=%r",
                f"0x{err_code:08x}" if err_code is not None else "?",
                reason,
            )
        # 0x78AE = DRAIN_WEBTRANSPORT_SESSION
        elif cap_type == 0x78AE:
            logging.debug("WT capsule DRAIN_WEBTRANSPORT_SESSION")
        else:
            logging.debug(
                "WT session capsule type=0x%x len=%d data=%s",
                cap_type, cap_len, payload.hex(),
            )
    except Exception:
        logging.debug(
            "Session capsule data (unparsed): %d bytes %s",
            len(data), data.hex(),
        )


class WebTransportProbeProtocol(QuicConnectionProtocol):
    """HTTP/3 client protocol for probing MoQT over WebTransport."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._h3: Optional[Any] = None
        self._h3_init_error: Optional[str] = None
        self._handshake_complete = asyncio.Event()
        self._setup_complete = asyncio.Event()
        self._wt_session_ready = asyncio.Event()
        self._response_data = bytearray()
        self._wt_bidi_stream_id: Optional[int] = None
        self._wt_session_id: Optional[int] = None
        self._error: Optional[str] = None
        self._http_status: Optional[int] = None
        self._wt_draft: Optional[str] = None  # e.g. "draft02"
        self._wt_negotiated_protocol: Optional[str] = None  # e.g. "moqt-16"
        self._wt_use_legacy = True  # WT uses legacy CLIENT_SETUP (0x40)

    def quic_event_received(self, event):
        # Log ALL events for diagnostics
        evt_name = type(event).__name__
        if hasattr(event, 'stream_id'):
            logging.debug(
                "QUIC event: %s stream=%d", evt_name,
                event.stream_id,
            )
        else:
            logging.debug("QUIC event: %s", evt_name)

        if isinstance(event, HandshakeCompleted):
            self._handshake_complete.set()
            # Log transport parameters for diagnostics
            try:
                rmsb = getattr(
                    self._quic, '_remote_max_streams_bidi', 'N/A'
                )
                rmsu = getattr(
                    self._quic, '_remote_max_streams_uni', 'N/A'
                )
                logging.debug(
                    "Remote MAX_STREAMS: bidi=%s uni=%s",
                    rmsb, rmsu,
                )
            except Exception:
                pass
            try:
                self._h3 = H3Connection(
                    self._quic, enable_webtransport=True
                )
            except TypeError:
                # Older aioquic without enable_webtransport kwarg
                try:
                    self._h3 = H3Connection(self._quic)
                except Exception as e:
                    self._h3_init_error = str(e)
                    self._wt_session_ready.set()
                    return
            except Exception as e:
                self._h3_init_error = str(e)
                self._wt_session_ready.set()
                return
            # Flush H3 SETTINGS immediately so the server knows
            # we support WebTransport before we send the CONNECT.
            self.transmit()

        elif isinstance(event, StreamDataReceived):
            # Log ALL stream data for diagnostics
            logging.debug(
                "StreamDataReceived: stream=%d len=%d "
                "hex=%s",
                event.stream_id, len(event.data),
                event.data[:64].hex() if event.data else '',
            )
            # Capture MoQT data on our WT bidi stream
            if (
                self._wt_bidi_stream_id is not None
                and event.stream_id == self._wt_bidi_stream_id
            ):
                logging.debug(
                    "WT bidi data: %d bytes on stream %d",
                    len(event.data), event.stream_id,
                )
                self._response_data.extend(event.data)
                complete, _, _ = _try_parse_response(
                    bytes(self._response_data),
                    self._wt_use_legacy,
                )
                if complete:
                    self._setup_complete.set()
                # Don't pass raw MoQT data to H3 — it would
                # misinterpret it as HTTP/3 framing and potentially
                # send error frames that kill the connection
                return

        elif isinstance(event, StreamReset):
            if (
                self._wt_bidi_stream_id is not None
                and event.stream_id == self._wt_bidi_stream_id
            ):
                wt_app_err = _decode_wt_error(event.error_code)
                self._error = (
                    f"WT bidi stream reset by server "
                    f"({wt_app_err})"
                )
                self._setup_complete.set()
                return

        elif isinstance(event, ConnectionTerminated):
            err_name = SESSION_ERROR_NAMES.get(
                event.error_code, f"0x{event.error_code:x}"
            )
            self._error = f"Connection closed: {err_name}"
            self._wt_session_ready.set()
            self._setup_complete.set()

        # Pass events to H3 for internal state (CONNECT response headers)
        if self._h3 is not None:
            try:
                for h3_event in self._h3.handle_event(event):
                    self._h3_event_received(h3_event)
            except Exception as e:
                logging.debug("H3 event processing error: %s", e)

    def _h3_event_received(self, event):
        """Handle H3-level events."""
        if isinstance(event, HeadersReceived):
            headers = dict(event.headers)
            status_raw = headers.get(b":status", b"0")
            try:
                self._http_status = int(status_raw.decode())
            except (ValueError, UnicodeDecodeError):
                self._http_status = 0

            logging.debug("H3 headers: status=%d stream=%d",
                          self._http_status, event.stream_id)
            # Log all response headers for diagnostics
            for k, v in event.headers:
                if k != b":status":
                    logging.debug("  %s: %s", k.decode(errors='replace'),
                                  v.decode(errors='replace'))

            # Capture WT draft version from response headers
            wt_draft_raw = headers.get(
                b"sec-webtransport-http3-draft", b""
            )
            if wt_draft_raw:
                self._wt_draft = wt_draft_raw.decode(errors='replace')
                logging.debug("Server WT draft: %s", self._wt_draft)

            # Capture negotiated WT protocol from response headers
            wt_protocol_raw = headers.get(b"wt-protocol", b"")
            if wt_protocol_raw:
                # Strip RFC 8941 Structured Fields String quoting
                proto = wt_protocol_raw.decode(errors='replace').strip()
                if proto.startswith('"') and proto.endswith('"'):
                    proto = proto[1:-1]
                self._wt_negotiated_protocol = proto
                logging.debug(
                    "Server WT protocol: %s",
                    self._wt_negotiated_protocol,
                )

            if self._http_status == 200:
                self._wt_session_id = event.stream_id
                self._wt_session_ready.set()
            else:
                self._error = f"HTTP {self._http_status}"
                self._wt_session_ready.set()
                self._setup_complete.set()

        elif (
            hasattr(event, 'data')
            and self._wt_session_id is not None
            and not self._setup_complete.is_set()
        ):
            # Fallback: capture data delivered via H3-level events
            # (DataReceived, WebTransportStreamDataReceived, etc.)
            stream_id = getattr(event, 'stream_id', -1)

            # Skip session-level data (capsules on the CONNECT stream)
            if stream_id == self._wt_session_id:
                # Try to parse as WebTransport capsule for diagnostics
                _log_wt_capsule(event.data)
                return

            # Only accept data from our WT bidi stream
            if (
                self._wt_bidi_stream_id is not None
                and stream_id != self._wt_bidi_stream_id
            ):
                logging.debug(
                    "Ignoring data from unrelated stream %d "
                    "(expected %d)",
                    stream_id, self._wt_bidi_stream_id,
                )
                return

            logging.debug(
                "H3 data event: %d bytes on stream %d",
                len(event.data), stream_id,
            )
            self._response_data.extend(event.data)
            complete, _, _ = _try_parse_response(
                bytes(self._response_data),
                self._wt_use_legacy,
            )
            if complete:
                self._setup_complete.set()


async def probe_webtransport(
    host: str,
    port: int,
    wt_path: str,
    alpns_to_try: List[str],
    verify_cert: bool,
    timeout: float,
    wt_protocol: Optional[str] = None,
) -> ProbeResult:
    """Probe MoQT over WebTransport (HTTP/3).

    1. Establish HTTP/3 connection (ALPN = h3).
    2. Send extended CONNECT request for WebTransport.
       If wt_protocol is set, include WT-Available-Protocols header
       to negotiate the specific MoQT version over WebTransport.
    3. If 200 OK, open a bidi stream and send CLIENT_SETUP.
    4. Wait for SERVER_SETUP.
    """
    if not H3_AVAILABLE:
        return ProbeResult(
            transport="webtransport",
            alpn="h3",
            status=ProbeStatus.ERROR,
            message="aioquic H3 module not available",
        )

    start = time.monotonic()

    configuration = QuicConfiguration(is_client=True, alpn_protocols=H3_ALPN)
    # WebTransport requires QUIC datagram support (RFC 9297)
    configuration.max_datagram_frame_size = 65536
    if not verify_cert:
        configuration.verify_mode = ssl.CERT_NONE

    phase = "connecting"  # Track probe phase for timeout diagnostics

    try:
        async with asyncio.timeout(timeout):
            async with connect(
                host,
                port,
                configuration=configuration,
                create_protocol=WebTransportProbeProtocol,
            ) as client:
                client: WebTransportProbeProtocol
                phase = "h3_init"

                # Wait for QUIC handshake and H3 initialization.
                # H3 SETTINGS are flushed immediately in the handshake
                # handler so the server knows we support WebTransport.
                if not client._handshake_complete.is_set():
                    await asyncio.wait_for(
                        client._handshake_complete.wait(), timeout=5.0
                    )

                # Small yield to let SETTINGS reach the server before
                # we send the CONNECT request.
                await asyncio.sleep(0.05)

                if client._h3 is None:
                    elapsed = (time.monotonic() - start) * 1000
                    init_err = client._h3_init_error or "unknown"
                    return ProbeResult(
                        transport="webtransport",
                        alpn="h3",
                        status=ProbeStatus.ERROR,
                        message=(
                            f"HTTP/3 connection failed to initialize: "
                            f"{init_err}"
                        ),
                        duration_ms=elapsed,
                    )

                # Build WebTransport CONNECT request (RFC 9297).
                # If wt_protocol is set, include WT-Available-Protocols
                # to negotiate a specific MoQT version.  Otherwise
                # the server stays in legacy mode and version
                # negotiation happens in-band via CLIENT_SETUP.
                authority_val = (
                    f"{host}:{port}" if port != 443 else host
                )
                stream_id = client._quic.get_next_available_stream_id(
                    is_unidirectional=False
                )
                headers = [
                    (b":method", b"CONNECT"),
                    (b":protocol", b"webtransport"),
                    (b":scheme", b"https"),
                    (b":authority", authority_val.encode()),
                    (b":path", wt_path.encode()),
                    (b"origin", f"https://{authority_val}".encode()),
                    (b"sec-webtransport-http3-draft", b"draft02"),
                ]
                if wt_protocol:
                    # Encode as RFC 8941 Structured Fields String
                    # (quoted).  Proxygen's getWTAvailableProtocols
                    # decodes via StructuredHeadersDecoder::decodeList
                    # and checks that items have type STRING — without
                    # quotes the parser sees a Token and rejects it.
                    sf_value = f'"{wt_protocol}"'
                    headers.append(
                        (
                            b"wt-available-protocols",
                            sf_value.encode(),
                        ),
                    )
                    logging.debug(
                        "Including WT-Available-Protocols: %s",
                        sf_value,
                    )
                phase = "sending_connect"
                client._h3.send_headers(stream_id=stream_id, headers=headers)
                client.transmit()
                logging.debug("CONNECT sent on stream %d", stream_id)

                # Wait for WebTransport session response
                phase = "awaiting_session"
                try:
                    remaining = max(1.0, timeout - (time.monotonic() - start))
                    await asyncio.wait_for(
                        client._wt_session_ready.wait(), timeout=remaining
                    )
                except asyncio.TimeoutError:
                    elapsed = (time.monotonic() - start) * 1000
                    return ProbeResult(
                        transport="webtransport",
                        alpn="h3",
                        status=ProbeStatus.TIMEOUT,
                        message="Timeout waiting for WebTransport session",
                        duration_ms=elapsed,
                    )

                if client._error:
                    elapsed = (time.monotonic() - start) * 1000
                    early_alpn = (
                        f"wt:{wt_protocol}"
                        if wt_protocol
                        else "h3"
                    )
                    return ProbeResult(
                        transport="webtransport",
                        alpn=early_alpn,
                        status=ProbeStatus.HANDSHAKE_ERROR,
                        message=f"WebTransport rejected: {client._error}",
                        duration_ms=elapsed,
                    )

                # WebTransport session established — send CLIENT_SETUP
                logging.debug(
                    "WT session OK, session_id=%d, HTTP status=%d, "
                    "draft=%s",
                    client._wt_session_id, client._http_status or 0,
                    client._wt_draft or "unknown",
                )

                # Brief delay to let any server-sent capsules arrive
                # and be processed (some servers send settings capsules
                # that must be processed before bidi streams are opened).
                await asyncio.sleep(0.1)

                # Create WT bidi stream through the H3 API so the
                # client H3 layer registers it as a WebTransport stream
                # (prevents it from being treated as an H3 request).
                # NOTE: create_webtransport_stream() writes the 0x41
                # stream-type header + session_id internally, so we
                # must NOT add them again manually.
                use_api = False
                try:
                    wt_bidi = client._h3.create_webtransport_stream(
                        session_id=client._wt_session_id,
                        is_unidirectional=False,
                    )
                    use_api = True
                    logging.debug(
                        "create_webtransport_stream OK: stream %d "
                        "(API writes 0x41 + session_id internally)",
                        wt_bidi,
                    )
                except Exception as e:
                    logging.debug(
                        "create_webtransport_stream unavailable: %s, "
                        "using manual stream with 0x41 framing", e,
                    )
                    wt_bidi = client._quic.get_next_available_stream_id(
                        is_unidirectional=False
                    )
                client._wt_bidi_stream_id = wt_bidi

                # Flush the WT stream header (0x41 + session_id)
                # BEFORE writing CLIENT_SETUP.  This ensures the
                # server's WT layer recognises the stream and calls
                # onNewBidiStream before application data arrives.
                client.transmit()
                await asyncio.sleep(0.05)

                # Check if the stream is blocked by MAX_STREAMS
                try:
                    stream_obj = client._quic._streams.get(wt_bidi)
                    if stream_obj:
                        logging.debug(
                            "Stream %d state: blocked=%s, "
                            "max_stream_data_remote=%d",
                            wt_bidi,
                            stream_obj.is_blocked,
                            stream_obj.max_stream_data_remote,
                        )
                    else:
                        logging.debug(
                            "Stream %d not found in QUIC streams!",
                            wt_bidi,
                        )
                except Exception as e:
                    logging.debug(
                        "Could not inspect stream %d state: %s",
                        wt_bidi, e,
                    )

                # Determine CLIENT_SETUP format based on whether we
                # negotiated a WT protocol via the header.
                negotiated = client._wt_negotiated_protocol
                if wt_protocol and not negotiated:
                    # We requested a specific WT protocol but the
                    # server did not echo it back — protocol not
                    # supported over WebTransport.
                    elapsed = (time.monotonic() - start) * 1000
                    return ProbeResult(
                        transport="webtransport",
                        alpn=f"wt:{wt_protocol}",
                        status=ProbeStatus.HANDSHAKE_ERROR,
                        message=(
                            f"Server did not negotiate "
                            f"wt-protocol: {wt_protocol}"
                        ),
                        duration_ms=elapsed,
                    )
                if negotiated:
                    # WT protocol pre-negotiated — version is already
                    # set on the server side.  CLIENT_SETUP omits the
                    # version array and uses the negotiated draft's
                    # parameter encoding.
                    wt_draft_ver = 14  # default
                    if negotiated.startswith("moqt-"):
                        try:
                            wt_draft_ver = int(
                                negotiated.split("-")[1]
                            )
                        except (IndexError, ValueError):
                            wt_draft_ver = 16
                    logging.debug(
                        "WT protocol negotiated: %s → draft %d",
                        negotiated, wt_draft_ver,
                    )
                    setup_msg = build_client_setup(
                        for_webtransport=True,
                        draft_version=wt_draft_ver,
                    )
                    # Pre-negotiated → no selectedVersion in
                    # SERVER_SETUP; parse as draft-15+ format.
                    client._wt_use_legacy = False
                else:
                    # No WT protocol negotiated — fall back to
                    # in-band version negotiation (draft-14).
                    # Uses version array in CLIENT_SETUP.
                    setup_msg = build_client_setup_for_wt(
                        max_request_id=100,
                    )
                    client._wt_use_legacy = True

                if use_api:
                    # API already wrote the WT bidi header (0x41 +
                    # session_id).  Send only the CLIENT_SETUP payload.
                    wire_data = setup_msg
                else:
                    # Manual fallback: prepend WT bidi stream header.
                    wt_frame_header = (
                        encode_varint(0x41)
                        + encode_varint(client._wt_session_id)
                    )
                    wire_data = wt_frame_header + setup_msg

                phase = "sending_setup"
                client._quic.send_stream_data(wt_bidi, wire_data)
                client.transmit()
                logging.debug(
                    "WT bidi stream %d: sent %d bytes total "
                    "(%d CLIENT_SETUP, api=%s, session_id=%d) "
                    "hex=%s",
                    wt_bidi, len(wire_data),
                    len(setup_msg), use_api,
                    client._wt_session_id,
                    wire_data.hex(),
                )

                # Wait for SERVER_SETUP
                phase = "awaiting_setup"
                try:
                    remaining = max(1.0, timeout - (time.monotonic() - start))
                    await asyncio.wait_for(
                        client._setup_complete.wait(), timeout=remaining
                    )
                except asyncio.TimeoutError:
                    elapsed = (time.monotonic() - start) * 1000
                    pa = (
                        f"wt:{negotiated}"
                        if negotiated
                        else "h3"
                    )
                    return ProbeResult(
                        transport="webtransport",
                        alpn=pa,
                        status=ProbeStatus.NO_RESPONSE,
                        message=(
                            "WebTransport session OK; "
                            "timeout waiting for SERVER_SETUP"
                        ),
                        duration_ms=elapsed,
                    )

                elapsed = (time.monotonic() - start) * 1000

                if client._error:
                    pa = (
                        f"wt:{negotiated}"
                        if negotiated
                        else "h3"
                    )
                    return ProbeResult(
                        transport="webtransport",
                        alpn=pa,
                        status=ProbeStatus.HANDSHAKE_ERROR,
                        message=client._error,
                        duration_ms=elapsed,
                    )

                # Parse SERVER_SETUP.
                # When WT protocol was pre-negotiated via header,
                # SERVER_SETUP uses draft-15+ format (no selected-
                # Version).  Otherwise try draft-14 first, then
                # legacy 0x41, then draft-15+.
                resp_data = bytes(client._response_data)
                logging.debug(
                    "Parsing SERVER_SETUP: %d bytes, hex=%s",
                    len(resp_data),
                    resp_data[:64].hex() if resp_data else '',
                )

                # Display ALPN for this probe result
                probe_alpn = (
                    f"wt:{negotiated}"
                    if negotiated
                    else "h3"
                )

                if negotiated:
                    # Pre-negotiated: parse as draft-15+ (no version)
                    use_delta = wt_draft_ver >= 16
                    params, err = parse_server_setup(
                        resp_data, delta_decode=use_delta,
                    )
                    if err:
                        return ProbeResult(
                            transport="webtransport",
                            alpn=probe_alpn,
                            status=ProbeStatus.HANDSHAKE_ERROR,
                            message=f"Invalid SERVER_SETUP: {err}",
                            duration_ms=elapsed,
                        )
                    return ProbeResult(
                        transport="webtransport",
                        alpn=probe_alpn,
                        status=ProbeStatus.SUCCESS,
                        message=(
                            f"SERVER_SETUP received via WebTransport "
                            f"(wt-protocol: {negotiated})"
                        ),
                        server_params=params,
                        duration_ms=elapsed,
                    )

                # No WT protocol negotiated — try multiple parsers.
                # Try 1: type 0x21 with selectedVersion (draft-14)
                result_d14, err_d14 = parse_server_setup_draft14(
                    resp_data
                )
                if result_d14 and not err_d14:
                    sel_ver = result_d14.get("version")
                    sel_params = result_d14.get("params", {})
                    ver_str = (
                        f"draft-{sel_ver & 0xFFFF}"
                        if sel_ver and sel_ver >= 0xFF000000
                        else f"0x{sel_ver:x}" if sel_ver else "unknown"
                    )
                    return ProbeResult(
                        transport="webtransport",
                        alpn=probe_alpn,
                        status=ProbeStatus.SUCCESS,
                        message=(
                            f"SERVER_SETUP received via WebTransport "
                            f"(version {ver_str})"
                        ),
                        server_params=sel_params,
                        selected_version=sel_ver,
                        duration_ms=elapsed,
                    )

                # Try 2: legacy type 0x41
                result_legacy, err_legacy = parse_server_setup_legacy(
                    resp_data
                )
                if result_legacy and not err_legacy:
                    sel_ver = result_legacy.get("version")
                    sel_params = result_legacy.get("params", {})
                    ver_str = (
                        f"draft-{sel_ver & 0xFFFF}"
                        if sel_ver and sel_ver >= 0xFF000000
                        else f"0x{sel_ver:x}" if sel_ver else "unknown"
                    )
                    return ProbeResult(
                        transport="webtransport",
                        alpn=probe_alpn,
                        status=ProbeStatus.SUCCESS,
                        message=(
                            f"SERVER_SETUP received via WebTransport "
                            f"(version {ver_str})"
                        ),
                        server_params=sel_params,
                        selected_version=sel_ver,
                        duration_ms=elapsed,
                    )

                # Try 3: draft-15+ format (type 0x21, no version)
                params, err = parse_server_setup(resp_data)
                if err:
                    return ProbeResult(
                        transport="webtransport",
                        alpn=probe_alpn,
                        status=ProbeStatus.HANDSHAKE_ERROR,
                        message=f"Invalid SERVER_SETUP: {err}",
                        duration_ms=elapsed,
                    )

                return ProbeResult(
                    transport="webtransport",
                    alpn=probe_alpn,
                    status=ProbeStatus.SUCCESS,
                    message="SERVER_SETUP received via WebTransport",
                    server_params=params,
                    duration_ms=elapsed,
                )

    except ConnectionRefusedError:
        elapsed = (time.monotonic() - start) * 1000
        return ProbeResult(
            transport="webtransport",
            alpn="h3",
            status=ProbeStatus.CONNECTION_REFUSED,
            message="Connection refused",
            duration_ms=elapsed,
        )
    except asyncio.TimeoutError:
        elapsed = (time.monotonic() - start) * 1000
        if phase == "connecting":
            msg = (
                "QUIC connection timeout — server may not accept "
                "h3 ALPN on this port"
            )
        elif phase == "h3_init":
            msg = "HTTP/3 initialization timeout"
        elif phase in ("sending_connect", "awaiting_session"):
            msg = (
                "Timeout waiting for WebTransport session response — "
                "server may not support WebTransport on this endpoint"
            )
        elif phase == "awaiting_setup":
            msg = (
                "WebTransport session OK; "
                "timeout waiting for SERVER_SETUP"
            )
        else:
            msg = f"Timeout during {phase} phase"
        return ProbeResult(
            transport="webtransport",
            alpn="h3",
            status=ProbeStatus.TIMEOUT,
            message=msg,
            duration_ms=elapsed,
        )
    except Exception as e:
        elapsed = (time.monotonic() - start) * 1000
        return ProbeResult(
            transport="webtransport",
            alpn="h3",
            status=ProbeStatus.ERROR,
            message=str(e),
            duration_ms=elapsed,
        )


# ============================================================================
# Output Formatting
# ============================================================================


class Colors:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    CYAN = "\033[36m"
    DIM = "\033[2m"

    @classmethod
    def disable(cls):
        for attr in ("RESET", "BOLD", "RED", "GREEN", "YELLOW", "BLUE", "CYAN", "DIM"):
            setattr(cls, attr, "")


def _fmt_param_value(param_type: int, value: Any) -> str:
    """Format a setup parameter value for human-readable display."""
    if isinstance(value, bytes):
        try:
            return f'"{value.decode("utf-8")}"'
        except UnicodeDecodeError:
            return f"0x{value.hex()}"
    if isinstance(value, int):
        if param_type == PARAM_ROLE:
            return {1: "Publisher", 2: "Subscriber", 3: "PubSub"}.get(
                value, str(value)
            )
        return str(value)
    return str(value)


def _fmt_version(v: int) -> str:
    """Format a legacy MoQT version number."""
    if v >= 0xFF000000:
        draft = v & 0xFFFF
        return f"draft-{draft:02d} (0x{v:08x})"
    return f"0x{v:x}"


def print_banner():
    print(
        f"\n{Colors.CYAN}{Colors.BOLD}"
        "========================================================\n"
        "  moq-version-check  --  MoQT Server Detection & Version Probe\n"
        "  Based on draft-ietf-moq-transport-16\n"
        "========================================================"
        f"{Colors.RESET}\n"
    )


def print_result(result: ProbeResult, verbose: bool = False):
    """Print a single probe result line."""
    if result.status == ProbeStatus.SUCCESS:
        icon = f"{Colors.GREEN}[OK]{Colors.RESET}"
        status_text = f"{Colors.GREEN}SUPPORTED{Colors.RESET}"
    elif result.status == ProbeStatus.TLS_FAILED:
        icon = f"{Colors.RED}[--]{Colors.RESET}"
        status_text = f"{Colors.DIM}not supported{Colors.RESET}"
    elif result.status in (ProbeStatus.TIMEOUT, ProbeStatus.CONNECTION_REFUSED):
        icon = f"{Colors.YELLOW}[??]{Colors.RESET}"
        status_text = f"{Colors.YELLOW}{result.status.value}{Colors.RESET}"
    elif result.status == ProbeStatus.NO_RESPONSE:
        icon = f"{Colors.YELLOW}[!!]{Colors.RESET}"
        status_text = f"{Colors.YELLOW}ALPN accepted, no SETUP response{Colors.RESET}"
    else:
        icon = f"{Colors.RED}[!!]{Colors.RESET}"
        status_text = f"{Colors.RED}{result.status.value}{Colors.RESET}"

    transport_label = (
        "QUIC" if result.transport == "quic" else "WebTransport"
    )
    alpn_display = result.alpn
    # For WT probes, show "proto=" instead of "ALPN=" when
    # a specific wt-available-protocols was negotiated.
    if result.alpn.startswith("wt:"):
        proto_key = "proto"
    else:
        proto_key = "ALPN"

    print(
        f"  {icon}  {Colors.BOLD}{transport_label:14s}{Colors.RESET} "
        f"{proto_key}={Colors.CYAN}{alpn_display:10s}{Colors.RESET}  "
        f"{status_text}  "
        f"{Colors.DIM}({result.duration_ms:.0f} ms){Colors.RESET}"
    )

    if result.message and (verbose or result.status == ProbeStatus.SUCCESS):
        print(f"       {Colors.DIM}{result.message}{Colors.RESET}")

    if result.selected_version is not None:
        print(
            f"       {Colors.BLUE}Selected version: "
            f"{_fmt_version(result.selected_version)}{Colors.RESET}"
        )

    if result.server_params:
        items = list(result.server_params.items())
        for i, (ptype, pvalue) in enumerate(items):
            pname = SETUP_PARAM_NAMES.get(ptype, f"param_0x{ptype:02x}")
            pval_str = _fmt_param_value(ptype, pvalue)
            connector = "├─" if i < len(items) - 1 else "└─"
            print(
                f"       {Colors.BLUE}{connector} {pname}: "
                f"{pval_str}{Colors.RESET}"
            )


def print_summary(results: List[ProbeResult]):
    """Print a summary of all probe results."""
    successful = [r for r in results if r.status == ProbeStatus.SUCCESS]
    quic_ok = [r for r in successful if r.transport == "quic"]
    wt_ok = [r for r in successful if r.transport == "webtransport"]
    partial = [r for r in results if r.status == ProbeStatus.NO_RESPONSE]

    print(
        f"\n{Colors.BOLD}"
        "════════════════════════════════════════════════════════\n"
        "  SUMMARY\n"
        "════════════════════════════════════════════════════════"
        f"{Colors.RESET}"
    )

    if not successful and not partial:
        print(f"\n  {Colors.RED}No MoQT support detected.{Colors.RESET}\n")
        return

    if successful:
        print(f"\n  {Colors.GREEN}{Colors.BOLD}MoQT server detected!{Colors.RESET}")

    if quic_ok:
        quic_versions = []
        for r in quic_ok:
            if r.alpn == MOQT_ALPN_LEGACY and r.selected_version:
                quic_versions.append(_fmt_version(r.selected_version))
            else:
                quic_versions.append(r.alpn)
        versions = ", ".join(quic_versions)
        print(f"  {Colors.BOLD}Raw QUIC versions:{Colors.RESET}   {versions}")
    if wt_ok:
        # Separate WT results: those with negotiated protocol vs fallback
        wt_negotiated = [
            r for r in wt_ok if r.alpn.startswith("wt:")
        ]
        wt_fallback = [r for r in wt_ok if not r.alpn.startswith("wt:")]
        if wt_negotiated:
            wt_protos = ", ".join(
                r.alpn.removeprefix("wt:") for r in wt_negotiated
            )
            print(
                f"  {Colors.BOLD}WebTransport protocols:"
                f"{Colors.RESET} {wt_protos}"
            )
        if wt_fallback:
            wt_fallback_versions = []
            for r in wt_fallback:
                if r.selected_version:
                    wt_fallback_versions.append(
                        _fmt_version(r.selected_version)
                    )
            if wt_fallback_versions:
                versions = ", ".join(dict.fromkeys(wt_fallback_versions))
                print(
                    f"  {Colors.BOLD}WebTransport fallback:"
                    f"{Colors.RESET} {versions}"
                )
            else:
                print(
                    f"  {Colors.BOLD}WebTransport fallback:"
                    f"{Colors.RESET} Supported"
                )

    if partial and not successful:
        print(
            f"\n  {Colors.YELLOW}Partial results: server accepted ALPN "
            f"but did not complete MoQT handshake.{Colors.RESET}"
        )
        for r in partial:
            print(f"    - {r.alpn} ({r.transport}): {r.message}")

    # Show implementation info from the first successful probe
    for r in successful:
        if r.server_params and PARAM_MOQT_IMPLEMENTATION in r.server_params:
            impl = _fmt_param_value(
                PARAM_MOQT_IMPLEMENTATION,
                r.server_params[PARAM_MOQT_IMPLEMENTATION],
            )
            print(f"  {Colors.BOLD}Implementation:{Colors.RESET}      {impl}")
            break

    print()


# ============================================================================
# URL Parsing
# ============================================================================


def parse_endpoint_url(url_str: str) -> Tuple[str, str, int, str]:
    """Parse an endpoint URL into (scheme, host, port, path).

    Accepts:
      moqt://host:port/path
      https://host:port/path
      host:port/path
      host:port
      host
    """
    if "://" not in url_str:
        url_str = f"moqt://{url_str}"

    parsed = urlparse(url_str)
    scheme = parsed.scheme or "moqt"
    host = parsed.hostname or "localhost"
    port = parsed.port or DEFAULT_MOQT_PORT
    path = parsed.path or "/"
    if parsed.query:
        path += f"?{parsed.query}"
    return scheme, host, port, path


# ============================================================================
# Orchestration
# ============================================================================


async def run_all_probes(
    host: str,
    port: int,
    path: str,
    authority: str,
    verify_cert: bool,
    timeout: float,
    verbose: bool,
    skip_quic: bool,
    skip_wt: bool,
    alpns: List[str],
) -> List[ProbeResult]:
    """Run all QUIC and WebTransport probes against the target."""
    all_results: List[ProbeResult] = []
    semaphore = asyncio.Semaphore(4)

    async def _limited(coro):
        async with semaphore:
            return await coro

    # ---- Raw QUIC probes ----
    if not skip_quic:
        print(
            f"  {Colors.BOLD}=== Raw QUIC (UDP) probes → "
            f"{host}:{port} ==={Colors.RESET}\n"
        )

        tasks = [
            _limited(
                probe_quic_alpn(
                    host, port, alpn, path, authority, verify_cert, timeout
                )
            )
            for alpn in alpns
        ]

        quic_results = await asyncio.gather(*tasks, return_exceptions=True)

        # Deduplicate identical timeout/refused results for display clarity
        first_timeout_shown = False
        first_refused_shown = False

        for r in quic_results:
            if isinstance(r, Exception):
                pr = ProbeResult(
                    transport="quic",
                    alpn="?",
                    status=ProbeStatus.ERROR,
                    message=str(r),
                )
                all_results.append(pr)
                print_result(pr, verbose)
            else:
                all_results.append(r)

                # For non-verbose, collapse repeated timeout/refused
                if not verbose:
                    if r.status == ProbeStatus.TIMEOUT:
                        if first_timeout_shown:
                            continue
                        first_timeout_shown = True
                    elif r.status == ProbeStatus.CONNECTION_REFUSED:
                        if first_refused_shown:
                            continue
                        first_refused_shown = True

                print_result(r, verbose)

        # Show collapsed count
        if not verbose:
            n_timeout = sum(
                1
                for r in all_results
                if r.transport == "quic" and r.status == ProbeStatus.TIMEOUT
            )
            n_refused = sum(
                1
                for r in all_results
                if r.transport == "quic"
                and r.status == ProbeStatus.CONNECTION_REFUSED
            )
            if n_timeout > 1:
                print(
                    f"  {Colors.DIM}     ... {n_timeout} total "
                    f"QUIC ALPNs timed out{Colors.RESET}"
                )
            if n_refused > 1:
                print(
                    f"  {Colors.DIM}     ... {n_refused} total "
                    f"QUIC ALPNs connection refused{Colors.RESET}"
                )

        print()

    # ---- WebTransport probes ----
    if not skip_wt:
        print(
            f"  {Colors.BOLD}=== WebTransport (HTTP/3) probes → "
            f"{host}:{port}{path} ==={Colors.RESET}\n"
        )

        # For each ALPN, try the primary identifier first, then
        # known alternatives (some servers use extended names like
        # "moqt-16-meta-00" instead of "moqt-16").
        # Skip moq-00 — WT protocol negotiation is only supported
        # for draft-15+ ALPNs; moq-00 uses in-band negotiation.
        wt_probing_alpns = [
            a for a in alpns if a != MOQT_ALPN_LEGACY
        ]
        wt_results: List = []
        for alpn in wt_probing_alpns:
            alternatives = WT_PROTOCOL_ALTERNATIVES.get(
                alpn, [alpn]
            )
            negotiated = False
            for proto_id in alternatives:
                r = await probe_webtransport(
                    host, port, path, alpns, verify_cert, timeout,
                    wt_protocol=proto_id,
                )
                if r.status == ProbeStatus.SUCCESS:
                    wt_results.append(r)
                    negotiated = True
                    break
                elif r.status == ProbeStatus.HANDSHAKE_ERROR and (
                    "did not negotiate" in (r.message or "")
                ):
                    # Server didn't negotiate this name — try next
                    logging.debug(
                        "WT protocol %s not negotiated, "
                        "trying next alternative",
                        proto_id,
                    )
                    continue
                else:
                    # Non-negotiation error — report it
                    wt_results.append(r)
                    negotiated = True
                    break
            if not negotiated:
                # None of the alternatives worked
                wt_results.append(
                    ProbeResult(
                        transport="webtransport",
                        alpn=f"wt:{alpn}",
                        status=ProbeStatus.HANDSHAKE_ERROR,
                        message=(
                            f"Server did not negotiate "
                            f"wt-protocol for {alpn}"
                        ),
                    )
                )

        # Also probe without WT-Available-Protocols (fallback to
        # in-band version negotiation).
        wt_fallback = await probe_webtransport(
            host, port, path, alpns, verify_cert, timeout,
            wt_protocol=None,
        )
        wt_results.append(wt_fallback)

        first_wt_timeout_shown = False
        first_wt_refused_shown = False

        for r in wt_results:
            all_results.append(r)

            if not verbose:
                if r.status == ProbeStatus.TIMEOUT:
                    if first_wt_timeout_shown:
                        continue
                    first_wt_timeout_shown = True
                elif r.status == ProbeStatus.CONNECTION_REFUSED:
                    if first_wt_refused_shown:
                        continue
                    first_wt_refused_shown = True
                elif r.status == ProbeStatus.HANDSHAKE_ERROR and (
                    r.alpn.startswith("wt:")
                ):
                    # In non-verbose, suppress WT protocol
                    # negotiation failures — they just mean the
                    # server doesn't support that protocol via WT.
                    continue

            print_result(r, verbose)

        print()

    return all_results


# ============================================================================
# Main
# ============================================================================


def main():
    parser = argparse.ArgumentParser(
        description=(
            "moq-version-check: Determine if an endpoint is a MoQT server "
            "and discover supported protocol versions."
        ),
        epilog=(
            "Examples:\n"
            "  moq-version-check moqt://relay.example.com:4443/moq\n"
            "  moq-version-check https://cdn.example.com/moq\n"
            "  moq-version-check relay.example.com:4443\n"
            "  moq-version-check --skip-wt relay.example.com:4443/moq\n"
            "  moq-version-check --verbose --alpns moqt-16,moqt-15 relay.example.com\n"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "url",
        help=(
            "Target endpoint URL.  Formats: moqt://host:port/path, "
            "https://host:port/path, host:port, host"
        ),
    )
    parser.add_argument(
        "--verify-cert",
        action="store_true",
        default=False,
        help="Verify TLS certificates (default: skip verification)",
    )
    parser.add_argument(
        "--timeout",
        "-t",
        type=float,
        default=DEFAULT_TIMEOUT,
        help=f"Timeout per probe in seconds (default: {DEFAULT_TIMEOUT})",
    )
    parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Show detailed output for all probes (including failures)",
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable colored output",
    )
    parser.add_argument(
        "--skip-quic",
        action="store_true",
        help="Skip raw QUIC probes",
    )
    parser.add_argument(
        "--skip-wt",
        action="store_true",
        help="Skip WebTransport probe",
    )
    parser.add_argument(
        "--alpns",
        type=str,
        default=None,
        help=(
            "Comma-separated ALPNs to test "
            "(default: moqt,moqt-16..moqt-06,moq-00)"
        ),
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Enable debug logging (very verbose)",
    )

    args = parser.parse_args()

    # Dependency check
    if not AIOQUIC_AVAILABLE:
        print(
            "Error: 'aioquic' library is required but not installed.\n"
            "Install it with:\n"
            "  pip install aioquic\n"
            "Run from source with: python3 moq-version-check.py <url>"
        )
        sys.exit(2)

    if args.debug:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.WARNING)

    if args.no_color or not sys.stdout.isatty():
        Colors.disable()

    # Parse URL
    scheme, host, port, path = parse_endpoint_url(args.url)

    # Determine authority for raw QUIC
    if port != DEFAULT_MOQT_PORT:
        authority = f"{host}:{port}"
    else:
        authority = host

    # Determine ALPNs to test
    if args.alpns:
        alpns = [a.strip() for a in args.alpns.split(",")]
    else:
        alpns = MOQT_ALPNS_NEW + [MOQT_ALPN_LEGACY]

    print_banner()
    print(f"  {Colors.BOLD}Target:{Colors.RESET}      {scheme}://{host}:{port}{path}")
    print(f"  {Colors.BOLD}Authority:{Colors.RESET}   {authority}")
    print(f"  {Colors.BOLD}ALPNs:{Colors.RESET}       {len(alpns)} version(s) to probe")
    print(f"  {Colors.BOLD}Timeout:{Colors.RESET}     {args.timeout}s per probe")
    print(
        f"  {Colors.BOLD}TLS verify:{Colors.RESET}  "
        f"{'Yes' if args.verify_cert else 'No (use --verify-cert to enable)'}"
    )
    transports = []
    if not args.skip_quic:
        transports.append("Raw QUIC")
    if not args.skip_wt:
        transports.append("WebTransport")
    print(
        f"  {Colors.BOLD}Transports:{Colors.RESET}  {', '.join(transports)}"
    )
    print()

    # Run probes
    results = asyncio.run(
        run_all_probes(
            host=host,
            port=port,
            path=path,
            authority=authority,
            verify_cert=args.verify_cert,
            timeout=args.timeout,
            verbose=args.verbose,
            skip_quic=args.skip_quic,
            skip_wt=args.skip_wt,
            alpns=alpns,
        )
    )

    # Summary
    print_summary(results)

    # Exit code: 0 if any version supported, 1 if not
    has_support = any(r.status == ProbeStatus.SUCCESS for r in results)
    sys.exit(0 if has_support else 1)


if __name__ == "__main__":
    main()
