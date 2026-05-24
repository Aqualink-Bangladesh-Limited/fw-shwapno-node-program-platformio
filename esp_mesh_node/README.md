# ESP Mesh Node

Firmware for an ESP32-S3 mesh leaf node (Modbus sensors, IR AC control, captive-portal OTA).

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

### Typical register traffic

Same MBAP header; length and PDU follow normal Modbus TCP rules. Examples: FC `03`/`06`/`10` with register addresses defined in `request_handler.cpp`.

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

## Build

```bash
pio run -d esp_mesh_node
pio run -d esp_mesh_node -t upload
```

## Related docs

- `docs/portal_ota_design.md` - portal UI, OTA, NVS post-update boot
- `docs/portal_ota_issues.md` - known issues, fixes, test checklist
