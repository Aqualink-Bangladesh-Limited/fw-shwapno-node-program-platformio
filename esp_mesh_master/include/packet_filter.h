#pragma once

#include <cstdint>

bool isValidPacket(const uint8_t *packet, int len);
bool isPortalTriggerForMaster(const uint8_t *packet, int len);
