#include <Arduino.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <deque>
#include <vector>
#include <EEPROM.h>
#include "app_config.h"
#include "debug_print.h"

// Runtime UDP objects/variables
WiFiUDP udp;
int udpPort = UDP_DEFAULT_PORT;
IPAddress udpAddress(UDP_BROADCAST_IP[0], UDP_BROADCAST_IP[1], UDP_BROADCAST_IP[2], UDP_BROADCAST_IP[3]);

unsigned long lastUdpUpdateTime = 0;
unsigned long lastRxtxUpdateTime = 0;
constexpr unsigned long LED_UPDATE_INTERVAL = 500;
bool ledUdpOn = false;
bool ledRxtxOn = false;

constexpr int MAX_PACKET_SIZE = 256;
uint8_t udpBuffer[MAX_PACKET_SIZE];
uint8_t rxBuffer[MAX_PACKET_SIZE];
int rxBufferIndex = 0;

unsigned long lastReceivedTime = 0;
constexpr unsigned long SERIAL_TIMEOUT = 10;

unsigned long lastBlinkTime = 0;
constexpr unsigned long BLINK_INTERVAL = 500;
bool ledState = false;

std::deque<std::vector<uint8_t>> packetQueue;
constexpr int MAX_QUEUE_SIZE = 20;

unsigned long lastConnectedTime = 0;
constexpr unsigned long WIFI_TIMEOUT = 300000; // 5 minutes
int restartCount = 0;
constexpr int MAX_RESTARTS = 50;

constexpr unsigned long RESTART_TIMEOUT = 900000; // 15 minutes
bool wifiConnected = false;

void initWiFi();
void connectToWiFi();
void processUdpAndSerial();
void sendMessagesFromQueue();
void sendTcpRequest(int nodeId, int requestId);
bool isValidPacket(const uint8_t *packet, int len);

void setup()
{
  Serial.begin(115200);

  esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.println("Watchdog timer set for " + String(WATCHDOG_TIMEOUT) + " seconds");

  pinMode(LED_WIFI_STATUS, OUTPUT);
  pinMode(LED_RXTX_STATUS, OUTPUT);
  pinMode(LED_UDP_STATUS, OUTPUT);

  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  initWiFi();

  udp.begin(udpPort);

  Serial.println("UDP server is listening on port " + String(udpPort));
}

void loop()
{
  esp_task_wdt_reset();

  unsigned long currentMillis = millis();

  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
    wifiConnected = false;
  }
  else
  {
    lastConnectedTime = millis();

    if (!wifiConnected)
    {
      wifiConnected = true;

      Serial.println("Connected to Wi-Fi!");
      Serial.print("WiFi Name (SSID): ");
      Serial.println(WiFi.SSID());
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());

      restartCount = 0;
      EEPROM.write(0, restartCount);
      EEPROM.commit();
    }
  }

  processUdpAndSerial();

  if (currentMillis - lastRxtxUpdateTime < 2000 && ledRxtxOn)
  {
    digitalWrite(LED_RXTX_STATUS, HIGH);
  }
  else
  {
    digitalWrite(LED_RXTX_STATUS, LOW);
    ledRxtxOn = false;
  }

  if (currentMillis - lastUdpUpdateTime < 2000 && ledUdpOn)
  {
    digitalWrite(LED_UDP_STATUS, HIGH);
  }
  else
  {
    digitalWrite(LED_UDP_STATUS, LOW);
    ledUdpOn = false;
  }

  sendMessagesFromQueue();

  if (currentMillis - lastUdpUpdateTime > RESTART_TIMEOUT)
  {
    Serial.println("No UDP data received in 15 minutes. Restarting ESP...");
    ESP.restart();
  }

  if (currentMillis - lastRxtxUpdateTime > RESTART_TIMEOUT)
  {
    Serial.println("No TX/RX data received in 15 minutes. Restarting ESP...");
    ESP.restart();
  }

  static unsigned long lastWifiBlinkTime = 0;
  static bool wifiLedState = false;

  if (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_WIFI_STATUS, HIGH);
  }
  else
  {
    long rssi = WiFi.RSSI();
    unsigned long blinkInterval;
    if (rssi > -40)
    {
      blinkInterval = 500;
    }
    else if (rssi > -60)
    {
      blinkInterval = 1000;
    }
    else
    {
      blinkInterval = 2000;
    }

    if (millis() - lastWifiBlinkTime >= blinkInterval)
    {
      lastWifiBlinkTime = millis();
      wifiLedState = !wifiLedState;
      digitalWrite(LED_WIFI_STATUS, wifiLedState ? HIGH : LOW);
    }
  }
}

void initWiFi()
{
  Serial.println("Initializing Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToWiFi()
{
  if (millis() - lastConnectedTime >= WIFI_TIMEOUT)
  {
    Serial.println("Failed to connect to Wi-Fi in 1 minute. Restarting...");

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
      if (packetQueue.size() >= MAX_QUEUE_SIZE)
        packetQueue.pop_front();

      std::vector<uint8_t> packet;
      for (int i = 0; i < 4; i++)
        packet.push_back(udpAddress[i]);
      packet.push_back(udpPort >> 8);
      packet.push_back(udpPort & 0xFF);
      packet.insert(packet.end(), udpBuffer, udpBuffer + len);
      packetQueue.push_back(packet);

      ledUdpOn = true;
      lastUdpUpdateTime = currentMillis;
    }
    else
    {
      Serial.println("Packet discarded: Not a TCP request.");
    }
  }

  while (Serial2.available())
  {
    uint8_t incomingByte = Serial2.read();
    lastReceivedTime = currentMillis;

    if (rxBufferIndex < MAX_PACKET_SIZE - 1)
    {
      rxBuffer[rxBufferIndex++] = incomingByte;
    }
    else
    {
      Serial.println("Buffer overflow, discarding data...");
      rxBufferIndex = 0;
    }
  }

  rxBuffer[rxBufferIndex] = '\0';

  if (millis() - lastReceivedTime > SERIAL_TIMEOUT && rxBufferIndex > 0)
  {
    Serial.print("Received from RX/TX: ");
    printPacket(rxBuffer, rxBufferIndex);

    if (rxBufferIndex > 6)
    {
      IPAddress destinationIP(rxBuffer[0], rxBuffer[1], rxBuffer[2], rxBuffer[3]);
      uint16_t destinationPort = (rxBuffer[4] << 8) | rxBuffer[5];
      uint8_t *packetData = rxBuffer + 6;
      int packetDataLength = rxBufferIndex - 6;

      Serial.print("Sending data to IP: ");
      Serial.print(destinationIP);
      Serial.print(", Port: ");
      Serial.println(destinationPort);

      udp.beginPacket(destinationIP, destinationPort);
      udp.write(packetData, packetDataLength);
      udp.endPacket();

      ledRxtxOn = true;
      lastRxtxUpdateTime = currentMillis;
    }
    else
    {
      Serial.println("Packet length is less than 7 bytes, discarding data.");
    }

    rxBufferIndex = 0;
  }
}

void sendMessagesFromQueue()
{
  unsigned long currentMillis = millis();
  static unsigned long lastSendTime = 0;
  constexpr unsigned long interval = 100;

  if (currentMillis - lastSendTime >= interval)
  {
    lastSendTime = currentMillis;

    if (!packetQueue.empty())
    {
      const std::vector<uint8_t> &packet = packetQueue.front();
      Serial2.write(packet.data(), packet.size());
      Serial.print("Sending packet to Root: ");
      printPacket(packet);
      packetQueue.pop_front();
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
