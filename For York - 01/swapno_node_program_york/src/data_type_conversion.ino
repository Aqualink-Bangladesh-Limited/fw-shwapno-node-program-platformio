
#include "data_type_conversion.h"

int hexStringToByteArray(String hexStr, uint8_t *byteArray)
{
  hexStr.replace(" ", "");

  int length = hexStr.length() / 2;
  for (int i = 0; i < length; i++)
  {
    byteArray[i] = (hexCharToByte(hexStr.charAt(i * 2)) << 4) | hexCharToByte(hexStr.charAt(i * 2 + 1));
    ;
  }
  return length;
}

uint8_t hexCharToByte(char hex)
{
  if (hex >= '0' && hex <= '9')
    return hex - '0';
  if (hex >= 'A' && hex <= 'F')
    return hex - 'A' + 10;
  if (hex >= 'a' && hex <= 'f')
    return hex - 'a' + 10;
  return 0;
}

String byteArrayToHexString(uint8_t *byteArray, int length)
{
  String hexStr = "";
  for (int i = 0; i < length; i++)
  {
    if (byteArray[i] < 0x10)
    {
      hexStr += "0";
    }
    hexStr += String(byteArray[i], HEX);
    hexStr += " ";
  }
  return hexStr;
}