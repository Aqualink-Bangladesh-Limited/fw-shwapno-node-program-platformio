#include "led_handler.h"

unsigned long mesh_blink_interval = 500;
unsigned long last_mesh_led_blink = 0;
bool mesh_led_state = false;
int ac_led_state = 0;
int led_blink_state = 0;
unsigned long last_led_blink = 0;
unsigned long led_blink_interval = 5000;
int restart_count = 0;

void handle_ac_led(uint16_t ac_state);
void blink_ac_led();
void update_mesh_blink_interval(unsigned long now);

void led_handler()
{
  unsigned long now = millis();

  update_mesh_blink_interval(now);

  // Mesh LED blink
  if (now - last_mesh_led_blink >= mesh_blink_interval)
  {
    last_mesh_led_blink = now;
    mesh_led_state = !mesh_led_state;
    digitalWrite(LED_MESH, mesh_led_state);
  }

  // Status LED blink
  if (now - last_led_blink >= led_blink_interval)
  {
    led_blink_interval = led_blink_state ? 5000 : 1000;
    last_led_blink = now;
    led_blink_state = !led_blink_state;
    digitalWrite(LED_BLINK, led_blink_state);
  }
}

void update_mesh_blink_interval(unsigned long now)
{
  if (now - lastMeshReceivedTime <= MESH_INTERVAL)
  {
    mesh_blink_interval = 2000;
  }
  else
  {
    mesh_blink_interval = 500;
  }
}