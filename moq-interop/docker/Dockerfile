# =============================================================================
# moq-interop — Multi-stage Docker build
#
# Stages
#   builder  — Ubuntu 22.04 + full C++20 toolchain; builds moxygen and all
#              Meta OSS deps via getdeps, then compiles the interop tests.
#   runtime  — Minimal Ubuntu 22.04; contains only the two binaries and TLS
#              certs.  Everything else is static-linked Meta OSS code.
#
# Layer-cache strategy
#   1. System packages        (invalidated only on Ubuntu upgrade)
#   2. moxygen submodule COPY (invalidated only when submodule changes)
#   3. getdeps full build     (30–60 min cold; cache reused when layer 2 hits)
#   4. moq-interop_tests COPY (invalidated on every test-source change)
#   5. interop_tests build    (~1 min; runs whenever source changes)
#
# Build context: moq-interop/
# Usage:        see ../Makefile  (make build / make run / make relay / …)
# =============================================================================

# ── Build arguments ──────────────────────────────────────────────────────────
ARG UBUNTU_VERSION=22.04

# =============================================================================
# Stage 1 — builder
# =============================================================================
FROM ubuntu:${UBUNTU_VERSION} AS builder

ARG DEBIAN_FRONTEND=noninteractive
# Respect parallelism passed from the host (Makefile sets this automatically).
ARG NPROC=4

# ── System toolchain + getdeps runtime requirements ──────────────────────────
# We keep this intentionally lean: getdeps builds its own copies of folly,
# fizz, wangle, mvfst, proxygen, boost, libevent, etc.
# System deps are for the compiler/build system and a few build-script tools.
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        # Compiler — gcc-11 ships with build-essential on Ubuntu 22.04
        build-essential \
        # Build systems
        cmake \
        ninja-build \
        make \
        # getdeps runtime
        python3 \
        python3-dev \
        python3-pip \
        # Source control / downloads
        git \
        wget \
        curl \
        ca-certificates \
        unzip \
        # Autotools (required by several getdeps packages)
        autoconf \
        automake \
        libtool \
        pkg-config \
        bison \
        flex \
        # Headers that getdeps packages rely on from the system
        libssl-dev \
        zlib1g-dev \
        libjemalloc-dev \
        libunwind-dev \
        libiberty-dev \
        # Miscellaneous utilities used by build scripts
        patch \
        perl \
    && rm -rf /var/lib/apt/lists/*

# Use GCC — gcc-11 ships with build-essential on Ubuntu 22.04 and handles
# the full moxygen/proxygen/folly dependency tree without the structured-
# binding capture strictness that clang-14 enforces.
ENV CC=gcc CXX=g++

# All sources live here; this path is embedded in the getdeps scratch-dir hash
# so it must stay stable across builds.
WORKDIR /workspace

# ── Layer 2 — moxygen submodule (heavy cached layer) ─────────────────────────
# Copy only the moxygen submodule and helper scripts first.
# As long as the submodule commit does not change, Docker reuses the next
# (expensive) layer from cache instead of re-running the 30-60 min build.
COPY moqt_implementation/moxygen/moxygen/ ./moqt_implementation/moxygen/moxygen/
COPY moqt_implementation/moxygen/scripts/ ./moqt_implementation/moxygen/scripts/

# ── Layer 3 — Meta OSS deps + moxygen relay (expensive, heavily cached) ──────
# getdeps.py fetches, patches, and compiles:
#   folly → fizz → wangle → mvfst → proxygen → moxygen
# On a cache hit this layer is skipped entirely.
RUN cd /workspace/moqt_implementation/moxygen && \
    bash scripts/setup-deps.sh

# ── Layer 4 — interop tests source (changes frequently, cheap to rebuild) ────
COPY moq-interop_tests/ ./moq-interop_tests/

# ── Layer 5 — build interop_tests binary (~1 min) ────────────────────────────
RUN cd /workspace/moqt_implementation/moxygen && \
    bash scripts/build.sh moxygen

# ── Extract binaries to a well-known path ────────────────────────────────────
# Query getdeps for the moxygen install dir and copy both binaries into
# /opt/moq/bin/ so the runtime stage can reference them with a fixed path.
RUN \
    GETDEPS="python3 /workspace/moqt_implementation/moxygen/moxygen/build/fbcode_builder/getdeps.py" && \
    MOXYGEN_INST=$($GETDEPS show-inst-dir moxygen 2>/dev/null | tail -1) && \
    echo "Moxygen install dir: ${MOXYGEN_INST}" && \
    mkdir -p /opt/moq/bin && \
    cp "${MOXYGEN_INST}/bin/moq_relay"                         /opt/moq/bin/moq_relay    && \
    cp /workspace/moq-interop_tests/build/bin/interop_tests    /opt/moq/bin/interop_tests && \
    chmod +x /opt/moq/bin/moq_relay /opt/moq/bin/interop_tests && \
    # Print shared-lib deps so they can be verified during CI.
    echo "==> moq_relay shared libs:" && ldd /opt/moq/bin/moq_relay    || true && \
    echo "==> interop_tests shared libs:" && ldd /opt/moq/bin/interop_tests || true

# =============================================================================
# Stage 2 — runtime
# =============================================================================
FROM ubuntu:${UBUNTU_VERSION} AS runtime

ARG DEBIAN_FRONTEND=noninteractive

# Install only what the dynamic linker will need at runtime.
# (All Meta OSS code is statically linked; the only dynamic deps are
#  standard system libs and OpenSSL below.)
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        # Dynamic linker requirements
        libssl3 \
        zlib1g \
        libunwind8 \
        libjemalloc2 \
        # TLS cert generation for the relay
        openssl \
        # Useful for debugging inside a running container
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy the two compiled binaries from the builder stage.
COPY --from=builder /opt/moq/bin/ /opt/moq/bin/

# Copy the entrypoint script.
COPY docker/entrypoint.sh /opt/moq/entrypoint.sh
RUN chmod +x /opt/moq/entrypoint.sh /opt/moq/bin/moq_relay /opt/moq/bin/interop_tests

# Ensure binaries are on PATH for convenience inside the container.
ENV PATH="/opt/moq/bin:${PATH}"

# Generate a self-signed TLS certificate for the relay at build time.
# Regenerate at runtime by bind-mounting /opt/moq/certs/ or with
# `docker run … /bin/sh -c "openssl req … && /opt/moq/entrypoint.sh relay"`.
RUN mkdir -p /opt/moq/certs && \
    openssl req -x509 -newkey rsa:2048 \
        -keyout /opt/moq/certs/cert.key \
        -out    /opt/moq/certs/cert.pem \
        -days   3650 \
        -nodes  \
        -subj   "/C=US/O=moq-interop/CN=localhost" \
    && chmod 600 /opt/moq/certs/cert.key

# ── Ports ────────────────────────────────────────────────────────────────────
# 4433 UDP — QUIC / WebTransport (primary)
# 4433 TCP — HTTP/3 fallback
EXPOSE 4433/udp 4433/tcp

# ── Environment knobs ────────────────────────────────────────────────────────
ENV RELAY_PORT=4433 \
    RELAY_ENDPOINT=/moq \
    MOQT_VERSIONS=16 \
    GLOG_logtostderr=1

ENTRYPOINT ["/opt/moq/entrypoint.sh"]
# Default: start the relay in the background then run all interop tests.
CMD ["all"]
