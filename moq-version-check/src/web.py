#!/usr/bin/env python3
"""
moq-version-check API — Flask app that wraps the moq-version-check probing
engine and exposes a JSON API endpoint.  No UI is served.

For production use with gunicorn:
    gunicorn -w 4 -k gevent --bind 0.0.0.0:8080 --timeout 120 web:app
"""

import asyncio
import importlib.util
import io
import logging
import os
import sys
import time
from dataclasses import asdict
from pathlib import Path
from typing import Any, Dict, List

from flask import Flask, jsonify, request

# Load probe engine from hyphenated module filename.
_probe_module_path = Path(__file__).with_name("moq-version-check.py")
_probe_spec = importlib.util.spec_from_file_location(
    "moq_version_check", _probe_module_path
)
if _probe_spec is None or _probe_spec.loader is None:
    raise ImportError(f"Failed to load probe module: {_probe_module_path}")
_probe_module = importlib.util.module_from_spec(_probe_spec)
_probe_spec.loader.exec_module(_probe_module)

MOQT_ALPN_LEGACY = _probe_module.MOQT_ALPN_LEGACY
MOQT_ALPNS_NEW = _probe_module.MOQT_ALPNS_NEW
PARAM_ROLE = _probe_module.PARAM_ROLE
SETUP_PARAM_NAMES = _probe_module.SETUP_PARAM_NAMES
ProbeResult = _probe_module.ProbeResult
ProbeStatus = _probe_module.ProbeStatus
parse_endpoint_url = _probe_module.parse_endpoint_url
run_all_probes = _probe_module.run_all_probes

app = Flask(__name__)

# Production configuration
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024  # 16 KB max request size
app.config['JSON_SORT_KEYS'] = False

# Configure logging
if not app.debug:
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(name)s: %(message)s',
        stream=sys.stdout
    )


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _fmt_param_value(param_type: int, value: Any) -> str:
    if isinstance(value, bytes):
        try:
            return value.decode("utf-8")
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
    if v >= 0xFF000000:
        draft = v & 0xFFFF
        return f"draft-{draft:02d} (0x{v:08x})"
    return f"0x{v:x}"


def _result_to_dict(r: ProbeResult) -> Dict:
    """Convert a ProbeResult to a JSON-friendly dict."""
    params_list = []
    if r.server_params:
        for ptype, pvalue in r.server_params.items():
            params_list.append(
                {
                    "name": SETUP_PARAM_NAMES.get(
                        ptype, f"param_0x{ptype:02x}"
                    ),
                    "value": _fmt_param_value(ptype, pvalue),
                }
            )
    return {
        "transport": r.transport,
        "alpn": r.alpn,
        "status": r.status.value,
        "message": r.message,
        "selected_version": (
            _fmt_version(r.selected_version) if r.selected_version else None
        ),
        "params": params_list,
        "duration_ms": round(r.duration_ms, 1),
    }


# ---------------------------------------------------------------------------
# Error Handlers
# ---------------------------------------------------------------------------


@app.errorhandler(400)
def bad_request(e):
    return jsonify({"error": "Bad request"}), 400


@app.errorhandler(404)
def not_found(e):
    return jsonify({"error": "Not found"}), 404


@app.errorhandler(413)
def request_too_large(e):
    return jsonify({"error": "Request too large"}), 413


@app.errorhandler(500)
def internal_error(e):
    app.logger.error(f"Internal error: {e}")
    return jsonify({"error": "Internal server error"}), 500


@app.errorhandler(Exception)
def handle_exception(e):
    app.logger.exception("Unhandled exception")
    return jsonify({"error": "An unexpected error occurred"}), 500


# ---------------------------------------------------------------------------
# Routes
# ---------------------------------------------------------------------------


