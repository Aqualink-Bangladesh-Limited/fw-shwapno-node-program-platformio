# ESP Mesh Root

Firmware for the ESP32-S3 **mesh root**: bridges the mesh network to the master over UART (Serial2). Forwards Modbus-style traffic to mesh nodes and handles **root-local** requests (portal, mesh RSSI read).

## Prerequisites

- [PlatformIO](https://platformio.org/), ESP32-S3, 8 MB flash (`partitions_ota.csv`)
- UART link to master on Serial2 (`RX_PIN` / `TX_PIN` per board version)
- See [repo README](../README.md) for monorepo layout and shared protocol

## Configuration (`include/app_config.h`)

| Setting | Purpose |
|---------|---------|
| `ROOT_ID` | Local Modbus unit ID |
| `MESH_PREFIX`, `MESH_PASSWORD`, `MESH_PORT`, `MESH_CHANNEL` | painlessMesh network (nodes must use same values) |
| `START_NODE` / `END_NODE` | Expected node ID range on this mesh |
| `MESH_DUMMY_BROADCAST_MSG` | Keepalive broadcast payload (`DUMMY_BROADCAST`) |
| `BOARD_VERSION_04` or `_03` | LED pins and UART pins |
| `FIRMWARE_VERSION` | Release label (portal logs, debug) |
| `PORTAL_PASSWORD` | Captive portal AP password |

## Architecture

```text
[UDP client] <--WiFi STA--> [Master] <--Serial2 UART--> [Root] --> [Nodes START_NODE–END_NODE]
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
| **`ROOT_ID`** | `0x41` | Start portal locally (`OTA-ROOT-<ROOT_ID>`) — **not** forwarded |
| **`ROOT_ID`** | `0x03` read | Respond locally (register map below) |
| **`ROOT_ID`** | `0x06` / `0x10` write | Modbus exception (illegal function) |
| **`START_NODE`–`END_NODE`** | Valid MBAP | Forward to mesh node |
| Invalid | — | Discarded |

## Root Modbus registers (unit ID = `ROOT_ID`)

Standard **FC `0x03`** read holding registers.

| Register | Meaning | Value |
|----------|---------|--------|
| `0x0001` | Mesh RSSI | Positive dBm (e.g. `-45` → `45`), same convention as nodes. `0` if no RSSI yet. |
| Other | — | `0xFFFF` |

### Read mesh RSSI example

**Request** (byte 6 = `ROOT_ID`; example below uses unit ID `221` → `DD`):

```text
00 01 00 00 00 06 DD 03 00 01 00 01
```

| Field | Value |
|-------|--------|
| Unit ID | `ROOT_ID` (one byte at offset 6) |
| FC | `03` |
| Start address | `0x0001` |
| Count | `1` |

**Response** (RSSI −45 dBm → register `45` = `0x002D`; unit ID byte matches request):

```text
00 01 00 00 00 05 DD 03 02 00 2D
```

## Mesh keepalive

Every **120 s** the root broadcasts `DUMMY_BROADCAST` on the mesh. Nodes ignore it silently (no Modbus parse). This keeps mesh RX activity visible for idle-restart logic when no client traffic is present.

## Portal / OTA

### UART trigger

Target **`ROOT_ID`** (example: ID `221` → `DD`):

```text
00 01 00 00 00 02 DD 41
```

- Length `0x0002` = unit ID + FC only.
- FC `0x41` = `PORTAL_TRIGGER_FC`.
- With master prefix: 6-byte IP/port prefix before this MBAP.

### Local button

- GPIO **0**, active **LOW**, hold **3 s** (logs at 1 s … 3 s, then portal starts).
- Ignored while portal is already active.
- Avoid holding the button low at **power-on** (ESP32 strapping pin).

### Portal AP

| Setting | Value |
|---------|--------|
| SSID | `OTA-ROOT-<ROOT_ID>` |
| Password | `PORTAL_PASSWORD` in `app_config.h` |
| IP | `192.168.4.1` |

Open `http://192.168.4.1/` if the captive-portal popup does not appear.

### Web UI

- Firmware upload (OTA)
- Live logs (WebSocket + polling), **Copy logs**, clear logs
- **Exit portal** → reboot → returns to **mesh root** mode
- **Restart** (`/restart`) → reboot while staying in portal mode

### Portal timeout

| Condition | Behavior |
|-----------|----------|
| No device on portal AP for **10 min** | Idle timeout → reboot → mesh root |
| OTA upload in progress | Timeout paused |
| First **60 s** after portal start | Timeout blocked (anti false-trigger) |
| Device connected to AP | Activity timer refreshed |

### Post-OTA verify hold

After a successful OTA flash:

1. NVS `otaVerify` flag re-enters portal on next boot for validation.
2. For **60 s** (`PORTAL_OTA_VERIFY_MS`), **Exit** and **Restart** are blocked; serial logs show remaining time.
3. After the hold expires, exit returns to mesh root mode.

Bootloader app rollback is enabled in `platformio.ini` (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`).

### Idle restart lockout

| Condition | Behavior |
|-----------|----------|
| No UART for **15 min** | Idle restart (counts toward limit) |
| Paired slaves exist and no mesh RX for **15 min** | Idle restart (counts toward limit) |
| **10** consecutive idle restarts (NVS `root_boot`) | **Lockout** — no more auto-restart until healthy UART traffic |
| UART forward or root Modbus reply on UART | Clears consecutive idle restart count |
| Portal exit / OTA reboot | Does **not** clear the counter |

To recover from lockout: fix master↔root UART connectivity and send valid Modbus traffic (counter clears on UART activity).

## LED indicators (BOARD_VERSION_04)

| GPIO | Define | Mesh root mode | Portal mode |
|------|--------|----------------|-------------|
| 21 | `LED_MESH` | RSSI-based blink | Solid ON |
| 11 | `LED_BLINK` | Heartbeat blink | Fast 200 ms blink |
| 14 | `LED_RXTX_STATUS` | ON 2 s after serial activity | Off |

`LED_RXTX_STATUS` auto-clears after 2 s with no UART activity (avoids stuck ON).

## Debug logging

Status every **5 s** via `printDebugInfo()` (Serial + portal log buffer).

**Mesh mode** summary line plus:

- Root ID and slave range, mesh node ID / prefix
- RSSI register, UART idle and mesh RX idle times
- Node map (`nodeId` → `meshId`) and unpaired peers
- `Idle restart N/10 [LOCKOUT]`
- Reply queue depth when non-empty

Every **30 s** in mesh mode, `printConnectedNodes()` prints peers and a mesh layout JSON tree (`nodeId`, `meshId`, online state).

**Portal mode:** summary line plus `Mode: portal | AP … | clients … | OTA yes/no | heap …`. `portalInfo()` logs mesh context again when portal opens.

**Log buffer:** `DEBUG_LOG_BUFFER_BYTES` (16 KB).

## Source layout

| Module | Role |
|--------|------|
| `main.cpp` | Setup, loop, portal vs mesh orchestration, UART routing, dummy broadcast |
| `mesh_handler.cpp` | Mesh start/stop/update, RSSI sample |
| `root_modbus_handler.cpp` | Local Modbus for `ROOT_ID` |
| `packet_filter.cpp` | MBAP validation, portal trigger detect |
| `portal_handler.cpp` / `portal_web.cpp` | Captive portal AP, OTA, web UI |
| `button_handler.cpp` | 3 s hold → portal |
| `led_handler.cpp` | LED patterns (mesh + portal) |
| `restart_guard.cpp` | Limits consecutive auto-restarts (NVS); cleared on UART activity |
| `ota_rollback.cpp` | Post-OTA verify hold and portal re-entry |
| `debug_print.cpp` / `debug_log.cpp` | Status logs, ring buffer |

## Build

```bash
pio run -d esp_mesh_root
pio run -d esp_mesh_root -t upload
```

Uses `partitions_ota.csv` and ESPAsyncWebServer (see `platformio.ini`).

## Quick test checklist

1. **Mesh forward:** Modbus to a node ID in `START_NODE`–`END_NODE` range → forwarded on mesh.
2. **Root RSSI:** FC `0x03` read reg `0x01` targeting `ROOT_ID` on UART → response with mesh RSSI register.
3. **Portal UART:** FC `0x41` targeting `ROOT_ID` → AP `OTA-ROOT-<ROOT_ID>`, mesh stopped.
4. **Portal button:** Hold GPIO 0 for 3 s → same AP.
5. **Exit portal:** Web **Exit** or 10 min idle → reboot → mesh root.

See also: [esp_mesh_master/README.md](../esp_mesh_master/README.md), [esp_mesh_node/README.md](../esp_mesh_node/README.md).
