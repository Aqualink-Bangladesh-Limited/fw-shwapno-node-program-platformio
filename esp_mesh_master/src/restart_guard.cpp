#include "restart_guard.h"
#include "app_config.h"
#include "debug_log.h"
#include <EEPROM.h>
#include <Arduino.h>

static int restartCount = 0;
static bool restartHalted = false;

void restart_guard_init()
{
  EEPROM.begin(512);
  restartCount = EEPROM.read(RESTART_COUNT_EEPROM_ADDR);
  if (restartCount < 0 || restartCount > 255)
    restartCount = 0;

  restartHalted = (restartCount >= MAX_CONSECUTIVE_RESTARTS);
  if (restartHalted)
  {
    debugLog("Restart guard: halted (%d consecutive restarts in EEPROM)", restartCount);
  }
}

void restart_guard_clear()
{
  restartCount = 0;
  restartHalted = false;
  EEPROM.write(RESTART_COUNT_EEPROM_ADDR, 0);
  EEPROM.commit();
}

bool restart_guard_is_halted()
{
  return restartHalted;
}

void restart_guard_request(const char *reason)
{
  if (restartHalted)
  {
    debugLog("%s (auto-restart blocked, max %d reached)", reason ? reason : "restart",
             MAX_CONSECUTIVE_RESTARTS);
    return;
  }

  restartCount++;
  if (restartCount > 255)
    restartCount = 255;

  EEPROM.write(RESTART_COUNT_EEPROM_ADDR, static_cast<uint8_t>(restartCount));
  EEPROM.commit();

  if (restartCount < MAX_CONSECUTIVE_RESTARTS)
  {
    debugLog("%s - restarting (%d/%d)", reason ? reason : "restart", restartCount,
             MAX_CONSECUTIVE_RESTARTS);
    delay(200);
    ESP.restart();
  }

  restartHalted = true;
  debugLog("%s - max restarts (%d) reached; halting auto-restart", reason ? reason : "restart",
           MAX_CONSECUTIVE_RESTARTS);
}
