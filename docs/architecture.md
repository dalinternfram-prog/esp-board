# architecture

Ports & Adapters (hexagonal) with four folders. Dependencies point **inward**:
the application core knows nothing about ESP-IDF, FreeRTOS, KCP, WiFi or the board.

```
main/
  interface.hpp        ports + AudioConfig + IAppServices + ITransportFactory
  main.cpp             thin entry: hardware::boot(config) + start core/app task
  app_context.*        composition root (plain class) -> core::IAppServices
  core/                application + domain (pure C++)
    app/               prog, audio_service, net worker, media_controller,
                       session_factory
    session/           SessionManager (handshake, liveness, reconnect)
    audio/             AudioPipeline
  general/             portable libraries (no core/ or esp_hardware/ deps)
    kcp/               ikcp + KcpSession + DatagramIo
    crypto/            PSA AES-256-GCM + ECDH P-256 (backend injected)
    opus/              Opus wrapper
    audio/             frame math
    io/                path join, stdio file store, random-access file
    fs/                directory lister
    input/             button debounce / long-press state machine
    net/               POSIX UDP socket helpers (getaddrinfo/sendto/recvfrom)
    util/              MAC / device-id formatting
  esp_hardware/        the ONLY folder allowed to touch ESP-IDF
    hardware.hpp/.cpp  boot(const core::AudioConfig&) + board bring-up
    adapters/          port implementations (incl. media/UI inputs)
    drivers/           bsp, i2s, buttons, led, speech, io expander
    wifi/              WiFi manager / provisioning
    net/               UDP DatagramIo adapter (POSIX socket code is in general/net)
    storage/           LittleFS mount + OTA backend
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
| `IAudioSource` | open/close/read one raw PCM frame | `EspI2sAudioSource` | any mic |
| `IAudioEncoder` | encode raw PCM frame bytes | `OpusEncoder` | LC3 / other |
| `ITaskRunner` | create/stop/delay/exit tasks | `EspTaskRunner` (FreeRTOS) | `std::thread` / RTOS |
| `IFirmwareUpdater` | OTA init / apply | `EspFirmwareUpdater` | SD / serial |
| `IMediaPlayer` | play/pause/stop/volume | `EspMediaPlayer` | any player |
| `IIndicator` | RGB / display status | `EspIndicator` | any UI |
| `IButtonInput` | physical buttons | `EspButtonInput` | any input |
| `ISpeechInput` | voice commands | `EspSpeechInput` | any ASR |
| `IPlaylist` | track listing | `EspSdPlaylist` | any FS |
| `ITransportFactory` | create the session transport | `AppContext` | any transport |

`IAppServices` (also in `interface.hpp`) bundles references to every port above
plus `ITransportFactory`; `core/app` receives it by reference. `AudioConfig`
carries the sample rate / channels / bit depth / frame duration.

Audio frames cross the ports as raw `Bytes`: `IAudioSource::readFrame(Bytes&)`
returns exactly the bytes the I2S/codec configuration produces, and
`IAudioEncoder::encode(const Bytes&, Bytes&)` consumes them. The slot layout
(2 x 16-bit vs 4 x 16-bit vs 32-bit words) is a board/encoder contract, not
something `AudioPipeline` re-interprets as `int16` samples.

## General logic rescued from `esp_hardware`

Deleting `main/esp_hardware/` now only removes board glue; these behaviours are
compiled from `core/` and `general/`:

| Logic | Lives in | Hardware counterpart |
|---|---|---|
| KCP session, tuning, reassembly, liveness | `general/kcp/KcpSession` | `net/EspUdpDatagramIo` (over `general/net` POSIX sockets) |
| playlist, volume clamp, play/pause, voice feedback | `core/app/MediaController` | `hardware.cpp` + drivers |
| button debounce / short / long press | `general/input/ButtonStateMachine` | `drivers/button` |
| directory scan + extension filter | `general/fs/DirectoryLister` | `drivers/bsp` |
| path join, stdio read/write | `general/io/Path`, `general/io/StdioFileStore` | `adapters/storage/EspFileStore` |
| random-access file ops (offset/append/shift/truncate) | `general/io/RandomAccessFile` | `storage/file_manager` |
| MAC / device-id formatting | `general/util/format.hpp` | `adapters/device/EspDeviceInfo` |
| session wiring / conv id | `core/app/session_factory` + `Network::init` | `AppContext::create` |
| audio frame sizing | `general/audio/frame_math.hpp` | `drivers/audio_capture` |

## Layer rules (enforced)

`tools/check_layers.sh` (grep + include gate):

* `core/` never mentions ESP-IDF, FreeRTOS, NVS, WiFi or PSA.
* `core/` never includes `app_context.hpp`, `esp_hardware/` or `adapters/`.
* `general/` never mentions ESP-IDF, FreeRTOS, NVS or WiFi. PSA/Opus/KCP are
  allowed: they are portable libraries, not ESP-IDF headers.
* `general/` never includes `interface.hpp`, `core/` or `esp_hardware/`.
* `esp_hardware/` never includes `core/`; it may use `interface.hpp`, `general/`
  and ESP-IDF.
* `main/` (`app_context`, `main.cpp`) is the composition root and may include
  everything.

`tools/host_check.sh` (host compiler, C++20):

* `core/app` (incl. `prog.cpp` and `audio_service.cpp`), `core/audio`,
  `core/session` compile with `-fsyntax-only` on the host using the proto
  headers (C++20 is required by `components/proto`).
* `general/` pure sources + `general/kcp/ikcp.c` (C99) compile on the host.
* The POSIX UDP `DatagramIo` adapter (`esp_hardware/net`) is host-checked too.
* `general/crypto` and `general/opus` need PSA/libopus headers, so they are
  validated by the IDF build.

`tools/run_host_tests.sh` (host unit tests, no hardware):

* `ButtonStateMachine`: short press, long press fires once while held, and a
  release exactly at the threshold still yields `LongPress`.
* `AudioPipeline`: one encode per source frame, monotonic timestamps (first
  frame is 0), source exhaustion stops the loop.
* `SessionManager` with fake `ITransport` / `ILink` / `IClock` / `IRandom` /
  `ICrypto`: handshake -> established, encrypted `sendReliable`, link-down
  reset, reopen throttle, silent-server reset with a fresh conv id, WaitAck
  timeout, dead-link early reset + epoch increments.

Run the three gates (layers, host compile, host tests) after every change. Run `idf.py reconfigure` after adding a source
directory (the CMake glob has no `CONFIGURE_DEPENDS`).

## Domain core (`main/core`)

* `session/SessionManager` - handshake / liveness / reconnect / encryption:
  ```
  IDLE --link up--> HANDSHAKING(WAIT_ACK) --peer key--> ESTABLISHED
  ESTABLISHED --no RX for 8s--> IDLE (reset transport => fresh KCP sequence numbers)
  any --link down--> IDLE (reset transport)
  ```
  * handshake retries forever with backoff 600ms -> 3s (server may boot late)
  * every reset reopens the transport with a FRESH conversation id (random.u32)
    so a restarted/alive server and the client never desync KCP sequences
  * KCP dead_link (retransmission limit) triggers an early reset instead of
    waiting for the full 8 s server timeout
  * transport.open() is attempted only while the link is up (no getaddrinfo
    stalls when offline)
  * inbound messages are drained in a loop per tick; KCP datagrams drained
    per poll with a 32-datagram bound
  * outbound data queued (max 128) and dropped when not established
    (real-time audio: dropping is better than buffering)
* `audio/AudioPipeline` - capture -> encode -> emit `(compressed, timestampMs)`.

## Application (`main/core/app`)

* `prog.cpp` - device registration + server message dispatch. Receives
  `core::IAppServices&` from the platform layer; no ESP-IDF/root includes.
  Registration is sent on (re)connect transitions detected via the session
  epoch, then as a 3 s keepalive; the RX queue is drained per loop iteration;
  `IFirmwareUpdater::markAppValid()` runs once on the first successful query.
* `audio_service.cpp` - start/stop the capture task through `ITaskRunner`;
  record acks use `IClock::nowUnixMs()`.
* `net_manager.cpp` - builds the transport/session via `ITransportFactory` +
  `SessionManager`, then runs the worker task through `ITaskRunner`.

## Hardware (`main/esp_hardware`)

* `hardware.cpp` - board bring-up: BSP/I2S, IO expander and SD card.
  `main.cpp` includes it through the `HARDWARE_HEADER` macro.
* `adapters/` - `KcpUdpTransport`, `EspWifiLink`, `EspCrypto`, `EspClock`,
  `EspRandom`, `EspLogger`, `EspTaskRunner`, `EspDeviceInfo`,
  `EspNvsStorage`, `EspFileStore`, `EspFirmwareUpdater`,
  `EspI2sAudioSource`, `OpusEncoder`, `EspMediaPlayer`, `EspIndicator`,
  `EspButtonInput`, `EspSpeechInput`, `EspSdPlaylist`.
* `drivers/`, `wifi/`, `net/`, `storage/` - the ESP-IDF implementation
  details behind those adapters.

## Composition root

`main/app_context.{hpp,cpp}` is a **plain class** owned by `main.cpp`. It
constructs every concrete adapter and exposes `core::IAppServices& services()`.
There is no `app::context()` singleton: `main.cpp` passes
`&context.services()` to `app_run_loop`, so `core/app` receives ports by
reference and never sees the composition root.

**Swapping hardware** means replacing `main/esp_hardware/` (or pointing
`HARDWARE_HEADER` at another folder). `main/core/` and `main/general/` stay
untouched; `main/main.cpp` only calls `hardware::boot(context.audio_config)`,
starts the media controller and the application task.

## Crypto backend

`general/crypto` holds the AES-256-GCM / ECDH protocol logic on top of PSA
(mbedTLS is a portable library, so that code stays general).
`encryption::CryptoBackend` is the only platform seam:
`esp_hardware/adapters/crypto/EspCrypto.hpp` implements `EspCryptoBackend`
(`psa_crypto_init()` + `psa_generate_random()`), so a different MCU only
replaces that thin wrapper.

## Migration status

- [x] Phase 0/1: ports, SessionManager, KcpUdpTransport, liveness + KCP reset.
- [x] Phase 2: `IDeviceInfo` + handshake gated on got-IP.
- [x] Phase 3: `IAudioSource` / `IAudioEncoder` + `AudioPipeline`.
- [x] Phase 4: `IStorage` / `IFileStore` + adapters.
- [x] Phase 5: `AppContext` composition root; `ITaskRunner`; `IFirmwareUpdater`.
- [x] Four-folder reorg: `interface.hpp` / `core/` / `general/` / `esp_hardware/`.
- [x] Gates: `tools/check_layers.sh` + `tools/host_check.sh`.
- [x] General logic rescued from `esp_hardware/` (KCP session, media controller,
      button state machine, directory lister, file I/O, formatting,
      session factory, audio frame math).
- [x] Dead code removed: duplicate protocols, `core/ports.hpp` shim, test blocks.
- [x] Deleted now-unused hardware leftovers (`storage/nvs`, `KcpClient`,
      duplicate `FileManager` paths, `Folder_retrieval`, commented duplicates).
- [x] Dependency directions enforced: `core/` → interface/general only;
      `esp_hardware/` → interface/general only; `general/` is a leaf.
- [x] `AppContext` is a plain class in `main/`; `app::context()` singleton removed.
- [x] Audio config + frame math moved to the contract / `general/audio/`.
- [x] Crypto backend injected (`CryptoBackend` / `EspCryptoBackend`).
- [x] Audio ports carry raw PCM `Bytes` (no `int16` smuggling); `ICrypto` keys
      are passed by const reference. `mic_speech` single-mic AFE now reports
      `ESP_SR_EVT_AWAKEN` like the multi-channel path.
- [x] Real host unit tests with fake ports (`tools/run_host_tests.sh`).
- [x] Robustness pass: RGB boot crash, UDP FD leak, keepalive instead of
      registration spam, fresh conv per reset, dead-link early reset,
      NetState/MediaController mutexes, graceful task shutdown, ECDH key
      ownership (`EcdhHandshake`), OTA `markAppValid()`, Opus scratch buffers,
      atomic `shiftBytes()` replace.
- [x] Second robustness pass: tick-clock `receive(now)` + underflow guard,
      WaitAck timeout with key retention, task-handle publication races,
      requestId-echoing acks after real init, 0-based frame timestamps,
      correct AudioPacket codec/rate/channels, SNTP, connected UDP,
      legacy/new file-list decoding, proto CMake excludes lib/main.cpp.
- [ ] Next: 4G adapter implementing `ILink`/`ITransport`;
      `IFirmwareUpdater::applyFromUrl()` streaming.

## Portability snapshot

| Layer | survives an MCU swap |
|---|---|
| `components/proto` | 100% |
| `core/` (session, pipeline, app) | 100% |
| `general/` (ikcp, crypto, opus) | ~95% (PSA/libopus backends) |
| `esp_hardware/` (WiFi, socket, I2S, NVS, clock) | 0% (isolated, mechanical) |
