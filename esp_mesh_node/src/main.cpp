#include <esp_task_wdt.h>
#include "mesh_handler.h"
#include "sensor_handler.h"
#include "ir_handler.h"
#include "led_handler.h"
#include "debug_print.h"
#include "debug_log.h"
#include <Preferences.h>
#include "portal_handler.h"
#include "button_handler.h"
#include "restart_guard.h"
#include "ota_rollback.h"

uint16_t arr[5] = {0, 0, 0, 5, 30};
uint16_t sensor_data[2] = {0xFFFF, 0xFFFF};
uint16_t temperature = 0xFFFF, humidity = 0xFFFF;
bool flag = false;
unsigned long last_read_time = 0;
unsigned long last_ir_send_time = 0;
unsigned long lastMeshReceivedTime = 0;

// Portal flags (defined here because app_config.h declares them extern)
bool portalRequested = false;
unsigned long portalRequestTime = 0;
bool portalBootOnNextBoot = false;
uint8_t portalStoredNodeId = NODE_ID;
bool meshTrafficSeen = false;

void setup()
{
  Serial.begin(115200);
  delay(500);

  // Disable Arduino default 5s idle WDT before long mesh/WiFi setup.
  esp_task_wdt_deinit();
  debugLog("boot: setup start");
  ota_rollback_early_check();
  restart_guard_init();

  // Post-OTA: re-enter portal for verification (before mesh starts).
  Preferences prefs;
  if (prefs.begin("portal", false))
  {
    bool pb = prefs.getBool("portalBoot", false);
    if (pb)
    {
      portalBootOnNextBoot = true;
      uint32_t nid = prefs.getUInt("nodeId", NODE_ID);
      portalStoredNodeId = (uint8_t)(nid & 0xFF);
    }
    else if (ota_rollback_requires_portal_boot())
    {
      portalBootOnNextBoot = true;
      uint32_t nid = prefs.getUInt("nodeId", NODE_ID);
      portalStoredNodeId = (uint8_t)(nid & 0xFF);
      debugLog("boot: pending OTA verify, forcing portal");
    }
    prefs.end();
  }

  debugLog("boot: modbus_init");
  modbus_init();

  pinMode(LED_MESH_SIGNAL_STATUS, OUTPUT);
  pinMode(LED_AC_STATUS, OUTPUT);
  pinMode(LED_MESH, OUTPUT);

  button_init();

  if (portalBootOnNextBoot)
  {
    debugLog("boot: portal mode (post-OTA)");
    ir_init();
    enterPortalMode(portalStoredNodeId);
  }
  else
  {
    debugLog("boot: mesh_init");
    mesh_init();

    debugLog("boot: ir_init");
    ir_init();

    lastMeshReceivedTime = millis();
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
  modbus_task();

  if (isPortalActive())
    portal_task();
  else
    mesh_task();

  unsigned long currentMillis = millis();
  static unsigned long last_print_time = 0;
  constexpr unsigned long printInterval = 5000;

  if (currentMillis - last_print_time >= printInterval)
  {
    const bool portalStatusReady = !isPortalActive() ||
                                   (currentMillis - portal_enteredAtMs() >= printInterval);
    if (portalStatusReady)
    {
      last_print_time = currentMillis;
      printDebugInfo();
    }
  }

  unsigned long sensor_read_interval = arr[4] * ONE_SECOND;
  if (currentMillis - last_read_time >= sensor_read_interval)
  {
    last_read_time = currentMillis;
    ReadTemperatureHumidity();
  }

  unsigned long ir_cooldown_time = arr[3] * ONE_SECOND;
  if (flag && (currentMillis - last_ir_send_time >= ir_cooldown_time))
  {
    last_ir_send_time = currentMillis;
    ir_handler();
    flag = false;
  }

  if (!isPortalActive() && portalRequested)
  {
    portalRequested = false;
    enterPortalMode(NODE_ID);
  }

  if (!isPortalActive() && meshTrafficSeen &&
      currentMillis - lastMeshReceivedTime >= RESTART_TIMEOUT_MS)
  {
    restart_guard_request_idle_restart("no mesh RX for 15 min");
  }
}
