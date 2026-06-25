#include "app_config.h"
#include <painlessMesh.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include "request_handler.h"
#include "data_type_conversion.h"
#include "debug_print.h"
#include "debug_log.h"
#include "packet_filter.h"
#include "restart_guard.h"

painlessMesh mesh;
long mesh_rssi = 0;
static bool meshStarted = false;

bool mesh_is_started()
{
  return meshStarted;
}

void meshInfo();

void receivedCallback(uint32_t from, String &msg)
{
  lastMeshReceivedTime = millis();
  meshTrafficSeen = true;
  mesh_rssi = WiFi.RSSI();
  debugLog("Received message from node %u: %s", from, msg.c_str());

  if (msg.startsWith("SLAVE_ID_REQUEST:"))
  {
    int requestedId = msg.substring(17).toInt();
    if (requestedId == NODE_ID)
    {
      String response = "SLAVE_ID_RESPONSE:" + String(NODE_ID);
      mesh.sendSingle(from, response);
    }
    return;
  }

  if (msg.equals(MESH_DUMMY_BROADCAST_MSG))
    return;

  uint8_t modbusPacket[256];
  int modbusPacketSize = hexStringToByteArray(msg, modbusPacket);

  if (isPortalTriggerForNode(modbusPacket, modbusPacketSize))
  {
    portalRequested = true;
    portalRequestTime = millis();
    debugLog("Portal requested for this node.");
    return;
  }

  int mbapLen = 0;
  const uint8_t *mbap = getMbapPayload(modbusPacket, modbusPacketSize, &mbapLen);
  if (mbap == nullptr || mbapLen < 6)
  {
    debugLog("Invalid packet size. Discarding.");
    return;
  }

  const bool hasRoutePrefix = (mbap != modbusPacket);
  if (!hasRoutePrefix)
  {
    debugLog("Invalid packet: missing route prefix. Discarding.");
    return;
  }

  uint8_t slaveId = mbap[6];

  if (slaveId == NODE_ID)
  {
    uint8_t responsePacket[256];
    uint8_t len = 0;

    // Copy IP and port
    for (int i = 0; i < 6; ++i)
    {
      responsePacket[i] = modbusPacket[i];
    }

    len = modbusRequestHandler(&modbusPacket[6], &responsePacket[6]);

    // Copy Modbus TCP header fields
    responsePacket[6] = modbusPacket[6];   // Transaction ID high byte
    responsePacket[7] = modbusPacket[7];   // Transaction ID low byte
    responsePacket[8] = modbusPacket[8];   // Protocol ID high byte
    responsePacket[9] = modbusPacket[9];   // Protocol ID low byte
    responsePacket[10] = (len - 6) >> 8;   // Length high byte
    responsePacket[11] = (len - 6) & 0xFF; // Length low byte
    responsePacket[12] = modbusPacket[12]; // Slave ID

    String responseHexStr = byteArrayToHexString(responsePacket, len + 6);
    responseHexStr.toUpperCase();

    debugLog("Response hex:");
    printPacket(responsePacket, len + 6);

    mesh.sendSingle(from, responseHexStr);
    restart_guard_note_activity();
  }
}

void mesh_init()
{
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.setContainsRoot(true);
  mesh.onReceive(receivedCallback);
  meshStarted = true;
  meshInfo();
}

void mesh_task()
{
  mesh.update();
}

void mesh_stop()
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
  debugLog("mesh stopped");
}

void meshInfo()
{
  debugLog("Mesh Network Details:");
  debugLog("Prefix: %s", MESH_PREFIX);
  debugLog("Password: %s", MESH_PASSWORD);
  debugLog("Port: %u", (unsigned)MESH_PORT);
  debugLog("Channel: %u", (unsigned)MESH_CHANNEL);
  debugLog("Node ID: %u", (unsigned)NODE_ID);
  debugLog("Mesh ID: %u", mesh.getNodeId());
}
