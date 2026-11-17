# architecture

Ports & Adapters (hexagonal) with four folders. Dependencies point **inward**:
the application core knows nothing about ESP-IDF, FreeRTOS, KCP, WiFi or the board.

```
main/
  interface.hpp        ports: the contract every layer speaks
  core/                application + domain (pure C++)
    app/               prog (registration/dispatch), audio_service, net worker
    session/           SessionManager (handshake, liveness, reconnect)
    audio/             AudioPipeline (capture -> encode -> emit)
    device/ storage/   thin facades over IDeviceInfo / IStorage / IFileStore
  general/             portable libraries (no core/ or esp_hardware/ deps)
    kcp/               ikcp
    crypto/            PSA AES-256-GCM + ECDH P-256
    opus/              Opus wrapper
  esp_hardware/        the ONLY folder allowed to touch ESP-IDF
    hardware.hpp/.cpp  boot() + board bring-up (HARDWARE_HEADER in main.cpp)
    app_context.*      composition root: concrete adapters + session
    adapters/          port implementations
    drivers/           bsp, i2s, buttons, led, speech, io expander
    wifi/              WiFi manager / provisioning
    net/kcp/           KCP client + UDP socket
    storage/           NVS, LittleFS, OTA, AES key store
  main.cpp             thin entry: hardware::boot() + start core/app task
```

`main/CMakeLists.txt` adds `main/`, `main/general` and `main/esp_hardware` as
include roots, so includes use stable prefixes (`interface.hpp`, `core/...`,
`general/...`, `esp_hardware/...`) regardless of where a file lives.

## Ports (`main/interface.hpp`)

| Port | Responsibility | Today | Tomorrow |
|---|---|---|---|
| `ILink` | start the path + report usable link | `EspWifiLink` | 4G / Ethernet |
| `ITransport` | move bytes to/from server; own conv id | `KcpUdpTransport` | PPP / TCP |
| `ICrypto` | ECDH + AES-GCM | `EspCrypto` | other backend |
| `IClock` | monotonic ms + unix ms | `esp_timer` / `gettimeofday` | any OS |
| `IRandom` | session id | `esp_random` | any OS |
| `ILogger` | logging (`log` / `logf`) | `ESP_LOGx` | any OS |
| `IDeviceInfo` | MAC / chip / device id | `EspDeviceInfo` | any board |
| `IStorage` | small blob key-value | `EspNvsStorage` | any KV store |
| `IFileStore` | mount + file persistence | `EspFileStore` | any FS |
| `IAudioSource` | open/close/read PCM | `EspI2sAudioSource` | any mic |
| `IAudioEncoder` | audio codec | `OpusEncoder` | LC3 / other |
| `ITaskRunner` | create/stop/delay/exit tasks | `EspTaskRunner` (FreeRTOS) | `std::thread` / RTOS |
| `IFirmwareUpdater` | OTA init / apply | `EspFirmwareUpdater` | SD / serial |

## Layer rules (enforced)

`tools/check_layers.sh` (grep gate):

* `core/` never mentions ESP-IDF, FreeRTOS, NVS, WiFi or PSA.
* `general/` never mentions ESP-IDF, FreeRTOS, NVS or WiFi (PSA/Opus/KCP allowed).
* `general/` never includes `interface.hpp`, `core/` or `esp_hardware/`.

`tools/host_check.sh` (host compiler):

* `core/audio` + `core/session` compile with `-fsyntax-only` on the host.
* `general/kcp/ikcp.c` compiles as C99.
* `general/crypto` and `general/opus` need PSA/libopus headers, so they are
  validated by the IDF build.

Run both after every change. Run `idf.py reconfigure` after adding a source
directory (the CMake glob has no `CONFIGURE_DEPENDS`).

## Domain core (`main/core`)

* `session/SessionManager` - handshake / liveness / reconnect / encryption:
  ```
  IDLE --link up--> HANDSHAKING(WAIT_ACK) --peer key--> ESTABLISHED
  ESTABLISHED --no RX for 8s--> IDLE (reset transport => fresh KCP sequence numbers)
  any --link down--> IDLE (reset transport)
  ```
  * handshake retries forever with backoff 600ms -> 3s (server may boot late)
  * session reset recreates the transport, so a restarted server and the client
    start from sequence number 0 again (no KCP desync)
  * outbound data queued (max 128) and dropped when not established
    (real-time audio: dropping is better than buffering)
* `audio/AudioPipeline` - capture -> encode -> emit `(compressed, timestampMs)`.

## Application (`main/core/app`)

* `prog.cpp` - device registration + server message dispatch. Uses only
  `app::context()` ports; no ESP-IDF/FreeRTOS includes.
* `audio_service.cpp` - start/stop the capture task through `ITaskRunner`;
  record acks use `IClock::nowUnixMs()`.
* `net_manager.cpp` - network worker task through `ITaskRunner`; delegates
  protocol work to `core::SessionManager`.

## Hardware (`main/esp_hardware`)

* `hardware.cpp` - board bring-up: BSP/I2S, IO expander, SD card, playback,
  buttons, speech engine and music-library scan. `main.cpp` includes it through
  the `HARDWARE_HEADER` macro.
* `app_context.*` - composition root; the only place that names concrete
  adapters and owns the network session.
* `adapters/` - `KcpUdpTransport`, `EspWifiLink`, `EspCrypto`, `EspClock`,
  `EspRandom`, `EspLogger`, `EspTaskRunner`, `EspDeviceInfo`,
  `EspNvsStorage`, `EspFileStore`, `EspFirmwareUpdater`,
  `EspI2sAudioSource`, `OpusEncoder`.
* `drivers/`, `wifi/`, `net/kcp/`, `storage/` - the ESP-IDF implementation
  details behind those adapters.

## Composition root

`main/esp_hardware/app_context.{hpp,cpp}` -> `app::context()` owns every
concrete adapter and the network session. Modules (`Network`, `device::info()`,
`storage::nvs()/files()`) delegate to it; no scattered module singletons.

**Swapping hardware** means replacing `main/esp_hardware/` (or pointing
`HARDWARE_HEADER` at another folder). `main/core/` and `main/general/` stay
untouched; `main/main.cpp` only calls `hardware::boot()` and starts the
application task.

## Migration status

- [x] Phase 0/1: ports, SessionManager, KcpUdpTransport, liveness + KCP reset.
- [x] Phase 2: `IDeviceInfo` + handshake gated on got-IP.
- [x] Phase 3: `IAudioSource` / `IAudioEncoder` + `AudioPipeline`.
- [x] Phase 4: `IStorage` / `IFileStore` + adapters.
- [x] Phase 5: `AppContext` composition root; `ITaskRunner`; `IFirmwareUpdater`.
- [x] Four-folder reorg: `interface.hpp` / `core/` / `general/` / `esp_hardware/`.
- [x] Gates: `tools/check_layers.sh` + `tools/host_check.sh`.
- [x] Dead code removed: duplicate protocols, `core/ports.hpp` shim, test blocks.
- [ ] Next: 4G adapter implementing `ILink`/`ITransport`;
      `IFirmwareUpdater::applyFromUrl()` streaming.

## Portability snapshot

| Layer | survives an MCU swap |
|---|---|
| `components/proto` | 100% |
| `core/` (session, pipeline, app) | 100% |
| `general/` (ikcp, crypto, opus) | ~95% (PSA/libopus backends) |
| `esp_hardware/` (WiFi, socket, I2S, NVS, clock) | 0% (isolated, mechanical) |
