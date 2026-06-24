#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#define ROOT_ID 221

#define MESH_PREFIX "meshNetwork2"
#define MESH_PASSWORD "meshPassword2"
#define MESH_PORT 5552
#define MESH_CHANNEL 6

// #define BOARD_VERSION_03
#define BOARD_VERSION_04

#define START_NODE 8
#define END_NODE 15

#define WATCHDOG_TIMEOUT 30
#define ONE_SECOND 1000

#define FIRMWARE_VERSION "20260624.1645"

#define PORTAL_PASSWORD "aqualink@321"
#define PORTAL_TIMEOUT_MS (600 * ONE_SECOND)
#define PORTAL_OTA_VERIFY_MS (60UL * ONE_SECOND) /* hold exit/reboot while new firmware validates */
#define DEBUG_LOG_BUFFER_BYTES 16384
#define PORTAL_AP_IP_1 192
#define PORTAL_AP_IP_2 168
#define PORTAL_AP_IP_3 4
#define PORTAL_AP_IP_4 1
#define PORTAL_TRIGGER_FC 0x41 /* MBAP: ... 00 02 <rootId> 41 -> start portal */

/* Root holding registers (Modbus FC 0x03, unit ID = ROOT_ID) */
#define ROOT_REG_MESH_RSSI 0x01

/* Portal button: GPIO0 active LOW, 5s hold. Avoid held-low at power-on (strapping). */
#define PORTAL_BUTTON_PIN 0
#define PORTAL_BUTTON_HOLD_MS (5 * ONE_SECOND)
#define PORTAL_BUTTON_DEBOUNCE_MS 50

/*
 * LED pattern guide (root):
 *   LED_MESH        -> mesh: RSSI blink | portal: solid ON
 *   LED_BLINK       -> mesh: heartbeat blink | portal: fast blink LED_PORTAL_SIGNAL_MS
 *   LED_RXTX_STATUS -> mesh: serial activity | portal: off
 */
#define LED_PORTAL_SIGNAL_MS 200

#define DEBUG_PRINT_INTERVAL_MESH_MS (5 * ONE_SECOND)
#define DEBUG_PRINT_INTERVAL_PORTAL_MS (5 * ONE_SECOND)

#define MESH_INTERVAL (5 * 60 * 1000)

/* Idle auto-restart: stop after this many consecutive 15-min idle reboots (NVS-backed). */
#define MAX_CONSECUTIVE_IDLE_RESTARTS 10
#define RESTART_TIMEOUT_MS (15UL * 60UL * 1000UL)

extern bool portalRequested;
extern unsigned long portalRequestTime;
extern bool portalBootOnNextBoot;
extern uint8_t portalStoredDeviceId;
extern long mesh_rssi;

#if defined(BOARD_VERSION_03)

#define LED_BLINK 11
#define LED_RXTX_STATUS 14
#define LED_MESH 21

#define RX_PIN 5
#define TX_PIN 4

#elif defined(BOARD_VERSION_04)

#define LED_BLINK 11
#define LED_RXTX_STATUS 14
#define LED_MESH 21

#define RX_PIN 18
#define TX_PIN 17
#endif
