#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#define MESH_PREFIX "meshNetwork2"
#define MESH_PASSWORD "meshPassword2"
#define MESH_PORT 5552
#define MESH_CHANNEL 6

#define NODE_ID 12

// #define CARRIER_01
// #define GENERAL_01
// #define GREE_01
// #define GREE_02
// #define MIDEA_01
// #define MIDEA_02
// #define MIDEA_03
// #define MIDEA_04
// #define SAMSUNG_01
// #define UNKNOWN_01
// #define UNKNOWN_02
// #define MALAYSIAN_BOARD_01
// #define YORK_01
// #define YORK_02
// #define CHIGO_01
#define QUNDA_01

// #define BOARD_VERSION_01
// #define BOARD_VERSION_02
// #define BOARD_VERSION_03
#define BOARD_VERSION_04

/* 1 = external Modbus RTU temp/humidity sensor on Serial2; 0 = IR-only node */
#define TEMP_SENSOR 0

#if TEMP_SENSOR
// #define SENSOR_VERSION_01
#define SENSOR_VERSION_02
#if !defined(SENSOR_VERSION_01) && !defined(SENSOR_VERSION_02)
#error "TEMP_SENSOR requires SENSOR_VERSION_01 or SENSOR_VERSION_02"
#endif
#endif

#define FIRMWARE_VERSION "20260625.1613"

#if TEMP_SENSOR
#define SLAVE_ID 1
#define START_ADDRESS 0
#define NUM_REGS 2
#endif

#define WATCHDOG_TIMEOUT 30
#define MESH_INTERVAL (5 * 60 * 1000)
#define ONE_SECOND 1000

extern uint16_t arr[5];
extern uint16_t sensor_data[2];
extern uint16_t temperature, humidity;
#define PORTAL_PASSWORD "aqualink@321"
#define PORTAL_TIMEOUT_MS (600 * ONE_SECOND) /* 10 min idle; disabled during OTA upload */
#define PORTAL_OTA_VERIFY_MS (60UL * ONE_SECOND) /* hold exit/reboot while new firmware validates */
/* Portal log ring buffer (RAM). 16 KB is safe on ESP32-S3 N8R2 (~200+ KB free heap in portal). */
#define DEBUG_LOG_BUFFER_BYTES 16384
/* AP address 192.168.4.1 (use IPAddress in portal_handler, not raw hex) */
#define PORTAL_AP_IP_1 192
#define PORTAL_AP_IP_2 168
#define PORTAL_AP_IP_3 4
#define PORTAL_AP_IP_4 1
#define PORTAL_TRIGGER_FC 0x41 /* MBAP: ... 00 02 <nodeId> 41 -> start portal */

#define MAX_CONSECUTIVE_IDLE_RESTARTS 10
#define RESTART_TIMEOUT_MS (15UL * 60UL * 1000UL)

/* Portal button: GPIO0 active LOW, 3s hold (BOARD_VERSION_04). Avoid held-low at power-on (strapping). */
#define PORTAL_BUTTON_PIN 0
#define PORTAL_BUTTON_HOLD_MS (3 * ONE_SECOND)
#define PORTAL_BUTTON_DEBOUNCE_MS 50

/*
 * LED pattern guide (BOARD_VERSION_04: LED_MESH=14, LED_MESH_SIGNAL=11, LED_AC=21)
 *
 * LED_MESH (link):
 *   Portal mode     -> solid ON (setup/AP active, mesh stopped)
 *   Mesh recent RX  -> solid ON (connected, traffic within MESH_INTERVAL)
 *   Mesh idle       -> slow blink LED_MESH_SEARCH_MS (searching / no recent traffic)
 *
 * LED_MESH_SIGNAL (signal / alive):
 *   Portal mode     -> fast blink LED_PORTAL_SIGNAL_MS per phase (200ms on/off)
 *   mesh_rssi == 0  -> double-blink LED_NO_RSSI_* (MCU alive, no mesh RSSI yet)
 *   Linked weak     -> slowest steady blink LED_RSSI_WEAK_MS
 *   Linked medium   -> LED_RSSI_MEDIUM_MS
 *   Linked strong   -> fastest steady blink LED_RSSI_STRONG_MS
 *   Rule: faster steady blink = stronger link; double-blink = not linked yet
 *
 * LED_AC (AC only, ignored in portal):
 *   arr[1] 0 OFF | 1/2 ON | 3 blink LED_AC_OFF_BLINK_MS
 */
#define LED_MESH_SEARCH_MS 500
#define LED_RSSI_STRONG_MS 500
#define LED_RSSI_MEDIUM_MS 1000
#define LED_RSSI_WEAK_MS 2000
#define LED_NO_RSSI_PULSE_MS 120
#define LED_NO_RSSI_PAUSE_MS 2800
#define LED_PORTAL_SIGNAL_MS 200
#define LED_AC_OFF_BLINK_MS 500

#define MESH_DUMMY_BROADCAST_MSG "DUMMY_BROADCAST"

extern bool portalRequested;
extern unsigned long portalRequestTime;
extern bool portalBootOnNextBoot;
extern bool meshTrafficSeen;
extern bool flag;
extern unsigned long last_read_time;
extern unsigned long last_ir_send_time;
extern unsigned long lastMeshReceivedTime;
extern long mesh_rssi;

#if defined(BOARD_VERSION_01)

#define LED_MESH_SIGNAL_STATUS 2
#define LED_AC_STATUS 15
#define LED_MESH 2
#define IR_PIN 4

#define RX_PIN 16
#define TX_PIN 17
#define DIRECTION_PIN 32

#elif defined(BOARD_VERSION_02)

#define LED_MESH_SIGNAL_STATUS 2
#define LED_AC_STATUS 15
#define LED_MESH 2
#define IR_PIN 4

#define RX_PIN 16
#define TX_PIN 17
#define DIRECTION_PIN 32

#elif defined(BOARD_VERSION_03)

#define LED_MESH_SIGNAL_STATUS 11
#define LED_AC_STATUS 21
#define LED_MESH 14
#define IR_PIN 45

#define RX_PIN 5
#define TX_PIN 4
#define DIRECTION_PIN 13

#elif defined(BOARD_VERSION_04)
#define LED_MESH_SIGNAL_STATUS 11
#define LED_AC_STATUS 21
#define LED_MESH 14
#define IR_PIN 45

#define RX_PIN 18
#define TX_PIN 17
#define DIRECTION_PIN 13

#endif


#if defined(CARRIER_01)

#define irRawLength 91
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(CARRIER_02)

#define irRawLength 197
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(GENERAL_01)

#define irRawLength 243
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(GREE_01)

#define irRawLength 279
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(GREE_02)

#define irRawLength 139
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(MIDEA_01)

#define irRawLength 211
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(MIDEA_02)

#define irRawLength 199
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(MIDEA_03)

#define irRawLength 199
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(MIDEA_04)

#define irRawLength 199
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(SAMSUNG_01)

#define irOnRawLength 349
#define irOffRawLength 349
#define irRawLength 233
extern uint16_t ac_on_raw[irOnRawLength];
extern uint16_t ac_off_raw[irOffRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(UNKNOWN_01)

#define irRawLength 83
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(UNKNOWN_02)

#define irRawLength 227
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(MALAYSIAN_BOARD_01)

#define irRawLength 267
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(YORK_01)

#define irRawLength 147
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(YORK_02)

#define irRawLength 199
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(CHIGO_01)

#define irRawLength 197
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];

#elif defined(QUNDA_01)

#define irRawLength 83
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];
#endif