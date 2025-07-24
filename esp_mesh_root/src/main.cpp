#include <Arduino.h>
#include <esp_task_wdt.h>
#include <painlessMesh.h>
#include <HardwareSerial.h>
#include <deque>
#include <vector>
#include "app_config.h"
#include "debug_print.h"
#include "led_handler.h"
#include "packet_conversion.h"

constexpr int MAX_PACKET_SIZE = 256;
constexpr int MAX_QUEUE_SIZE = 20;
constexpr unsigned long UART_TIMEOUT = 10;

painlessMesh mesh;

uint8_t rxBuffer[MAX_PACKET_SIZE];
int rxBufferIndex = 0;
unsigned long lastReceivedTime = 0;
unsigned long lastMeshReceivedTime = 0;

extern unsigned long lastLedUpdateTime;
extern bool ledRxtxOn;

std::deque<std::vector<uint8_t>> packetQueue;

void handleMeshReceive(uint32_t from, String& msg);
void processUartInput();
void processPacketQueue();

void setup() {
    Serial.begin(115200);
    esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
    esp_task_wdt_add(nullptr);

    pinMode(LED_BLINK, OUTPUT);
    pinMode(LED_RXTX_STATUS, OUTPUT);
    pinMode(LED_MESH, OUTPUT);

    mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
    mesh.setRoot(true);
    mesh.setContainsRoot(true);
    mesh.onReceive(handleMeshReceive);

    printMeshInfo();
    Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
    esp_task_wdt_reset();
    mesh.update();
    processUartInput();

    static unsigned long lastNodePrint = 0;
    unsigned long now = millis();
    if (now - lastNodePrint >= 30000) {
        lastNodePrint = now;
        printConnectedNodes();
    }

    processPacketQueue();
    led_handler();
}

void handleMeshReceive(uint32_t from, String& msg) {
    lastMeshReceivedTime = millis();
    Serial.printf("Received message from node %u: %s\n", from, msg.c_str());

    if (msg.isEmpty() || msg.length() > MAX_PACKET_SIZE) return;

    if (packetQueue.size() >= MAX_QUEUE_SIZE) {
        packetQueue.pop_front();
    }

    uint8_t modbusPacket[MAX_PACKET_SIZE];
    int modbusPacketSize = hexStringToByteArray(msg, modbusPacket);
    if (modbusPacketSize > 0) {
        packetQueue.emplace_back(modbusPacket, modbusPacket + modbusPacketSize);
    }
}

void processUartInput() {
    unsigned long now = millis();
    while (Serial2.available()) {
        uint8_t incomingByte = Serial2.read();
        lastReceivedTime = now;
        if (rxBufferIndex < MAX_PACKET_SIZE - 1) {
            rxBuffer[rxBufferIndex++] = incomingByte;
        } else {
            Serial.println("Buffer overflow, discarding incomplete packet...");
            printPacket(rxBuffer, rxBufferIndex);
            rxBufferIndex = 0;
        }
    }

    if (now - lastReceivedTime > UART_TIMEOUT && rxBufferIndex > 0) {
        Serial.print("Received UART data: ");
        printPacket(rxBuffer, rxBufferIndex);
        String hexMessage = convertHexToString(rxBuffer, rxBufferIndex);
        mesh.sendBroadcast(hexMessage);
        Serial.println("Broadcasting TCP Packet");
        rxBufferIndex = 0;
        
        ledRxtxOn = true;
        lastLedUpdateTime = now;
    }
}

void processPacketQueue() {
    static unsigned long lastSendTime = 0;
    constexpr unsigned long SEND_INTERVAL = 100;
    unsigned long now = millis();

    if (now - lastSendTime >= SEND_INTERVAL && !packetQueue.empty()) {
        lastSendTime = now;
        const auto& packet = packetQueue.front();
        for (uint8_t b : packet) {
            Serial2.write(b);
        }
        Serial.print("Packet sent to Master: ");
        printPacket(packet.data(), packet.size());
        packetQueue.pop_front();
    }
}
