#!/usr/bin/env bash
# Builds and runs the Storage Card tests on the host. No Teensy, no PlatformIO.
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"
mkdir -p build

c++ -std=gnu++17 -Wall -Wextra -O1 \
    -I shim -I ../../lib/6502 -I ../../lib/6502/IO \
    test_storage.cpp ../../lib/6502/IO/StorageCard.cpp \
    -o build/test_storage

./build/test_storage
