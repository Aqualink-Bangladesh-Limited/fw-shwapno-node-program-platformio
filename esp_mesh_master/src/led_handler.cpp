#include "app_config.h"
#include "led_handler.h"
#include "portal_handler.h"
#include "bridge_handler.h"
#include <Arduino.h>
#include <WiFi.h>

/* See app_config.h for the full LED pattern guide. */

static bool wasPortalActive = false;

static void handle_portal_leds(unsigned long now)
{
  const bool signalOn = ((now / LED_PORTAL_SIGNAL_MS) & 1U) == 0U;

  digitalWrite(LED_WIFI_STATUS, HIGH);
  digitalWrite(LED_UDP_STATUS, signalOn ? HIGH : LOW);
  digitalWrite(LED_RXTX_STATUS, LOW);
}

static void handle_bridge_leds(unsigned long now)
{
  bridge_refresh_led_flags();

  if (now - bridge_last_rxtx_activity() < 2000 && bridge_led_rxtx_on())
    digitalWrite(LED_RXTX_STATUS, HIGH);
  else
    digitalWrite(LED_RXTX_STATUS, LOW);

  if (now - bridge_last_udp_activity() < 2000 && bridge_led_udp_on())
    digitalWrite(LED_UDP_STATUS, HIGH);
  else
    digitalWrite(LED_UDP_STATUS, LOW);

  static unsigned long lastWifiBlinkTime = 0;
  static bool wifiLedState = false;

  if (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_WIFI_STATUS, HIGH);
    return;
  }

  long rssi = WiFi.RSSI();
  unsigned long blinkInterval;
  if (rssi > -40)
    blinkInterval = 500;
  else if (rssi > -60)
    blinkInterval = 1000;
  else
    blinkInterval = 2000;

  if (now - lastWifiBlinkTime >= blinkInterval)
  {
    lastWifiBlinkTime = now;
    wifiLedState = !wifiLedState;
    digitalWrite(LED_WIFI_STATUS, wifiLedState ? HIGH : LOW);
  }
}

void led_handler()
{
  const unsigned long now = millis();
  const bool portalActive = isPortalActive();

  if (portalActive != wasPortalActive)
    wasPortalActive = portalActive;

  if (portalActive)
    handle_portal_leds(now);
  else
    handle_bridge_leds(now);
}
