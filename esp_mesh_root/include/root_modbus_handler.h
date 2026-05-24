#pragma once

#include <stdint.h>

bool isModbusRequestForRoot(const uint8_t *packet, int len);
int rootModbusHandler(const uint8_t *requestBuffer, int requestLen,
                      uint8_t *responseBuffer, int responseMaxLen);
