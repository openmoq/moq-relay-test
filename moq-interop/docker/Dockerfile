# syntax=docker/dockerfile:1
# =============================================================================
# moq-interop — Multi-stage Docker build
#
# Stages
#   builder  — Ubuntu 22.04 + full C++20 toolchain; builds moxygen and all
#              Meta OSS deps via getdeps, then compiles the interop tests.
#   runtime  — Minimal Ubuntu 22.04; contains the interop_tests binary.
#              All Meta OSS deps are statically linked.
#
# Cache strategy
#   getdeps compiles folly/fizz/wangle/mvfst/proxygen/moxygen from source
#   (30–60 min cold build) into /tmp/fbcode_builder_getdeps-*/. That directory
#   is wired to a BuildKit persistent cache volume (--mount=type=cache) rather
#   than a regular Docker layer, so it survives Dockerfile edits without
#   needing to be invalidated or re-built.
#
#   Subsequent builds (e.g. after editing interop_tests source) still get the
#   full cached getdeps tree and only spend the ~1 min on the test compile.
#
# Build context: moq-interop/
# Usage:        see Makefile  (make build / make run / make tests / …)
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
        # Required at link time: getdeps-built libunwind.a references liblzma
        liblzma-dev \
        # Used in the staging step to strip embedded RPATH from the binary so
        # the runtime stage can resolve shared libs via /opt/moq/lib + ldconfig
        patchelf \
    && rm -rf /var/lib/apt/lists/*

# Use GCC — gcc-11 ships with build-essential on Ubuntu 22.04 and handles
# the full moxygen/proxygen/folly dependency tree without the structured-
# binding capture strictness that clang-14 enforces.
ENV CC=gcc CXX=g++

# All sources live here; this path is embedded in the getdeps scratch-dir hash
# so it must stay stable across builds.
WORKDIR /workspace

# ── Layer 2 — moxygen submodule ───────────────────────────────────────────────────────
# Copy only the moxygen submodule and helper scripts.
COPY moqt_implementation/moxygen/moxygen/ ./moqt_implementation/moxygen/moxygen/
COPY moqt_implementation/moxygen/scripts/ ./moqt_implementation/moxygen/scripts/

# ── Layer 3 — Meta OSS deps + moxygen (BuildKit persistent cache) ────────────
# getdeps.py fetches, patches, and compiles:
#   folly → fizz → wangle → mvfst → proxygen → moxygen
# All artefacts go into GETDEPS_CACHE which is mounted as a BuildKit
# persistent cache volume.  This volume survives across ALL builds regardless
# of whether earlier Dockerfile layers changed, so the 30-60 min compile is
# paid only once (cold) and never again unless the moxygen submodule changes.
RUN --mount=type=cache,id=moq-interop-getdeps,target=/tmp/fbcode_builder_getdeps-ZworkspaceZmoqt_implementationZmoxygenZmoxygenZbuildZfbcode_builder-root,sharing=locked \
    cd /workspace/moqt_implementation/moxygen && \
    bash scripts/setup-deps.sh

# ── Layer 4 — interop tests source (changes frequently, cheap to rebuild) ────
COPY moq-interop_tests/ ./moq-interop_tests/

# ── Layer 5 — build interop_tests binary and stage it ───────────────────────
# setup-deps generates ~2 MiB of verbose getdeps/Boost output that clips the
# log before configure/link errors become visible.  We redirect it to a file;
# on failure the last 60 lines are printed so the actual error is always shown.
RUN --mount=type=cache,id=moq-interop-getdeps,target=/tmp/fbcode_builder_getdeps-ZworkspaceZmoqt_implementationZmoxygenZmoxygenZbuildZfbcode_builder-root,sharing=locked \
    set -e; \
    cd /workspace/moq-interop_tests; \
    echo "==> docker/CMakeLists.txt: $(ls /workspace/moq-interop_tests/docker/CMakeLists.txt)"; \
    bash scripts/build.sh docker > /tmp/build-docker.log 2>&1 || { echo "==> build.sh docker FAILED:"; tail -80 /tmp/build-docker.log; exit 1; }; \
    echo "==> build.sh docker succeeded. Last configure/build lines:"; \
    tail -20 /tmp/build-docker.log; \
    mkdir -p /opt/moq/bin /opt/moq/lib; \
    cp /workspace/moq-interop_tests/build/bin/interop_tests /opt/moq/bin/interop_tests; \
    chmod +x /opt/moq/bin/interop_tests; \
    patchelf --remove-rpath /opt/moq/bin/interop_tests; \
    echo "==> Staging glog/gflags (.so files):"; \
    for pattern in "glog*/lib/libglog.so*" "gflags*/lib/libgflags.so*"; do \
        find /tmp/fbcode_builder_getdeps-*/installed/ -path "*/${pattern}" 2>/dev/null | while read f; do \
            cp -L "$f" "/opt/moq/lib/$(basename "$f")" && echo "  staged: $(basename "$f")" || true; \
        done; \
    done; \
    echo "==> Staged libs:"; ls -la /opt/moq/lib/

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
        libssl3 \
        zlib1g \
        libunwind8 \
        libjemalloc2 \
        liblzma5 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/moq/bin/interop_tests /opt/moq/bin/interop_tests
# Bundle any getdeps-built shared libs the binary was dynamically linked against
# (e.g. libglog, libgflags built with a different ABI than the system package).
COPY --from=builder /opt/moq/lib/ /opt/moq/lib/
# Register /opt/moq/lib in the dynamic linker cache so the binary can find
# the bundled glog/gflags at runtime without an LD_LIBRARY_PATH override.
RUN echo "/opt/moq/lib" > /etc/ld.so.conf.d/moq.conf && ldconfig

COPY docker/entrypoint.sh /opt/moq/entrypoint.sh
RUN chmod +x /opt/moq/entrypoint.sh /opt/moq/bin/interop_tests

ENV PATH="/opt/moq/bin:${PATH}"

ENTRYPOINT ["/opt/moq/entrypoint.sh"]
CMD ["--relay=https://localhost:4433/moq"]
