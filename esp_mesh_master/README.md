# ESP Mesh Master

Firmware for the ESP32-S3 mesh **master bridge**: connects a Wi-Fi network to the mesh root over UART (Serial2). UDP clients send Modbus TCP-style commands; the master forwards them to the mesh or handles **master-local** requests (portal, WiFi RSSI read).

## Prerequisites

- [PlatformIO](https://platformio.org/), ESP32-S3, 8 MB flash (`partitions_ota.csv`)
- UART link to mesh root on Serial2 (`RX_PIN` / `TX_PIN` per board version)
- See [repo README](../README.md) for monorepo layout and shared protocol

## Configuration (`include/app_config.h`)

| Setting | Purpose |
|---------|---------|
| `MASTER_ID` | Local Modbus unit ID |
| `WIFI_SSID` / `WIFI_PASSWORD` | Wi-Fi STA credentials (bridge mode) |
| `BOARD_VERSION_04` or `_03` | LED pins and UART pins |
| `FIRMWARE_VERSION` | Release label (portal logs, debug) |
| `PORTAL_PASSWORD` | Captive portal AP password |

## Architecture

```text
[UDP client] <--WiFi STA--> [Master] <--Serial2 UART--> [Mesh root] --> [Nodes]
```

| Mode | Wi-Fi role | Purpose |
|------|------------|---------|
| **Bridge** (default) | STA client | UDP ↔ UART forwarding + local Modbus for master ID |
| **Portal** | Soft AP | OTA firmware upload, live debug logs, exit back to bridge |

## UDP / MBAP frame layout

**UDP to master** (port `12345`, 8 bytes minimum):

| Offset | Field |
|--------|--------|
| 0-1 | Transaction ID |
| 2-3 | Protocol ID (`0x0000`) |
| 4-5 | Length (unit ID + function code + PDU) |
| 6 | Target ID (node ID or `MASTER_ID`) |
| 7+ | Function code and PDU |

**On mesh** (after master prepends client return route):

```text
[Client IP:4][Client Port:2][MBAP + PDU]
```

On the mesh, target ID is at byte **12**, function code at byte **13** (with the 6-byte prefix).

## Packet routing (master)

| Target ID | FC / type | Master action |
|-----------|-----------|---------------|
| **`MASTER_ID`** | `0x41` | Start portal locally (`OTA-MASTER-<MASTER_ID>`) — **not** forwarded |
| **`MASTER_ID`** | `0x03` read | Respond locally (register map below) |
| **`MASTER_ID`** | `0x06` / `0x10` write | Modbus exception (illegal function) |
| **Other** | Valid MBAP | Forward to mesh root via UART |
| Invalid | — | Discarded |

## Master Modbus registers (unit ID = `MASTER_ID`)

Standard **FC `0x03`** read holding registers. Same MBAP rules as mesh nodes.

| Register | Meaning | Value |
|----------|---------|--------|
| `0x0001` | WiFi RSSI | Positive dBm (e.g. `-45` → `45`), same convention as node reg `0x23`. `0` if STA not connected. |
| Other | — | `0xFFFF` |

### Read WiFi RSSI example

**Request** (byte 6 = `MASTER_ID`; example below uses unit ID `201` → `C9`):

```text
00 01 00 00 00 06 C9 03 00 01 00 01
```

| Field | Value |
|-------|--------|
| Unit ID | `MASTER_ID` (one byte at offset 6) |
| FC | `03` |
| Start address | `0x0001` |
| Count | `1` |

**Response** (RSSI −45 dBm → register `45` = `0x002D`; unit ID byte matches request):

```text
00 01 00 00 00 05 C9 03 02 00 2D
```

## Portal / OTA

### UDP trigger

Target **`MASTER_ID`**, transaction ID `1` (example: ID `201` → `C9`):

```text
00 01 00 00 00 02 C9 41
```

- Length `0x0002` = target ID + FC only.
- FC `0x41` = `PORTAL_TRIGGER_FC`.

### Local button

- GPIO **0**, active **LOW**, hold **3 s** (logs at 1 s … 3 s, then portal starts).
- Ignored while portal is already active.
- Avoid holding the button low at **power-on** (ESP32 strapping pin).

### Portal AP

| Setting | Value |
|---------|--------|
| SSID | `OTA-MASTER-<MASTER_ID>` |
| Password | `PORTAL_PASSWORD` in `app_config.h` |
| IP | `192.168.4.1` |

Open `http://192.168.4.1/` if the captive-portal popup does not appear.

### Web UI

- Firmware upload (OTA)
- Live logs (WebSocket + polling), **Copy logs**, clear logs
- **Exit portal** → reboot → returns to **bridge / Wi-Fi STA** mode
- **Restart** (`/restart`) → reboot while staying in portal mode

### Portal timeout

| Condition | Behavior |
|-----------|----------|
| No device on portal AP for **10 min** | Idle timeout → reboot → bridge mode |
| OTA upload in progress | Timeout paused |
| First **60 s** after portal start | Timeout blocked (anti false-trigger) |
| Device connected to AP | Activity timer refreshed (portal stays up while associated) |

### Post-OTA verify hold

After a successful OTA flash:

1. NVS `otaVerify` flag re-enters portal on next boot for validation.
2. For **60 s** (`PORTAL_OTA_VERIFY_MS`), **Exit** and **Restart** are blocked; serial logs show remaining time.
3. After the hold expires (or on manual verify completion), exit returns to bridge mode.

Bootloader app rollback is enabled in `platformio.ini` (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`).

## Bridge idle watchdog (15 minutes)

Restart (via restart guard, see below) **only after both** have been idle for **15 minutes**:

- **UDP** — no ingress (including forwarded traffic to mesh, master Modbus, portal trigger)
- **UART** — no complete serial frame outbound to LAN

So one active path alone does **not** force a reboot. If neither side sees traffic for 15 minutes, the bridge is stuck or disused and reboot is requested.

### Idle restart lockout

| Condition | Behavior |
|-----------|----------|
| Both UDP and UART idle **15 min** | Idle restart (counts toward limit) |
| **10** consecutive idle restarts (NVS `master_boot`) | **Lockout** — no more auto-restart until healthy traffic |
| UDP RX/TX or UART RX/TX | Clears consecutive idle restart count |
| Portal exit / OTA reboot | Does **not** clear the counter |

Periodic debug shows `UDP idle Ns | UART idle Ns | idle restart N/10` and `LOCKOUT` when active.

## LED indicators (BOARD_VERSION_04)

| GPIO | Role |
|------|------|
| 21 | `LED_WIFI_STATUS` — bridge: RSSI blink; portal: solid ON |
| 11 | `LED_UDP_STATUS` — bridge: UDP activity flash; portal: fast 200 ms blink |
| 14 | `LED_RXTX_STATUS` — bridge: serial activity flash; portal: off |

### Bridge mode

| LED | Pattern |
|-----|---------|
| **WiFi** | Disconnected: solid ON. Connected: blink speed by RSSI (500 / 1000 / 2000 ms) |
| **UDP** | ON for 2 s after UDP RX/TX |
| **RX/TX** | ON for 2 s after serial activity |

### Portal mode

| LED | Pattern |
|-----|---------|
| **WiFi** | Solid ON |
| **UDP** | Fast blink (200 ms) |
| **RX/TX** | Off |

## Debug logging

Status block printed periodically (also in portal web log buffer):

| Mode | Interval |
|------|----------|
| Bridge | Every **5 s** |
| Portal | Every **5 s** |

Each block includes a summary line (`Status: <board> <firmware> | master <MASTER_ID> | uptime … | heap …`) plus mode-specific lines:

- **Bridge:** Wi-Fi SSID/IP/RSSI, then `UDP idle … | UART idle … | idle restart N/10 [LOCKOUT]`
- **Portal:** `Mode: portal | AP … | clients … | OTA yes/no | heap …`

On boot, `masterInfo()` prints bridge network details once.

Packet hex dumps are **one line** per event, two hex digits per byte (e.g. `AC 10 …`). Hex dumps are suppressed in portal mode to keep logs readable.

**Log buffer:** `DEBUG_LOG_BUFFER_BYTES` (16 KB). Oldest lines drop when full.

## Source layout

| Module | Role |
|--------|------|
| `main.cpp` | Setup, loop, portal vs bridge orchestration |
| `bridge_handler.cpp` | Wi-Fi STA, UDP ↔ Serial2, routing |
| `master_modbus_handler.cpp` | Local Modbus for `MASTER_ID` |
| `packet_filter.cpp` | MBAP validation, portal trigger detect |
| `portal_handler.cpp` / `portal_web.cpp` | Captive portal AP, OTA, web UI |
| `button_handler.cpp` | 3 s hold → portal |
| `led_handler.cpp` | LED patterns |
| `restart_guard.cpp` | Limits consecutive auto-restarts (NVS); cleared on UDP or UART activity |
| `ota_rollback.cpp` | Post-OTA verify hold and portal re-entry |
| `debug_print.cpp` / `debug_log.cpp` | Status logs, ring buffer |

## Build

```bash
pio run -d esp_mesh_master
pio run -d esp_mesh_master -t upload
```

Uses `partitions_ota.csv` and ESPAsyncWebServer (see `platformio.ini`).

## Quick test checklist

1. **Bridge:** Modbus read to a node ID → packet forwarded on Serial2 with client IP:port prefix.
2. **Master RSSI:** FC `0x03` read reg `0x01` targeting `MASTER_ID` → UDP response with RSSI register.
3. **Portal UDP:** FC `0x41` targeting `MASTER_ID` → AP `OTA-MASTER-<MASTER_ID>`, not forwarded.
4. **Portal button:** Hold GPIO 0 for 3 s → same AP.
5. **Exit portal:** Web **Exit** or 10 min idle (no AP client) → reboot → bridge / Wi-Fi STA.
6. **Node portal:** FC `0x41` targeting `NODE_ID` → forwarded to root.

See also: [esp_mesh_root/README.md](../esp_mesh_root/README.md), [esp_mesh_node/README.md](../esp_mesh_node/README.md).
