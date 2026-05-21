# ESP Mesh Node

Firmware for an ESP32-S3 mesh leaf node (Modbus sensors, IR AC control, optional captive-portal OTA).

## Mesh / UDP command format

Commands use a **Modbus TCP-style MBAP** header on all hops.

### Frame layout

**UDP to master** (port `12345`, 8 bytes minimum):

| Offset | Field |
|--------|--------|
| 0–1 | Transaction ID |
| 2–3 | Protocol ID (`0x0000`) |
| 4–5 | Length (unit ID + function code + PDU) |
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
| `0x41` | **Portal / OTA** — start captive portal on target node |

Register read/write is unchanged; `0x41` is the only new command.

### Portal trigger example

Target **node 4**, transaction ID `1`:

```text
00 01 00 00 00 02 04 41
```

- Length `0x0002` = node ID + FC only (no extra PDU).
- FC `0x41` = `PORTAL_TRIGGER_FC` in `app_config.h`.

Send as UDP payload to the mesh **master**. Master prepends client IP/port; root routes on byte 12; the matching node sets `portalRequested` and enters portal mode (SSID `OTA-NODE-<nodeId>`, e.g. `OTA-NODE-4`, password `PORTAL_PASSWORD`, AP `192.168.4.1`). Phones should get a captive-portal popup; open `http://192.168.4.1/` if not.

### Typical register traffic

Same MBAP header; length and PDU follow normal Modbus TCP rules. Examples: FC `03`/`06`/`10` with register addresses defined in `request_handler.cpp`.

## Build

```bash
pio run -d esp_mesh_node
pio run -d esp_mesh_node -t upload
```

## Related docs

- `docs/portal_ota_design.md` — portal UI, OTA, NVS post-update boot
