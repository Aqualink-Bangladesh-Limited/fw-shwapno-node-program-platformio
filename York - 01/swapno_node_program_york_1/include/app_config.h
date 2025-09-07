#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#define MESH_PREFIX      "meshNetwork2"
#define MESH_PASSWORD    "meshPassword2"
#define MESH_PORT        5552
#define MESH_CHANNEL     5

#define NODE_ID          8

#define irRawLength      147

#define LED_BLINK        11
#define LED_AC_STATUS    21
#define LED_MESH         14
#define IR_PIN           45

#define RX_PIN           5
#define TX_PIN           4
#define DIRECTION_PIN    13

#define SLAVE_ID         1
#define START_ADDRESS    0
#define NUM_REGS         2

#define WATCHDOG_TIMEOUT 30
#define MESH_INTERVAL    (5 * 60 * 1000)
#define ONE_SECOND       1000

extern uint16_t arr[5];
extern uint16_t sensor_data[2];
extern uint16_t temperature, humidity;
extern bool flag;
extern unsigned long last_read_time;
extern unsigned long last_ir_send_time;
extern unsigned long lastMeshReceivedTime;
