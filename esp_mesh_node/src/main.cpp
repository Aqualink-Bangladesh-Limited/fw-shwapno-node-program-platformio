#include <esp_task_wdt.h>
#include <WiFi.h>
#include "mesh_handler.h"
#include "sensor_handler.h"
#include "ir_handler.h"
#include "led_handler.h"
#include "debug_print.h"

uint16_t arr[5] = {0, 0, 0, 5, 30};
uint16_t sensor_data[2] = {0xFFFF, 0xFFFF};
uint16_t temperature = 0xFFFF, humidity = 0xFFFF;
bool flag = false;
unsigned long last_read_time = 0;
unsigned long last_ir_send_time = 0;
unsigned long lastMeshReceivedTime = 0;

constexpr unsigned long RESTART_TIMEOUT = 900000; // 15 minutes

void setup() {
  Serial.begin(115200);

  esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.printf("Watchdog timer set for %d seconds\n", WATCHDOG_TIMEOUT);

  modbus_init();

  pinMode(LED_BLINK, OUTPUT);
  pinMode(LED_AC_STATUS, OUTPUT);
  pinMode(LED_MESH, OUTPUT);

  mesh_init();
  ir_init();
}

void loop() {
  esp_task_wdt_reset();
  modbus_task();
  mesh_task();

  unsigned long currentMillis = millis();
  static unsigned long last_print_time = 0;

  if (currentMillis - last_print_time >= 5000) {
    last_print_time = currentMillis;
    printDebugInfo();
  }

  unsigned long sensor_read_interval = arr[4] * ONE_SECOND;
  if (currentMillis - last_read_time >= sensor_read_interval) {
    last_read_time = currentMillis;
    ReadTemperatureHumidity();
  }

  unsigned long ir_cooldown_time = arr[3] * ONE_SECOND;
  if (flag && (currentMillis - last_ir_send_time >= ir_cooldown_time)) {
    last_ir_send_time = currentMillis;
    ir_handler();
    flag = false;
  }

  led_handler();

  if (currentMillis - lastMeshReceivedTime >= RESTART_TIMEOUT)
  {
    Serial.println("Mesh disconnected for 15 minutes. ESP Restarting.....");
    ESP.restart();
  }
}