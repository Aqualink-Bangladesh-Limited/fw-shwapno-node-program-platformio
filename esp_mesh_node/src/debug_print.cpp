
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

void printDebugInfo() {
  // Print AC model configuration
  #if defined(CARRIER_01)
    Serial.print("AC: CARRIER_01 | ");
  #elif defined(GENERAL_01)
    Serial.print("AC: GENERAL_01 | ");
  #elif defined(GREE_01)
    Serial.print("AC: GREE_01 | ");
  #elif defined(GREE_02)
    Serial.print("AC: GREE_02 | ");
  #elif defined(MIDEA_01)
    Serial.print("AC: MIDEA_01 | ");
  #elif defined(MIDEA_02)
    Serial.print("AC: MIDEA_02 | ");
  #elif defined(MIDEA_03)
    Serial.print("AC: MIDEA_03 | ");
  #elif defined(MIDEA_04)
    Serial.print("AC: MIDEA_04 | ");
  #elif defined(SAMSUNG_01)
    Serial.print("AC: SAMSUNG_01 | ");
  #elif defined(UNKNOWN_01)
    Serial.print("AC: UNKNOWN_01 | ");
  #elif defined(UNKNOWN_02)
    Serial.print("AC: UNKNOWN_02 | ");
  #elif defined(MALAYSIAN_BOARD_01)
    Serial.print("AC: MALAYSIAN_BOARD_01 | ");
  #elif defined(YORK_01)
    Serial.print("AC: YORK_01 | ");
  #elif defined(YORK_02)
    Serial.print("AC: YORK_02 | ");
  #elif defined(CHIGO_01)
    Serial.print("AC: CHIGO_01 | ");
  #elif defined(QUNDA)
    Serial.print("AC: QUNDA | ");
  #else
    Serial.print("AC: NONE | ");
  #endif

  // Print board version configuration
  #if defined(BOARD_VERSION_01)
    Serial.print("Board: V01\n");
  #elif defined(BOARD_VERSION_02)
    Serial.print("Board: V02\n");
  #elif defined(BOARD_VERSION_03)
    Serial.print("Board: V03\n");
  #elif defined(BOARD_VERSION_04)
    Serial.print("Board: V04\n");
  #else
    Serial.print("Board: NONE\n");
  #endif

  Serial.printf("AC Set Temp: %d  AC Status: %d  AC Hit: %d  AC Cooldown: %d\n", arr[0], arr[1], arr[2], arr[3]);
  Serial.printf("Sensor Temp: %d  Humidity: %d\n", temperature, humidity);
  Serial.printf("RSSI : %d\n", mesh_rssi);
}