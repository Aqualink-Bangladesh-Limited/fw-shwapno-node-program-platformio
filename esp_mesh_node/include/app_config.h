#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#define MESH_PREFIX "meshNetwork1"
#define MESH_PASSWORD "meshPassword1"
#define MESH_PORT 5551
#define MESH_CHANNEL 1

#define NODE_ID 07

// #define CARRIER_01
// #define GENERAL_01
// #define GREE_01
// #define GREE_02
// #define MIDEA_01
// #define MIDEA_02
#define MIDEA_03
// #define MIDEA_04
// #define SAMSUNG_01
// #define UNKNOWN_01
// #define UNKNOWN_02
// #define MALAYSIAN_BOARD_01
// #define YORK_01
// #define YORK_02
// #define CHIGO_01
// #define QUNDA

// #define BOARD_VERSION_01
// #define BOARD_VERSION_02
// #define BOARD_VERSION_03
#define BOARD_VERSION_04

// #define SENSOR_VERSION_01
#define SENSOR_VERSION_02

#define FIRMWARE_VERSION "20250918.1407"

#define SLAVE_ID 1
#define START_ADDRESS 0
#define NUM_REGS 2

#define WATCHDOG_TIMEOUT 30
#define MESH_INTERVAL (5 * 60 * 1000)
#define ONE_SECOND 1000

extern uint16_t arr[5];
extern uint16_t sensor_data[2];
extern uint16_t temperature, humidity;
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

#elif defined(QUNDA)

#define irRawLength 83
extern uint16_t ac_on_raw[irRawLength];
extern uint16_t ac_off_raw[irRawLength];
extern uint16_t temp_array[13][irRawLength];
#endif