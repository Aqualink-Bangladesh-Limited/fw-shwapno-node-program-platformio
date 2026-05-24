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

static bool isPortalTriggerMbap(const uint8_t *mbap, int len)
{
  if (len < 8 || !isValidPacket(mbap, len))
    return false;

  uint16_t pduLen = (static_cast<uint16_t>(mbap[4]) << 8) | mbap[5];
  if (pduLen != 2)
    return false;

  if (mbap[2] != 0 || mbap[3] != 0)
    return false;

  return mbap[6] == static_cast<uint8_t>(ROOT_ID) &&
         mbap[7] == PORTAL_TRIGGER_FC;
}

bool isPortalTriggerForRoot(const uint8_t *packet, int len)
{
  if (isPortalTriggerMbap(packet, len))
    return true;

  if (len > 6 && isPortalTriggerMbap(packet + 6, len - 6))
    return true;

  return false;
}

const uint8_t *getMbapPayload(const uint8_t *packet, int len, int *outLen)
{
  if (!packet || !outLen || len < 8)
  {
    *outLen = 0;
    return nullptr;
  }

  if (len > 6 && isValidPacket(packet + 6, len - 6))
  {
    *outLen = len - 6;
    return packet + 6;
  }

  if (isValidPacket(packet, len))
  {
    *outLen = len;
    return packet;
  }

  *outLen = 0;
  return nullptr;
}
