# MoQ Relay Test

A suite of containerized tools for conformance, interoperability, and performance testing of MoQT relay implementations.

## Sub-projects

| Project | Description |
| :--- | :--- |
| [moq-test-ui](moq-test-ui/) | Web frontend for running and visualising test results |
| [moq-version-check](moq-version-check/) | Containerized relay version test tool |
| [moq-interop-tests](moq-interop-tests/) | Containerized conformance and interop test tool suite |
| moq-bench *(planned)* | Benchmark tooling for relay throughput and latency |
| moq-sec *(planned)* | Security and authentication test tools |

Each sub-project builds one or more containers that can be used individually, composed into a full test suite, or driven via the `moq-test-ui` frontend or REST APIs.

## Requirements

Test requirements documentation is maintained in the [requirements/](requirements/) folder.

- [requirements/relay-test-requirements.md](requirements/relay-test-requirements.md) — MoQ Relay Test Requirements