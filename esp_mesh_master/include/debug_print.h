

#pragma once

#include <vector>
#include <cstdint>

void printPacket(const uint8_t *packet, int packetSize);
void printPacket(const std::vector<uint8_t> &packet);