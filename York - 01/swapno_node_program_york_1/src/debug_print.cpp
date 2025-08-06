
#include "debug_print.h"

void printPacket(uint8_t *packet, int packetSize)
{
  for (int i = 0; i < packetSize; i++)
  {
    if (packet[i] < 0x10)
    {
      Serial.print("0");
    }
    Serial.print(packet[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}