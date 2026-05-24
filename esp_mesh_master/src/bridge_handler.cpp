#include "bridge_handler.h"
#include "app_config.h"
#include "packet_filter.h"
#include "master_modbus_handler.h"
#include "debug_print.h"
#include "debug_log.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <deque>
#include <vector>

static WiFiUDP udp;
static int udpPort = UDP_DEFAULT_PORT;
static IPAddress udpAddress(UDP_BROADCAST_IP[0], UDP_BROADCAST_IP[1],
                            UDP_BROADCAST_IP[2], UDP_BROADCAST_IP[3]);

static unsigned long lastUdpUpdateTime = 0;
static unsigned long lastRxtxUpdateTime = 0;
static bool ledUdpOn = false;
static bool ledRxtxOn = false;

static constexpr int MAX_PACKET_SIZE = 256;
static uint8_t udpBuffer[MAX_PACKET_SIZE];
static uint8_t modbusResponseBuffer[MAX_PACKET_SIZE];
static uint8_t rxBuffer[MAX_PACKET_SIZE];
static int rxBufferIndex = 0;

static unsigned long lastReceivedTime = 0;
static constexpr unsigned long SERIAL_TIMEOUT = 10;

static std::deque<std::vector<uint8_t>> packetQueue;
static constexpr int MAX_QUEUE_SIZE = 20;

static unsigned long lastConnectedTime = 0;
static constexpr unsigned long WIFI_TIMEOUT = 300000;
static int restartCount = 0;
static constexpr int MAX_RESTARTS = 50;
static constexpr unsigned long RESTART_TIMEOUT = 900000;
static bool wifiConnected = false;
static bool bridgeStarted = false;

bool bridge_is_started()
{
  return bridgeStarted;
}

unsigned long bridge_last_udp_activity()
{
  return lastUdpUpdateTime;
}

unsigned long bridge_last_rxtx_activity()
{
  return lastRxtxUpdateTime;
}

bool bridge_led_udp_on()
{
  return ledUdpOn;
}

bool bridge_led_rxtx_on()
{
  return ledRxtxOn;
}

void bridge_refresh_led_flags()
{
  const unsigned long now = millis();
  if (now - lastRxtxUpdateTime >= 2000)
    ledRxtxOn = false;
  if (now - lastUdpUpdateTime >= 2000)
    ledUdpOn = false;
}

static void initWiFi()
{
  debugLog("Initializing Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

static void connectToWiFi()
{
  if (millis() - lastConnectedTime >= WIFI_TIMEOUT)
  {
    debugLog("Failed to connect to Wi-Fi in 5 minutes. Restarting...");

    restartCount++;
    EEPROM.write(0, restartCount);
    EEPROM.commit();

    if (restartCount < MAX_RESTARTS)
    {
      debugLog("ESP Restarting.....");
      ESP.restart();
    }
  }
}

void bridge_init()
{
  const unsigned long now = millis();
  lastUdpUpdateTime = now;
  lastRxtxUpdateTime = now;

  EEPROM.begin(512);
  initWiFi();
  udp.begin(udpPort);
  bridgeStarted = true;
  masterInfo();
  debugLog("UDP server listening on port %d", udpPort);
}

void bridge_wifi_update()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
    wifiConnected = false;
    return;
  }

  lastConnectedTime = millis();

  if (wifiConnected)
    return;

  wifiConnected = true;
  debugLog("Connected to Wi-Fi: %s", WiFi.SSID().c_str());
  debugLog("IP Address: %s", WiFi.localIP().toString().c_str());
  printDebugInfo();

  restartCount = 0;
  EEPROM.write(0, restartCount);
  EEPROM.commit();
}

