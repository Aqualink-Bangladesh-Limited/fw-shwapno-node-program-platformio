
#include <esp_task_wdt.h>
#include "mesH_handler.h"
#include "sensor_handler.h"
#include "ir_handler.h"
#include "led_handler.h"

uint16_t arr[5] = {0, 0, 0, 5, 30}; // Default 5 second
uint16_t sensor_data[2] = {0xFFFF, 0xFFFF};
uint16_t temperature = 0xFFFF, humidity = 0xFFFF;
bool flag = false;
unsigned long last_read_time = 0;
unsigned long last_ir_send_time = 0;
unsigned long lastMeshReceivedTime = 0;

void setup()
{
  Serial.begin(115200);

  esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.println("Watchdog timer set for " + String(WATCHDOG_TIMEOUT) + " seconds");

  modbus_init();

  pinMode(LED_BLINK, OUTPUT);
  pinMode(LED_AC_STATUS, OUTPUT);
  pinMode(LED_MESH, OUTPUT);

  mesh_init();

  ir_init();
}

void loop()
{
  esp_task_wdt_reset();

  modbus_task();

  mesh_task();

  unsigned long currentMillis = millis();
  static unsigned long last_print_time = 0;
  if (currentMillis - last_print_time >= 5000)
  {
    last_print_time = currentMillis;
    Serial.println("AC Set Temperature : " + String(arr[0]) + "  AC Status : " + String(arr[1]) + "  AC Hit : " + String(arr[2]) + "  AC Cooldown Time : " + String(arr[3]));
    Serial.println("Sensor Temperature : " + String(temperature) + "  Sensor Humidity : " + String(humidity));
  }

  unsigned long sensor_read_interval = (arr[4] * ONE_SECOND);
  if (currentMillis - last_read_time >= sensor_read_interval)
  {
    Serial.println("Temperature Read Request Send");
    last_read_time = currentMillis;
    ReadTemperatureHumidity();
  }

  unsigned long ir_cooldown_time = (arr[3] * ONE_SECOND);
  if (flag && (currentMillis - last_ir_send_time >= ir_cooldown_time))
  {
    last_ir_send_time = currentMillis;
    ir_handler();
    flag = false; // Reset flag after operation
  }

  led_handler();
}
