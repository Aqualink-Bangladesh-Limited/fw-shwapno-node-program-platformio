#include "packet_filter.h"
#include "app_config.h"

bool isValidPacket(const uint8_t *packet, int len)
{
  if (len < 6)
    return false;

  uint16_t length = (static_cast<uint16_t>(packet[4]) << 8) | packet[5];
  uint16_t expectedLength = static_cast<uint16_t>(6 + length);
  return len == static_cast<int>(expectedLength);
}

bool isPortalTriggerForMaster(const uint8_t *packet, int len)
{
  if (len < 8 || !isValidPacket(packet, len))
    return false;

  uint16_t pduLen = (static_cast<uint16_t>(packet[4]) << 8) | packet[5];
  if (pduLen != 2)
    return false;

  if (packet[2] != 0 || packet[3] != 0)
    return false;

  return packet[6] == static_cast<uint8_t>(MASTER_ID) &&
         packet[7] == PORTAL_TRIGGER_FC;
}
