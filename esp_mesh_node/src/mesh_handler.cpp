#include "app_config.h"
#include <painlessMesh.h>
#include <WiFi.h>
#include "request_handler.h"
#include "data_type_conversion.h"
#include "debug_print.h"

painlessMesh mesh;
long mesh_rssi = 0;

void meshInfo();

void receivedCallback(uint32_t from, String &msg) {
  lastMeshReceivedTime = millis();
  mesh_rssi = WiFi.RSSI();

  Serial.printf("Received message from node %u: %s\n", from, msg.c_str());

  if (msg.startsWith("SLAVE_ID_REQUEST:")) {
      int requestedId = msg.substring(17).toInt();
      if (requestedId == NODE_ID) {
          String response = "SLAVE_ID_RESPONSE:" + String(NODE_ID);
          mesh.sendSingle(from, response);
      }
      return;
  }

  uint8_t modbusPacket[256];
  int modbusPacketSize = hexStringToByteArray(msg, modbusPacket);

  if (modbusPacketSize < 6) {
    Serial.println("Invalid packet size. Discarding.");
    return;
  }

  uint8_t slaveId = modbusPacket[12]; // Slave ID is at position 12 in the Modbus TCP packet

  if (slaveId == NODE_ID) {
    uint8_t responsePacket[256];
    uint8_t len = 0;

    // Copy IP and port
    for (int i = 0; i < 6; ++i) {
      responsePacket[i] = modbusPacket[i];
    }

    len = modbusRequestHandler(&modbusPacket[6], &responsePacket[6]);

    // Copy Modbus TCP header fields
    responsePacket[6]  = modbusPacket[6];   // Transaction ID high byte
    responsePacket[7]  = modbusPacket[7];   // Transaction ID low byte
    responsePacket[8]  = modbusPacket[8];   // Protocol ID high byte
    responsePacket[9]  = modbusPacket[9];   // Protocol ID low byte
    responsePacket[10] = (len - 6) >> 8;    // Length high byte
    responsePacket[11] = (len - 6) & 0xFF;  // Length low byte
    responsePacket[12] = modbusPacket[12];  // Slave ID

    String responseHexStr = byteArrayToHexString(responsePacket, len + 6);
    responseHexStr.toUpperCase();

    Serial.print("Response Hex String: ");
    printPacket(responsePacket, len + 6);

    mesh.sendSingle(from, responseHexStr);
  }
}

void mesh_init() {
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.setContainsRoot(true);
  mesh.onReceive(receivedCallback);
  meshInfo();
}

void mesh_task() {
  mesh.update();
}

void meshInfo() {
  Serial.println("Mesh Network Details:");
  Serial.print("Prefix: ");   Serial.println(MESH_PREFIX);
  Serial.print("Password: "); Serial.println(MESH_PASSWORD);
  Serial.print("Port: ");     Serial.println(MESH_PORT);
  Serial.print("Channel: ");  Serial.println(MESH_CHANNEL);
  Serial.print("Node ID: ");  Serial.println(NODE_ID);
  Serial.print("Mesh ID: ");  Serial.println(mesh.getNodeId());
}