#include "debug_print.h"
#include "debug_log.h"
#include "app_config.h"
#include "portal_handler.h"
#include "portal_web.h"
#include "mesh_handler.h"
#include "restart_guard.h"
#include <WiFi.h>
#include <Arduino.h>

bool debugShouldLogPacketDetails()
{
  return !isPortalActive();
}

static const char *acModelString()
{
#if defined(CARRIER_01)
  return "CARRIER_01";
#elif defined(GENERAL_01)
  return "GENERAL_01";
#elif defined(GREE_01)
  return "GREE_01";
#elif defined(GREE_02)
  return "GREE_02";
#elif defined(MIDEA_01)
  return "MIDEA_01";
#elif defined(MIDEA_02)
  return "MIDEA_02";
#elif defined(MIDEA_03)
  return "MIDEA_03";
#elif defined(MIDEA_04)
  return "MIDEA_04";
#elif defined(SAMSUNG_01)
  return "SAMSUNG_01";
#elif defined(UNKNOWN_01)
  return "UNKNOWN_01";
#elif defined(UNKNOWN_02)
  return "UNKNOWN_02";
#elif defined(MALAYSIAN_BOARD_01)
  return "MALAYSIAN_BOARD_01";
#elif defined(YORK_01)
  return "YORK_01";
#elif defined(YORK_02)
  return "YORK_02";
#elif defined(CHIGO_01)
  return "CHIGO_01";
#elif defined(QUNDA_01)
  return "QUNDA_01";
#else
  return "NONE";
#endif
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

static const char *sensorVersionString()
{
#if !TEMP_SENSOR
  return "none";
#elif defined(SENSOR_VERSION_01)
  return "V01";
#elif defined(SENSOR_VERSION_02)
  return "V02";
#else
  return "NONE";
#endif
}

static void formatPortalSsid(char *buf, size_t len)
{
  snprintf(buf, len, "OTA-NODE-%u", (unsigned)NODE_ID);
}

static void logNodeConfig()
{
  debugLog("Firmware: %s", FIRMWARE_VERSION);
  debugLog("Board: %s", boardVersionString());
  debugLog("AC: %s", acModelString());
  debugLog("Sensor: %s", sensorVersionString());
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

void portalInfo()
{
  char portalSsid[32];
  formatPortalSsid(portalSsid, sizeof(portalSsid));

  debugLog("Portal opened (mesh was running)");
  logNodeConfig();
  meshInfo();
  logPortalApDetails(portalSsid);
}

static void logPeriodicSummary()
{
  debugLog("Status: %s %s | node %u | uptime %lus | heap %u",
           boardVersionString(), FIRMWARE_VERSION, (unsigned)NODE_ID,
           (unsigned long)(millis() / 1000UL), (unsigned)ESP.getFreeHeap());
}

static void printPortalPeriodicStatus()
{
  char portalSsid[32];
  formatPortalSsid(portalSsid, sizeof(portalSsid));

  debugLog("Mode: portal | AP %s | clients %u | OTA %s | heap %u",
           portalSsid, (unsigned)WiFi.softAPgetStationNum(),
           portalWeb_isOtaInProgress() ? "yes" : "no", (unsigned)ESP.getFreeHeap());
}

static void printMeshPeriodicStatus()
{
  logNodeConfig();
  debugLog("AC Set Temp: %d  AC Status: %d  AC Hit: %d  AC Cooldown: %d", arr[0], arr[1], arr[2], arr[3]);
#if TEMP_SENSOR
  debugLog("Sensor Temp: %d  Humidity: %d", temperature, humidity);
#else
  debugLog("Sensor: not fitted");
#endif
  debugLog("RSSI : %d", mesh_rssi);
  debugLog("Idle restart %u/%u%s",
           (unsigned)restart_guard_get_count(),
           (unsigned)MAX_CONSECUTIVE_IDLE_RESTARTS,
           restart_guard_is_lockout() ? " LOCKOUT" : "");
}

void printPacket(uint8_t *packet, int packetSize)
{
  if (!debugShouldLogPacketDetails())
    return;

  String line;
  for (int i = 0; i < packetSize; i++)
  {
    char hexBuf[4];
    snprintf(hexBuf, sizeof(hexBuf), "%02X", packet[i]);
    line += hexBuf;
    if (i < packetSize - 1)
      line += ' ';
  }
  debugLog("%s", line.c_str());
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
  printMeshPeriodicStatus();
}
