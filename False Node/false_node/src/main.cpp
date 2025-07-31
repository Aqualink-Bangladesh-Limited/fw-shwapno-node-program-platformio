#include <esp_task_wdt.h>
#include "mesh_handler.h"
#include "led_handler.h"

uint16_t arr[5] = {0, 0, 0, 5, 30};
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
  Serial.printf("Watchdog timer set for %d seconds\n", WATCHDOG_TIMEOUT);

  pinMode(LED_BLINK, OUTPUT);
  pinMode(LED_MESH, OUTPUT);

  mesh_init();
}

void loop()
{
  esp_task_wdt_reset();
  mesh_task();
  led_handler();
}