#!/usr/bin/env python3
"""Send independent authenticated MoQT requests and report relay responses."""

import argparse
import asyncio
import base64
import binascii
import json
import logging
import sys
import time
import uuid
from datetime import datetime, timezone
from urllib.parse import urlsplit, urlunsplit

from aiomoqt.client import MOQTClient
from aiomoqt.messages import AuthToken
from aiomoqt.types import MOQTException, MOQTRequestError, ParamType, SetupParamType

METHODS = (
    "client_setup", "publish_namespace", "subscribe_namespace",
    "subscribe", "publish", "fetch", "track_status",
)
TIMEOUT = 12


def decode_token(raw):
    raw = raw.strip()
    if not raw:
        raise ValueError("Token is required")
    if raw.startswith("base64:"):
        encoded = raw[7:]
        try:
            token = base64.b64decode(
                encoded + "=" * (-len(encoded) % 4), altchars=b"-_", validate=True,
            )
        except (ValueError, binascii.Error):
            raise ValueError("Invalid base64 token") from None
    elif raw.startswith("hex:"):
        try:
            token = bytes.fromhex(raw[4:])
        except ValueError:
            raise ValueError("Invalid hex token") from None
    else:
        token = raw.encode()
    if not token:
        raise ValueError("Token must contain at least one byte")
    return token


def validate_config(config):
    for key in ("relay_url", "transport", "draft_version", "namespace", "auth_token"):
        if not isinstance(config.get(key), str) or not config[key].strip():
            raise ValueError(f"{key} is required and must be a non-empty string")
    if config["transport"] not in ("quic", "wt"):
        raise ValueError("Transport must be quic or wt")
    if config["draft_version"] not in ("14", "16", "18", "20", "22"):
        raise ValueError("Unsupported draft version")
    if not config["namespace"].strip("/"):
        raise ValueError("Namespace must contain a component")
    if not isinstance(config.get("track_name", ""), str):
        raise ValueError("Track name must be a string")
    token_type = config.get("token_type", 0)
    if type(token_type) is not int or not 0 <= token_type <= 2**53 - 1:
        raise ValueError("Token type must be an integer between 0 and 9007199254740991")
    parsed = urlsplit(config["relay_url"])
    if (parsed.scheme not in ("https", "http", "moqt") or not parsed.hostname
            or parsed.username is not None or parsed.password is not None or parsed.fragment):
        raise ValueError("Relay URL must be an https:// or moqt:// endpoint without credentials or fragment")
    port = parsed.port or 443
    scheme = "moqt" if config["transport"] == "quic" else "https"
    url = urlunsplit((scheme, parsed.netloc, parsed.path, parsed.query, ""))
    return {
        "url": url,
        "host": parsed.hostname,
        "port": port,
        "path": urlunsplit(("", "", parsed.path or "/", parsed.query, "")),
        "draft": int(config["draft_version"]),
        "transport": config["transport"],
        "token_type": token_type,
        "namespace": config["namespace"],
        "track": config.get("track_name") or f"auth-token-test-{uuid.uuid4().hex}",
    }


def redact(detail, raw_token, token):
    # A peer may echo a credential in its rejection reason.
    secrets = {
        raw_token.strip(), raw_token.strip().removeprefix("base64:").removeprefix("hex:"),
        base64.b64encode(token).decode(), base64.urlsafe_b64encode(token).decode(),
        token.hex(), str(token), repr(token),
    }
    try:
        secrets.add(token.decode())
    except UnicodeDecodeError:
        pass  # Binary tokens have no UTF-8 representation to redact.
    for secret in sorted(secrets, key=len, reverse=True):
        if secret:
            detail = detail.replace(secret, "[redacted]")
    return detail


def request_authorization(method, draft, error):
    if error.response is None:
        return "unknown"
    auth_errors = (1, 0x10, 0x12) if draft == 14 else (1, 4, 5)
    if error.error_code in auth_errors:
        return "denied"
    missing_code = 4 if draft == 14 else 0x10
    if method in ("subscribe", "fetch", "track_status") and error.error_code == missing_code:
        return "allowed_inferred"
    return "unknown"