@app.route("/api/probe", methods=["POST"])
def api_probe():
    try:
        data = request.get_json(force=True)
    except Exception:
        return jsonify({"error": "Invalid JSON"}), 400
    
    url_str = data.get("url", "").strip()
    if not url_str:
        return jsonify({"error": "URL is required"}), 400
    
    # Validate and sanitize timeout
    try:
        timeout = float(data.get("timeout", 10))
        if timeout < 1 or timeout > 60:
            return jsonify({"error": "Timeout must be between 1 and 60 seconds"}), 400
    except (ValueError, TypeError):
        return jsonify({"error": "Invalid timeout value"}), 400
    
    verify_cert = bool(data.get("verify_cert", False))
    skip_quic = bool(data.get("skip_quic", False))
    skip_wt = bool(data.get("skip_wt", False))
    
    # Skip both transports is invalid
    if skip_quic and skip_wt:
        return jsonify({"error": "Cannot skip both QUIC and WebTransport"}), 400

    # Parse URL
    try:
        scheme, host, port, path = parse_endpoint_url(url_str)
    except Exception as e:
        app.logger.warning(f"Invalid URL '{url_str}': {e}")
        return jsonify({"error": f"Invalid URL: {e}"}), 400

    if port != 443:
        authority = f"{host}:{port}"
    else:
        authority = host

    # ALPNs
    alpns_str = data.get("alpns", "").strip()
    if alpns_str:
        alpns = [a.strip() for a in alpns_str.split(",") if a.strip()]
        if not alpns:
            return jsonify({"error": "Invalid ALPN list"}), 400
        # Limit number of ALPNs
        if len(alpns) > 10:
            return jsonify({"error": "Too many ALPNs (max 10)"}), 400
    else:
        alpns = MOQT_ALPNS_NEW + [MOQT_ALPN_LEGACY]

    # Capture debug log output
    debug_buffer = io.StringIO()
    debug_handler = logging.StreamHandler(debug_buffer)
    debug_handler.setLevel(logging.DEBUG)
    debug_handler.setFormatter(
        logging.Formatter("%(levelname)s %(name)s: %(message)s")
    )

    # Attach to root logger temporarily
    root_logger = logging.getLogger()
    original_level = root_logger.level
    root_logger.setLevel(logging.DEBUG)
    root_logger.addHandler(debug_handler)

    start_time = time.monotonic()
    try:
        results: List[ProbeResult] = asyncio.run(
            run_all_probes(
                host=host,
                port=port,
                path=path,
                authority=authority,
                verify_cert=verify_cert,
                timeout=timeout,
                verbose=True,
                skip_quic=skip_quic,
                skip_wt=skip_wt,
                alpns=alpns,
            )
        )
    except Exception as e:
        return jsonify({"error": str(e)}), 500
    finally:
        root_logger.removeHandler(debug_handler)
        root_logger.setLevel(original_level)

    total_ms = (time.monotonic() - start_time) * 1000
    debug_log = debug_buffer.getvalue()

    # Build response
    result_dicts = [_result_to_dict(r) for r in results]

    # Build summary
    successful = [r for r in results if r.status == ProbeStatus.SUCCESS]
    quic_ok = [r for r in successful if r.transport == "quic"]
    wt_ok = [r for r in successful if r.transport == "webtransport"]
    wt_negotiated = [r for r in wt_ok if r.alpn.startswith("wt:")]
    wt_fallback = [r for r in wt_ok if not r.alpn.startswith("wt:")]

    quic_versions = []
    for r in quic_ok:
        if r.alpn == MOQT_ALPN_LEGACY and r.selected_version:
            quic_versions.append(_fmt_version(r.selected_version))
        else:
            quic_versions.append(r.alpn)

    wt_inband_versions = [
        _fmt_version(r.selected_version)
        for r in wt_fallback
        if r.selected_version
    ]

    summary = {
        "detected": len(successful) > 0,
        "quic_versions": quic_versions,
        "wt_protocols": [
            r.alpn.removeprefix("wt:") for r in wt_negotiated
        ],
        "wt_inband": len(wt_fallback) > 0,
        "wt_inband_versions": list(dict.fromkeys(wt_inband_versions)),
        "total_ms": round(total_ms, 1),
    }

    # Extract implementation info
    for r in successful:
        if r.server_params and 0x07 in r.server_params:
            impl = r.server_params[0x07]
            if isinstance(impl, bytes):
                try:
                    impl = impl.decode("utf-8")
                except UnicodeDecodeError:
                    impl = f"0x{impl.hex()}"
            summary["implementation"] = str(impl)
            break

    return jsonify(
        {
            "target": f"{scheme}://{host}:{port}{path}",
            "authority": authority,
            "results": result_dicts,
            "summary": summary,
            "debug_log": debug_log,
        }
    )


@app.route("/health")
def health():
    """Health check endpoint for monitoring."""
    return jsonify({"status": "ok", "service": "moq-version-check"}), 200


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    # Development mode only - use gunicorn for production
    port = int(os.environ.get("PORT", 8080))
    print(f"moq-version-check API → http://localhost:{port}/api/probe")
    print("⚠️  Development mode - use gunicorn for production")
    print(f"   gunicorn -w 4 -k gevent --bind 0.0.0.0:{port} --timeout 120 web:app")
    app.run(host="0.0.0.0", port=port, debug=True)

