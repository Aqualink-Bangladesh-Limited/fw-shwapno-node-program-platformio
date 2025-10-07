

#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

// WiFi credentials
constexpr const char *WIFI_SSID = "Aqualink_Sensometer";
constexpr const char *WIFI_PASSWORD = "aqualink@321";

// Serial pins
constexpr int RX_PIN = 18;
constexpr int TX_PIN = 17;

#define NODE_ID 1


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
#define CHIGO_01


// LED pins
constexpr int LED_WIFI_STATUS = 21;
constexpr int LED_RXTX_STATUS = 14;
constexpr int LED_BLINK = 11;
constexpr int IR_PIN = 45;

// Watchdog timeout (seconds)
constexpr int WATCHDOG_TIMEOUT = 30;

// UDP defaults
constexpr int UDP_DEFAULT_PORT = 12345;
constexpr uint8_t UDP_BROADCAST_IP[4] = {255, 255, 255, 255};

extern uint16_t arr[3];
extern bool flag;





#if defined(CARRIER_01)

#define irRawLength 91
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
extern uint16_t ac_on_raw[2][irRawLength];
extern uint16_t ac_off_raw[2][irRawLength];
extern uint16_t temp_array[13][2][irRawLength];

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
#endif




