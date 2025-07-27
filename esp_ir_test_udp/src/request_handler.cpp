
#include <Arduino.h>
#include "request_handler.h"

uint8_t handleReadRequest(uint8_t *requestBuffer, uint8_t *responseBuffer);
uint8_t handleWriteSingleRequest(uint8_t *requestBuffer, uint8_t *responseBuffer);
uint8_t handleWriteMultipleRequest(uint8_t *requestBuffer, uint8_t *responseBuffer);

uint8_t modbusRequestHandler(uint8_t *requestBuffer, uint8_t *responseBuffer)
{
  uint8_t functionCode = requestBuffer[7]; // 8th byte in Modbus TCP packet
  uint8_t len = 0;

  switch (functionCode)
  {
  case 0x03:
    len = handleReadRequest(requestBuffer, responseBuffer);
    break;
  case 0x06:
    len = handleWriteSingleRequest(requestBuffer, responseBuffer);
    break;
  case 0x10:
    len = handleWriteMultipleRequest(requestBuffer, responseBuffer);
    break;
  default:
    responseBuffer[7] = 0x83; // Error response
    responseBuffer[8] = 0x01; // Illegal function code error
    len = 9;
    break;
  }
  return len;
}

uint8_t handleReadRequest(uint8_t *requestBuffer, uint8_t *responseBuffer)
{
  uint16_t startAddress = (requestBuffer[8] << 8) | requestBuffer[9];
  uint16_t registerCount = (requestBuffer[10] << 8) | requestBuffer[11];
  uint8_t len = 0;

  responseBuffer[7] = 0x03;
  responseBuffer[8] = registerCount * 2;

  for (uint16_t i = 0; i < registerCount; i++)
  {
    uint16_t regAddress = startAddress + i;
    uint16_t value = 0;

    switch (regAddress)
    {
    case 0x01:
      value = arr[0];
      break;
    case 0x02:
      value = arr[1];
      break;
    case 0x03:
      value = arr[2];
      break;
    case 0x04:
      value = arr[3];
      break;
    case 0x05:
      value = arr[4];
      break;
    default:
      value = 0xFFFF;
      break;
    }

    responseBuffer[9 + (i * 2)] = highByte(value);
    responseBuffer[10 + (i * 2)] = lowByte(value);
  }
  len = 9 + (registerCount * 2);
  return len;
}

uint8_t handleWriteSingleRequest(uint8_t *requestBuffer, uint8_t *responseBuffer)
{
  uint16_t startAddress = (requestBuffer[8] << 8) | requestBuffer[9];
  uint16_t value = (requestBuffer[10] << 8) | requestBuffer[11];
  uint8_t len = 0;

  responseBuffer[7] = requestBuffer[7];

  switch (startAddress)
  {
  case 0x01:
    arr[0] = value;
    break;
  case 0x02:
    arr[1] = value;
    break;
  case 0x03:
    arr[2] = value;
    flag = true;
    break;
  case 0x04:
    arr[3] = value;
    break;
  case 0x05:
    arr[4] = value;
    break;
  default:
    responseBuffer[7] = 0x83;
    responseBuffer[8] = 0x02;
    len = 9;
    return len;
  }

  responseBuffer[8] = startAddress >> 8;
  responseBuffer[9] = startAddress & 0xFF;
  responseBuffer[10] = value >> 8;
  responseBuffer[11] = value & 0xFF;

  len = 12;
  return len;
}

uint8_t handleWriteMultipleRequest(uint8_t *requestBuffer, uint8_t *responseBuffer)
{
  uint16_t startAddress = (requestBuffer[8] << 8) | requestBuffer[9];
  uint16_t registerCount = (requestBuffer[10] << 8) | requestBuffer[11];
  uint8_t len = 0;

  responseBuffer[7] = requestBuffer[7];

  for (uint16_t i = 0; i < registerCount; i++)
  {
    uint16_t regAddress = startAddress + i;
    uint16_t value = (requestBuffer[13 + (i * 2)] << 8) | requestBuffer[14 + (i * 2)];

    switch (regAddress)
    {
    case 0x01:
      arr[0] = value;
      break;
    case 0x02:
      arr[1] = value;
      break;
    case 0x03:
      arr[2] = value;
      flag = true;
      break;
    case 0x04:
      arr[3] = value;
      break;
    case 0x05:
      arr[4] = value;
      break;
    default:
      responseBuffer[7] = 0x83;
      responseBuffer[8] = 0x02;
      len = 9;
      return len;
    }
  }

  responseBuffer[8] = startAddress >> 8;
  responseBuffer[9] = startAddress & 0xFF;
  responseBuffer[10] = registerCount >> 8;
  responseBuffer[11] = registerCount & 0xFF;

  len = 12;
  return len;
}