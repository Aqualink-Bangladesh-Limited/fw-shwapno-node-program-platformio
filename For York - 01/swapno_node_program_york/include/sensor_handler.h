#pragma once
#include "app_config.h"

typedef struct {
  uint8_t slave_address;
  uint16_t reg_address;
  uint16_t *data_response;
  uint8_t data_size;
} ModbusRegister;

void modbus_init();
void modbus_task();
void ReadTemperatureHumidity();
