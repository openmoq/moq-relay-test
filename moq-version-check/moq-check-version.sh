#!/usr/bin/env bash
#
# moqt-check — MoQT Server Detection & Version Discovery Tool
#
# Wrapper script that automatically sets up a Python virtual environment,
# installs dependencies, and runs the moqt_check.py probe tool.
#
# Based on draft-ietf-moq-transport-16
# https://datatracker.ietf.org/doc/draft-ietf-moq-transport/16/
#
# Usage:
#   ./moqt-check <url> [options]
#
# Examples:
#   ./moqt-check moqt://relay.example.com:4443/moq
#   ./moqt-check https://cdn.example.com:443/moq --verbose
#   ./moqt-check relay.example.com:4443 --skip-wt
#   ./moqt-check --help
#
# Requirements: Python 3.10+ (Ubuntu 24.04 ships Python 3.12)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${SCRIPT_DIR}/.venv"
REQUIREMENTS="${SCRIPT_DIR}/requirements.txt"
TOOL="${SCRIPT_DIR}/moqt_check.py"

# ---------- Color helpers ----------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
NC='\033[0m'  # No Color

info()  { echo -e "${GREEN}[*]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# ---------- Preflight checks ----------

# Locate python3
PYTHON=""
for candidate in python3.12 python3.11 python3.10 python3; do
    if command -v "$candidate" &>/dev/null; then
        PYTHON="$candidate"
        break
    fi
done

if [[ -z "$PYTHON" ]]; then
    error "Python 3.10+ is required but not found."
    echo "  On Ubuntu 24.04:  sudo apt install python3 python3-venv python3-pip"
    exit 2
fi

# Check minimum version
PY_VERSION=$("$PYTHON" -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
PY_MAJOR=$("$PYTHON" -c 'import sys; print(sys.version_info.major)')
PY_MINOR=$("$PYTHON" -c 'import sys; print(sys.version_info.minor)')

if (( PY_MAJOR < 3 || (PY_MAJOR == 3 && PY_MINOR < 10) )); then
    error "Python 3.10+ is required (found ${PY_VERSION})."
    exit 2
fi

# ---------- Virtual environment ----------

if [[ ! -d "$VENV_DIR" ]]; then
    info "Creating Python virtual environment in ${VENV_DIR} ..."

    # Ensure python3-venv is available
    if ! "$PYTHON" -m venv --help &>/dev/null; then
        error "python3-venv is required."
        echo "  On Ubuntu:  sudo apt install python3-venv"
        exit 2
    fi

    "$PYTHON" -m venv "$VENV_DIR"
    info "Virtual environment created."
fi

# Activate
# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"

# ---------- Install dependencies ----------

# Quick check: is aioquic importable?
if ! python -c "import aioquic" &>/dev/null; then
    info "Installing dependencies (aioquic) ..."
    pip3 install --quiet --upgrade pip3
    pip3 install --quiet -r "$REQUIREMENTS"
    info "Dependencies installed."
fi

# ---------- Run the tool ----------

exec python "$TOOL" "$@"
