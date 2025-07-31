#include <painlessMesh.h>
#include "app_config.h"

extern unsigned long lastMeshReceivedTime;

painlessMesh mesh;

void meshInfo();

void receivedCallback(uint32_t from, String &msg)
{
  lastMeshReceivedTime = millis();
  Serial.printf("Received message from node %u: %s\n", from, msg.c_str());
}

void mesh_init()
{
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.onReceive(receivedCallback);
  meshInfo();
}

void mesh_task()
{
  mesh.update();
}

void meshInfo()
{
  Serial.println("Mesh Network Details:");
  Serial.print("Prefix: ");
  Serial.println(MESH_PREFIX);
  Serial.print("Password: ");
  Serial.println(MESH_PASSWORD);
  Serial.print("Port: ");
  Serial.println(MESH_PORT);
  Serial.print("Channel: ");
  Serial.println(MESH_CHANNEL);
  Serial.print("Mesh ID: ");
  Serial.println(mesh.getNodeId());
}