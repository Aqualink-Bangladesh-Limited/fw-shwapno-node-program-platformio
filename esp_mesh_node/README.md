# ESP Mesh Node

Firmware for an ESP32-S3 mesh leaf node (Modbus sensors, IR AC control, captive-portal OTA).

## Prerequisites

- [PlatformIO](https://platformio.org/), ESP32-S3, 8 MB flash (`partitions_ota.csv`)
- Mesh root in range with matching `MESH_*` settings in `app_config.h`
- See [repo README](../README.md) for monorepo layout and shared protocol

## Configuration (`include/app_config.h`)

| Setting | Purpose |
|---------|---------|
| `NODE_ID` | Unique unit ID on the mesh (must match routing) |
| `MESH_PREFIX`, `MESH_PASSWORD`, `MESH_PORT`, `MESH_CHANNEL` | Must match root |
| One AC brand `#define` | e.g. `QUNDA_01` — selects IR raw codes in `ir_raw_data.cpp` |
| `BOARD_VERSION_*` | LED and peripheral pins |
| `SENSOR_VERSION_*` | External Modbus sensor driver |
| `SLAVE_ID` | Modbus slave ID of the temp/humidity sensor |
| `FIRMWARE_VERSION` | Release label (portal logs, debug) |
| `PORTAL_PASSWORD` | Captive portal AP password |

## Architecture

```text
[UDP client] <--WiFi--> [Master] <--UART--> [Root] --painlessMesh--> [This node]
```

| Mode | Wi-Fi role | Purpose |
|------|------------|---------|
| **Mesh node** (default) | painlessMesh | Modbus + sensors + IR; forwards responses via mesh |
| **Portal** | Soft AP | OTA firmware upload, live debug logs, exit back to mesh |

See [repo README](../README.md) for the full chain overview.

## Mesh / UDP command format

Commands use a **Modbus TCP-style MBAP** header on all hops.

### Frame layout

**UDP to master** (port `12345`, 8 bytes minimum):

| Offset | Field |
|--------|--------|
| 0-1 | Transaction ID |
| 2-3 | Protocol ID (`0x0000`) |
| 4-5 | Length (unit ID + function code + PDU) |
| 6 | Target node ID (`NODE_ID` in `app_config.h`) |
| 7+ | Function code and PDU |

**On mesh** (after master adds client return route):

```text
[Client IP:4][Client Port:2][MBAP + PDU]
```

Node ID is at byte **12**, function code at byte **13** (with the 6-byte prefix).

### Function codes (node)

| FC | Purpose |
|----|---------|
| `0x03` | Read holding registers |
| `0x06` | Write single register |
| `0x10` | Write multiple registers |
| `0x41` | **Portal / OTA** - start captive portal on target node |

Register read/write is unchanged; `0x41` is the only new command.

### Portal trigger example

Target **node 4**, transaction ID `1`:

```text
00 01 00 00 00 02 04 41
```

- Length `0x0002` = node ID + FC only (no extra PDU).
- FC `0x41` = `PORTAL_TRIGGER_FC` in `app_config.h`.

Send as UDP payload to the mesh **master**. Master prepends client IP/port; root routes on byte 12; the matching node sets `portalRequested` and enters portal mode.

**Portal AP:** SSID `OTA-NODE-<nodeId>` (e.g. `OTA-NODE-4`), password `PORTAL_PASSWORD`, IP `192.168.4.1`. Open `http://192.168.4.1/` if the captive-portal popup does not appear.

**Local button:** GPIO **0**, active **LOW**, hold **5 s** (serial logs at 1s-5s, then portal starts). Ignored while already in portal. Avoid holding the button low at power-on (ESP32 strapping pin).

**Web UI:** firmware upload, live logs (WebSocket + polling), **Copy logs**, clear logs, exit portal (returns to mesh).

**Log buffer:** `DEBUG_LOG_BUFFER_BYTES` (16 KB default) in `app_config.h`. Oldest lines drop when full.

## Node Modbus registers

Standard **FC `0x03`** read; **FC `0x06` / `0x10`** write for holding registers below.

| Register | Meaning | Read | Write | Notes |
|----------|---------|------|-------|-------|
| `0x01` | AC set temperature | `arr[0]` | Yes | Used by IR handler |
| `0x02` | AC status | `arr[1]` | Yes | `0` off, `1` on, `2` temp only, `3` off sequence; drives **LED_AC** |
| `0x03` | AC hit / trigger | `arr[2]` | Yes | Write sets `flag` → IR send on next loop |
| `0x04` | IR cooldown (seconds) | `arr[3]` | Yes | Min time between IR sends |
| `0x05` | Sensor read interval (seconds) | `arr[4]` | Yes | Modbus sensor poll period |
| `0x21` | Temperature | sensor | No | From Modbus sensor |
| `0x22` | Humidity | sensor | No | From Modbus sensor |
| `0x23` | Mesh RSSI | Yes | No | Positive dBm (e.g. `-45` → `45`); `0` if no RSSI yet |
| Other | — | — | — | Read returns `0xFFFF`; write → exception |

### AC / IR control

Set `arr[1]` (reg `0x02`) and `arr[0]` (reg `0x01`) as needed, then write reg `0x03` to set `flag` — IR sends on the next main loop (`ir_handler.cpp`).

| `arr[1]` | IR action |
|----------|-----------|
| `1` | AC on |
| `2` | Set temperature from `arr[0]` |
| `3` | AC off |

`arr[3]` (reg `0x04`) is the cooldown in seconds between IR sends. `arr[4]` (reg `0x05`) is the sensor poll interval in seconds.

## LED indicators

Each LED has one role. Timing constants are in `app_config.h` (`LED_*_MS`).

**BOARD_VERSION_04 pins:** `LED_MESH` = GPIO 14, `LED_MESH_SIGNAL_STATUS` = GPIO 11, `LED_AC_STATUS` = GPIO 21. Other board versions use different pins (see `app_config.h`).

### LED roles

| LED | Meaning |
|-----|---------|
| **LED_MESH** | Mesh link / portal mode |
| **LED_MESH_SIGNAL** | Signal strength or MCU alive (no RSSI yet) |
| **LED_AC** | AC state from Modbus (`arr[1]`) |

### LED_MESH (link)

| Situation | Pattern |
|-----------|---------|
| Portal mode | Solid **ON** (AP active, mesh stopped) |
| Mesh traffic within last 5 min | Solid **ON** |
| No mesh traffic for 5+ min | Slow blink (**500 ms**) |

### LED_MESH_SIGNAL (signal / alive)

| Situation | Pattern | How to read |
|-----------|---------|-------------|
| Portal mode | Fast blink (**200 ms** on / **200 ms** off) | Setup / OTA web server |
| `mesh_rssi == 0` | **Double-blink** `* * -----` | MCU running, no mesh RSSI yet |
| Weak link (RSSI <= -60 dBm) | Steady blink (**2000 ms**) | Linked, weak signal |
| Medium (-60 to -40 dBm) | Steady blink (**1000 ms**) | Linked, OK signal |
| Strong (> -40 dBm) | Steady blink (**500 ms**) | Linked, strong signal |

**Rules:**

- **Linked:** even on/off blink; faster = stronger signal.
- **Not linked yet (`mesh_rssi == 0`):** two short flashes, then a long pause (not the same as weak link).
- **Portal:** fastest blink on the signal LED; mesh LED solid.

### LED_AC (AC only)

| `arr[1]` | Pattern |
|----------|---------|
| 0 | Off |
| 1, 2 | Solid on |
| 3 | Blink (**500 ms**, AC off sequence) |

In **portal mode**, the AC LED stays **off** so it does not conflict with setup/OTA indication.

### Quick reference

```text
Portal:     MESH solid | SIGNAL fast (200ms on/off) | AC off
No RSSI:    MESH solid or slow blink | SIGNAL * * ----- | AC normal
Weak link:  MESH solid or slow blink | SIGNAL slow even (2s) | AC normal
Strong:     MESH solid or slow blink | SIGNAL fast even (500ms) | AC normal
```

## Source layout

| Module | Role |
|--------|------|
| `main.cpp` | Setup, loop, portal vs mesh orchestration |
| `mesh_handler.cpp` | Mesh start/stop/update, RSSI sample |
| `request_handler.cpp` | Modbus FC `03` / `06` / `10` |
| `sensor_handler.cpp` | External Modbus sensor reads |
| `ir_handler.cpp` / `ir_raw_data.cpp` | IR transmit, AC brand raw codes |
| `portal_handler.cpp` / `portal_web.cpp` | Captive portal AP, OTA, web UI |
| `button_handler.cpp` | 5 s hold → portal |
| `led_handler.cpp` | LED patterns (mesh + portal) |
| `debug_print.cpp` / `debug_log.cpp` | Status logs, ring buffer |

## Build

```bash
pio run -d esp_mesh_node
pio run -d esp_mesh_node -t upload
```

Uses `partitions_ota.csv` and ESPAsyncWebServer (see `platformio.ini`).

## Quick test checklist

1. **Modbus read:** FC `0x03` for reg `0x21` (temp) or `0x23` (RSSI) via master → response on UDP.
2. **AC write:** FC `0x06` to reg `0x03` → IR fires; **LED_AC** follows `arr[1]`.
3. **Portal UDP:** `00 01 00 00 00 02 <nodeId> 41` via master → AP `OTA-NODE-<nodeId>`.
4. **Portal button:** Hold GPIO 0 for 5 s → same AP.
5. **Exit portal:** Web **Exit** or 10 min idle → reboot → mesh node mode.

See also: [esp_mesh_master/README.md](../esp_mesh_master/README.md), [esp_mesh_root/README.md](../esp_mesh_root/README.md).
