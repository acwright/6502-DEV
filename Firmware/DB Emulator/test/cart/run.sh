#!/usr/bin/env bash
# Builds and runs the Cart tests on the host. No Teensy, no PlatformIO.
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"
mkdir -p build

c++ -std=gnu++17 -Wall -Wextra -O1 \
    -I shim -I ../../lib/6502 -I ../../lib/6502/Cart \
    test_cart.cpp ../../lib/6502/Cart/Cart.cpp \
    -o build/test_cart

./build/test_cart
