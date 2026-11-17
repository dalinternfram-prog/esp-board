#!/usr/bin/env bash
# Layer guard for the Ports & Adapters layout.
#
#   interface.hpp  <- core/ is the only folder allowed to include it
#   core/          pure C++ application (no ESP-IDF / FreeRTOS / WiFi / NVS)
#   general/       portable helpers (KCP, crypto, codecs) - no core/esp_hardware deps
#   esp_hardware/  the only place allowed to touch ESP-IDF
#
# Run from anywhere; exits non-zero on violations.

set -u
cd "$(dirname "$0")/.."

fail=0
PATTERN='esp_[a-z]|freertos/|driver/|nvs_|nvs\.h|wifi::|psa/|esp_ota'

check_pure() {
    local dir="$1" label="$2"
    [ -d "$dir" ] || return 0

    local hits
    hits=$(grep -rnE "$PATTERN" "$dir" \
               --include='*.cpp' --include='*.hpp' --include='*.h' 2>/dev/null \
           | grep -vE '^[^:]+:[0-9]+:[[:space:]]*(//|\*|/\*)' || true)

    if [ -n "$hits" ]; then
        echo "[FAIL] $label has platform leaks:"
        echo "$hits" | sed 's/^/       /'
        fail=1
    else
        echo "[ok]   $label"
    fi
}

check_pure main/core    "core     (pure C++)"
check_pure main/app     "app      (pure C++)"
check_pure main/general "general  (portable)"

# general/ is a leaf: it must never depend on the app core or the hardware layer.
if [ -d main/general ]; then
    hits=$(grep -rnE '#include[[:space:]]*"(interface\.hpp|core/|esp_hardware/)' main/general \
               --include='*.cpp' --include='*.hpp' --include='*.h' 2>/dev/null || true)
    if [ -n "$hits" ]; then
        echo "[FAIL] general depends on interface/core/esp_hardware:"
        echo "$hits" | sed 's/^/       /'
        fail=1
    fi
fi

if [ "$fail" -eq 0 ]; then
    echo "[ok]   layer rules satisfied"
fi
exit "$fail"
