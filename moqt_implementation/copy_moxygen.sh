#!/bin/bash

# Script to fetch moxygen code repository from Facebook Experimental
# Repository: https://github.com/facebookexperimental/moxygen

set -e  # Exit on any error

# Configuration
REPO_URL="https://github.com/facebookexperimental/moxygen.git"
TARGET_DIR="moxygen"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== Moxygen Repository Fetcher ==="
echo "Script directory: $SCRIPT_DIR"
echo "Target directory: $TARGET_DIR"
echo "Repository URL: $REPO_URL"
echo

# Check if git is installed
if ! command -v git &> /dev/null; then
    echo "Error: git is not installed. Please install git first."
    exit 1
fi

# Change to the script directory
cd "$SCRIPT_DIR"

# Remove existing directory if it exists
if [ -d "$TARGET_DIR" ]; then
    echo "Warning: Directory '$TARGET_DIR' already exists."
    read -p "Do you want to remove it and clone fresh? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Removing existing directory..."
        rm -rf "$TARGET_DIR"
    else
        echo "Aborted. Existing directory preserved."
        exit 0
    fi
fi

# Clone the repository
echo "Cloning moxygen repository..."
git clone "$REPO_URL" "$TARGET_DIR"

if [ $? -eq 0 ]; then
    echo "Successfully cloned moxygen repository to '$TARGET_DIR'"
    echo
    echo "Repository information:"
    cd "$TARGET_DIR"
    echo "- Location: $(pwd)"
    echo "- Latest commit: $(git log -1 --oneline)"
    echo "- Branch: $(git branch --show-current)"

    # Show directory contents
    echo
    echo "Contents of the moxygen directory:"
    ls -la
else
    echo "Failed to clone the repository"
    exit 1
fi

echo
echo "=== Done ==="