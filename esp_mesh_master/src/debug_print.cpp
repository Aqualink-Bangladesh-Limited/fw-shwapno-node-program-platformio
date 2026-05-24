#include "debug_print.h"
#include "debug_log.h"
#include "app_config.h"
#include "portal_handler.h"
#include "portal_web.h"
#include <WiFi.h>
#include <Arduino.h>
#include <vector>

static const char *boardVersionString()
{
#if defined(BOARD_VERSION_01)
  return "V01";
#elif defined(BOARD_VERSION_02)
  return "V02";
#elif defined(BOARD_VERSION_03)
  return "V03";
#elif defined(BOARD_VERSION_04)
  return "V04";
#else
  return "NONE";
#endif
}

bool debugShouldLogPacketDetails()
{
  return !isPortalActive();
}

void printPacket(uint8_t *packet, int packetSize)
{
  printPacket(static_cast<const uint8_t *>(packet), packetSize);
}

void printPacket(const uint8_t *packet, int packetSize)
{
  if (!debugShouldLogPacketDetails())
    return;

  String line;
  for (int i = 0; i < packetSize; i++)
  {
    if (packet[i] < 0x10)
      line += "0";
    char hexBuf[4];
    snprintf(hexBuf, sizeof(hexBuf), "%02X", packet[i]);
    line += hexBuf;
    line += " ";
  }
  debugLog("%s", line.c_str());
}

void printPacket(const std::vector<uint8_t> &packet)
{
  printPacket(packet.data(), static_cast<int>(packet.size()));
}

void debugLogPacket(const char *prefix, const uint8_t *packet, int packetSize)
{
  if (!debugShouldLogPacketDetails() || !prefix || !packet || packetSize <= 0)
    return;

  String line(prefix);
  line += ": ";
  line.reserve(line.length() + (static_cast<size_t>(packetSize) * 3) + 4);

  for (int i = 0; i < packetSize; i++)
  {
    if (packet[i] < 0x10)
      line += "0";
    char hexBuf[4];
    snprintf(hexBuf, sizeof(hexBuf), "%02X", packet[i]);
    line += hexBuf;
    if (i < packetSize - 1)
      line += " ";
  }

  debugLogText(line);
}

void debugLogPacket(const char *prefix, const std::vector<uint8_t> &packet)
{
  debugLogPacket(prefix, packet.data(), static_cast<int>(packet.size()));
}

void masterInfo()
{
  debugLog("Bridge Network Details:");
  debugLog("Board: %s", boardVersionString());
  debugLog("Firmware: %s", FIRMWARE_VERSION);
  debugLog("Master ID: %u", (unsigned)MASTER_ID);
  debugLog("SSID: %s", WIFI_SSID);
  debugLog("UDP Port: %u", (unsigned)UDP_DEFAULT_PORT);
}

void printDebugInfo()
{
  debugLog("--- Master Status ---");
  debugLog("Board: %s", boardVersionString());
  debugLog("Firmware: %s", FIRMWARE_VERSION);
  debugLog("Master ID: %u", (unsigned)MASTER_ID);

  if (isPortalActive())
  {
    char portalSsid[32];
    snprintf(portalSsid, sizeof(portalSsid), "OTA-MASTER-%u", (unsigned)MASTER_ID);

    debugLog("Mode: Portal");
    debugLog("WiFi: %s", portalSsid);
    debugLog("IP: %d.%d.%d.%d", PORTAL_AP_IP_1, PORTAL_AP_IP_2, PORTAL_AP_IP_3, PORTAL_AP_IP_4);
    debugLog("RSSI: N/A (portal AP mode)");
    debugLog("AP clients: %u", (unsigned)WiFi.softAPgetStationNum());
    debugLog("OTA upload: %s", portalWeb_isOtaInProgress() ? "yes" : "no");
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    const long rssi = WiFi.RSSI();

    debugLog("Mode: Bridge");
    debugLog("WiFi: %s", WiFi.SSID().c_str());
    debugLog("IP: %s", WiFi.localIP().toString().c_str());
    debugLog("RSSI: %ld dBm (%lu)", rssi, (unsigned long)(rssi * -1));
  }
  else
  {
    debugLog("Mode: Bridge");
    debugLog("WiFi: %s (not connected)", WIFI_SSID);
    debugLog("RSSI: N/A");
  }
}
