
#include <painlessMesh.h>
#include "request_handler.h"
#include "data_type_conversion.h"

painlessMesh mesh;

void meshInfo();

void receivedCallback(uint32_t from, String &msg)
{
  lastMeshReceivedTime = millis();

  Serial.print("Received message from node ");
  Serial.print(from);
  Serial.print(": ");
  Serial.println(msg);

  uint8_t modbusPacket[256];
  int modbusPacketSize = hexStringToByteArray(msg, modbusPacket);

  // Ensure the packet is large enough to contain the IP, port, and Modbus data
  if (modbusPacketSize < 6)
  {
    Serial.println("Invalid packet size. Discarding.");
    return;
  }

  uint8_t slaveId = modbusPacket[6 + 6]; // Slave ID is at position 6 in the Modbus TCP packet

  //  Serial.println("Slave Id : " + String(slaveId) + " Node Id : " + String(NODE_ID) + " modbusPacketSize : " + String(modbusPacketSize));

  if (slaveId == NODE_ID)
  {
    uint8_t responsePacket[256];
    uint8_t len = 0;

    responsePacket[0] = modbusPacket[0]; // IP first byte
    responsePacket[1] = modbusPacket[1]; // IP second byte
    responsePacket[2] = modbusPacket[2]; // IP third byte
    responsePacket[3] = modbusPacket[3]; // IP fourth byte
    responsePacket[4] = modbusPacket[4]; // Port high byte
    responsePacket[5] = modbusPacket[5]; // Port low byte

    len = modbusRequestHandler(&modbusPacket[6], &responsePacket[6]);

    responsePacket[6] = modbusPacket[6];   // Transaction ID high byte
    responsePacket[7] = modbusPacket[7];   // Transaction ID low byte
    responsePacket[8] = modbusPacket[8];   // Protocol ID high byte
    responsePacket[9] = modbusPacket[9];   // Protocol ID low byte
    responsePacket[10] = (len - 6) >> 8;   // Length high byte
    responsePacket[11] = (len - 6) & 0xFF; // Length low byte
    responsePacket[12] = modbusPacket[12]; // Slave ID

    // Convert the response packet to a string in hexadecimal format
    String responseHexStr = byteArrayToHexString(responsePacket, len + 6);
    responseHexStr.toUpperCase();

    Serial.println("Response Hex String: " + responseHexStr);

    mesh.sendSingle(from, responseHexStr); // Send to the node from where the request came
  }
}

void mesh_init()
{
  //  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
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
  Serial.print("Node ID: ");
  Serial.println(NODE_ID);
  Serial.print("Mesh ID: ");
  Serial.println(mesh.getNodeId());
}