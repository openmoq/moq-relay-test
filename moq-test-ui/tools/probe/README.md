# Relay Probe

## What This Test Does

Probes relay endpoint liveness and compatibility, including transport/draft coverage and measured latency where available.

## Parameters Exposed In UI

| Parameter | Type | Required | Default | Notes |
|---|---|---:|---|---|
| relay_url | url | yes | (empty) | Relay endpoint URL |
| transport | select | no | both | `both`, `quic`, or `wt` |
| timeout | number | no | 10 | Probe timeout (seconds) |

## How Invocation Works

Container image and command:

- Image: `ghcr.io/gmarzot/aiomoqt:0.8.2`
- Entrypoint: `/bin/sh -c`
- Command:

```text
python /app/aiomoqt/examples/relay_probe.py -f /tmp/relays.json -o /tmp/probe_out.json --once && cat /tmp/probe_out.json && echo
```

Input file preparation:

- The runner generates `/tmp/relays.json` inside the container from `relay_url` and `transport` via a built-in generator.
- This file is bind-mounted read-only into the tool container.

Notes:

- Network mode is `host`.

## What Output Looks Like

Raw logs include a JSON report printed near completion.

Renderer behavior:

- Buffers output and parses the JSON block.
- Renders a probe table with:
  - relay and endpoint
  - transport
  - live/down state
  - draft/probe details
  - latency and error fields
- Shows summary chips by draft and total live/down probe counts.

## Pass / Fail Criteria

Process-level pass:

- Container exit code is `0`.

Probe health interpretation (renderer):

- `live == total` probes: healthy
- `0 < live < total`: partial
- `live == 0`: down

This tool is primarily diagnostic; partial results are useful and expected in mixed environments.
