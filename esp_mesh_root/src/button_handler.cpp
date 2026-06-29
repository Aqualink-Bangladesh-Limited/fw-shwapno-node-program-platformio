#include "button_handler.h"
#include "app_config.h"
#include "portal_handler.h"
#include "debug_log.h"

static constexpr uint8_t PORTAL_BUTTON_HOLD_SEC =
    (uint8_t)(PORTAL_BUTTON_HOLD_MS / ONE_SECOND);

static bool buttonStablePressed(unsigned long now)
{
  static bool lastRaw = false;
  static bool stable = false;
  static unsigned long lastChangeMs = 0;

  const bool raw = (digitalRead(PORTAL_BUTTON_PIN) == LOW);
  if (raw != lastRaw)
  {
    lastRaw = raw;
    lastChangeMs = now;
  }
  else if (now - lastChangeMs >= PORTAL_BUTTON_DEBOUNCE_MS)
  {
    stable = raw;
  }
  return stable;
}

void button_init()
{
  pinMode(PORTAL_BUTTON_PIN, INPUT_PULLUP);
  debugLog("portal button: ready on GPIO%d (hold %us for OTA portal)",
           PORTAL_BUTTON_PIN, (unsigned)PORTAL_BUTTON_HOLD_SEC);
}

void button_task()
{
  static unsigned long holdStartMs = 0;
  static uint8_t lastSecondLogged = 0;
  static bool holding = false;

  if (isPortalActive())
  {
    holdStartMs = 0;
    lastSecondLogged = 0;
    holding = false;
    return;
  }

  const unsigned long now = millis();
  const bool pressed = buttonStablePressed(now);

  if (!pressed)
  {
    if (holding)
    {
      const unsigned long heldSec = (now - holdStartMs) / 1000UL;
      debugLog("portal button: released at %lus (portal not started)", heldSec);
    }
    holdStartMs = 0;
    lastSecondLogged = 0;
    holding = false;
    return;
  }

  if (!holding)
  {
    holding = true;
    holdStartMs = now;
    lastSecondLogged = 0;
    debugLog("portal button: pressed (hold %us to open OTA portal)",
             (unsigned)PORTAL_BUTTON_HOLD_SEC);
    return;
  }

  const uint8_t heldSec = (uint8_t)((now - holdStartMs) / 1000UL);

  for (uint8_t sec = 1; sec <= PORTAL_BUTTON_HOLD_SEC; sec++)
  {
    if (heldSec < sec || lastSecondLogged >= sec)
      continue;

    lastSecondLogged = sec;
    debugLog("portal button: hold %u s", (unsigned)sec);

    if (sec == PORTAL_BUTTON_HOLD_SEC)
    {
      debugLog("portal button: %us complete - enabling portal mode",
               (unsigned)PORTAL_BUTTON_HOLD_SEC);
      extern bool portalRequested;
      extern unsigned long portalRequestTime;
      portalRequested = true;
      portalRequestTime = now;
      holdStartMs = 0;
      lastSecondLogged = 0;
      holding = false;
    }
    break;
  }
}
