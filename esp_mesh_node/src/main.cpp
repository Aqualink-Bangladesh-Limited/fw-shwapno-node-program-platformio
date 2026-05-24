#include <esp_task_wdt.h>
#include "mesh_handler.h"
#include "sensor_handler.h"
#include "ir_handler.h"
#include "led_handler.h"
#include "debug_print.h"
#include "debug_log.h"
#include <Preferences.h>
#include "portal_handler.h"

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

constexpr unsigned long RESTART_TIMEOUT = 900000; // 15 minutes

void setup()
{
  Serial.begin(115200);
  delay(500);

  // Disable Arduino default 5s idle WDT before long mesh/WiFi setup.
  esp_task_wdt_deinit();
  debugLog("boot: setup start");

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
      prefs.putBool("portalBoot", false);
    }
    prefs.end();
  }

  debugLog("boot: modbus_init");
  modbus_init();

  pinMode(LED_MESH_SIGNAL_STATUS, OUTPUT);
  pinMode(LED_AC_STATUS, OUTPUT);
  pinMode(LED_MESH, OUTPUT);

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
  portal_process_deferred_actions();
  modbus_task();

  if (isPortalActive())
    portal_task();
  else
    mesh_task();

  unsigned long currentMillis = millis();
  static unsigned long last_print_time = 0;

  if (currentMillis - last_print_time >= 5000)
  {
    last_print_time = currentMillis;
    printDebugInfo();
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
      currentMillis - lastMeshReceivedTime >= RESTART_TIMEOUT)
  {
    debugLog("Mesh disconnected for 15 minutes. ESP Restarting.....");
    ESP.restart();
  }
}
