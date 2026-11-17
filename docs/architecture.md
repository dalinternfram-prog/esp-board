# architecture

Ports & Adapters (hexagonal). Dependencies point **inward**: the domain core
knows nothing about ESP-IDF, FreeRTOS, KCP, WiFi or the board.

```
 main.cpp / Network::init           <- composition root (only place that knows adapters)
   |
   +-- application   (audio pipeline, record control, registration)
   |
   +-- domain core   (SessionManager, protocol codec)   <- pure C++, unit-testable
   |        |
   |        +-- components/proto   (already platform-agnostic)
   |
   +-- ports         (ITransport, ILink, IClock, IRandom, ILogger, ICrypto)
   |
   +-- adapters      (KcpUdpTransport, WifiLink, EspClock, EspCrypto, ...)
```

## Ports (`main/core/ports.hpp`)

| Port | Responsibility | Today | Tomorrow |
|---|---|---|---|
| `ILink` | "is there a network path?" | `wifi::isConnected()` | 4G/Ethernet link |
| `ITransport` | move bytes to/from server; own conv id | `KcpUdpTransport` (UDP+KCP) | PPP / TCP transport |
| `ICrypto` | ECDH + AES-GCM | `EspCrypto` (mbedTLS/PSA) | other crypto backend |
| `IClock` | monotonic ms | `esp_timer` | any OS |
| `IRandom` | session id | `esp_random` | any OS |
| `ILogger` | logging | `ESP_LOGx` | any OS |

## Domain core (`main/core/session/SessionManager`)

State machine, no globals, no platform headers:

```
IDLE --link up--> HANDSHAKING(WAIT_ACK) --peer key--> ESTABLISHED
ESTABLISHED --no RX for 8s--> IDLE (reset transport => fresh KCP sequence numbers)
any --link down--> IDLE (reset transport)
```

* handshake retries forever with backoff 600ms -> 3s (server may be booting late)
* session reset always recreates the transport, so a restarted server and the
  client start from sequence number 0 again (no KCP desync)
* outbound app data is queued (max 128) and dropped when not established
  (real-time audio: buffering is worse than dropping)

## Adapters

* `main/adapters/transport/KcpUdpTransport` - wraps `KcpClient` (UDP + KCP).
* `main/adapters/link/EspWifiLink` - wraps `wifi::isConnected()`.
* `main/adapters/crypto/EspCrypto` - wraps `main/Network/encryption`.
* `main/adapters/platform/EspPlatform` - `EspClock`, `EspRandom`, `EspLogger`.

## Migration status

- [x] Phase 0/1: ports, SessionManager, KcpUdpTransport, platform adapters;
      `Network::` API preserved as a thin composition root.
- [x] Phase 2: `IDeviceInfo` (MAC/chip/deviceId) + handshake gated on got-IP.
- [x] Phase 3: `IAudioSource` / `IAudioEncoder` + `AudioPipeline`.
- [ ] Phase 4: `IStorage` / `IFileStore`.
- [ ] Phase 5: `AppContext` returned by `main`, remove last module globals.

## Portability snapshot

| Layer | survives an MCU swap |
|---|---|
| `components/proto` | 100% |
| domain core (session, pipeline) | 100% |
| application logic | ~100% |
| portable libs (ikcp, crypto math) | ~90% |
| adapters (WiFi, socket, I2S, NVS, clock) | 0% (isolated, mechanical) |
