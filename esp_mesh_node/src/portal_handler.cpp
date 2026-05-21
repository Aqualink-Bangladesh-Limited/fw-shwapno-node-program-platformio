#include <WiFi.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "portal_handler.h"
#include "app_config.h"
#include "mesh_handler.h"
#include "debug_print.h"
#include "debug_log.h"
#include "portal_web.h"

static bool portalActive = false;
static unsigned long lastActivity = 0;
static uint8_t activeNodeId = 0;

DNSServer dnsServer;

void portal_init()
{
  // nothing to do yet; keep as placeholder for future init
}

static IPAddress ipFromMacro(uint32_t ip)
{
  return IPAddress((ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

void enterPortalMode(uint8_t nodeId)
{
  if (portalActive)
    return;

  activeNodeId = nodeId;

  // Stop mesh participation
  mesh_stop();
  delay(300);

  // Ensure WiFi is reset
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);

  IPAddress apIP = ipFromMacro(PORTAL_AP_IP);
  IPAddress netmask(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netmask);

  char ssid[32];
  snprintf(ssid, sizeof(ssid), "OTA-%u", (unsigned int)activeNodeId);
  WiFi.softAP(ssid, PORTAL_PASSWORD);

  // Start DNS server to capture captive portal redirects
  dnsServer.start(53, "*", apIP);

  // Start web server for portal
  portalWeb_start();

  portalActive = true;
  portal_touchActivity();

  debugLog("Entered PORTAL MODE. SSID=%s IP=%s", ssid, apIP.toString().c_str());
}

void exitPortalMode()
{
  if (!portalActive)
    return;

  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  delay(200);

  // Restart the ESP to rejoin mesh in a clean state
  portalActive = false;
  // stop web server
  portalWeb_stop();
  debugLog("Exiting PORTAL MODE, restarting...");
  Preferences prefs;
  if (prefs.begin("portal", false))
  {
    prefs.putBool("portalBoot", false);
    prefs.end();
  }
  ESP.restart();
}

void portal_task()
{
  if (!portalActive)
    return;

  dnsServer.processNextRequest();

  // Enforce idle timeout
  if (millis() - lastActivity >= PORTAL_TIMEOUT_MS)
  {
    debugLog("Portal idle timeout reached. Rebooting.");
    ESP.restart();
  }
}

bool isPortalActive()
{
  return portalActive;
}

void portal_touchActivity()
{
  lastActivity = millis();
}
