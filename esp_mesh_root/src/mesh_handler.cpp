#include "mesh_handler.h"
#include "app_config.h"
#include "debug_log.h"
#include <painlessMesh.h>
#include <WiFi.h>

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

  mesh.stop();
  meshStarted = false;
  delay(200);
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
