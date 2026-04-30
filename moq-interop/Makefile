# =============================================================================
# moq-interop Makefile
# =============================================================================
#
# A single command builds the moq-interop test suite anywhere Docker
# is available — no local C++ toolchain required.
#
# Targets
#   build        Build (or rebuild) the moq-interop Docker image.
#   tests        Run the interop tests against RELAY_URL.
#   conformance  Run moxygen moqtest conformance tests against CONFORMANCE_URL.
#   shell        Open an interactive shell inside the container.
#   clean        Remove the local Docker image.
#   push         Tag and push the image to REGISTRY/IMAGE:TAG.
#   help         Print this help text.
#
# Common variables (override on the command line)
#   IMAGE             Local image name                   (default: moq-interop)
#   TAG               Image tag                          (default: latest)
#   REGISTRY          Registry prefix                    (default: empty)
#   RELAY_URL         Relay URL used by 'tests'          (default: https://localhost:4433/moq)
#   TESTS_ARGS        Extra flags for interop_tests      (default: empty)
#   WITH_CONFORMANCE  Include conformance tooling (0/1)  (default: 1)
#   CONFORMANCE_URL   Relay URL used by 'conformance'    (default: http://localhost:9999)
#   CONFORMANCE_ARGS  Extra args for conformance_test.sh (default: empty)
#
# Examples
#   make build
#   make tests RELAY_URL=https://relay.example.com:4433/moq
#   make tests RELAY_URL=https://localhost:9668/moq-relay   # localhost auto-rewritten
#   make tests TESTS_ARGS="--categories=Publisher,Subscriber"
#   make conformance CONFORMANCE_URL=http://relay.example.com:9999
#   make conformance CONFORMANCE_ARGS="16"                  # restrict to MoQT version 16
#   make build WITH_CONFORMANCE=0                           # omit conformance tooling
#   make push  REGISTRY=ghcr.io/myorg TAG=v1.2.3
#   make build DOCKER_BUILD_FLAGS="--progress=plain"        # verbose build log
# =============================================================================

# ── Image coordinates ────────────────────────────────────────────────────────────────────────────
IMAGE     ?= moq-interop
TAG       ?= latest
REGISTRY  ?=

ifeq ($(REGISTRY),)
  IMAGE_REF := $(IMAGE):$(TAG)
else
  IMAGE_REF := $(REGISTRY)/$(IMAGE):$(TAG)
endif

# ── Runtime parameters ────────────────────────────────────────────────────────────────────
RELAY_URL   ?= https://localhost:4433/moq
TESTS_ARGS  ?=

# Rewrite localhost / 127.0.0.1 → host.docker.internal so the container can
# reach a relay running on the host machine.  On macOS Docker Desktop
# host.docker.internal is provided automatically; the --add-host flag below
# makes it work on Linux as well.
RELAY_URL_DOCKER := $(subst 127.0.0.1,host.docker.internal,$(subst localhost,host.docker.internal,$(RELAY_URL)))

# ── Conformance test parameters ───────────────────────────────────────────────────────
# WITH_CONFORMANCE=1 (default) stages moqtest_client + conformance_test.sh in
# the image.  Set to 0 to build a smaller image without conformance tooling.
WITH_CONFORMANCE  ?= 1
CONFORMANCE_URL   ?= http://localhost:9999
CONFORMANCE_ARGS  ?=

CONFORMANCE_URL_DOCKER := $(subst 127.0.0.1,host.docker.internal,$(subst localhost,host.docker.internal,$(CONFORMANCE_URL)))

# ── Build parameters ────────────────────────────────────────────────────────────────────────────
# Auto-detect CPU count from the host so getdeps can parallelise its build.
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)

# Pass --progress=plain here to get verbose (non-TTY) Docker build output.
DOCKER_BUILD_FLAGS ?=

# ── Paths ─────────────────────────────────────────────────────────────────────────────────────
# Build context = the directory this Makefile lives in (moq-interop/).
BUILD_CTX  := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
DOCKERFILE := $(BUILD_CTX)/docker/Dockerfile

DOCKER := docker
# BuildKit is required for --mount=type=cache in the Dockerfile.
# It is the default builder since Docker 23.x; this export ensures it is
# active on any Docker version that supports it.
export DOCKER_BUILDKIT := 1

# ── Phony declarations ────────────────────────────────────────────────────────────────────────
.PHONY: all build tests conformance shell clean push help

all: tests

# ── build ─────────────────────────────────────────────────────────────────────────────
# The build context is moq-interop/ (this directory).  .dockerignore in the
# same directory excludes heavy artefacts (.scratch/, build/ dirs, etc.) so
# only source files are transferred to the Docker daemon.
build:
	$(DOCKER) build \
		$(DOCKER_BUILD_FLAGS) \
		--build-arg NPROC=$(NPROC) \
		--build-arg WITH_CONFORMANCE=$(WITH_CONFORMANCE) \
		-t $(IMAGE_REF) \
		-f $(DOCKERFILE) \
		$(BUILD_CTX)


