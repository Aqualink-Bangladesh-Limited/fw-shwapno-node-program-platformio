#include "app_config.h"
#include "led_handler.h"

unsigned long mesh_blink_interval = 500;
unsigned long last_mesh_led_blink = 0;
bool mesh_led_state = false;
int ac_led_state = 0;
unsigned long last_rssi_led_blink = 0;
bool rssi_led_state = false;

void handle_ac_led(uint16_t ac_state);
void blink_ac_led();
void handle_rssi_led(unsigned long now);

void led_handler()
{
  unsigned long now = millis();

  handle_ac_led(arr[1]);

  // Mesh LED logic
  if (now - lastMeshReceivedTime <= MESH_INTERVAL)
  {
    digitalWrite(LED_MESH, HIGH);
  }
  else
  {
    if (now - last_mesh_led_blink >= mesh_blink_interval)
    {
      last_mesh_led_blink = now;
      mesh_led_state = !mesh_led_state;
      digitalWrite(LED_MESH, mesh_led_state);
    }
  }

  // RSSI LED logic
  handle_rssi_led(now);
}

void handle_ac_led(uint16_t ac_state)
{
  switch (ac_state)
  {
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

void blink_ac_led()
{
  static unsigned long last_blink = 0;
  unsigned long now = millis();
  if (now - last_blink >= 500)
  {
    last_blink = now;
    ac_led_state = !ac_led_state;
    digitalWrite(LED_AC_STATUS, ac_led_state);
  }
}

void handle_rssi_led(unsigned long now)
{
  unsigned long interval;

  if (mesh_rssi == 0)
  {
    // No signal → LED OFF
    digitalWrite(LED_MESH_SIGNAL_STATUS, LOW);
    rssi_led_state = false;
    return;
  }
  else if (mesh_rssi > -40)
  {
    interval = 500; // strong signal
  }
  else if (mesh_rssi > -60)
  {
    interval = 1000; // medium signal
  }
  else
  {
    interval = 2000; // weak signal
  }

  if (now - last_rssi_led_blink >= interval)
  {
    last_rssi_led_blink = now;
    rssi_led_state = !rssi_led_state;
    digitalWrite(LED_MESH_SIGNAL_STATUS, rssi_led_state);
  }
}