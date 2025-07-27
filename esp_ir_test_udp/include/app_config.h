

#pragma once

// WiFi credentials
constexpr const char *WIFI_SSID = "Aqualink_Sensometer";
constexpr const char *WIFI_PASSWORD = "aqualink@321";

// Serial pins
constexpr int RX_PIN = 5;
constexpr int TX_PIN = 4;

#define NODE_ID 1
#define irRawLength 279

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