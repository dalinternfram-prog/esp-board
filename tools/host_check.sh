#!/usr/bin/env bash
# Proves that the portable layers compile on the host (no ESP-IDF, no Xtensa):
#   * core/app (prog, audio_service, network_service, media_controller, session_factory)
#   * core/audio, core/session
#   * every general/ pure source + ikcp.c (C99)
#   * the POSIX UDP DatagramIo adapter
#
# Built as C++20 because components/proto uses C++20 features (FixedString NTTP).
# general/crypto and general/opus need PSA/libopus headers, so they are
# validated by the IDF build; check_layers.sh keeps them free of ESP-IDF.

set -u
cd "$(dirname "$0")/.."

CXX=${CXX:-g++}
CC=${CC:-gcc}
fail=0

CXXFLAGS=(
    -std=c++20 -Imain
    -Icomponents/proto/lib
    -Icomponents/proto/lib/IoT
    -Icomponents/proto/lib/tool
    -Icomponents/proto/lib/phone
)

PURE_SOURCES="
main/core/app/media_controller.cpp
main/core/app/network_service.cpp
main/core/app/session_factory.cpp
main/core/app/prog.cpp
main/core/app/audio_service.cpp
main/esp_hardware/net/esp_udp_datagram_io.cpp"

for f in $PURE_SOURCES $(find main/core/audio main/core/session main/general/io main/general/fs main/general/input main/general/kcp -name '*.cpp' | sort); do
    if "$CXX" "${CXXFLAGS[@]}" -fsyntax-only "$f" 2>/tmp/host_check.err; then
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

[ "$fail" -eq 0 ] && echo "[ok]   core/ + general/ are portable"
exit "$fail"
