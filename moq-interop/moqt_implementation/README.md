# MoQT Implementation

This directory contains the MoQT relay implementation (Facebook moxygen) and the
scripts needed to build it along with all Meta OSS dependencies.

## Directory Structure

```
moqt_implementation/
├── moxygen/
│   ├── moxygen/          # Git submodule: Facebook moxygen MoQ relay
│   ├── certs/            # TLS certificates for local testing (not in git)
│   └── scripts/
│       ├── setup-deps.sh       # Full first-time build (moxygen + all deps)
│       ├── build-moxygen.sh    # Incremental rebuild of moxygen only
│       ├── build.sh            # Top-level driver (setup → configure → compile)
│       └── configure.sh        # CMake configure for the interop tests
└── README.md             # This file
```

> **Build artefacts** are stored in getdeps' default temp directory
> (`/tmp/fbcode_builder_getdeps-<hash>/`). getdeps handles all incrementality
> natively — a cold build takes 30–60 minutes; subsequent runs are fast.

---

## Prerequisites

| Tool | Version |
|---|---|
| Python 3 | 3.8+ |
| CMake | 3.20+ |
| Ninja | any recent |
| Clang or GCC | C++20 capable |
| git | submodule support |
| openssl | for certificate generation |

---

## 1 — Clone with Submodule

When cloning for the first time, use `--recursive`:

```bash
git clone --recursive <repo-url>
```

If you already have the repo, initialize the submodule:

```bash
git submodule update --init --recursive
```

To pull the latest moxygen upstream changes:

```bash
git submodule update --remote moqt_implementation/moxygen/moxygen
```

---

## 2 — Build moxygen and All Dependencies (first run)

From the `moqt_implementation/moxygen/` directory:

```bash
cd moq-interop/moqt_implementation/moxygen
bash scripts/setup-deps.sh
```

This fetches and compiles folly, fizz, wangle, mvfst, proxygen, and moxygen via
getdeps. On a cold build this takes **30–60 minutes**. Subsequent runs are
incremental.

### Rebuild moxygen only (after submodule updates)

```bash
bash scripts/build-moxygen.sh
```

---

## 3 — TLS Certificates

Certificates are **not tracked in git**. Generate self-signed ones for local
testing:

```bash
openssl req -x509 -newkey rsa:2048 \
    -keyout moq-interop/moqt_implementation/moxygen/certs/certificate.key \
    -out    moq-interop/moqt_implementation/moxygen/certs/certificate.pem \
    -days 365 -nodes \
    -subj "/C=US/ST=State/L=City/O=Org/CN=localhost"
```

To use your own certificates, place them in `moxygen/certs/` with those names
and set permissions on the key:

```bash
chmod 600 moq-interop/moqt_implementation/moxygen/certs/certificate.key
```

---

## 4 — Run the moxygen Relay

Query the install directory, then launch the relay:

```bash
cd moq-interop/moqt_implementation/moxygen/moxygen

RELAY_BIN="$(python3 build/fbcode_builder/getdeps.py show-inst-dir moxygen)/bin/moq_relay"

"$RELAY_BIN" \
    --port=4433 \
    --cert=../certs/certificate.pem \
    --key=../certs/certificate.key \
    --endpoint=/moq \
    --versions=16
```

The relay listens on `https://localhost:4433/moq` by default.

---

## 5 — Build and Run the Interop Tests

The interop tests are built and run via Docker — no local C++ toolchain needed
beyond what the Docker image provides. See the
[moq-interop_tests README](../moq-interop_tests/README.MD) for full details.

**Quick start:**

```bash
cd moq-interop

# Build the Docker image (first time ~30-60 min; subsequent runs are cached)
make build

# Run all tests against a local relay
# localhost is automatically rewritten to host.docker.internal inside Docker
make tests RELAY_URL=https://localhost:4433/moq

# Run against a specific path/port
make tests RELAY_URL=https://localhost:9668/moq-relay
```

For local (non-Docker) builds on macOS or Linux, use `scripts/build.sh`
directly — see the [agents guide](../../../agents.md) for the full workflow.
