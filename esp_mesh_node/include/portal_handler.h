#pragma once
#include <Arduino.h>

void portal_init();
void enterPortalMode(uint8_t nodeId);
void exitPortalMode();
void portal_task();
bool isPortalActive();
void portal_touchActivity();
