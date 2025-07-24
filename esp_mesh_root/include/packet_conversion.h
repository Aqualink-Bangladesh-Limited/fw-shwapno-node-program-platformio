#pragma once

int hexStringToByteArray(String hexStr, uint8_t* byteArray);
uint8_t hexCharToByte(char hex);
String convertHexToString(uint8_t* packet, int packetSize);