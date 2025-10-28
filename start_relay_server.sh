#!/bin/bash

# Script to start MOQ relay server on port 4433
# This script sets up the environment and runs the relay server

echo "Starting MOQ Relay Server on port 4433..."

# Change to the moxygen directory
cd moqt_implementation/moxygen

# Set up the environment
eval "$(./build/fbcode_builder/getdeps.py env moxygen)"

# Get the installation directory
INSTALL_DIR="$(./build/fbcode_builder/getdeps.py show-inst-dir moxygen)"

# Run the relay server with certificates and logging
"$INSTALL_DIR/bin/moqrelayserver" \
    -port 4433 \
    -cert ../certs/certificate.pem \
    -key ../certs/certificate.key \
    -endpoint "/moq" \
    -logtostderr \
    --logging DBG1