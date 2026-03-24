# =============================================================================
# moq-interop Makefile
# =============================================================================
#
# A single command builds and runs the full moq-interop suite anywhere Docker
# is available — no local C++ toolchain required.
#
# Targets
#   build     Build (or rebuild) the moq-interop Docker image.
#   run       Build → start relay → run all interop tests (DEFAULT).
#   relay     Run only the relay (foreground — Ctrl-C to stop).
#   tests     Run only the interop tests against RELAY_URL.
#   shell     Open an interactive shell inside the container.
#   clean     Remove the local Docker image.
#   push      Tag and push the image to REGISTRY/IMAGE:TAG.
#   help      Print this help text.
#
# Common variables (override on the command line)
#   IMAGE        Local image name              (default: moq-interop)
#   TAG          Image tag                     (default: latest)
#   REGISTRY     Registry prefix               (default: empty)
#   RELAY_PORT   Host port exposed for QUIC    (default: 4433)
#   RELAY_URL    Relay URL used by 'tests'     (default: https://localhost:4433/moq)
#   TESTS_ARGS   Extra flags for interop_tests (default: empty)
#
# Examples
#   make build
#   make run
#   make relay &                                        # relay in background
#   make tests RELAY_URL=https://relay.example.com:4433/moq
#   make run   TESTS_ARGS="--categories=Publisher,Subscriber"
#   make push  REGISTRY=ghcr.io/myorg TAG=v1.2.3
#   make build DOCKER_BUILD_FLAGS="--progress=plain"   # verbose build log
# =============================================================================

# ── Image coordinates ─────────────────────────────────────────────────────────
IMAGE     ?= moq-interop
TAG       ?= latest
REGISTRY  ?=

ifeq ($(REGISTRY),)
  IMAGE_REF := $(IMAGE):$(TAG)
else
  IMAGE_REF := $(REGISTRY)/$(IMAGE):$(TAG)
endif

# ── Runtime parameters ────────────────────────────────────────────────────────
RELAY_PORT  ?= 4433
RELAY_URL   ?= https://localhost:$(RELAY_PORT)/moq
TESTS_ARGS  ?=

# ── Build parameters ──────────────────────────────────────────────────────────
# Auto-detect CPU count from the host so getdeps can parallelise its build.
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)

# Pass --progress=plain here to get verbose (non-TTY) Docker build output.
DOCKER_BUILD_FLAGS ?=

# ── Paths ─────────────────────────────────────────────────────────────────────
# Build context = the directory this Makefile lives in (moq-interop/).
BUILD_CTX  := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
DOCKERFILE := $(BUILD_CTX)/docker/Dockerfile

DOCKER := docker

# ── Phony declarations ────────────────────────────────────────────────────────
.PHONY: all build run relay tests shell clean push help

all: run

# ── build ─────────────────────────────────────────────────────────────────────
# The build context is moq-interop/ (this directory).  .dockerignore in the
# same directory excludes heavy artefacts (.scratch/, build/ dirs, etc.) so
# only source files are transferred to the Docker daemon.
build:
	$(DOCKER) build \
		$(DOCKER_BUILD_FLAGS) \
		--build-arg NPROC=$(NPROC) \
		-t $(IMAGE_REF) \
		-f $(DOCKERFILE) \
		$(BUILD_CTX)

# ── run (relay + tests in one container) ──────────────────────────────────────
# The entrypoint starts moq_relay in the background, waits for it to bind,
# then runs the full interop_tests suite and exits with the test exit code.
run: build
	$(DOCKER) run --rm \
		--name moq-interop-run \
		-p $(RELAY_PORT):$(RELAY_PORT)/udp \
		-p $(RELAY_PORT):$(RELAY_PORT)/tcp \
		-e RELAY_PORT=$(RELAY_PORT) \
		$(IMAGE_REF) all $(TESTS_ARGS)

# ── relay only ────────────────────────────────────────────────────────────────
# Runs moq_relay in the foreground.  Press Ctrl-C to stop.
# Useful for manually testing against a running relay from the host.
relay: build
	$(DOCKER) run --rm \
		--name moq-relay \
		-p $(RELAY_PORT):$(RELAY_PORT)/udp \
		-p $(RELAY_PORT):$(RELAY_PORT)/tcp \
		-e RELAY_PORT=$(RELAY_PORT) \
		$(IMAGE_REF) relay

# ── tests only ────────────────────────────────────────────────────────────────
# Runs only interop_tests.  Requires a relay already reachable at RELAY_URL.
#
# Point at an external relay:
#   make tests RELAY_URL=https://relay.example.com:4433/moq
#
# Point at a relay running on the host (Docker bridge networking):
#   make tests RELAY_URL=https://host.docker.internal:4433/moq
tests: build
	$(DOCKER) run --rm \
		--name moq-interop-tests \
		$(IMAGE_REF) tests --relay=$(RELAY_URL) $(TESTS_ARGS)

# ── interactive shell ─────────────────────────────────────────────────────────
shell: build
	$(DOCKER) run --rm -it \
		--name moq-interop-shell \
		$(IMAGE_REF) /bin/bash

# ── clean ─────────────────────────────────────────────────────────────────────
clean:
	$(DOCKER) rmi -f $(IMAGE_REF) 2>/dev/null || true

# ── push ──────────────────────────────────────────────────────────────────────
push: build
	$(DOCKER) push $(IMAGE_REF)

# ── help ──────────────────────────────────────────────────────────────────────
help:
	@printf '\nmoq-interop — available make targets\n\n'
	@printf '  %-12s  %s\n' build  'Build (or rebuild) the Docker image'
	@printf '  %-12s  %s\n' run    'Build + start relay + run all interop tests (default)'
	@printf '  %-12s  %s\n' relay  'Run only the relay (foreground)'
	@printf '  %-12s  %s\n' tests  'Run only the interop tests (relay must be reachable)'
	@printf '  %-12s  %s\n' shell  'Open an interactive shell in the container'
	@printf '  %-12s  %s\n' clean  'Remove the local Docker image'
	@printf '  %-12s  %s\n' push   'Push the image to REGISTRY/IMAGE:TAG'
	@printf '\nCurrent variable values\n\n'
	@printf '  %-18s  %s\n' IMAGE        '$(IMAGE)'
	@printf '  %-18s  %s\n' TAG          '$(TAG)'
	@printf '  %-18s  %s\n' REGISTRY     '$(if $(REGISTRY),$(REGISTRY),(none — local only))'
	@printf '  %-18s  %s\n' IMAGE_REF    '$(IMAGE_REF)'
	@printf '  %-18s  %s\n' RELAY_PORT   '$(RELAY_PORT)'
	@printf '  %-18s  %s\n' RELAY_URL    '$(RELAY_URL)'
	@printf '  %-18s  %s\n' TESTS_ARGS   '$(if $(TESTS_ARGS),$(TESTS_ARGS),(none))'
	@printf '  %-18s  %s\n' NPROC        '$(NPROC)'
	@printf '\nExamples\n\n'
	@printf '  make build\n'
	@printf '  make run\n'
	@printf '  make relay                                     # relay in foreground\n'
	@printf '  make tests RELAY_URL=https://relay.example.com:4433/moq\n'
	@printf '  make run   TESTS_ARGS="--categories=Publisher,Subscriber"\n'
	@printf '  make push  REGISTRY=ghcr.io/myorg TAG=v1.2.3\n'
	@printf '  make build DOCKER_BUILD_FLAGS="--progress=plain"\n'
	@printf '\n'
