#pragma once

#include <cstdint>

bool isValidPacket(const uint8_t *packet, int len);
bool isPortalTriggerForRoot(const uint8_t *packet, int len);
const uint8_t *getMbapPayload(const uint8_t *packet, int len, int *outLen);
