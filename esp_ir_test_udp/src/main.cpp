#include <Arduino.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <deque>
#include <vector>
#include <EEPROM.h>
#include "app_config.h"
#include "debug_print.h"
#include "request_handler.h"
#include "ir_handler.h"

// Runtime UDP objects/variables
WiFiUDP udp;
int udpPort = UDP_DEFAULT_PORT;
IPAddress udpAddress(UDP_BROADCAST_IP[0], UDP_BROADCAST_IP[1], UDP_BROADCAST_IP[2], UDP_BROADCAST_IP[3]);

constexpr int MAX_PACKET_SIZE = 256;
uint8_t udpBuffer[MAX_PACKET_SIZE];

unsigned long lastBlinkTime = 0;
constexpr unsigned long BLINK_INTERVAL = 500;
bool ledState = false;

unsigned long lastAttemptTime = 0;
constexpr unsigned long WIFI_TIMEOUT = 60000;
int restartCount = 0;
constexpr int MAX_RESTARTS = 100;
unsigned long last_ir_send_time = 0;
bool flag = false;

void connectToWiFi();
void processUdpAndSerial();
void sendTcpRequest(int nodeId, int requestId);
bool isValidPacket(const uint8_t *packet, int len);

uint16_t arr[3] = {0, 0, 0};

void setup()
{
  Serial.begin(115200);

  esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.println("Watchdog timer set for " + String(WATCHDOG_TIMEOUT) + " seconds");

  pinMode(LED_WIFI_STATUS, OUTPUT);
  pinMode(LED_RXTX_STATUS, OUTPUT);
  pinMode(LED_BLINK, OUTPUT);

  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  connectToWiFi();

  udp.begin(udpPort);

  Serial.println("UDP server is listening on port " + String(udpPort));
  ir_init();
}

void loop()
{
  esp_task_wdt_reset();

  unsigned long currentMillis = millis();

  digitalWrite(LED_WIFI_STATUS, WiFi.status() == WL_CONNECTED ? HIGH : LOW);
  if (WiFi.status() != WL_CONNECTED)
    connectToWiFi();

  processUdpAndSerial();

  if (currentMillis - lastBlinkTime >= BLINK_INTERVAL)
  {
    lastBlinkTime = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_BLINK, ledState ? HIGH : LOW);
  }

  static unsigned long last_print_time = 0;

  if (currentMillis - last_print_time >= 5000)
  {
    last_print_time = currentMillis;
    Serial.printf("AC Set Temp: %d  AC Status: %d  AC Hit: %d\n", arr[0], arr[1], arr[2]);
  }

  if (flag)
  {
    last_ir_send_time = currentMillis;
    ir_handler();
    flag = false;
  }
}

void connectToWiFi()
{
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");

    if (millis() - lastAttemptTime >= WIFI_TIMEOUT)
    {
      Serial.println("\nFailed to connect to Wi-Fi in 1 minute. Restarting...");

      restartCount++;
      EEPROM.write(0, restartCount);
      EEPROM.commit();

      if (restartCount < MAX_RESTARTS)
      {
        Serial.println("ESP Restarting.....");
        ESP.restart();
      }
    }
  }
  Serial.println("Connected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  restartCount = 0;
  EEPROM.write(0, restartCount);
  EEPROM.commit();
}

void processUdpAndSerial()
{
  unsigned long currentMillis = millis();

  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    udpAddress = udp.remoteIP();
    udpPort = udp.remotePort();

    Serial.print("Received data from IP: ");
    Serial.print(udpAddress);
    Serial.print(", Port: ");
    Serial.println(udpPort);

    int len = udp.read(udpBuffer, MAX_PACKET_SIZE);
    if (len > 0)
      udpBuffer[len] = 0;

    Serial.print("Received UDP packet: ");
    printPacket(udpBuffer, len);

    if (isValidPacket(udpBuffer, len))
    {
      if (len < 7)
      {
        Serial.println("Invalid packet size. Discarding.");
        return;
      }

      uint8_t slaveId = udpBuffer[6]; // Slave ID is at position 12 in the Modbus TCP packet

      if (slaveId == NODE_ID)
      {
        uint8_t responsePacket[256];
        uint8_t len = 0;

        len = modbusRequestHandler(&udpBuffer[0], &responsePacket[0]);

        // Copy Modbus TCP header fields
        responsePacket[0] = udpBuffer[0];     // Transaction ID high byte
        responsePacket[1] = udpBuffer[1];     // Transaction ID low byte
        responsePacket[2] = udpBuffer[2];     // Protocol ID high byte
        responsePacket[3] = udpBuffer[3];     // Protocol ID low byte
        responsePacket[4] = (len - 6) >> 8;   // Length high byte
        responsePacket[5] = (len - 6) & 0xFF; // Length low byte
        responsePacket[6] = udpBuffer[6];     // Slave ID

        Serial.println("Response Packet: ");
        printPacket(responsePacket, len);

        Serial.print("Sending data to IP: ");
        Serial.print(udpAddress);
        Serial.print(", Port: ");
        Serial.println(udpPort);

        udp.beginPacket(udpAddress, udpPort);
        udp.write(responsePacket, len);
        udp.endPacket();
      }
    }
    else
    {
      Serial.println("Packet discarded: Not a TCP request.");
    }
  }
}

bool isValidPacket(const uint8_t *packet, int len)
{
  if (len < 6)
    return false;
  uint16_t length = (packet[4] << 8) | packet[5];
  uint16_t expectedLength = 6 + length;
  return (len == expectedLength);
}
