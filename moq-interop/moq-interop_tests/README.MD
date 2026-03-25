# Interop Tests

This directory contains the MoQ interoperability test suite for testing MoQ relay implementations.

There are two ways to build and run the tests:

| Method | When to use |
|---|---|
| **Docker** (recommended) | Any machine with Docker — no local C++ toolchain needed |
| **Local build** | macOS/Linux with moxygen already built |

---

## Method 1 — Docker (Recommended)

The `moq-interop/Makefile` wraps the full Docker workflow. Run everything from
the `moq-interop/` directory.

### Build the image

```bash
cd moq-interop
make build
```

The first build fetches and compiles all Meta OSS dependencies inside Docker
(30–60 min). The getdeps cache is stored in a BuildKit persistent cache volume
so subsequent builds are fast even after source changes.

### Run the tests

Start your relay first, then:

```bash
# Run all tests (localhost is auto-rewritten to host.docker.internal)
make tests RELAY_URL=https://localhost:4433/moq

# Custom port / path
make tests RELAY_URL=https://localhost:9668/moq-relay

# Remote relay
make tests RELAY_URL=https://relay.example.com:4433/moq

# Filter by category
make tests RELAY_URL=https://localhost:4433/moq TESTS_ARGS="--categories=Publisher,Subscriber"

# Run a specific test
make tests RELAY_URL=https://localhost:4433/moq TESTS_ARGS="--tests=PublishTest"

# Verbose build output
make build DOCKER_BUILD_FLAGS="--progress=plain"
```

> **Note:** `localhost` and `127.0.0.1` in `RELAY_URL` are automatically
> rewritten to `host.docker.internal` so the container can reach a relay
> running on the host.

### Open an interactive shell in the container

```bash
make shell
```

---

## Method 2 — Local Build

moxygen must already be built (see the
[moqt_implementation README](../moqt_implementation/README.md)) before
configuring or building the interop tests.

### One-step build (recommended)

From `moqt_implementation/moxygen/`:

```bash
bash scripts/build.sh              # default: moxygen preset
bash scripts/build.sh moxygen-san  # AddressSanitizer build
bash scripts/build.sh core         # no moxygen_adapter
```

The compiled binary is placed at `moq-interop_tests/build/bin/interop_tests`.

### Manual configure + build

Source the getdeps environment first, then:

```bash
# Source getdeps env
cd moqt_implementation/moxygen/moxygen
eval "$(python3 build/fbcode_builder/getdeps.py env moxygen)"

# Configure
cmake -S ../../../moq-interop_tests \
      -B ../../../moq-interop_tests/build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DWITH_MOXYGEN_ADAPTER=ON

# Build
cmake --build ../../../moq-interop_tests/build -- -j$(nproc)
```

### Run

Start the relay first, then from `moq-interop_tests/`:

```bash
./build/bin/interop_tests
```

Or with the logging wrapper:

```bash
bash run_with_logging.sh
bash run_with_logging.sh --asan   # ASAN build
```

---

## CLI Reference

The binary accepts these flags (passed via `TESTS_ARGS` in Docker, or directly for local builds):

| Flag | Default | Description |
|---|---|---|
| `--relay=<url>` | `https://localhost:4433/moq` | Relay URL to test against |
| `--tests=<names>` | _(all)_ | Comma-separated test names to run |
| `--categories=<cats>` | _(all)_ | Comma-separated categories (see below) |
| `--list` | `false` | List available tests without running them |

**Available categories:** `Publisher`, `Subscriber`, `Namespace`, `ErrorHandling`, `Update`, `Connection`, `All`

Category names are case-insensitive (`error_handling` and `ErrorHandling` are equivalent).

### Examples (Docker)

```bash
# List all available tests
make tests RELAY_URL=https://localhost:4433/moq TESTS_ARGS="--list"

# Run all tests against local relay
make tests RELAY_URL=https://localhost:4433/moq

# Run only publisher tests
make tests RELAY_URL=https://localhost:4433/moq TESTS_ARGS="--categories=Publisher"

# Run publisher and subscriber tests
make tests RELAY_URL=https://localhost:4433/moq TESTS_ARGS="--categories=Publisher,Subscriber"

# Run a specific named test
make tests RELAY_URL=https://localhost:4433/moq TESTS_ARGS="--tests=PublishTest"

# Run against a remote relay
make tests RELAY_URL=https://relay.example.com:4433/moq
```

### Examples (local binary)

```bash
# List all available tests
./build/bin/interop_tests --list

# Run all tests
./build/bin/interop_tests --relay=https://localhost:4433/moq

# Run by category
./build/bin/interop_tests --relay=https://localhost:4433/moq --categories=Subscriber,ErrorHandling

# Run a specific test
./build/bin/interop_tests --relay=https://localhost:4433/moq --tests=PublishTest
```

---

## Exit Codes

| Code | Meaning |
|---|---|
| `0` | All tests passed |
| `1` | One or more tests failed |

---

## Test Structure

```
moq-interop_tests/
├── publisher_tests/    # Publisher-related tests
├── subscriber_tests/   # Subscriber-related tests
├── base/               # Base test framework and utilities
└── moxygen_adapter/    # Adapter layer for the moxygen implementation
```
