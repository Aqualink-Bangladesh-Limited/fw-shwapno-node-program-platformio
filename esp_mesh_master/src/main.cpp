#include <Arduino.h>
#include <esp_task_wdt.h>
#include <Preferences.h>
#include "app_config.h"
#include "bridge_handler.h"
#include "button_handler.h"
#include "led_handler.h"
#include "debug_log.h"
#include "portal_handler.h"
#include "debug_print.h"

// Portal flags (defined here because app_config.h declares them extern)
bool portalRequested = false;
unsigned long portalRequestTime = 0;
bool portalBootOnNextBoot = false;
uint8_t portalStoredDeviceId = MASTER_ID;

void setup()
{
  Serial.begin(115200);
  delay(500);

  // Disable Arduino default 5s idle WDT before long WiFi setup.
  esp_task_wdt_deinit();
  debugLog("boot: setup start");

  // Post-OTA: re-enter portal for verification (before bridge starts).
  Preferences prefs;
  if (prefs.begin("portal", false))
  {
    bool pb = prefs.getBool("portalBoot", false);
    if (pb)
    {
      portalBootOnNextBoot = true;
      uint32_t nid = prefs.getUInt("nodeId", MASTER_ID);
      portalStoredDeviceId = (uint8_t)(nid & 0xFF);
      prefs.putBool("portalBoot", false);
    }
    prefs.end();
  }

  pinMode(LED_WIFI_STATUS, OUTPUT);
  pinMode(LED_RXTX_STATUS, OUTPUT);
  pinMode(LED_UDP_STATUS, OUTPUT);

  button_init();
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  if (portalBootOnNextBoot)
  {
    debugLog("boot: portal mode (post-OTA)");
    enterPortalMode(portalStoredDeviceId);
  }
  else
  {
    debugLog("boot: bridge_init");
    bridge_init();
    portal_init();
  }

  // App watchdog (30s) after setup; default 5s WDT stays off until here.
  esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  debugLog("Watchdog timer set for %d seconds", WATCHDOG_TIMEOUT);

  debugLog("boot: setup done");
}

void loop()
{
  esp_task_wdt_reset();
  led_handler();
  button_task();
  portal_process_deferred_actions();

  if (isPortalActive())
    portal_task();
  else
  {
    bridge_wifi_update();
    bridge_process_udp_serial();
    bridge_send_queue();
    bridge_check_idle_restart();
  }

  if (!isPortalActive() && portalRequested)
  {
    portalRequested = false;
    enterPortalMode(MASTER_ID);
  }

  unsigned long currentMillis = millis();
  static unsigned long last_print_time = 0;
  const unsigned long printInterval = isPortalActive()
                                          ? DEBUG_PRINT_INTERVAL_PORTAL_MS
                                          : DEBUG_PRINT_INTERVAL_BRIDGE_MS;

  if (currentMillis - last_print_time >= printInterval)
  {
    last_print_time = currentMillis;
    printDebugInfo();
  }
}
