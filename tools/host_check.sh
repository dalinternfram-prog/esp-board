#!/usr/bin/env bash
# Proves that core/ (and later general/) compile without ESP-IDF.
# Uses the host compiler with -fsyntax-only, so no Xtensa toolchain is needed.

set -u
cd "$(dirname "$0")/.."

CXX=${CXX:-g++}
SCAN=(main/core)
[ -d main/general ] && SCAN+=(main/general)

fail=0
for f in $(find "${SCAN[@]}" -name '*.cpp' | sort); do
    if "$CXX" -std=c++17 -Imain -fsyntax-only "$f" 2>/tmp/host_check.err; then
        echo "[ok]   host compile: $f"
    else
        echo "[FAIL] host compile: $f"
        sed 's/^/       /' /tmp/host_check.err
        fail=1
    fi
done

[ "$fail" -eq 0 ] && echo "[ok]   core/ is portable"
exit "$fail"
