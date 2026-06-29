# Shwapno Node — ESP32 Mesh Firmware

PlatformIO monorepo for the Shwapno mesh network: UDP/Modbus control of field nodes (sensors, IR AC) through a master–root–node chain.

## System overview

```text
[UDP client] --WiFi STA--> [Master] --UART Serial2--> [Root] --painlessMesh--> [Nodes]
```

| Layer | Folder | Role |
|-------|--------|------|
| Bridge | [`esp_mesh_master/`](esp_mesh_master/) | Wi-Fi STA, UDP port `12345`, forwards MBAP to root; local portal/OTA |
| Mesh root | [`esp_mesh_root/`](esp_mesh_root/) | UART ↔ mesh; routes to `START_NODE`–`END_NODE`; local portal/OTA |
| Leaf node | [`esp_mesh_node/`](esp_mesh_node/) | Modbus sensors, IR AC control, mesh leaf; portal/OTA |
| Test harness | [`esp_ir_test_udp/`](esp_ir_test_udp/) | Standalone IR + UDP test firmware (not production) |

Configure IDs, Wi-Fi, mesh credentials, and board version in each project’s `include/app_config.h` before building.

## Prerequisites

- [PlatformIO](https://platformio.org/)
- Target board: **ESP32-S3** (8 MB flash; production projects use `partitions_ota.csv`)
- Serial upload cable for the device under test

## Build & upload

Run from the repo root:

```bash
pio run -d esp_mesh_master -t upload
pio run -d esp_mesh_root -t upload
pio run -d esp_mesh_node -t upload
```

Replace the directory name for the project you are flashing. Add `-e esp32-s3-devkitc-1` if your default env differs (see each `platformio.ini`).

## Shared protocol (summary)

- **Transport:** Modbus TCP-style **MBAP** over UDP (master) or mesh (after master adds a 6-byte return route).
- **UDP port:** `12345` on the master.
- **Portal trigger:** function code **`0x41`** on the target unit ID (`MASTER_ID`, `ROOT_ID`, or `NODE_ID` per `app_config.h`).
- **Portal button:** GPIO **0**, active **LOW**, hold **3 s** on all roles (avoid held-low at power-on — strapping pin).
- **Return route on mesh:** `[Client IP:4][Client Port:2][MBAP + PDU]` — target ID at byte **12**, FC at byte **13**.
- **OTA verify hold:** After a successful OTA flash, a **60 s** post-boot window blocks **Exit** and **Restart** in the portal web UI (and logs the remaining time). Exit after verify returns to normal bridge/mesh mode.
- **Idle restart guard:** Each role tracks consecutive 15-minute idle reboots in **NVS**; after **10** in a row, **lockout** disables further auto-restart until healthy traffic clears the counter (policy differs per role — see project READMEs).

Details, register maps, LED patterns, and test steps are in each project README (links below).

## Documentation

| Document | Description |
|----------|-------------|
| [esp_mesh_master/README.md](esp_mesh_master/README.md) | Master bridge, routing, Wi-Fi RSSI register, portal/OTA |
| [esp_mesh_root/README.md](esp_mesh_root/README.md) | Mesh root, forwarding, mesh RSSI register, portal/OTA |
| [esp_mesh_node/README.md](esp_mesh_node/README.md) | Node MBAP, sensors, IR AC, LEDs, portal/OTA |
| [esp_ir_test_udp/README.md](esp_ir_test_udp/README.md) | Test firmware (IR + UDP harness) |
