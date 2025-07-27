
#include <Arduino.h>
#include <vector>
#include "debug_print.h"

void printPacket(const uint8_t *packet, int packetSize)
{
  for (int i = 0; i < packetSize; i++)
  {
    if (i < packetSize - 1)
    {
      if (packet[i] < 0x10)
      {
        Serial.print("0");
      }
      Serial.print(packet[i], HEX);
      Serial.print(" ");
    }
    else
    {
      if (packet[i] < 0x10)
      {
        Serial.print("0");
      }
      Serial.println(packet[i], HEX);
    }
  }
}

void printPacket(const std::vector<uint8_t> &packet)
{
  printPacket(packet.data(), static_cast<int>(packet.size()));
}