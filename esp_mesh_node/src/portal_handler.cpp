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

void enterPortalMode(uint8_t nodeId)
{
  if (portalActive)
  {
    portal_touchActivity();
    return;
  }

  activeNodeId = nodeId;
  portal_touchActivity();

  // Stop mesh participation
  mesh_stop();
  delay(300);

  // Clean WiFi state before AP (mesh/AsyncTCP can leave STA active)
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  const IPAddress apIP(PORTAL_AP_IP_1, PORTAL_AP_IP_2, PORTAL_AP_IP_3, PORTAL_AP_IP_4);
  const IPAddress netmask(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netmask);

  char ssid[32];
  snprintf(ssid, sizeof(ssid), "OTA-NODE-%u", (unsigned int)activeNodeId);
  WiFi.softAP(ssid, PORTAL_PASSWORD, 1, 0, 4);

  // Captive portal: resolve all DNS names to the AP IP
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
  portalWeb_poll();

  /* Any WiFi client or open WebSocket keeps the session alive */
  if (WiFi.softAPgetStationNum() > 0 || portalWeb_clientCount() > 0)
    portal_touchActivity();

  unsigned long now = millis();
  unsigned long idleMs = now - lastActivity;
  if (idleMs >= PORTAL_TIMEOUT_MS)
  {
    debugLog("Portal idle timeout (%lu s). Rebooting.", idleMs / 1000);
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
