
#include "app_config.h"

typedef struct
{
  uint8_t slave_address;   // Modbus slave address (0-254)
  uint16_t reg_address;    // Register address
  uint16_t *data_response; // Data response from the register
  uint8_t data_size;       // Size of the data in bytes
} ModbusRegister;

void modbus_init();
void modbus_task();
void ReadTemperatureHumidity();
