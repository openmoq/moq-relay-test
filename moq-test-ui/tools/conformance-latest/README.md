# MoQ Conformance (latest)

## What This Test Does

Runs the latest MoQTest conformance suite against a relay using MoQT draft 18 only. For drafts 14 and 16 use the `conformance` tool (Moxygen Conformance (pre 18)).

## Parameters Exposed In UI

| Parameter | Type | Required | Default | Notes |
|---|---|---:|---|---|
| relay_url | url | yes | (empty) | Relay endpoint URL |
| draft | select | no | 18 | Only Draft 18 is offered |
| transport | select | no | WebTransport | `Q` (QUIC), `X` (QMUX-on-TCP) or empty for WebTransport |

## How Invocation Works

- Image: `moq-interop:latest` (built locally with `make build` in `moq-interop/`)
- Entrypoint: `/opt/moq/conformance/run_conformance.sh`
- Arguments: `{relay_url} {draft} {transport}`
- Network mode is `host`, with `host.docker.internal:host-gateway` mapped.

## Output And Pass / Fail

Same output format and criteria as the `conformance` tool; the renderer is shared with it (`../conformance/renderer.js`).
