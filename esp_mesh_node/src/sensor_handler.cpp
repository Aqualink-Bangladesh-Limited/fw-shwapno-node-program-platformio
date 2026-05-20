#include "sensor_handler.h"
#include <ModbusRTU.h>
#include "debug_log.h"

ModbusRTU mb_RTU;

ModbusRegister modbus_sensors[] = {
    {0x01, 0x00, sensor_data, 2}
};

void modbus_init() {
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  pinMode(DIRECTION_PIN, OUTPUT);
  mb_RTU.begin(&Serial2, DIRECTION_PIN, true);
  mb_RTU.master();
}

void modbus_task() {
  mb_RTU.task();
  yield();
}

bool modbus_callback(Modbus::ResultCode event, uint16_t, void *) {
  if (event == Modbus::EX_SUCCESS) {
    debugLog("Modbus success");
  } else {
    debugLog("Modbus failed");
  }

#if defined(SENSOR_VERSION_01)
  temperature = sensor_data[1];
  humidity = sensor_data[0];
#elif defined(SENSOR_VERSION_02)
  temperature = sensor_data[0];
  humidity = sensor_data[1];
#endif

  debugLog("Humidity: %d  Temperature: %d", humidity, temperature);

  return true;
}

void ReadTemperatureHumidity() {
  debugLog("Modbus request to address: %d", modbus_sensors[0].slave_address);
  mb_RTU.readHreg(
    modbus_sensors[0].slave_address,
    modbus_sensors[0].reg_address,
    modbus_sensors[0].data_response,
    modbus_sensors[0].data_size,
    modbus_callback
  );
}