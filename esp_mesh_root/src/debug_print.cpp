
#include <Arduino.h>
#include <painlessMesh.h>
#include "app_config.h"

extern painlessMesh mesh;

void printPacket(uint8_t* packet, int packetSize) {
  for (int i = 0; i < packetSize; i++) {
    if (i < packetSize - 1) {
      if (packet[i] < 0x10) {
        Serial.print("0");  // Add leading zero for single digit
      }
      Serial.print(packet[i], HEX);
      Serial.print(" ");
    } else {
      if (packet[i] < 0x10) {
        Serial.print("0");  // Add leading zero for single digit
      }
      Serial.println(packet[i], HEX);  // Print the last byte on a new line
    }
  }
}

void printMeshInfo() {
  Serial.println("Mesh Network Details:");
  Serial.print("Prefix: ");
  Serial.println(MESH_PREFIX);
  Serial.print("Password: ");
  Serial.println(MESH_PASSWORD);
  Serial.print("Port: ");
  Serial.println(MESH_PORT);
  Serial.print("Channel: ");
  Serial.println(MESH_CHANNEL);
}

void printConnectedNodes() {

  Serial.print("Root Node ID: ");
  Serial.println(mesh.getNodeId());

  Serial.println("Connected nodes:");
  std::list<uint32_t> nodeList = mesh.getNodeList();  // This gets the list of connected nodes
  for (uint32_t node : nodeList) {
    Serial.print("Node ID: ");
    Serial.println(node);
  }
}