#!/usr/bin/env bash
# Builds and runs the host unit tests for the portable core.
set -u
cd "$(dirname "$0")/.."

CXX=${CXX:-g++}
OUT=${TMPDIR:-/tmp}/factory_01_host_tests

"$CXX" -std=c++20 -Imain -Wall -Wextra \
    tools/host_tests/host_tests.cpp \
    main/core/audio/AudioPipeline.cpp \
    main/core/session/SessionManager.cpp \
    main/general/input/button_state_machine.cpp \
    -o "$OUT" || exit 1

"$OUT"
