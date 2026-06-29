#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#define MASTER_ID 202

// WiFi credentials (STA bridge mode)
#define WIFI_SSID "Aqualink_Sensometer"
#define WIFI_PASSWORD "aqualink@321"

// #define BOARD_VERSION_03
#define BOARD_VERSION_04

#define WATCHDOG_TIMEOUT 30
#define ONE_SECOND 1000

// UDP defaults
#define UDP_DEFAULT_PORT 12345
static const uint8_t UDP_BROADCAST_IP[4] = {255, 255, 255, 255};

#define FIRMWARE_VERSION "20260629.1440"

#define PORTAL_PASSWORD "aqualink@321"
#define PORTAL_TIMEOUT_MS (600 * ONE_SECOND) /* 10 min idle; disabled during OTA upload */
#define PORTAL_OTA_VERIFY_MS (60UL * ONE_SECOND) /* hold exit/reboot while new firmware validates */
#define DEBUG_LOG_BUFFER_BYTES 16384
#define PORTAL_AP_IP_1 192
#define PORTAL_AP_IP_2 168
#define PORTAL_AP_IP_3 4
#define PORTAL_AP_IP_4 1
#define PORTAL_TRIGGER_FC 0x41 /* MBAP: ... 00 02 <masterId> 41 -> start portal */

/* Master holding registers (Modbus FC 0x03, unit ID = MASTER_ID) */
#define MASTER_REG_WIFI_RSSI 0x01

/* Portal button: GPIO0 active LOW, 3s hold. Avoid held-low at power-on (strapping). */
#define PORTAL_BUTTON_PIN 0
#define PORTAL_BUTTON_HOLD_MS (3 * ONE_SECOND)
#define PORTAL_BUTTON_DEBOUNCE_MS 50

/*
 * LED pattern guide (master):
 *   LED_WIFI_STATUS   -> bridge: RSSI blink | portal: solid ON
 *   LED_UDP_STATUS    -> bridge: UDP activity | portal: fast blink LED_PORTAL_SIGNAL_MS
 *   LED_RXTX_STATUS   -> bridge: serial activity | portal: off
 */
#define LED_PORTAL_SIGNAL_MS 200

#define DEBUG_PRINT_INTERVAL_BRIDGE_MS (5 * ONE_SECOND)
#define DEBUG_PRINT_INTERVAL_PORTAL_MS (5 * ONE_SECOND)

#define MAX_CONSECUTIVE_IDLE_RESTARTS 10
#define RESTART_TIMEOUT_MS (15UL * 60UL * 1000UL)

extern bool portalRequested;
extern unsigned long portalRequestTime;
extern bool portalBootOnNextBoot;
extern uint8_t portalStoredDeviceId;


#if defined(BOARD_VERSION_03)

#define LED_UDP_STATUS 11
#define LED_WIFI_STATUS 21
#define LED_RXTX_STATUS 14

#define RX_PIN 5
#define TX_PIN 4

#elif defined(BOARD_VERSION_04)
#define LED_UDP_STATUS 11
#define LED_WIFI_STATUS 21
#define LED_RXTX_STATUS 14

#define RX_PIN 18
#define TX_PIN 17

#endif
