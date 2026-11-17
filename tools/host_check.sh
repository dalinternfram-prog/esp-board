#!/usr/bin/env bash
# Proves that core/ compiles without ESP-IDF, and that the portable KCP core
# is valid C.
#
# general/crypto and general/opus need external headers (PSA / libopus), so they
# are validated by the IDF build; check_layers.sh keeps them free of ESP-IDF.

set -u
cd "$(dirname "$0")/.."

CXX=${CXX:-g++}
CC=${CC:-gcc}
fail=0

for f in $(find main/core -name '*.cpp' | sort); do
    if "$CXX" -std=c++17 -Imain -fsyntax-only "$f" 2>/tmp/host_check.err; then
        echo "[ok]   host compile: $f"
    else
        echo "[FAIL] host compile: $f"
        sed 's/^/       /' /tmp/host_check.err
        fail=1
    fi
done

if [ -f main/general/kcp/ikcp.c ]; then
    if "$CC" -std=c99 -Imain/general/kcp -fsyntax-only main/general/kcp/ikcp.c 2>/tmp/host_check.err; then
        echo "[ok]   host compile: main/general/kcp/ikcp.c"
    else
        echo "[FAIL] host compile: main/general/kcp/ikcp.c"
        sed 's/^/       /' /tmp/host_check.err
        fail=1
    fi
fi

[ "$fail" -eq 0 ] && echo "[ok]   core/ is portable"
exit "$fail"

