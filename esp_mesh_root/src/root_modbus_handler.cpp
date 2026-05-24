#include "root_modbus_handler.h"
#include "packet_filter.h"
#include "app_config.h"
#include "debug_log.h"
#include <Arduino.h>

static uint8_t handleReadRequest(const uint8_t *requestBuffer, uint8_t *responseBuffer);
static uint8_t handleWriteSingleRequest(const uint8_t *requestBuffer, uint8_t *responseBuffer);
static uint8_t handleWriteMultipleRequest(const uint8_t *requestBuffer, uint8_t *responseBuffer);

bool isModbusRequestForRoot(const uint8_t *packet, int len)
{
  if (len < 8 || !isValidPacket(packet, len))
    return false;

  if (packet[2] != 0 || packet[3] != 0)
    return false;

  return packet[6] == static_cast<uint8_t>(ROOT_ID);
}

static uint16_t readRootRegister(uint16_t regAddress)
{
  switch (regAddress)
  {
  case ROOT_REG_MESH_RSSI:
    if (mesh_rssi != 0)
      return static_cast<uint16_t>(mesh_rssi * -1);
    return 0;
  default:
    return 0xFFFF;
  }
}

static uint8_t handleReadRequest(const uint8_t *requestBuffer, uint8_t *responseBuffer)
{
  uint16_t startAddress = (requestBuffer[8] << 8) | requestBuffer[9];
  uint16_t registerCount = (requestBuffer[10] << 8) | requestBuffer[11];

  if (registerCount == 0 || registerCount > 125)
  {
    responseBuffer[7] = 0x83;
    responseBuffer[8] = 0x03;
    return 9;
  }

  responseBuffer[7] = 0x03;
  responseBuffer[8] = registerCount * 2;

  for (uint16_t i = 0; i < registerCount; i++)
  {
    uint16_t value = readRootRegister(startAddress + i);
    responseBuffer[9 + (i * 2)] = highByte(value);
    responseBuffer[10 + (i * 2)] = lowByte(value);
  }

  return 9 + (registerCount * 2);
}

static uint8_t handleWriteSingleRequest(const uint8_t *requestBuffer, uint8_t *responseBuffer)
{
  (void)requestBuffer;
  responseBuffer[7] = 0x86;
  responseBuffer[8] = 0x01;
  return 9;
}

static uint8_t handleWriteMultipleRequest(const uint8_t *requestBuffer, uint8_t *responseBuffer)
{
  (void)requestBuffer;
  responseBuffer[7] = 0x90;
  responseBuffer[8] = 0x01;
  return 9;
}

int rootModbusHandler(const uint8_t *requestBuffer, int requestLen,
                      uint8_t *responseBuffer, int responseMaxLen)
{
  if (requestLen < 8 || responseMaxLen < 9)
    return 0;

  for (int i = 0; i < 6; i++)
    responseBuffer[i] = requestBuffer[i];

  responseBuffer[6] = requestBuffer[6];

  uint8_t pduLen = 0;
  const uint8_t functionCode = requestBuffer[7];

  switch (functionCode)
  {
  case 0x03:
    if (requestLen < 12)
      return 0;
    pduLen = handleReadRequest(requestBuffer, responseBuffer);
    break;
  case 0x06:
    if (requestLen < 12)
      return 0;
    pduLen = handleWriteSingleRequest(requestBuffer, responseBuffer);
    break;
  case 0x10:
    if (requestLen < 13)
      return 0;
    pduLen = handleWriteMultipleRequest(requestBuffer, responseBuffer);
    break;
  default:
    responseBuffer[7] = 0x80 | functionCode;
    responseBuffer[8] = 0x01;
    pduLen = 9;
    break;
  }

  if (pduLen == 0)
    return 0;

  const uint16_t mbapLength = static_cast<uint16_t>(pduLen - 6);
  responseBuffer[4] = highByte(mbapLength);
  responseBuffer[5] = lowByte(mbapLength);

  debugLog("Root Modbus response (%u bytes)", (unsigned)pduLen);
  return pduLen;
}