void bridge_process_udp_serial()
{
  const unsigned long currentMillis = millis();

  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    udpAddress = udp.remoteIP();
    udpPort = udp.remotePort();

    int len = udp.read(udpBuffer, MAX_PACKET_SIZE);
    if (len > 0)
      udpBuffer[len] = 0;

    if (debugShouldLogPacketDetails())
    {
      char prefix[64];
      snprintf(prefix, sizeof(prefix), "UDP from %s:%d", udpAddress.toString().c_str(), udpPort);
      debugLogPacket(prefix, udpBuffer, len);
    }

    if (isPortalTriggerForMaster(udpBuffer, len))
    {
      debugLog("Portal trigger for master %u", (unsigned)MASTER_ID);
      portalRequested = true;
      portalRequestTime = currentMillis;
      lastUdpUpdateTime = currentMillis;
    }
    else if (isModbusRequestForMaster(udpBuffer, len))
    {
      const int respLen = masterModbusHandler(udpBuffer, len, modbusResponseBuffer,
                                              MAX_PACKET_SIZE);
      if (respLen > 0)
      {
        char prefix[80];
        snprintf(prefix, sizeof(prefix), "Master Modbus reply to %s:%d",
                 udpAddress.toString().c_str(), udpPort);
        debugLogPacket(prefix, modbusResponseBuffer, respLen);
        udp.beginPacket(udpAddress, udpPort);
        udp.write(modbusResponseBuffer, respLen);
        udp.endPacket();
        ledUdpOn = true;
        lastUdpUpdateTime = currentMillis;
      }
    }
    else if (isValidPacket(udpBuffer, len))
    {
      if (packetQueue.size() >= MAX_QUEUE_SIZE)
        packetQueue.pop_front();

      std::vector<uint8_t> packet;
      for (int i = 0; i < 4; i++)
        packet.push_back(udpAddress[i]);
      packet.push_back(static_cast<uint8_t>(udpPort >> 8));
      packet.push_back(static_cast<uint8_t>(udpPort & 0xFF));
      packet.insert(packet.end(), udpBuffer, udpBuffer + len);
      packetQueue.push_back(packet);

      ledUdpOn = true;
      lastUdpUpdateTime = currentMillis;
    }
    else
    {
      debugLog("Packet discarded: invalid MBAP frame");
    }
  }

  while (Serial2.available())
  {
    uint8_t incomingByte = Serial2.read();
    lastReceivedTime = currentMillis;

    if (rxBufferIndex < MAX_PACKET_SIZE - 1)
      rxBuffer[rxBufferIndex++] = incomingByte;
    else
    {
      debugLog("Serial buffer overflow, discarding data");
      rxBufferIndex = 0;
    }
  }

  rxBuffer[rxBufferIndex] = '\0';

  if (millis() - lastReceivedTime > SERIAL_TIMEOUT && rxBufferIndex > 0)
  {
    if (debugShouldLogPacketDetails())
      debugLogPacket("Serial RX packet", rxBuffer, rxBufferIndex);

    if (rxBufferIndex > 6)
    {
      IPAddress destinationIP(rxBuffer[0], rxBuffer[1], rxBuffer[2], rxBuffer[3]);
      uint16_t destinationPort = (static_cast<uint16_t>(rxBuffer[4]) << 8) | rxBuffer[5];
      uint8_t *packetData = rxBuffer + 6;
      int packetDataLength = rxBufferIndex - 6;

      debugLog("UDP reply to %s:%u", destinationIP.toString().c_str(), destinationPort);

      udp.beginPacket(destinationIP, destinationPort);
      udp.write(packetData, packetDataLength);
      udp.endPacket();

      ledRxtxOn = true;
      lastRxtxUpdateTime = currentMillis;
    }
    else
    {
      debugLog("Serial packet too short, discarding");
    }

    rxBufferIndex = 0;
  }
}

void bridge_send_queue()
{
  static unsigned long lastSendTime = 0;
  constexpr unsigned long interval = 100;

  if (millis() - lastSendTime < interval)
    return;

  lastSendTime = millis();

  if (packetQueue.empty())
    return;

  const std::vector<uint8_t> &packet = packetQueue.front();
  Serial2.write(packet.data(), packet.size());
  if (debugShouldLogPacketDetails())
    debugLogPacket("Sending packet to root", packet);
  packetQueue.pop_front();
}

void bridge_check_idle_restart()
{
  const unsigned long currentMillis = millis();

  if (currentMillis - lastUdpUpdateTime > RESTART_TIMEOUT)
  {
    debugLog("No UDP data in 15 minutes. Restarting...");
    ESP.restart();
  }

  if (currentMillis - lastRxtxUpdateTime > RESTART_TIMEOUT)
  {
    debugLog("No TX/RX data in 15 minutes. Restarting...");
    ESP.restart();
  }
}

void bridge_stop()
{
  udp.stop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  packetQueue.clear();
  rxBufferIndex = 0;
  wifiConnected = false;
  bridgeStarted = false;
  delay(200);
  debugLog("Bridge stopped for portal mode");
}
