#include "led_handler.h"

#define MAX_RESTART 10

unsigned long mesh_blink_interval = 500;
unsigned long last_mesh_led_blink = 0;
bool mesh_led_state = false;
int ac_led_state = 0;
int led_blink_state = 0;
unsigned long last_led_blink = 0;
unsigned long led_blink_interval = 5000;

void handle_ac_led(uint16_t ac_state);
void blink_ac_led();
void update_mesh_blink_interval(unsigned long now);

void led_handler() {
  unsigned long now = millis();

  handle_ac_led(arr[1]);
  update_mesh_blink_interval(now);

  // Mesh LED blink
  if (now - last_mesh_led_blink >= mesh_blink_interval) {
    last_mesh_led_blink = now;
    mesh_led_state = !mesh_led_state;
    digitalWrite(LED_MESH, mesh_led_state);
  }

  // Status LED blink
  if (now - last_led_blink >= led_blink_interval) {
    led_blink_interval = led_blink_state ? 5000 : 1000;
    last_led_blink = now;
    led_blink_state = !led_blink_state;
    digitalWrite(LED_BLINK, led_blink_state);
  }
}

void handle_ac_led(uint16_t ac_state) {
  switch (ac_state) {
    case 0:
      digitalWrite(LED_AC_STATUS, LOW);
      break;
    case 1:
    case 2:
      digitalWrite(LED_AC_STATUS, HIGH);
      break;
    case 3:
      blink_ac_led();
      break;
    default:
      digitalWrite(LED_AC_STATUS, LOW);
      break;
  }
}

void blink_ac_led() {
  static unsigned long last_blink = 0;
  unsigned long now = millis();
  if (now - last_blink >= 500) {
    last_blink = now;
    ac_led_state = !ac_led_state;
    digitalWrite(LED_AC_STATUS, ac_led_state);
  }
}

void update_mesh_blink_interval(unsigned long now) {
  if (now - lastMeshReceivedTime <= MESH_INTERVAL) {
    mesh_blink_interval = 2000;
  } else {
    mesh_blink_interval = 500;
  }
}