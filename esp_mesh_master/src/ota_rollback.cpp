#include "ota_rollback.h"
#include "portal_handler.h"
#include "debug_log.h"
#include "app_config.h"
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <Arduino.h>

static constexpr uint8_t MAX_PENDING_BOOT_FAILS = 3;

static bool runningImagePendingVerify()
{
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (!running)
    return false;

  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(running, &state) != ESP_OK)
    return false;

  return state == ESP_OTA_IMG_PENDING_VERIFY;
}

bool ota_rollback_requires_portal_boot()
{
  return runningImagePendingVerify();
}

static void clearPortalBootFlag()
{
  Preferences prefs;
  if (prefs.begin("portal", false))
  {
    prefs.putBool("portalBoot", false);
    prefs.end();
  }
}

bool ota_rollback_is_verify_hold_active()
{
  if (!isPortalActive() || !runningImagePendingVerify())
    return false;

  return (millis() - portal_enteredAtMs()) < PORTAL_OTA_VERIFY_MS;
}

unsigned long ota_rollback_verify_hold_seconds_left()
{
  if (!ota_rollback_is_verify_hold_active())
    return 0;

  const unsigned long elapsed = millis() - portal_enteredAtMs();
  if (elapsed >= PORTAL_OTA_VERIFY_MS)
    return 0;

  return (PORTAL_OTA_VERIFY_MS - elapsed + 999UL) / 1000UL;
}

static const char *resetReasonString(esp_reset_reason_t reason)
{
  switch (reason)
  {
  case ESP_RST_POWERON:
    return "poweron";
  case ESP_RST_SW:
    return "sw";
  case ESP_RST_PANIC:
    return "panic";
  case ESP_RST_INT_WDT:
    return "int_wdt";
  case ESP_RST_TASK_WDT:
    return "task_wdt";
  case ESP_RST_WDT:
    return "wdt";
  case ESP_RST_BROWNOUT:
    return "brownout";
  case ESP_RST_DEEPSLEEP:
    return "deepsleep";
  default:
    return "other";
  }
}

static bool isCrashReset(esp_reset_reason_t reason)
{
  return reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT ||
         reason == ESP_RST_TASK_WDT || reason == ESP_RST_WDT ||
         reason == ESP_RST_BROWNOUT;
}

static void clearPendingBootFails()
{
  Preferences prefs;
  if (prefs.begin("ota_boot", false))
  {
    prefs.putUChar("pendingFails", 0);
    prefs.end();
  }
}

void ota_rollback_early_check()
{
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (!running)
    return;

  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(running, &state) != ESP_OK)
    return;

  if (state != ESP_OTA_IMG_PENDING_VERIFY)
  {
    clearPendingBootFails();
    return;
  }

  const esp_reset_reason_t reason = esp_reset_reason();
  if (!isCrashReset(reason))
  {
    debugLog("ota_rollback: pending verify, %s reset (not counted toward rollback)",
             resetReasonString(reason));
    return;
  }

  Preferences prefs;
  uint8_t fails = 0;
  if (prefs.begin("ota_boot", false))
  {
    fails = prefs.getUChar("pendingFails", 0);
    fails++;
    prefs.putUChar("pendingFails", fails);
    prefs.end();
  }

  debugLog("ota_rollback: crash while pending verify (%u/%u, reason %s)",
           (unsigned)fails, (unsigned)MAX_PENDING_BOOT_FAILS, resetReasonString(reason));

  if (fails >= MAX_PENDING_BOOT_FAILS)
  {
    debugLog("ota_rollback: too many crash boots, rolling back");
    esp_ota_mark_app_invalid_rollback_and_reboot();
  }
}

void ota_rollback_portal_task()
{
  static bool markedValid = false;

  if (!isPortalActive())
  {
    markedValid = false;
    return;
  }

  if (markedValid)
    return;

  if (millis() - portal_enteredAtMs() < PORTAL_OTA_VERIFY_MS)
    return;

  const esp_partition_t *running = esp_ota_get_running_partition();
  if (!running)
  {
    markedValid = true;
    return;
  }

  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(running, &state) != ESP_OK)
  {
    markedValid = true;
    return;
  }

  if (state == ESP_OTA_IMG_PENDING_VERIFY)
  {
    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK)
    {
      debugLog("ota_rollback: firmware marked valid after 60s in portal");
      clearPendingBootFails();
      clearPortalBootFlag();
    }
    else
    {
      debugLog("ota_rollback: mark valid failed");
    }
  }

  markedValid = true;
}
