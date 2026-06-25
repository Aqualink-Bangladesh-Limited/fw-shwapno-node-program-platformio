#include "debug_print.h"
#include "debug_log.h"
#include "app_config.h"
#include "portal_handler.h"
#include "portal_web.h"
#include "bridge_handler.h"
#include "restart_guard.h"
#include <WiFi.h>
#include <Arduino.h>
#include <vector>

static unsigned long secondsSince(unsigned long eventMs)
{
  return (millis() - eventMs) / 1000UL;
}

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

static void formatPortalSsid(char *buf, size_t len)
{
  snprintf(buf, len, "OTA-MASTER-%u", (unsigned)MASTER_ID);
}

static void logMasterIdentity()
{
  debugLog("Board: %s", boardVersionString());
  debugLog("Firmware: %s", FIRMWARE_VERSION);
  debugLog("Master ID: %u", (unsigned)MASTER_ID);
}

static void logPortalApDetails(const char *portalSsid)
{
  debugLog("Portal: on");
  debugLog("AP SSID %s, password %s", portalSsid, PORTAL_PASSWORD);
  debugLog("Open http://%d.%d.%d.%d/ for OTA", PORTAL_AP_IP_1, PORTAL_AP_IP_2,
           PORTAL_AP_IP_3, PORTAL_AP_IP_4);
  debugLog("Exit in web UI or %lu min idle (paused during OTA)",
           (unsigned long)(PORTAL_TIMEOUT_MS / 60000UL));
}

bool debugShouldLogPacketDetails()
{
  return !isPortalActive();
}

static void printPortalPeriodicStatus()
{
  char portalSsid[32];
  formatPortalSsid(portalSsid, sizeof(portalSsid));

  debugLog("Mode: portal | AP %s | clients %u | OTA %s | heap %u",
           portalSsid, (unsigned)WiFi.softAPgetStationNum(),
           portalWeb_isOtaInProgress() ? "yes" : "no", (unsigned)ESP.getFreeHeap());
}

static void logPeriodicSummary()
{
  debugLog("Status: %s %s | master %u | uptime %lus | heap %u",
           boardVersionString(), FIRMWARE_VERSION, (unsigned)MASTER_ID,
           (unsigned long)(millis() / 1000UL), (unsigned)ESP.getFreeHeap());
}

static void printBridgePeriodicStatus()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    const long rssi = WiFi.RSSI();
    debugLog("Mode: bridge | %s | %s | RSSI %ld dBm (%lu)",
             WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), rssi,
             (unsigned long)(rssi * -1));
  }
  else
  {
    debugLog("Mode: bridge | %s (not connected)", WIFI_SSID);
  }

  if (bridge_is_started())
  {
    const unsigned long udpIdleSec = secondsSince(bridge_last_udp_activity());
    const unsigned long uartIdleSec = secondsSince(bridge_last_rxtx_activity());
    debugLog("UDP idle %lus | UART idle %lus | idle restart %u/%u%s",
             udpIdleSec, uartIdleSec,
             (unsigned)restart_guard_get_count(),
             (unsigned)MAX_CONSECUTIVE_IDLE_RESTARTS,
             restart_guard_is_lockout() ? " LOCKOUT" : "");
  }
  else
  {
    debugLog("Idle restart %u/%u%s",
             (unsigned)restart_guard_get_count(),
             (unsigned)MAX_CONSECUTIVE_IDLE_RESTARTS,
             restart_guard_is_lockout() ? " LOCKOUT" : "");
  }
}

static void appendPacketHex(String &line, const uint8_t *packet, int packetSize)
{
  for (int i = 0; i < packetSize; i++)
  {
    char hexBuf[4];
    snprintf(hexBuf, sizeof(hexBuf), "%02X", packet[i]);
    line += hexBuf;
    if (i < packetSize - 1)
      line += ' ';
  }
}

void printPacket(uint8_t *packet, int packetSize)
{
  printPacket(static_cast<const uint8_t *>(packet), packetSize);
}

void printPacket(const uint8_t *packet, int packetSize)
{
  if (!debugShouldLogPacketDetails() || !packet || packetSize <= 0)
    return;

  String line;
  line.reserve(static_cast<size_t>(packetSize) * 3 + 8);
  appendPacketHex(line, packet, packetSize);
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
    char hexBuf[4];
    snprintf(hexBuf, sizeof(hexBuf), "%02X", packet[i]);
    line += hexBuf;
    if (i < packetSize - 1)
      line += ' ';
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
  logMasterIdentity();
  debugLog("SSID: %s", WIFI_SSID);
  debugLog("UDP Port: %u", (unsigned)UDP_DEFAULT_PORT);
}

void portalInfo()
{
  char portalSsid[32];
  formatPortalSsid(portalSsid, sizeof(portalSsid));

  debugLog("Portal opened (bridge was running)");
  masterInfo();
  logPortalApDetails(portalSsid);
}

void printDebugInfo()
{
  if (isPortalActive())
  {
    logPeriodicSummary();
    printPortalPeriodicStatus();
    return;
  }

  logPeriodicSummary();
  printBridgePeriodicStatus();
}
