#include <WiFi.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Update.h>
#include "portal_handler.h"
#include "app_config.h"
#include "mesh_handler.h"
#include "debug_print.h"
#include "debug_log.h"
#include "portal_web.h"

static bool portalActive = false;
static volatile unsigned long lastActivity = 0;
static unsigned long portalEnteredAt = 0;

static unsigned long portal_elapsed_ms(unsigned long now, unsigned long last)
{
  if (now >= last)
    return now - last;
  /* now < last: millis wrap (last very old) vs brief race (last just updated) */
  if ((last - now) < 60000UL)
    return 0;
  return (ULONG_MAX - last) + now + 1;
}

static bool portal_idle_timeout_expired(unsigned long now, unsigned long lastSnap)
{
  if (portalWeb_isOtaInProgress())
    return false;
  /* Ignore false triggers right after portal starts */
  if (portalEnteredAt != 0 && portal_elapsed_ms(now, portalEnteredAt) < 60000UL)
    return false;
  return portal_elapsed_ms(now, lastSnap) >= PORTAL_TIMEOUT_MS;
}
static uint8_t activeNodeId = 0;
static volatile bool portalExitPending = false;
static volatile bool portalOtaRebootPending = false;

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
  portalEnteredAt = millis();
  portal_touchActivity();

  /* Only stop mesh if it was started (skip on post-OTA portal boot) */
  if (mesh_is_started())
    mesh_stop();

  WiFi.mode(WIFI_OFF);
  delay(200);
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

  debugLog("Entered PORTAL MODE. SSID=%s IP=%d.%d.%d.%d", ssid,
           PORTAL_AP_IP_1, PORTAL_AP_IP_2, PORTAL_AP_IP_3, PORTAL_AP_IP_4);
}

void exitPortalMode()
{
  if (!portalActive)
    return;

  portalActive = false;
  portalWeb_stop();
  delay(100);
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  delay(200);

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

  if (!portalWeb_isOtaInProgress())
  {
    static unsigned long lastDnsMs = 0;
    if (millis() - lastDnsMs >= 10)
    {
      lastDnsMs = millis();
      dnsServer.processNextRequest();
    }
  }

  const unsigned long now = millis();
  const unsigned long lastSnap = lastActivity;

  /* Keep session alive: WiFi client connected or firmware uploading */
  if (WiFi.softAPgetStationNum() > 0 || portalWeb_isOtaInProgress())
    portal_touchActivity();

  if (portal_idle_timeout_expired(now, lastSnap))
  {
    const unsigned long idleSec = portal_elapsed_ms(now, lastSnap) / 1000;
    debugLog("Portal idle timeout (%lu s). Rebooting.", idleSec);
    portal_schedule_exit();
  }
}

void portal_schedule_exit()
{
  portalExitPending = true;
}

void portal_schedule_ota_reboot()
{
  portalOtaRebootPending = true;
}

void portal_process_deferred_actions()
{
  if (portalOtaRebootPending)
  {
    portalOtaRebootPending = false;
    portalActive = false;
    debugLog("OTA reboot (full chip reset, skip web teardown)");
    delay(500);
    ESP.restart();
  }

  if (portalExitPending)
  {
    portalExitPending = false;
    if (portalWeb_isOtaInProgress() && Update.isRunning())
      Update.abort();
    portalActive = false;
    Preferences prefs;
    if (prefs.begin("portal", false))
    {
      prefs.putBool("portalBoot", false);
      prefs.end();
    }
    debugLog("Portal restarting (clean reset)...");
    delay(100);
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
