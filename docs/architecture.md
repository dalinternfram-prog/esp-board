# architecture

Ports & Adapters (hexagonal). Dependencies point **inward**: the domain core
knows nothing about ESP-IDF, FreeRTOS, KCP, WiFi or the board.

```
 main() / Network::init  ──►  app::context()   <- composition root (main/app_context)
   |
   +-- application   (audio pipeline, record control, registration)   main/app
   |
   +-- domain core   (SessionManager, AudioPipeline)   <- pure C++, unit-testable
   |        |                                          main/core
   |        +-- components/proto   (already platform-agnostic)
   |
   +-- ports         (ILink ITransport ICrypto IClock IRandom ILogger
   |                  IDeviceInfo IStorage IFileStore IAudioSource IAudioEncoder)
   |
   +-- adapters      (KcpUdpTransport, EspWifiLink, EspClock, ...)     main/adapters
```

## Folder layout

```
main/
  main.cpp  app_context.{hpp,cpp}   entry point + composition root
  core/         ports, SessionManager, AudioPipeline   (platform-agnostic)
  adapters/     ESP implementations of the ports
  device/       accessor for IDeviceInfo
  storage/      accessors for IStorage / IFileStore
  app/          prog.cpp, audio_service, audio arguments
  codecs/opus/  Opus wrapper
  drivers/      audio_capture, audio_play, bsp, button, rgb_led, speech, tca9555
  network/      net_manager, kcp, encryption, wifi
  database/     fifo, file_manager, nvs, firmware_switch
```

## Ports (`main/core/ports.hpp`)

| Port | Responsibility | Today | Tomorrow |
|---|---|---|---|
| `ILink` | network path (requires got-IP) | `EspWifiLink` | 4G / Ethernet |
| `ITransport` | move bytes to/from server; own conv id | `KcpUdpTransport` | PPP / TCP |
| `ICrypto` | ECDH + AES-GCM | `EspCrypto` | other backend |
| `IClock` | monotonic ms | `esp_timer` | any OS |
| `IRandom` | session id | `esp_random` | any OS |
| `ILogger` | logging | `ESP_LOGx` | any OS |
| `IDeviceInfo` | MAC / chip / device id | `EspDeviceInfo` | any board |
| `IStorage` | small blob key-value | `EspNvsStorage` | any KV store |
| `IFileStore` | file persistence | `EspFileStore` | any FS |
| `IAudioSource` | PCM capture | `EspI2sAudioSource` | any mic |
| `IAudioEncoder` | audio codec | `OpusEncoder` | LC3 / other |

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

## Adapters (`main/adapters`)

* `transport/KcpUdpTransport` - UDP + KCP client.
* `link/EspWifiLink` - WiFi link (associated + real IP).
* `crypto/EspCrypto` - wraps `main/network/encryption`.
* `platform/EspPlatform` - `EspClock`, `EspRandom`, `EspLogger`.
* `device/EspDeviceInfo` - MAC / chip / device id.
* `storage/EspNvsStorage` - `IStorage` over NVS.
* `storage/EspFileStore` - `IFileStore` over LittleFS.
* `audio/EspI2sAudioSource`, `audio/OpusEncoder` - I2S capture + Opus.

## Composition root

`main/app_context.{hpp,cpp}` -> `app::context()` owns every concrete adapter and
the network session. Modules (`Network`, `device::info()`, `storage::nvs()/files()`)
delegate to it; no scattered module singletons.

## Migration status

- [x] Phase 0/1: ports, SessionManager, KcpUdpTransport, liveness + KCP reset.
- [x] Phase 2: `IDeviceInfo` + handshake gated on got-IP.
- [x] Phase 3: `IAudioSource` / `IAudioEncoder` + `AudioPipeline`.
- [x] Phase 4: `IStorage` / `IFileStore` + adapters.
- [x] Phase 5: `AppContext` composition root.
- [x] Reorg: `app/`, `drivers/`, `network/`, `database/`, `codecs/`; dead files removed.

## Portability snapshot

| Layer | survives an MCU swap |
|---|---|
| `components/proto` | 100% |
| domain core (session, pipeline) | 100% |
| application logic | ~100% |
| portable libs (ikcp, crypto math) | ~90% |
| adapters (WiFi, socket, I2S, NVS, clock) | 0% (isolated, mechanical) |
