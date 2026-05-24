#include <Arduino.h>
#include "app_config.h"
#include "led_handler.h"
#include "portal_handler.h"
#include "mesh_handler.h"

unsigned long lastMeshReceivedTime = 0;
unsigned long lastLedBlink = 0;
unsigned long lastMeshLedBlink = 0;
unsigned long lastLedUpdateTime = 0;
const unsigned long blinkInterval = 500;
unsigned long meshBlinkInterval = 500;
int ledState = 0;
int meshLedState = 0;
bool ledRxtxOn = false;

static void handle_portal_leds(unsigned long now)
{
  const bool signalOn = ((now / LED_PORTAL_SIGNAL_MS) & 1U) == 0U;

  digitalWrite(LED_MESH, HIGH);
  digitalWrite(LED_BLINK, signalOn ? HIGH : LOW);
  digitalWrite(LED_RXTX_STATUS, LOW);
}

static void handle_mesh_leds(unsigned long now)
{
  updateMeshBlinkInterval(now);

  if (now - lastMeshLedBlink >= meshBlinkInterval)
  {
    lastMeshLedBlink = now;
    meshLedState = !meshLedState;
    digitalWrite(LED_MESH, meshLedState);
  }

  if (now - lastLedBlink >= blinkInterval)
  {
    lastLedBlink = now;
    ledState = !ledState;
    digitalWrite(LED_BLINK, ledState);
  }

  if (now - lastLedUpdateTime < 2000 && ledRxtxOn)
    digitalWrite(LED_RXTX_STATUS, HIGH);
  else
  {
    digitalWrite(LED_RXTX_STATUS, LOW);
    ledRxtxOn = false;
  }
}

void led_handler()
{
  const unsigned long now = millis();

  if (isPortalActive())
    handle_portal_leds(now);
  else
    handle_mesh_leds(now);
}

void updateMeshBlinkInterval(unsigned long now)
{
  if (now - lastMeshReceivedTime <= MESH_INTERVAL)
    meshBlinkInterval = 2000;
  else
    meshBlinkInterval = 500;
}
