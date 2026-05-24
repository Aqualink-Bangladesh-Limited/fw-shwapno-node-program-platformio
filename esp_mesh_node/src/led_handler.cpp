#include "app_config.h"
#include "led_handler.h"
#include "portal_handler.h"

/* See app_config.h for the full LED pattern guide. */

typedef struct
{
  unsigned long durationMs;
  bool on;
} LedPhaseStep;

static bool meshLedState = false;
static bool signalLedState = false;
static bool acLedState = false;

static unsigned long meshLedLastToggle = 0;
static unsigned long signalLedLastToggle = 0;

static uint8_t signalPhaseIndex = 0;
static unsigned long signalPhaseStart = 0;
static bool signalUsingPhasePattern = false;
static bool wasPortalActive = false;

static void led_write(uint8_t pin, bool on)
{
  digitalWrite(pin, on ? HIGH : LOW);
}

static void led_toggle_interval(uint8_t pin, unsigned long halfPeriodMs, unsigned long now,
                                unsigned long *lastToggle, bool *state)
{
  if (now - *lastToggle < halfPeriodMs)
    return;
  *lastToggle = now;
  *state = !*state;
  led_write(pin, *state);
}

static void led_run_phase_pattern(uint8_t pin, unsigned long now,
                                  const LedPhaseStep *steps, uint8_t stepCount,
                                  uint8_t *phaseIndex, unsigned long *phaseStart)
{
  if (*phaseStart == 0)
  {
    *phaseStart = now;
    *phaseIndex = 0;
    led_write(pin, steps[0].on);
    return;
  }

  if (now - *phaseStart < steps[*phaseIndex].durationMs)
    return;

  *phaseStart = now;
  *phaseIndex = (uint8_t)((*phaseIndex + 1) % stepCount);
  led_write(pin, steps[*phaseIndex].on);
}

static void reset_signal_phase_pattern()
{
  signalPhaseIndex = 0;
  signalPhaseStart = 0;
  signalUsingPhasePattern = false;
}

static void handle_portal_leds(unsigned long now)
{
  /* Time-based blink: correct rate even if loop() was delayed (mesh_stop, WiFi, etc.) */
  const bool signalOn = ((now / LED_PORTAL_SIGNAL_MS) & 1U) == 0U;

  led_write(LED_MESH, true);
  led_write(LED_AC_STATUS, false);
  led_write(LED_MESH_SIGNAL_STATUS, signalOn);
}

static void handle_mesh_link_led(unsigned long now)
{
  if (now - lastMeshReceivedTime <= MESH_INTERVAL)
  {
    led_write(LED_MESH, true);
    return;
  }

  led_toggle_interval(LED_MESH, LED_MESH_SEARCH_MS, now, &meshLedLastToggle, &meshLedState);
}

static void handle_signal_led(unsigned long now)
{
  if (mesh_rssi == 0)
  {
    static const LedPhaseStep noRssiPattern[] = {
        {LED_NO_RSSI_PULSE_MS, true},
        {LED_NO_RSSI_PULSE_MS, false},
        {LED_NO_RSSI_PULSE_MS, true},
        {LED_NO_RSSI_PAUSE_MS, false},
    };

    if (!signalUsingPhasePattern)
    {
      signalUsingPhasePattern = true;
      signalPhaseIndex = 0;
      signalPhaseStart = 0;
      signalLedLastToggle = now;
    }

    led_run_phase_pattern(LED_MESH_SIGNAL_STATUS, now, noRssiPattern,
                          (uint8_t)(sizeof(noRssiPattern) / sizeof(noRssiPattern[0])),
                          &signalPhaseIndex, &signalPhaseStart);
    return;
  }

  if (signalUsingPhasePattern)
  {
    reset_signal_phase_pattern();
    signalLedLastToggle = now;
    signalLedState = false;
    led_write(LED_MESH_SIGNAL_STATUS, false);
  }

  unsigned long halfPeriodMs;
  if (mesh_rssi > -40)
    halfPeriodMs = LED_RSSI_STRONG_MS;
  else if (mesh_rssi > -60)
    halfPeriodMs = LED_RSSI_MEDIUM_MS;
  else
    halfPeriodMs = LED_RSSI_WEAK_MS;

  led_toggle_interval(LED_MESH_SIGNAL_STATUS, halfPeriodMs, now,
                      &signalLedLastToggle, &signalLedState);
}

static void handle_ac_led(uint16_t acState)
{
  switch (acState)
  {
  case 0:
    led_write(LED_AC_STATUS, false);
    break;
  case 1:
  case 2:
    led_write(LED_AC_STATUS, true);
    break;
  case 3:
  {
    static unsigned long acOffLastToggle = 0;
    led_toggle_interval(LED_AC_STATUS, LED_AC_OFF_BLINK_MS, millis(),
                        &acOffLastToggle, &acLedState);
    break;
  }
  default:
    led_write(LED_AC_STATUS, false);
    break;
  }
}

void led_handler()
{
  const unsigned long now = millis();
  const bool portalActive = isPortalActive();

  if (portalActive != wasPortalActive)
  {
    wasPortalActive = portalActive;
    meshLedLastToggle = now;
    signalLedLastToggle = now;
    reset_signal_phase_pattern();
    meshLedState = false;
    signalLedState = false;
    if (portalActive)
      handle_portal_leds(now);
  }

  if (portalActive)
  {
    handle_portal_leds(now);
    return;
  }

  handle_ac_led(arr[1]);
  handle_mesh_link_led(now);
  handle_signal_led(now);
}
