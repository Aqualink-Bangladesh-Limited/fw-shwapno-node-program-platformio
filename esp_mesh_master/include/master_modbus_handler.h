#pragma once

#include <stdint.h>

bool isModbusRequestForMaster(const uint8_t *packet, int len);
int masterModbusHandler(uint8_t *requestBuffer, int requestLen,
                        uint8_t *responseBuffer, int responseMaxLen);
