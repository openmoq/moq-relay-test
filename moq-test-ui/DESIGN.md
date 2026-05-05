# MoQ Test UI Design

## Overview

`moq-test-ui` is a generic web runner for Docker-backed MoQ test tools. It provides a single framework that can:

- discover tools from manifest files
- render parameter forms dynamically
- run each tool inside a Docker container
- stream output to the browser over WebSocket
- apply client-side regex filters to logs
- load optional per-tool renderers for richer result views
- persist run history as JSON
- run a relay self-test workflow across multiple tools

The implementation is centered on a lightweight Node.js server plus a no-build browser client.

## Current Architecture

```text
Browser
  - tools view
  - self-test view
  - history view
  - dynamic parameter form
  - raw output panel + optional renderer panel
        |
        | WebSocket + small REST surface
        v
Node.js server
  - Express static hosting and read APIs
  - ws message handling
  - tool registry
  - Docker executor
  - self-test orchestrator
  - results store
        |
        | Docker socket
        v
Docker engine
  - tool containers
  - optional compose deployment
  - nginx TLS proxy for containerized deployment
```

## Backend Design

The backend is a small Node.js service using Express and `ws`.

- `src/server.js` initializes the tool registry, Docker executor, results store, and self-test orchestrator.
- Static assets are served from `public/`.
- Tool renderers are exposed directly from `tools/` so the browser can load them on demand.
- REST endpoints are intentionally narrow: list tools, list results, and fetch a saved result.
- Real-time activity uses a single WebSocket connection per browser session.

### Tool Registry

The registry scans `tools/*/tool.json` at startup, validates the required fields, and exposes normalized tool metadata to the UI. The manifest is the primary extension point for the system.

Each tool can define:

- Docker image, command, entrypoint, network mode, extra hosts, and environment
- parameter schema for form generation
- regex log filters
- optional renderer module
- optional `selfTest` configuration
- optional generated input files via `prepareFiles`

### Docker Execution

The Docker executor is responsible for container lifecycle and output streaming.

- Containers are created through `dockerode` using the local Docker socket.
- Commands are built from manifest templates with user parameters substituted into `buildCommand`.
- Output is demultiplexed, ANSI escape sequences are stripped, and lines are forwarded live to the browser.
- Temporary input files can be generated on the host and bind-mounted into sibling containers.
- A duration-based auto-stop path exists for tools that declare a timeout parameter.
- Runs are stopped on request and also when the owning WebSocket session closes.

Generated input files are written under `/tmp/generic-runner` when available so sibling containers can access them correctly when the UI itself runs inside Docker.

### Results Storage

Each completed run is saved as structured JSON under `results/<tool>/...json`. Self-test runs are saved separately under `results/self-test/`.

Stored result data includes:

- tool name
- input parameters
- timestamps
- exit code
- session identifier
- full captured output

This keeps the history view simple and makes later re-rendering possible without rerunning the tool.

## Frontend Design

The frontend is plain ES modules with no build step.

- `public/js/app.js` is the coordinator for tool selection, runs, history, and self-test.
- `param-form.js` renders forms directly from manifest metadata.
- `output-panel.js` renders the raw log stream and applies client-side filters.
- `renderer-loader.js` dynamically imports `tools/<name>/renderer.js` when a tool has a renderer.
- `self-test-ui.js` reuses the same renderer model during self-test runs.
- `history-ui.js` lists saved results and displays them in a modal overlay.

The UI is split into three views:

- Tools: pick a tool, configure inputs, run it, inspect logs and renderer output
- Automated Tests: run all self-test-enabled tools against one relay target
- History: browse and reopen saved result files

## Tool Model

The current tool set is manifest-driven and includes these implemented tool directories:

- `probe`
- `conformance`
- `interop-tests`
- `adaptive-bench`
- `multi-sub-bench`

## Self-Test Design

Self-test is implemented as a server-side orchestrated sequence.

- The orchestrator collects all tools with enabled `selfTest` entries.
- It expands single or array-based self-test configs.
- Tools run sequentially in manifest-defined order.
- Shared relay configuration is merged with per-tool defaults.
- Live output is streamed to the browser as normal progress events.
- Final aggregated results are saved to disk.

The browser-side self-test UI intentionally reuses each tool renderer instead of introducing a separate parsing layer. That keeps self-test support mostly data-driven: adding `selfTest` to a tool plus a renderer is generally enough.

## WebSocket Protocol

The current protocol covers:

- tool listing
- run start and stop
- self-test start and stop
- live output events
- result listing and result fetch

The protocol is intentionally simple and session-scoped. Session isolation is achieved by tracking runs per socket and stopping those runs when the socket disconnects.

## Current Characteristics And Limitations

The current implementation intentionally keeps several areas simple:

- The terminal output view is currently a custom `pre`-based log panel, not a full `xterm.js` terminal. The page still links xterm CSS, but runtime behavior is the simpler implementation.
- Self-test runs all enabled tools; there is no per-run checklist UI.
- Session isolation exists, but reconnect-based active run recovery is not implemented. A disconnect stops the session's runs.
- History is implemented and backed by JSON files, but the list view is intentionally minimal and opens raw saved output in a modal.
- Local startup has been simplified and documented as `npm install` then `npm start`, with Docker Compose as an optional nginx-backed deployment path.

## Deployment Model

Two operating modes are supported.

### Local development

- install dependencies with `npm install`
- start the server with `npm start`
- open `http://localhost:3000`

### Containerized deployment

- `docker compose up --build`
- nginx terminates TLS and proxies to the Node.js app
- the app container mounts the Docker socket so it can launch sibling tool containers
- the compose setup expects certificates in `./certs`

## Operational Notes

The UI stores runtime artifacts that should stay out of source control.

- `results/` contains saved run output and input parameters
- `certs/` contains deployment TLS material

These are operational assets, not source files.

## Design Intent Going Forward

The design remains intentionally data-driven:

- add tools by dropping in a manifest and optional renderer
- keep the backend generic and unaware of tool-specific parsing
- keep self-test composition driven by tool manifests
- preserve saved JSON output as the system-of-record for history and later analysis

That keeps the framework extensible without reintroducing bespoke UI code for each test tool.