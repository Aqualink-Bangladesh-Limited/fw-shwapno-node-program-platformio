#pragma once

#include <vector>
#include <cstdint>

void printPacket(uint8_t *packet, int packetSize);
void printPacket(const uint8_t *packet, int packetSize);
void printPacket(const std::vector<uint8_t> &packet);
void debugLogPacket(const char *prefix, const uint8_t *packet, int packetSize);
void debugLogPacket(const char *prefix, const std::vector<uint8_t> &packet);
void printDebugInfo();
void rootInfo();
void portalInfo();
void printMeshInfo();
void printConnectedNodes();
bool debugShouldLogPacketDetails();