async def probe(method, target, token, client_factory=MOQTClient):
    stage = "client_setup"
    started = time.monotonic()
    result = {"allowed": False, "authorization": "unknown"}
    try:
        async with asyncio.timeout(TIMEOUT):
            client = client_factory(
                target["host"], target["port"], path=target["path"],
                use_quic=target["transport"] == "quic",
                supported_drafts=target["draft"],
            )
            async with client.connect() as session:
                await session.client_session_init(
                    timeout=TIMEOUT - 2,
                    parameters={SetupParamType.AUTH_TOKEN: token},
                )
                if session.negotiated_draft != target["draft"]:
                    raise ValueError(f"Relay negotiated unexpected draft {session.negotiated_draft}")
                if method == "client_setup":
                    result.update(allowed=True, authorization="allowed", detail="Session setup accepted")
                else:
                    stage = method
                    kwargs = {"parameters": {ParamType.AUTH_TOKEN: token}, "wait_response": True}
                    args = [target["namespace"]]
                    if method not in ("publish_namespace", "subscribe_namespace"):
                        args.append(target["track"])
                    response = await getattr(session, method)(*args, **kwargs)
                    if response is None:
                        raise ValueError("No relay acknowledgement received")
                    result.update(allowed=True, authorization="allowed", detail=f"Relay accepted: {type(response).__name__}")
    except MOQTRequestError as exc:
        result.update(
            detail=f"{stage}: request error {exc.error_code}: {exc.reason}",
            error_code=int(exc.error_code),
            authorization=request_authorization(stage, target["draft"], exc),
        )
        if result["authorization"] == "allowed_inferred":
            result["detail"] += (
                "; namespace/track not found. Authorization inferred to have passed "
                "on relays that authorize before lookup (including moqx); "
                "this does not prove token validity or successful delivery"
            )
    except MOQTException as exc:
        result.update(
            detail=f"{stage}: session error {exc.error_code}: {exc.reason_phrase}",
            error_code=int(exc.error_code),
        )
    except TimeoutError:
        result["detail"] = f"{stage}: timeout after {TIMEOUT}s; permission not established"
    except (OSError, ConnectionError, ValueError, NotImplementedError) as exc:
        result["detail"] = f"{stage}: {type(exc).__name__}: {exc}"
    result["stage"] = stage
    result["latency_ms"] = round((time.monotonic() - started) * 1000, 1)
    return result


async def run(config, client_factory=MOQTClient):
    target = validate_config(config)
    token = AuthToken(decode_token(config["auth_token"]), token_type=target["token_type"])
    report = {
        "type": "auth-token-test", **target,
        "timestamp": datetime.now(timezone.utc).isoformat(), "tests": {},
    }
    for method in METHODS:
        if method != "client_setup" and not report["tests"]["client_setup"]["allowed"]:
            result = {
                "allowed": False, "stage": "client_setup", "latency_ms": None,
                "authorization": "unknown",
                "detail": "Not attempted: client_setup failed; permission not established",
            }
        else:
            result = await probe(method, target, token, client_factory)
        result["detail"] = redact(result["detail"], config["auth_token"], token)
        report["tests"][method] = result
        request_result = "Accepted" if result["allowed"] else "Not accepted"
        print(
            f"[auth] {method}: authorization={result['authorization']}; request={request_result}",
            flush=True,
        )
        print(json.dumps(report), flush=True)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", required=True)
    args = parser.parse_args()
    # aiomoqt message logs can include token parameter values.
    logging.disable(logging.CRITICAL)
    try:
        with open(args.config, encoding="utf-8") as source:
            config = json.load(source)
        if not isinstance(config, dict):
            raise ValueError("Configuration must be a JSON object")
        asyncio.run(run(config))
    except (OSError, ValueError) as exc:
        print(json.dumps({"type": "auth-token-test", "error": str(exc)}), flush=True)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
