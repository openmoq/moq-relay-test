# MoQ Relay Test

A suite of containerized tools for conformance, interoperability, and performance testing of MoQT relay implementations.

## Sub-projects

| Project | Description |
| :--- | :--- |
| [moq-test-ui](moq-test-ui/) | Web frontend for running and visualising test results |
| [moq-interop-tests](moq-interop-tests/) | Containerized conformance and interop test tool suite |

## External tools

| Project | Description |
| :--- | :--- |
| [aiomoqt](https://github.com/gmarzot/aiomoqt) | Benchmark tooling for relay throughput and latency |

## Tests implemented 

| Test | Type | Description |
| :--- | :--- | :--- |
| [probe](moq-test-ui/tools/probe/README.md) | Diagnostics | Relay endpoint liveness and compatibility probe across transports and draft versions |
| [interop-tests](moq-test-ui/tools/interop-tests/README.md) | Conformance | Interoperability test suite for relay operations such as goaway, subscribe, publish, and fetch |
| [conformance](moq-test-ui/tools/conformance/README.md) | Conformance | MoQ conformance suite validating protocol behavior across structured test sections |
| [multi-sub-bench](moq-test-ui/tools/multi-sub-bench/README.md) | Performance | Multi-subscriber load benchmark measuring completion rate, output throughput, and latency |
| [adaptive-bench](moq-test-ui/tools/adaptive-bench/README.md) | Performance | Adaptive bitrate benchmark for throughput/latency behavior under changing network conditions |


Each sub-project and external tool builds one or more containers that can be used individually, composed into a full test suite, or driven via the `moq-test-ui` frontend or REST APIs.

## Requirements

Test requirements documentation is maintained in the [requirements/](requirements/) folder.

- [requirements/relay-test-requirements.md](requirements/relay-test-requirements.md) — MoQ Relay Test Requirements