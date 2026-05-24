# ESP Mesh Root

Firmware for the ESP32-S3 **mesh root**: bridges the mesh network to the master over UART (Serial2). Forwards Modbus-style traffic to mesh nodes and handles **root-local** requests (portal, mesh RSSI read).

Configure `ROOT_ID`, mesh settings, and board version in `include/app_config.h`.

## Architecture

```text
[UDP client] <--WiFi STA--> [Master 201] <--Serial2 UART--> [Root 221] --> [Nodes 8-15]
```

| Mode | Wi-Fi role | Purpose |
|------|------------|---------|
| **Mesh root** (default) | painlessMesh AP_STA | UART ↔ mesh forwarding + local Modbus for root ID |
| **Portal** | Soft AP | OTA firmware upload, live debug logs, exit back to mesh root |

## UART / MBAP frame layout

**From master** (with 6-byte return-route prefix):

```text
[Client IP:4][Client Port:2][MBAP + PDU]
```

MBAP unit ID is at byte **12** of the full UART frame (byte **6** of the MBAP slice).

**Direct MBAP** (no prefix) is also accepted for root-local commands.

## Packet routing (root)

| Target ID | FC / type | Root action |
|-----------|-----------|-------------|
| **221** | `0x41` | Start portal locally (`OTA-ROOT-221`) — **not** forwarded |
| **221** | `0x03` read | Respond locally (register map below) |
| **221** | `0x06` / `0x10` write | Modbus exception (illegal function) |
| **8–15** | Valid MBAP | Forward to mesh node |
| Invalid | — | Discarded |

## Root Modbus registers (unit ID 221)

Standard **FC `0x03`** read holding registers.

| Register | Meaning | Value |
|----------|---------|--------|
| `0x0001` | Mesh RSSI | Positive dBm (e.g. `-45` → `45`), same convention as nodes. `0` if no RSSI yet. |
| Other | — | `0xFFFF` |

### Read mesh RSSI example

**Request:**

```text
00 01 00 00 00 06 DD 03 00 01 00 01
```

| Field | Value |
|-------|--------|
| Unit ID | `DD` (221) |
| FC | `03` |
| Start address | `0x0001` |
| Count | `1` |

**Response** (RSSI −45 dBm → register `45` = `0x002D`):

```text
00 01 00 00 00 05 DD 03 02 00 2D
```

## Portal / OTA

### UART trigger

Target **root 221**:

```text
00 01 00 00 00 02 DD 41
```

- Length `0x0002` = unit ID + FC only.
- FC `0x41` = `PORTAL_TRIGGER_FC`.
- With master prefix: 6-byte IP/port prefix before this MBAP.

### Local button

- GPIO **0**, active **LOW**, hold **5 s** (logs at 1 s … 5 s, then portal starts).
- Ignored while portal is already active.
- Avoid holding the button low at **power-on** (ESP32 strapping pin).

### Portal AP

| Setting | Value |
|---------|--------|
| SSID | `OTA-ROOT-221` |
| Password | `PORTAL_PASSWORD` in `app_config.h` |
| IP | `192.168.4.1` |

Open `http://192.168.4.1/` if the captive-portal popup does not appear.

### Web UI

- Firmware upload (OTA)
- Live logs (WebSocket + polling), **Copy logs**, clear logs
- **Exit portal** → reboot → returns to **mesh root** mode

### Portal timeout

| Condition | Behavior |
|-----------|----------|
| No device on portal AP for **10 min** | Idle timeout → reboot → mesh root |
| OTA upload in progress | Timeout paused |
| First **60 s** after portal start | Timeout blocked (anti false-trigger) |
| Device connected to AP | Activity timer refreshed |

Post-OTA: NVS flag re-enters portal once for verification, then exit returns to mesh root.

### Idle restart lockout

| Condition | Behavior |
|-----------|----------|
| No UART **or** no mesh RX for **15 min** | Idle restart (counts toward limit) |
| **10** consecutive idle restarts (NVS) | **Lockout** — no more auto-restart until healthy traffic or portal exit clears counter |
| UART packet or mesh Modbus RX | Clears consecutive idle restart count |
| Portal exit / OTA reboot | Clears consecutive idle restart count |

To recover from lockout: fix connectivity, send traffic, use portal exit, or power-cycle after fixing the root cause (counter clears on healthy activity).

## LED indicators (BOARD_VERSION_04)

| GPIO | Define | Mesh root mode | Portal mode |
|------|--------|----------------|-------------|
| 21 | `LED_MESH` | RSSI-based blink | Solid ON |
| 11 | `LED_BLINK` | Heartbeat blink | Fast 200 ms blink |
| 14 | `LED_RXTX_STATUS` | ON 2 s after serial activity | Off |

`LED_RXTX_STATUS` auto-clears after 2 s with no UART activity (avoids stuck ON).

## Debug logging

Status every **5 s** via `printDebugInfo()` (Serial + portal log buffer).

In **portal mode**, each status block includes `Boot: mesh root ...` and full mesh credentials (same as mesh boot), plus AP/OTA details. `portalInfo()` logs this again when portal opens (mesh → portal). WebSocket clients receive the full log buffer on connect.

Every **30 s** in mesh mode, `printConnectedNodes()` prints peers and mesh layout JSON.

**Log buffer:** `DEBUG_LOG_BUFFER_BYTES` (16 KB).

## Source layout

| Module | Role |
|--------|------|
| `main.cpp` | Setup, loop, portal vs mesh orchestration, UART routing |
| `mesh_handler.cpp` | Mesh start/stop/update, RSSI sample |
| `root_modbus_handler.cpp` | Local Modbus for `ROOT_ID` |
| `packet_filter.cpp` | MBAP validation, portal trigger detect |
| `portal_handler.cpp` / `portal_web.cpp` | Captive portal AP, OTA, web UI |
| `button_handler.cpp` | 5 s hold → portal |
| `led_handler.cpp` | LED patterns (mesh + portal) |
| `debug_print.cpp` / `debug_log.cpp` | Status logs, ring buffer |

## Build

```bash
pio run -d esp_mesh_root
pio run -d esp_mesh_root -t upload
```

Uses `partitions_ota.csv` and ESPAsyncWebServer (see `platformio.ini`).

## Quick test checklist

1. **Mesh forward:** Modbus to node 8 → forwarded on mesh.
2. **Root RSSI:** `00 01 00 00 00 06 DD 03 00 01 00 01` on UART → response with mesh RSSI register.
3. **Portal UART:** `00 01 00 00 00 02 DD 41` → AP `OTA-ROOT-221`, mesh stopped.
4. **Portal button:** Hold GPIO 0 for 5 s → same AP.
5. **Exit portal:** Web **Exit** or 10 min idle → reboot → mesh root.
