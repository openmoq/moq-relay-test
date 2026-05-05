# Multi-Subscriber

## What This Test Does

Launches many subscribers in parallel and measures completion, throughput, and latency under load.

## Parameters Exposed In UI

| Parameter | Type | Required | Default | Notes |
|---|---|---:|---|---|
| relay_url | url | yes | (empty) | Relay endpoint URL |
| draft | select | no | 16 | Draft 16 or 14 |
| subscribers | number | no | 100 | Number of subscriber sessions |
| video | select | no | 720p | Stream profile |
| duration | number | no | 60 | Test duration in seconds |

## How Invocation Works

Container image and entrypoint:

- Image: `ghcr.io/gmarzot/aiomoqt:0.8.2`
- Entrypoint: `python`
- Base command: `/app/aiomoqt/examples/multi_sub_bench.py`

Arguments assembled by the runner:

```text
{relay_url} --draft {draft} -n {subscribers} --video {video} -t {duration}
```

Notes:

- Network mode is `host`.

## What Output Looks Like

Raw logs include progress lines and final summary lines.

Renderer behavior:

- Parses progress lines such as:

```text
[10s] 20/100 complete, 80 active
```

- Parses final summary lines such as:
  - `Subscribers: 81/100 ok (0 errors, 19 resets)`
  - `Total objects: ...`
  - `Total output: ... Mbps ...`
  - `Avg latency: ... ms`
- Renders:
  - live line chart (complete vs active subscribers)
  - final metrics table
  - compact summary text

## Pass / Fail Criteria

Tool-level pass in renderer is strict and requires all conditions:

- Container exit code is `0`
- Subscriber success is complete (`ok == total`)
- Average latency is below `100 ms`

If any condition is not met, run is marked failed in renderer summary.
