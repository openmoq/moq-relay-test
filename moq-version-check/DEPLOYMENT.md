# moq-version-check Deployment Guide

Container-first deployment guide for `moq-version-check`.

## Overview

This project is deployed as a Docker container running Gunicorn + Flask.

- App port in container: `8080`
- Default image name: `moq-version-check`
- Default target platform: `linux/amd64`
- Cross-arch builds from macOS are supported via Docker `buildx`

## Prerequisites

- Docker Desktop (or Docker Engine + Buildx)
- Docker Hub account (`docker login`) for image publishing
- Optional: Docker Compose plugin for local orchestration

## Build and Push (Linux AMD64)

From the project root:

```bash
# One-time setup
make builder

# Build linux/amd64 image into local Docker daemon
make build

# Push to Docker Hub
make push DOCKER_USER=<your-dockerhub-user> TAG=v1.0.0
```

### Defaults

- `PLATFORM=linux/amd64`
- `IMAGE_NAME=moq-version-check`
- `TAG=latest`
- `BUILDER_NAME=moq-version-check-builder`

## Local Runtime

### Using Makefile

```bash
# Run container on http://localhost:8080
make run

# Stop container
make stop
```

### Using Docker Compose

```bash
# Build and run in background
docker compose up -d --build

# Stream logs
docker compose logs -f

# Stop and remove
docker compose down
```

If your environment lacks Compose support, use the Makefile flow.

## Linux Host Deployment

On your Linux server:

```bash
# Pull published image
docker pull <your-dockerhub-user>/moq-version-check:v1.0.0

# Run service
docker run -d \
  --name moq-version-check \
  --restart unless-stopped \
  -p 8080:8080 \
  <your-dockerhub-user>/moq-version-check:v1.0.0
```

## Reverse Proxy (Optional)

Expose `moq-version-check` behind your preferred ingress/reverse proxy (Nginx, Traefik, cloud load balancer) and route external traffic to `http://<host>:8080`.

## Environment Variables

Supported runtime variables include:

- `PORT` (default `8080`)
- `WORKERS` (Gunicorn workers)
- `LOG_LEVEL` (default `info`)

Example:

```bash
docker run -d \
  --name moq-version-check \
  --restart unless-stopped \
  -p 8080:8080 \
  -e LOG_LEVEL=debug \
  -e WORKERS=4 \
  <your-dockerhub-user>/moq-version-check:v1.0.0
```

## Health and Logs

```bash
# Logs
docker logs -f moq-version-check

# Health endpoint
curl http://localhost:8080/health
```

## Updating

```bash
# Pull new image
docker pull <your-dockerhub-user>/moq-version-check:<new-tag>

# Recreate container
docker stop moq-version-check && docker rm moq-version-check
docker run -d \
  --name moq-version-check \
  --restart unless-stopped \
  -p 8080:8080 \
  <your-dockerhub-user>/moq-version-check:<new-tag>
```

## Uninstall

```bash
docker stop moq-version-check || true
docker rm moq-version-check || true
docker rmi moq-version-check:latest || true
```
