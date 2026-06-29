# ESP IR Test (UDP)

**Test firmware only** — not deployed in production. Standalone harness to exercise **UDP Modbus** and **IR AC control** on a single ESP32-S3 without mesh, master, or root.

Use this to validate IR codes, register writes, and UDP responses before merging changes into [`esp_mesh_node/`](../esp_mesh_node/). For production mesh, portal, and OTA behavior see the [repo README](../README.md) and [esp_mesh_node/README.md](../esp_mesh_node/README.md).

## Configuration (`include/app_config.h`)

| Setting | Purpose |
|---------|---------|
| `WIFI_SSID` / `WIFI_PASSWORD` | Wi-Fi STA credentials |
| `NODE_ID` | Modbus unit ID (byte 6 in MBAP) |
| One AC brand `#define` | e.g. `MIDEA_03` — selects IR raw codes in `ir_raw_data.cpp` |
| `IR_PIN` | GPIO for IR LED (default `45`) |
| `UDP_DEFAULT_PORT` | UDP listen port (default `12345`) |

## Architecture

```text
[UDP client] --WiFi STA--> [This device] --GPIO--> IR LED
```

No UART bridge, no mesh, no portal/OTA.

## UDP / MBAP

Same Modbus TCP-style **MBAP** as production (port `12345`, unit ID = `NODE_ID`):

| Offset | Field |
|--------|--------|
| 0-1 | Transaction ID |
| 2-3 | Protocol ID (`0x0000`) |
| 4-5 | Length |
| 6 | Unit ID (`NODE_ID`) |
| 7+ | Function code and PDU |

Supported FC: `0x03` read, `0x06` write single, `0x10` write multiple.

## Holding registers

| Register | Maps to | Read | Write | Notes |
|----------|---------|------|-------|-------|
| `0x01` | `arr[0]` | Yes | Yes | AC set temperature |
| `0x02` | `arr[1]` | Yes | Yes | AC status (`1` on, `2` temp only, `3` off) |
| `0x03` | `arr[2]` | Yes | Yes | Write sets `flag` → IR send on next loop |
| `0x04`–`0x05` | `arr[3–4]` | Yes | Yes | Available in handler; not used in main loop |

IR runs when `flag` is set after a write to `0x03` (or `0x10` including reg `0x03`). See `ir_handler.cpp` for AC state handling.

## LEDs

| GPIO | Role |
|------|------|
| 21 | Wi-Fi connected |
| 14 | RX/TX activity (reserved pattern) |
| 11 | Heartbeat blink (500 ms) |

## Build

```bash
pio run -d esp_ir_test_udp
pio run -d esp_ir_test_udp -t upload
```

## Quick test

1. Flash with Wi-Fi and `NODE_ID` set in `app_config.h`.
2. Send Modbus read FC `0x03` for reg `0x01` to the device IP, port `12345`.
3. Write reg `0x03` (FC `0x06`) to trigger IR; watch serial for `AC Set Temp / AC Status / AC Hit` every 5 s.
