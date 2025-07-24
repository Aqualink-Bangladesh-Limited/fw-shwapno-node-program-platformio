#include <Arduino.h>
#include <painlessMesh.h>
#include "app_config.h"

extern painlessMesh mesh;

void printPacket(const uint8_t* packet, int packetSize) {
    for (int i = 0; i < packetSize; i++) {
        if (packet[i] < 0x10) Serial.print("0");
        Serial.print(packet[i], HEX);
        if (i < packetSize - 1) Serial.print(" ");
        else Serial.println();
    }
}

void printMeshInfo() {
    Serial.println("Mesh Network Details:");
    Serial.print("Prefix: ");   Serial.println(MESH_PREFIX);
    Serial.print("Password: "); Serial.println(MESH_PASSWORD);
    Serial.print("Port: ");     Serial.println(MESH_PORT);
    Serial.print("Channel: ");  Serial.println(MESH_CHANNEL);
}

void printConnectedNodes() {
    Serial.print("Root Node ID: ");
    Serial.println(mesh.getNodeId());
    Serial.println("Connected nodes:");
    std::list<uint32_t> nodeList = mesh.getNodeList();
    for (uint32_t node : nodeList) {
        Serial.print("Node ID: ");
        Serial.println(node);
    }
}