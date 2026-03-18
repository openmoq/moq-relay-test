# MoQT Implementation

This directory contains the MoQT relay implementation and related components.

## Directory Structure

```
moqt_implementation/
├── moxygen/
│   ├── moxygen/          # Git submodule: Facebook's moxygen MoQ relay implementation
│   └── certs/            # Certificate files for local testing
└── README.md             # This file
```

## Setup Instructions

### Cloning with Submodule

When cloning this repository for the first time, use the `--recursive` flag to automatically initialize and clone submodules:

```bash
git clone --recursive https://github.com/facebookexperimental/moq-relay-test.git
```

### Initializing Submodules (if not cloned recursively)

If you already have the repository cloned, initialize and update submodules:

```bash
git submodule update --init --recursive
```

### Updating Submodule to Latest Changes

To fetch the latest changes from the moxygen submodule:

```bash
git submodule update --remote moqt_implementation/moxygen/moxygen
```

## Certificate Setup

The `moxygen/certs/` directory contains TLS certificates required for local testing:

- `certificate.pem` — Server certificate
- `certificate.key` — Private key

### Important: Certificates Not in Git

The certificate files are **not tracked in git** (see `.gitignore`) to ensure sensitive credentials are not accidentally committed. 

### Using Your Own Certificates

To use your own certificates:

1. Place your certificate files in `moqt_implementation/moxygen/certs/`:
   ```bash
   cp your_cert.pem moqt_implementation/moxygen/certs/certificate.pem
   cp your_key.key moqt_implementation/moxygen/certs/certificate.key
   ```

2. Update permissions (if needed):
   ```bash
   chmod 600 moqt_implementation/moxygen/certs/certificate.key
   ```

### Generating Test Certificates

To generate self-signed certificates for local testing:

```bash
openssl req -x509 -newkey rsa:2048 -keyout moqt_implementation/moxygen/certs/certificate.key \
  -out moqt_implementation/moxygen/certs/certificate.pem -days 365 -nodes \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=localhost"
```

## Building Moxygen

To build the moxygen relay implementation, refer to the [moxygen README](moxygen/moxygen/README.md) for detailed build instructions.

Quick start from the moxygen directory:

```bash
cd moqt_implementation/moxygen/moxygen
./build.sh
```

## Running Tests

For information on running interoperability tests, see the [moq-interop_tests README](../moq-interop_tests/README.md).
