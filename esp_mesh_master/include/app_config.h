

#pragma once

// WiFi credentials
constexpr const char *WIFI_SSID = "shwapno_v2_mesh";
constexpr const char *WIFI_PASSWORD = "aqualink@321";

// Serial pins
constexpr int RX_PIN = 18;
constexpr int TX_PIN = 17;

// LED pins
constexpr int LED_WIFI_STATUS = 21;
constexpr int LED_RXTX_STATUS = 14;
constexpr int LED_UDP_STATUS = 11;

// Watchdog timeout (seconds)
constexpr int WATCHDOG_TIMEOUT = 30;

// UDP defaults
constexpr int UDP_DEFAULT_PORT = 12345;
constexpr uint8_t UDP_BROADCAST_IP[4] = {255, 255, 255, 255};