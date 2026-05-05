# MoQ Conformance

## What This Test Does

Executes the MoQ conformance suite against a relay to validate protocol behavior across conformance sections and individual tests.

## Parameters Exposed In UI

| Parameter | Type | Required | Default | Notes |
|---|---|---:|---|---|
| relay_url | url | yes | (empty) | Relay endpoint URL |
| draft | select | no | 16 | Draft 16 or 14 |
| transport | select | no | Q | `Q` (QUIC) or empty for WebTransport |

## How Invocation Works

Container image and entrypoint:

- Image: `moq-interop:latest`
- Entrypoint: `/opt/moq/conformance/run_conformance.sh`
- Base command: none

Arguments assembled by the runner:

```text
{relay_url} {draft} {transport}
```

Runtime notes:

- Network mode is `host`.
- Extra host mapping is added: `host.docker.internal:host-gateway`.

## What Output Looks Like

Raw logs include section headers, test start lines, and pass/fail lines.

Renderer behavior:

- Detects section lines like:

```text
=== SECTION <n>: <name> ===
```

- Detects test start lines like:

```text
[Test <n>] <test name>
```

- Detects result lines:
  - `✓ PASSED`
  - `✗ FAILED: <reason>`
- Renders:
  - live test table
  - pass/fail counters
  - per-section summary

## Pass / Fail Criteria

Tool-level pass in UI summary requires both:

- Container exit code is `0`
- Renderer observed `0` failed tests

Any failed test or non-zero exit code marks the run as failed.
