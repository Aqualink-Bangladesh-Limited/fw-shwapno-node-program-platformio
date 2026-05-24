#include "restart_guard.h"
#include "app_config.h"
#include "debug_log.h"
#include <Preferences.h>
#include <Arduino.h>

static uint8_t consecutiveIdleRestarts = 0;
static bool lockoutActive = false;

static void saveCount()
{
  Preferences prefs;
  if (prefs.begin("root_boot", false))
  {
    prefs.putUChar("consecIdle", consecutiveIdleRestarts);
    prefs.end();
  }
}

void restart_guard_init()
{
  Preferences prefs;
  if (!prefs.begin("root_boot", false))
  {
    debugLog("restart_guard: NVS unavailable");
    return;
  }

  consecutiveIdleRestarts = prefs.getUChar("consecIdle", 0);
  prefs.end();

  lockoutActive = consecutiveIdleRestarts >= MAX_CONSECUTIVE_IDLE_RESTARTS;

  if (lockoutActive)
  {
    debugLog("restart_guard: LOCKOUT after %u consecutive idle restarts (max %u). "
             "Auto-restart disabled.",
             (unsigned)consecutiveIdleRestarts, (unsigned)MAX_CONSECUTIVE_IDLE_RESTARTS);
  }
  else if (consecutiveIdleRestarts > 0)
  {
    debugLog("restart_guard: %u consecutive idle restart(s) on record",
             (unsigned)consecutiveIdleRestarts);
  }
}

void restart_guard_clear()
{
  consecutiveIdleRestarts = 0;
  lockoutActive = false;
  saveCount();
}

void restart_guard_note_activity()
{
  if (consecutiveIdleRestarts == 0 && !lockoutActive)
    return;

  restart_guard_clear();
  debugLog("restart_guard: healthy activity, idle restart counter cleared");
}

bool restart_guard_is_lockout()
{
  return lockoutActive;
}

uint8_t restart_guard_get_count()
{
  return consecutiveIdleRestarts;
}

void restart_guard_request_idle_restart(const char *reason)
{
  if (consecutiveIdleRestarts >= MAX_CONSECUTIVE_IDLE_RESTARTS)
  {
    lockoutActive = true;
    static unsigned long lastWarnMs = 0;
    const unsigned long now = millis();
    if (now - lastWarnMs >= 60000UL)
    {
      lastWarnMs = now;
      debugLog("restart_guard: idle restart blocked (%s). %u consecutive restarts.",
               reason ? reason : "unknown",
               (unsigned)consecutiveIdleRestarts);
    }
    return;
  }

  consecutiveIdleRestarts++;
  saveCount();

  debugLog("restart_guard: idle restart %u/%u (%s)",
           (unsigned)consecutiveIdleRestarts,
           (unsigned)MAX_CONSECUTIVE_IDLE_RESTARTS,
           reason ? reason : "unknown");

  delay(100);
  ESP.restart();
}
