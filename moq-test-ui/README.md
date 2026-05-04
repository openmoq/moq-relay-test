# MoQ Test UI

Container-first quick start for launching the UI.

For implementation details and structure, see [DESIGN.md](./DESIGN.md).

## Quick Start

- Docker
- Docker Compose
- TLS certs mounted at `./certs/fullchain.pem` and `./certs/privkey.pem`

Start the stack:

```bash
docker compose up -d
```

Then open the UI over HTTPS.

## What Starts

`docker compose up` launches:

- `generic-runner`: the Node.js UI server
- `nginx`: TLS termination and reverse proxy

The UI container mounts the Docker socket so it can start the underlying test-tool containers on demand.

## Local Development

If you want to run the UI without the nginx container:

```bash
npm install
npm start
```

Open http://localhost:3000.

## CI/CD Workflow (Implemented)

The project now includes an implemented GitHub Actions workflow at [../.github/workflows/moq-test-ui-image.yml](../.github/workflows/moq-test-ui-image.yml).

It runs on pushes to `main`, version tags (`v*`), pull requests that touch `moq-test-ui`, and manual dispatch.

Implemented jobs:

- `test`: installs dependencies and smoke-tests the Node server (`/api/tools`)
- `docker`: builds Docker image from `moq-test-ui/Dockerfile`
	- pull requests: build `linux/amd64` image and run container smoke test
	- `main`/tags: build `linux/amd64` and push to GHCR

The compose file defaults to:

- `ghcr.io/openmoq/moq-test-ui:latest`

You can override with `GENERIC_RUNNER_IMAGE` if needed.

End-user launch remains:

```bash
docker compose up -d
```