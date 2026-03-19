# ─────────────────────────────────────────────────────────────────────────────
# moq-version-check  –  Docker build / push helper
#
# Usage:
#   make              # build linux/amd64 image locally
#   make build        # same as above
#   make push         # build AND push to Docker Hub
#   make run          # run the container locally on port 8080
#   make cli          # run CLI probe inside container (pass ARGS="...")
#   make stop         # stop the running container
#   make clean        # remove the local image
#   make builder      # one-time setup of the buildx builder
#
# Override defaults on the command line:
#   make push DOCKER_USER=myusername TAG=v1.2.3
# ─────────────────────────────────────────────────────────────────────────────

DOCKER_USER  ?= $(shell docker info --format '{{.AuthConfig.Username}}' 2>/dev/null)
IMAGE_NAME   ?= moq-version-check
TAG          ?= latest
PLATFORM     ?= linux/amd64
BUILDER_NAME ?= moq-version-check-builder
PORT         ?= 8080
ARGS         ?=

FULL_IMAGE   := $(DOCKER_USER)/$(IMAGE_NAME):$(TAG)
LOCAL_IMAGE  := $(IMAGE_NAME):$(TAG)

.PHONY: all build push run cli stop clean builder help

all: build

## build  –  compile a linux/amd64 image and load it into the local Docker daemon
build: _require-buildx
	docker buildx build \
		--builder $(BUILDER_NAME) \
		--platform $(PLATFORM) \
		--tag $(LOCAL_IMAGE) \
		--load \
		.
	@echo ""
	@echo "Built: $(LOCAL_IMAGE) ($(PLATFORM))"

## push  –  build AND push to Docker Hub as $(DOCKER_USER)/$(IMAGE_NAME):$(TAG)
push: _require-buildx _require-docker-user
	docker buildx build \
		--builder $(BUILDER_NAME) \
		--platform $(PLATFORM) \
		--tag $(FULL_IMAGE) \
		--push \
		.
	@echo ""
	@echo "Pushed: $(FULL_IMAGE)"

## run  –  start the container locally
run:
	docker run --rm \
		--name $(IMAGE_NAME) \
		-p $(PORT):8080 \
		$(LOCAL_IMAGE)

## cli  –  run the probe CLI inside the container image (use ARGS="<url> [options]")
cli:
	@test -n "$(ARGS)" || { \
		echo ""; \
		echo "ERROR: ARGS is required."; \
		echo "Usage:"; \
		echo "  make cli ARGS='https://relay.example.com/moq-relay --verbose'"; \
		echo ""; \
		exit 1; \
	}
	docker run --rm \
		$(LOCAL_IMAGE) \
		python3 /app/src/moq-version-check.py $(ARGS)

## stop  –  stop the locally running container
stop:
	docker stop $(IMAGE_NAME) 2>/dev/null || true

## clean  –  remove the local image
clean:
	docker rmi $(LOCAL_IMAGE) 2>/dev/null || true
	@echo "Removed $(LOCAL_IMAGE)"

## builder  –  create the buildx builder (run once after installing Docker Desktop)
builder:
	docker buildx inspect $(BUILDER_NAME) >/dev/null 2>&1 \
		|| docker buildx create --name $(BUILDER_NAME) --driver docker-container --bootstrap
	docker buildx use $(BUILDER_NAME)
	@echo "Builder '$(BUILDER_NAME)' is ready."

## help  –  list available targets
help:
	@grep -E '^##' $(MAKEFILE_LIST) | sed 's/^## /  /'

_require-buildx:
	@docker buildx inspect $(BUILDER_NAME) >/dev/null 2>&1 || { \
		echo ""; \
		echo "ERROR: buildx builder '$(BUILDER_NAME)' not found."; \
		echo "       Run 'make builder' once to create it."; \
		echo ""; \
		exit 1; \
	}

_require-docker-user:
	@test -n "$(DOCKER_USER)" || { \
		echo ""; \
		echo "ERROR: DOCKER_USER is not set."; \
		echo "       Run 'docker login' or pass it explicitly:"; \
		echo "         make push DOCKER_USER=yourhubusername"; \
		echo ""; \
		exit 1; \
	}
