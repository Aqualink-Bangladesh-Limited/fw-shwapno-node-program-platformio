#pragma once

extern unsigned long lastLedUpdateTime;
extern bool ledRxtxOn;
extern unsigned long lastMeshReceivedTime;

void led_handler();
void updateMeshBlinkInterval(unsigned long currentMillis);
