#include <Arduino.h>
#include "app_config.h"
#include "led_handler.h"

#define MAX_RESTART 10

extern unsigned long lastMeshReceivedTime;

unsigned long lastLedBlink = 0;
unsigned long lastMeshLedBlink = 0;
unsigned long lastLedUpdateTime = 0;
const unsigned long blinkInterval = 500;
unsigned long meshBlinkInterval = 500;
int ledState = 0;
int meshLedState = 0;
int restartCount = 0;
bool ledRxtxOn = false;

void led_handler() {
    unsigned long now = millis();
    updateMeshBlinkInterval(now);

    if (now - lastMeshLedBlink >= meshBlinkInterval) {
        lastMeshLedBlink = now;
        meshLedState = !meshLedState;
        digitalWrite(LED_MESH, meshLedState);
    }
    if (now - lastLedBlink >= blinkInterval) {
        lastLedBlink = now;
        ledState = !ledState;
        digitalWrite(LED_BLINK, ledState);
    }
    
    if (now - lastLedUpdateTime < 2000 && ledRxtxOn) {
        digitalWrite(LED_RXTX_STATUS, HIGH);
    }else{
        digitalWrite(LED_RXTX_STATUS, LOW);
        ledRxtxOn = false;
    }

}

void updateMeshBlinkInterval(unsigned long now) {
    if (now - lastMeshReceivedTime <= MESH_INTERVAL) {
        meshBlinkInterval = 2000;
    } else {
        meshBlinkInterval = 500;
    }
}