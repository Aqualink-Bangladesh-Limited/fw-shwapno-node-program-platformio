#include "app_config.h"
#include <painlessMesh.h>
#include <WiFi.h>
#include "request_handler.h"
#include "data_type_conversion.h"
#include "debug_print.h"
#include "debug_log.h"

painlessMesh mesh;
long mesh_rssi = 0;

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

  uint8_t modbusPacket[256];
  int modbusPacketSize = hexStringToByteArray(msg, modbusPacket);

  /* Portal / OTA: FC 0x41 in MBAP frame (after IP:port prefix) */
  if (modbusPacketSize >= 14 &&
      modbusPacket[8] == 0 && modbusPacket[9] == 0 &&
      modbusPacket[10] == 0 && modbusPacket[11] == 2 &&
      modbusPacket[13] == PORTAL_TRIGGER_FC)
  {
    uint8_t targetNode = modbusPacket[12];
    if (targetNode == NODE_ID)
    {
      portalRequested = true;
      portalRequestTime = millis();
      debugLog("Portal requested for this node.");
    }
    return;
  }

  if (modbusPacketSize < 6)
  {
    debugLog("Invalid packet size. Discarding.");
    return;
  }

  uint8_t slaveId = modbusPacket[12]; // Slave ID is at position 12 in the Modbus TCP packet

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
  }
}

void mesh_init()
{
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.setContainsRoot(true);
  mesh.onReceive(receivedCallback);
  meshInfo();
}

void mesh_task()
{
  mesh.update();
}

void mesh_stop()
{
  mesh.stop();
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
