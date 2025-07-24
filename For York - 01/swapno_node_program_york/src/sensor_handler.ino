
#include "sensor_handler.h"
#include <ModbusRTU.h>

// ModbusIP object
ModbusRTU mb_RTU;

ModbusRegister modbus_sensors[3] = {
    {0x01, 0x00, sensor_data, 2}, // humi + temp sensor
};

void modbus_init()
{
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  pinMode(DIRECTION_PIN, OUTPUT);
  mb_RTU.begin(&Serial2, DIRECTION_PIN, true);
  mb_RTU.master();
}

void modbus_task()
{

  mb_RTU.task(); // Run Modbus RTU task
  yield();       // Yield to avoid blocking
}

bool modbus_callback(Modbus::ResultCode event, uint16_t transactionId, void *data)
{
  if (event == 0x00)
  {
    Serial.println("Modbus success");
  }
  else
  {
    Serial.println("Modbus Failed");
  }

  temperature = sensor_data[1];
  humidity = sensor_data[0];
  Serial.println("Humidity : " + String(humidity) + "  Temperature : " + String(temperature));

  return true;
}

void ReadTemperatureHumidity()
{
  Serial.println("Modbus request to address :" + String(modbus_sensors[0].slave_address));
  mb_RTU.readHreg(modbus_sensors[0].slave_address, modbus_sensors[0].reg_address, modbus_sensors[0].data_response, modbus_sensors[0].data_size, modbus_callback);
}