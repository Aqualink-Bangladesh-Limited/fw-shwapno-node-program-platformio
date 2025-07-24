
#include "request_handler.h"

uint8_t handleReadRequest(uint8_t *requestBuffer, uint8_t *responseBuffer);
uint8_t handleWriteSingleRequest(uint8_t *requestBuffer, uint8_t *responseBuffer);
uint8_t handleWriteMultipleRequest(uint8_t *requestBuffer, uint8_t *responseBuffer);

uint8_t modbusRequestHandler(uint8_t *requestBuffer, uint8_t *responseBuffer)
{
  uint8_t functionCode = requestBuffer[7]; // Function code is at the 8th byte in Modbus TCP packet
  uint16_t startAddress = (requestBuffer[8] << 8) | requestBuffer[9];
  uint16_t registerCount = (requestBuffer[10] << 8) | requestBuffer[11];
  uint8_t len = 0;

  switch (functionCode)
  {
  case 0x03: // Read Holding Registers
    len = handleReadRequest(requestBuffer, responseBuffer);
    break;
  case 0x06: // Write Single Register
    len = handleWriteSingleRequest(requestBuffer, responseBuffer);
    break;
  case 0x10: // Write Multiple Registers
    len = handleWriteMultipleRequest(requestBuffer, responseBuffer);
    break;
  default:
    // Invalid function code, return error
    responseBuffer[7] = 0x83; // Error response
    responseBuffer[8] = 0x01; // Illegal function code error
    len = 9;
    break;
  }
  return len;
}

// Handle read request
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

    if (regAddress == 0x01)
    {
      value = arr[0];
    }
    else if (regAddress == 0x02)
    {
      value = arr[1];
    }
    else if (regAddress == 0x03)
    {
      value = arr[2];
    }
    else if (regAddress == 0x04)
    {
      value = arr[3];
    }
    else if (regAddress == 0x05)
    {
      value = arr[4];
    }
    else if (regAddress == 0x21)
    {
      value = temperature;
    }
    else if (regAddress == 0x22)
    {
      value = humidity;
    }
    else
    {
      value = 0xFFFF;
    }

    responseBuffer[9 + (i * 2)] = highByte(value);
    responseBuffer[10 + (i * 2)] = lowByte(value);
  }
  len = 9 + (registerCount * 2); // Total response length
  return len;
}

// Handle Write Single Register Request (0x06)
uint8_t handleWriteSingleRequest(uint8_t *requestBuffer, uint8_t *responseBuffer)
{
  uint16_t startAddress = (requestBuffer[8] << 8) | requestBuffer[9];
  uint16_t value = (requestBuffer[10] << 8) | requestBuffer[11];
  uint8_t len = 0;

  responseBuffer[7] = requestBuffer[7];

  if (startAddress == 0x01)
  {
    arr[0] = value;
  }
  else if (startAddress == 0x02)
  {
    arr[1] = value;
  }
  else if (startAddress == 0x03)
  {
    arr[2] = value;
    flag = true; // Set flag if array[2] is written
  }
  else if (startAddress == 0x04)
  {
    arr[3] = value;
  }
  else if (startAddress == 0x05)
  {
    arr[4] = value;
  }
  else
  {
    // Invalid address for write request
    responseBuffer[7] = 0x83; // Error response function code (0x06 + 0x80 = 0x86)
    responseBuffer[8] = 0x02; // Illegal data address error
    len = 9;
    return len;
  }

  responseBuffer[8] = startAddress >> 8;   // Start address (high byte)
  responseBuffer[9] = startAddress & 0xFF; // Start address (low byte)
  responseBuffer[10] = value >> 8;         // Written value (high byte)
  responseBuffer[11] = value & 0xFF;       // Written value (low byte)

  len = 12; // Response length for write single register operation
  return len;
}

// Handle write request
uint8_t handleWriteMultipleRequest(uint8_t *requestBuffer, uint8_t *responseBuffer)
{
  uint16_t startAddress = (requestBuffer[8] << 8) | requestBuffer[9];
  uint16_t registerCount = (requestBuffer[10] << 8) | requestBuffer[11];
  uint8_t len = 0;

  responseBuffer[7] = requestBuffer[7]; // Function Code

  for (uint16_t i = 0; i < registerCount; i++)
  {
    uint16_t regAddress = startAddress + i;
    uint16_t value = (requestBuffer[13 + (i * 2)] << 8) | requestBuffer[14 + (i * 2)];

    if (regAddress == 0x01)
    {
      arr[0] = value;
    }
    else if (regAddress == 0x02)
    {
      arr[1] = value;
    }
    else if (regAddress == 0x03)
    {
      arr[2] = value;
      flag = true; // Set flag if array[2] is written
    }
    else if (regAddress == 0x04)
    {
      arr[3] = value;
    }
    else if (regAddress == 0x05)
    {
      arr[4] = value;
    }
    else
    {
      // Invalid address for write request
      responseBuffer[7] = 0x83; // Error response
      responseBuffer[8] = 0x02; // Illegal data address error
      len = 9;
      return len;
    }
  }

  // Respond with written values
  responseBuffer[8] = startAddress >> 8;     // Start address
  responseBuffer[9] = startAddress & 0xFF;   // Start address (low byte)
  responseBuffer[10] = registerCount >> 8;   // Register count (high byte)
  responseBuffer[11] = registerCount & 0xFF; // Register count (low byte)

  len = 12; // Response length for write operation
  return len;
}
