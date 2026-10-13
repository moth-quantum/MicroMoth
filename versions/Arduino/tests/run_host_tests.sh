#!/bin/sh
# Builds and runs the MicroMoth Arduino tests on a computer, using a stand-in for the Arduino core.
# The flags match the Arduino AVR build: C++11, no exceptions, no RTTI, size optimisation.
# Set SANITIZE=1 to also check memory and undefined behaviour with AddressSanitizer and UBSan.
set -e
cd "$(dirname "$0")"
CXX="${CXX:-c++}"
FLAGS="-std=gnu++11 -Os -fno-exceptions -fno-rtti -Wall -Wextra -pedantic -Werror"
if [ "${SANITIZE:-0}" = "1" ]; then
  FLAGS="$FLAGS -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all"
fi
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
$CXX $FLAGS -I host -I .. host_tests.cpp second_unit.cpp -o "$OUT/host_tests"
"$OUT/host_tests"
$CXX $FLAGS -I host -I .. example_run.cpp -o "$OUT/example_run"
"$OUT/example_run"
