#include "debug_print.h"
#include "debug_log.h"

void printPacket(uint8_t *packet, int packetSize)
{
  String line;
  for (int i = 0; i < packetSize; i++)
  {
    if (packet[i] < 0x10)
    {
      line += "0";
    }
    char hexBuf[4];
    snprintf(hexBuf, sizeof(hexBuf), "%02X", packet[i]);
    line += hexBuf;
    line += " ";
  }
  debugLog("%s", line.c_str());
}

void printDebugInfo() {
  debugLog("Firmware: %s", FIRMWARE_VERSION);

  // Print AC model configuration
  #if defined(CARRIER_01)
    debugLog("AC: CARRIER_01 |");
  #elif defined(GENERAL_01)
    debugLog("AC: GENERAL_01 |");
  #elif defined(GREE_01)
    debugLog("AC: GREE_01 |");
  #elif defined(GREE_02)
    debugLog("AC: GREE_02 |");
  #elif defined(MIDEA_01)
    debugLog("AC: MIDEA_01 |");
  #elif defined(MIDEA_02)
    debugLog("AC: MIDEA_02 |");
  #elif defined(MIDEA_03)
    debugLog("AC: MIDEA_03 |");
  #elif defined(MIDEA_04)
    debugLog("AC: MIDEA_04 |");
  #elif defined(SAMSUNG_01)
    debugLog("AC: SAMSUNG_01 |");
  #elif defined(UNKNOWN_01)
    debugLog("AC: UNKNOWN_01 |");
  #elif defined(UNKNOWN_02)
    debugLog("AC: UNKNOWN_02 |");
  #elif defined(MALAYSIAN_BOARD_01)
    debugLog("AC: MALAYSIAN_BOARD_01 |");
  #elif defined(YORK_01)
    debugLog("AC: YORK_01 |");
  #elif defined(YORK_02)
    debugLog("AC: YORK_02 |");
  #elif defined(CHIGO_01)
    debugLog("AC: CHIGO_01 |");
  #elif defined(QUNDA_01)
    debugLog("AC: QUNDA_01 |");
  #else
    debugLog("AC: NONE |");
  #endif

  // Print board version configuration
  #if defined(BOARD_VERSION_01)
    debugLog("Board: V01 |");
  #elif defined(BOARD_VERSION_02)
    debugLog("Board: V02 |");
  #elif defined(BOARD_VERSION_03)
    debugLog("Board: V03 |");
  #elif defined(BOARD_VERSION_04)
    debugLog("Board: V04 |");
  #else
    debugLog("Board: NONE |");
  #endif

  // Print sensor version configuration
  #if defined(SENSOR_VERSION_01)
    debugLog("Sensor: V01");
  #elif defined(SENSOR_VERSION_02)
    debugLog("Sensor: V02");
  #else
    debugLog("Sensor: NONE");
  #endif

  debugLog("AC Set Temp: %d  AC Status: %d  AC Hit: %d  AC Cooldown: %d", arr[0], arr[1], arr[2], arr[3]);
  debugLog("Sensor Temp: %d  Humidity: %d", temperature, humidity);
  debugLog("RSSI : %d", mesh_rssi);
}