#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#define MESH_PREFIX      "meshNetwork2"
#define MESH_PASSWORD    "meshPassword2"
#define MESH_PORT        5552
#define MESH_CHANNEL     2

#define LED_BLINK        11
#define LED_MESH         14

#define WATCHDOG_TIMEOUT 30
#define MESH_INTERVAL    (5 * 60 * 1000)
#define ONE_SECOND       1000

extern unsigned long lastMeshReceivedTime;
