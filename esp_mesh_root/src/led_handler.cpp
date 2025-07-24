
#include <Arduino.h>
#include "app_config.h"
#include "led_handler.h"
#include "debug_print.h"

#define MAX_RESTART 10

extern unsigned long lastMeshReceivedTime;

unsigned long last_led_blink = 0;
unsigned long last_mesh_led_blink = 0;
const unsigned long blink_interval = 500;  // LED update interval in milliseconds
unsigned long mesh_blink_interval = 500;   // LED update interval in milliseconds
int ledState = 0;
int meshledState = 0;
int restart_count = 0;

void led_handler() {
  unsigned long currentMillis = millis();

  // Handle mesh LED blinking interval based on the last received time
  updateMeshblink_interval(currentMillis);

  // Blink mesh LED based on calculated interval
  if (currentMillis - last_mesh_led_blink >= mesh_blink_interval) {
    last_mesh_led_blink = currentMillis;
    meshledState = !meshledState;          // Toggle LED state
    digitalWrite(LED_MESH, meshledState);  // Update mesh LED state
  }

  if (currentMillis - last_led_blink >= blink_interval) {
    last_led_blink = currentMillis;
    ledState = !ledState;               // Toggle AC LED state
    digitalWrite(LED_BLINK, ledState);  // Update AC LED state
  }
}

// Update mesh LED blink interval based on the last received time
void updateMeshblink_interval(unsigned long currentMillis) {
  // Check if the last received time is within a 5-minute window
  if (currentMillis - lastMeshReceivedTime <= MESH_INTERVAL) {
    mesh_blink_interval = 2000;  // Blink mesh LED every 2 seconds if within 5 minutes

    restart_count = 0;
  } else {
    mesh_blink_interval = 500;  // Blink mesh LED every 500 milliseconds if more than 5 minutes have passed

    restart_count++;
    if (restart_count > MAX_RESTART) {
      Serial.println("ESP Restarting.....");
      ESP.restart();
    }
  }
}