# ── tests ─────────────────────────────────────────────────────────────────────────────
# Runs the interop tests against a relay at RELAY_URL.
# localhost/127.0.0.1 are automatically rewritten to host.docker.internal so
# the container can reach a relay running on the host machine.
# The relay must already be running before invoking this target.
tests: build
	$(DOCKER) run --rm \
		--name moq-interop-tests \
		--add-host=host.docker.internal:host-gateway \
		$(IMAGE_REF) --relay=$(RELAY_URL_DOCKER) $(TESTS_ARGS)

# ── interactive shell ───────────────────────────────────────────────────────────────
shell: build
	$(DOCKER) run --rm -it \
		--name moq-interop-shell \
		--add-host=host.docker.internal:host-gateway \
		$(IMAGE_REF) /bin/bash

# ── clean ─────────────────────────────────────────────────────────────────────────────────────
clean:
	$(DOCKER) rmi -f $(IMAGE_REF) 2>/dev/null || true

# ── push ──────────────────────────────────────────────────────────────────────────────────────
push: build
	$(DOCKER) push $(IMAGE_REF)

# ── conformance ───────────────────────────────────────────────────────────────────────────────
# Runs moxygen's conformance_test.sh against CONFORMANCE_URL.
# The image must have been built with WITH_CONFORMANCE=1 (the default).
# The relay/server must already be running before invoking this target.
conformance: build
	@if [ "$(WITH_CONFORMANCE)" = "0" ]; then \
		echo "Error: image built without conformance tooling (WITH_CONFORMANCE=0)." >&2; \
		echo "  Rebuild with: make build WITH_CONFORMANCE=1" >&2; \
		exit 1; \
	fi
	$(DOCKER) run --rm \
		--name moq-interop-conformance \
		--add-host=host.docker.internal:host-gateway \
		--entrypoint /opt/moq/conformance/run_conformance.sh \
		$(IMAGE_REF) $(CONFORMANCE_URL_DOCKER) $(CONFORMANCE_ARGS)

# ── help ──────────────────────────────────────────────────────────────────────────────────────
help:
	@printf '\nmoq-interop — available make targets\n\n'
	@printf '  %-14s  %s\n' build        'Build (or rebuild) the Docker image'
	@printf '  %-14s  %s\n' tests        'Run the interop tests against RELAY_URL'
	@printf '  %-14s  %s\n' conformance  'Run moxygen moqtest conformance tests against CONFORMANCE_URL'
	@printf '  %-14s  %s\n' shell        'Open an interactive shell in the container'
	@printf '  %-14s  %s\n' clean        'Remove the local Docker image'
	@printf '  %-14s  %s\n' push         'Push the image to REGISTRY/IMAGE:TAG'
	@printf '\nCurrent variable values\n\n'
	@printf '  %-28s  %s\n' IMAGE                  '$(IMAGE)'
	@printf '  %-28s  %s\n' TAG                    '$(TAG)'
	@printf '  %-28s  %s\n' REGISTRY               '$(if $(REGISTRY),$(REGISTRY),(none — local only))'
	@printf '  %-28s  %s\n' IMAGE_REF              '$(IMAGE_REF)'
	@printf '  %-28s  %s\n' RELAY_URL              '$(RELAY_URL)'
	@printf '  %-28s  %s\n' RELAY_URL_DOCKER       '$(RELAY_URL_DOCKER) (used inside container)'
	@printf '  %-28s  %s\n' TESTS_ARGS             '$(if $(TESTS_ARGS),$(TESTS_ARGS),(none))'
	@printf '  %-28s  %s\n' WITH_CONFORMANCE       '$(WITH_CONFORMANCE)'
	@printf '  %-28s  %s\n' CONFORMANCE_URL        '$(CONFORMANCE_URL)'
	@printf '  %-28s  %s\n' CONFORMANCE_URL_DOCKER '$(CONFORMANCE_URL_DOCKER) (used inside container)'
	@printf '  %-28s  %s\n' CONFORMANCE_ARGS       '$(if $(CONFORMANCE_ARGS),$(CONFORMANCE_ARGS),(none))'
	@printf '  %-28s  %s\n' NPROC                  '$(NPROC)'
	@printf '\nExamples\n\n'
	@printf '  make build\n'
	@printf '  make tests RELAY_URL=https://relay.example.com:4433/moq\n'
	@printf '  make tests RELAY_URL=https://localhost:9668/moq-relay\n'
	@printf '  make tests TESTS_ARGS="--categories=Publisher,Subscriber"\n'
	@printf '  make conformance CONFORMANCE_URL=http://relay.example.com:9999\n'
	@printf '  make conformance CONFORMANCE_ARGS="16"\n'
	@printf '  make build WITH_CONFORMANCE=0\n'
	@printf '  make push  REGISTRY=ghcr.io/myorg TAG=v1.2.3\n'
	@printf '  make build DOCKER_BUILD_FLAGS="--progress=plain"\n'
	@printf '\n'
