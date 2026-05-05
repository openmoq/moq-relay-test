# MoQ Interop

## What This Test Does

Runs interoperability tests against a relay for operations such as goaway, publish, subscribe, fetch, and error paths.

## Parameters Exposed In UI

| Parameter | Type | Required | Default | Notes |
|---|---|---:|---|---|
| relay_url | url | yes | (empty) | Relay endpoint URL |
| tests | text | no | (empty) | Comma-separated test list; empty means run all |

## How Invocation Works

Container image and command:

- Image: `moq-interop:latest`
- Entrypoint: image default
- Base command from tool manifest: none

Arguments assembled by the runner:

```text
--relay={relay_url} --tests={tests}
```

Notes:

- If `tests` is empty, the runner removes the empty `--tests=` token, so all tests run.
- Network mode is `host`.
- Extra host mapping is added: `host.docker.internal:host-gateway`.

## What Output Looks Like

Raw logs include discovered test count, per-test start, description, and pass/fail lines.

Renderer behavior:

- Detects expected count:

```text
Found <n> test(s) to run
```

- Detects test start:

```text
=== Running Test: <TestName> ===
```

- Detects description:

```text
Description: <text>
```

- Detects results:
  - `<TestName> PASSED`
  - `<TestName> FAILED`
  - `<TestName> ERROR: ...`
- Renders a live table with test name, description, and status.

## Pass / Fail Criteria

Tool-level pass in UI summary requires both:

- Container exit code is `0`
- Renderer observed `0` failed/error tests

Any failed/error test or non-zero exit code marks the run as failed.
