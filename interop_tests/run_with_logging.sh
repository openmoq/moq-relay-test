#!/bin/bash

# Script to run the interop test framework with detailed logging

# Set Folly logging configuration
export FOLLY_LOG_LEVEL=DBG1

# Set specific logger levels
export GLOG_v=1
export GLOG_logtostderr=1

# Run the test
echo "Running interop test with detailed logging..."
echo "Log level: DBG1"
echo "----------------------------------------"

./build/bin/interop_test_runner "$@"