# MoQ Test UI

Barebones quick start for the test UI.

## Prerequisites

- Node.js 22+
- Docker running locally

## Install

```bash
npm install
```

## Start

```bash
npm start
```

Open http://localhost:3000.

## Docker Compose

If you want to run it behind nginx instead:

```bash
docker compose up --build
```

This expects TLS certs in `./certs`.