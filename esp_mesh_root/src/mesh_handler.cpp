#include "mesh_handler.h"
#include "app_config.h"
#include "debug_log.h"
#include <painlessMesh.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

extern painlessMesh mesh;

static bool meshStarted = false;

void mesh_handler_init()
{
  meshStarted = true;
}

void mesh_handler_update()
{
  if (meshStarted)
    mesh.update();
}

void mesh_handler_stop()
{
  if (!meshStarted)
    return;

  debugLog("mesh stop...");
  mesh.onReceive(nullptr);
  mesh.stop();
  const unsigned long deadline = millis() + 5000;
  while (millis() < deadline)
  {
    yield();
    delay(50);
    esp_task_wdt_reset();
  }
  WiFi.disconnect(true, true);
  delay(500);
  WiFi.mode(WIFI_OFF);
  delay(200);
  meshStarted = false;
  debugLog("Mesh stopped for portal mode");
}

bool mesh_handler_is_started()
{
  return meshStarted;
}

void mesh_handler_update_rssi()
{
  if (!meshStarted)
  {
    mesh_rssi = 0;
    return;
  }

  mesh_rssi = WiFi.RSSI();
}
