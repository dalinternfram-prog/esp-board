#!/usr/bin/env bash
# Layer guard for the Ports & Adapters layout.
#
#   interface.hpp  the contract: core/ + esp_hardware/ + main/ may include it
#   core/          pure C++ application (no ESP-IDF / FreeRTOS / WiFi / NVS)
#   general/       portable helpers (KCP, crypto, codecs) - leaf; no core/hardware deps
#   esp_hardware/  the only place allowed to touch ESP-IDF; never includes core/
#   main/          composition root (app_context + main.cpp); may include everything
#
# Run from anywhere; exits non-zero on violations.

set -u
cd "$(dirname "$0")/.."

fail=0
# core/app must be fully portable (PSA included); general may use portable
# libraries such as PSA/Opus/KCP, but never ESP-IDF or FreeRTOS.
PATTERN_APP='esp_[a-z]|freertos/|driver/|nvs_|nvs\.h|wifi::|psa/|esp_ota'
PATTERN_GENERAL='esp_[a-z]|freertos/|driver/|nvs_|nvs\.h|wifi::|esp_ota'

check_pure() {
    local dir="$1" label="$2" pattern="$3"
    [ -d "$dir" ] || return 0

    local hits
    hits=$(grep -rnE "$pattern" "$dir" \
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

check_pure main/core    "core     (pure C++)"  "$PATTERN_APP"
check_pure main/general "general  (portable)"  "$PATTERN_GENERAL"

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

# core/ must never include the composition root or the hardware layer.
hits=$(grep -rnE '#include[[:space:]]*"(app_context\.hpp|esp_hardware/|adapters/)' main/core \
           --include='*.cpp' --include='*.hpp' --include='*.h' 2>/dev/null || true)
if [ -n "$hits" ]; then
    echo "[FAIL] core depends on the composition root / hardware layer:"
    echo "$hits" | sed 's/^/       /'
    fail=1
else
    echo "[ok]   core     (no composition/hardware includes)"
fi

# esp_hardware/ may use interface.hpp + general/ + ESP-IDF only; never core/.
hits=$(grep -rnE '#include[[:space:]]*"core/' main/esp_hardware \
           --include='*.cpp' --include='*.hpp' --include='*.h' 2>/dev/null || true)
if [ -n "$hits" ]; then
    echo "[FAIL] esp_hardware depends on core/ (use interface.hpp or general/):"
    echo "$hits" | sed 's/^/       /'
    fail=1
else
    echo "[ok]   esp_hardware (no core/ includes)"
fi

if [ "$fail" -eq 0 ]; then
    echo "[ok]   layer rules satisfied"
fi
exit "$fail"
