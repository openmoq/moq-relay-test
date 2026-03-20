# moq-version-check

MoQT (Media over QUIC Transport) server detection and protocol version probe API.

Detects supported MoQT versions via raw QUIC and WebTransport probes.

## Quick Start

```bash
# Build and run locally
docker compose up -d --build

# Or build for linux/amd64 and push to Docker Hub
make builder
make push DOCKER_USER=<your-dockerhub-user> TAG=v1.0.0
```

Service runs on `http://localhost:8080`

## API Endpoint

**POST** `/api/probe`

Probe a MoQT server to detect supported protocol versions and transport methods.

### Request

```bash
curl -X POST http://localhost:8080/api/probe \
  -H 'Content-Type: application/json' \
  -d '{
    "url": "relay.example.com:4433/moq-relay",
    "alpns": "moqt-16, moqt-15",
    "timeout": 10,
    "skip_quic": false,
    "skip_wt": false,
    "verify_cert": false
  }'
```

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `url` | string | Yes | - | Server URL (host:port/path) |
| `alpns` | string | No | "moqt-16, moqt-15, moq-00" | Comma-separated ALPN versions to probe |
| `timeout` | integer | No | 10 | Connection timeout in seconds |
| `skip_quic` | boolean | No | false | Skip raw QUIC probes |
| `skip_wt` | boolean | No | false | Skip WebTransport probes |
| `verify_cert` | boolean | No | false | Verify SSL certificates |

### Response

```json
{
  "results": [
    {
      "transport": "QUIC",
      "alpn": "moqt-16",
      "status": "SUPPORTED",
      "params": {
        "MAX_REQUEST_ID": 100,
        "MAX_AUTH_TOKEN_CACHE_SIZE": 1024
      },
      "duration_ms": 102
    },
    {
      "transport": "WebTransport",
      "protocol": "moqt-16",
      "status": "SUPPORTED",
      "params": {},
      "duration_ms": 338
    }
  ],
  "summary": {
    "moqt_detected": true,
    "quic_versions": ["moqt-16", "moqt-15"],
    "wt_protocols": ["moqt-16"],
    "total_duration_ms": 1240
  },
  "debug_log": "..."
}
```

### Response Fields

- **results**: Array of probe results for each transport/version combination
  - `transport`: "QUIC" or "WebTransport"
  - `alpn`/`protocol`: Version identifier (moqt-16, moqt-15, moq-00)
  - `status`: "SUPPORTED", "UNSUPPORTED", or "ERROR"
  - `params`: Server parameters from SERVER_SETUP message
  - `duration_ms`: Probe duration in milliseconds

- **summary**: Aggregate results
  - `moqt_detected`: Whether any MoQT version was detected
  - `quic_versions`: List of supported QUIC ALPN versions
  - `wt_protocols`: List of supported WebTransport protocols
  - `total_duration_ms`: Total time for all probes

- **debug_log**: Detailed protocol-level debug output

## Standalone CLI Script

`moq-check-version.sh` is a standalone Bash script that replicates the core probing logic without requiring the web service. It is intended for quick manual testing and debugging directly from the command line, independent of Docker or the HTTP API.

### Health Check

**GET** `/health`

Returns `{"status": "ok"}` when the service is running.

## Supported Versions

| Version | ALPN | WebTransport | Status |
|---------|------|--------------|--------|
| draft-16 | moqt-16 | moqt-16 | ✅ Current |
| draft-15 | moqt-15 | moqt-15 | ✅ Supported |
| draft-14 | moq-00 | (fallback) | ✅ Legacy |

## Standards

- [draft-ietf-moq-transport-16](https://datatracker.ietf.org/doc/draft-ietf-moq-transport/16/)
- [RFC 9114](https://www.rfc-editor.org/rfc/rfc9114.html) (HTTP/3)
- [RFC 9000](https://www.rfc-editor.org/rfc/rfc9000.html) (QUIC)
- [RFC 8941](https://www.rfc-editor.org/rfc/rfc8941.html) (Structured Fields)
