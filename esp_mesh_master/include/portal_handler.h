#pragma once
#include <Arduino.h>

void portal_init();
void enterPortalMode(uint8_t deviceId);
void exitPortalMode();
void portal_task();
bool isPortalActive();
void portal_touchActivity();
void portal_schedule_exit();
void portal_schedule_ota_reboot();
void portal_process_deferred_actions();
unsigned long portal_enteredAtMs();
