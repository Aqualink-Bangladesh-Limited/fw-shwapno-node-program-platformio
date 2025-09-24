#include <Arduino.h>
#include <esp_task_wdt.h>
#include <painlessMesh.h>
#include <HardwareSerial.h>
#include <deque>
#include <vector>
#include <map>
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
std::map<int, uint32_t> slaveIdToNodeId;
std::map<int, unsigned long> lastSlaveSeen; 

constexpr unsigned long SLAVE_DISCOVERY_INTERVAL = 5000; 
constexpr unsigned long SLAVE_TIMEOUT = 300000; 
constexpr unsigned long DUMMY_BROADCAST_INTERVAL = 120000; 
unsigned long lastRetryMillis = 0;
int currentSlave = START_NODE; 

constexpr unsigned long RESTART_TIMEOUT = 900000; // 15 minutes

void handleMeshReceive(uint32_t from, String& msg);
void processUartInput();
void processPacketQueue();
void sendCommandToSlave(int slaveId, const String& command);
void retryMissingSlaves(unsigned long now);

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
    static unsigned long lastDiscoveryRetry = 0;
    static unsigned long lastDummyBroadcast = 0;
    unsigned long now = millis();

    if (now - lastNodePrint >= 30000) {
        lastNodePrint = now;
        printConnectedNodes();
    }

    processPacketQueue();
    led_handler();

    if (now - lastDiscoveryRetry >= SLAVE_DISCOVERY_INTERVAL) {
        lastDiscoveryRetry = now;
        retryMissingSlaves(now);
    }

    if (now - lastDummyBroadcast >= DUMMY_BROADCAST_INTERVAL) {
        lastDummyBroadcast = now;
        String dummyMsg = "DUMMY_BROADCAST";
        mesh.sendBroadcast(dummyMsg);
        Serial.println("Broadcasted dummy message to mesh: " + dummyMsg);
    }

    if (now - lastReceivedTime > RESTART_TIMEOUT) {
        Serial.println("Restarting: No UART RX/TX data for 15 minutes.");
        ESP.restart();
    }

    if (now - lastMeshReceivedTime > RESTART_TIMEOUT) {
        Serial.println("Restarting: No mesh data for 15 minutes.");
        ESP.restart();
    }
}

void handleMeshReceive(uint32_t from, String& msg) {
    if (msg.startsWith("SLAVE_ID_RESPONSE:")) {
        int slaveId = msg.substring(18).toInt();
        slaveIdToNodeId[slaveId] = from;
        lastSlaveSeen[slaveId] = millis(); 
        Serial.printf("Mapped slave_id %d to node %u\n", slaveId, from);
        return;
    }

    lastMeshReceivedTime = millis();

    if (msg.isEmpty() || msg.length() > MAX_PACKET_SIZE) return;

    if (packetQueue.size() >= MAX_QUEUE_SIZE) {
        packetQueue.pop_front();
    }

    uint8_t modbusPacket[MAX_PACKET_SIZE];
    int modbusPacketSize = hexStringToByteArray(msg, modbusPacket);
    if (modbusPacketSize > 0) {
        int slaveId = modbusPacket[6];
        lastSlaveSeen[slaveId] = millis();
        
        Serial.printf("Received message from node %u, slave id %d: ", from, slaveId);
        printPacket(modbusPacket, modbusPacketSize);
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

        int slaveId = rxBuffer[12];

        if(!(slaveId >= START_NODE && slaveId <= END_NODE)){ 
            Serial.printf("Out of range slave_id %d, ignoring packet.\n", slaveId);
            rxBufferIndex = 0;
            return;
        }

        String hexMessage = convertHexToString(rxBuffer, rxBufferIndex);
        sendCommandToSlave(slaveId, hexMessage);
        //mesh.sendBroadcast(hexMessage);
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

void sendCommandToSlave(int slaveId, const String& command) {
    if (slaveIdToNodeId.find(slaveId) != slaveIdToNodeId.end()) {
        mesh.sendSingle(slaveIdToNodeId[slaveId], command);
        Serial.printf("Sent command to slave_id %d (node %u)\n", slaveId, slaveIdToNodeId[slaveId]);
        Serial.println("Send Request Packet to Node:" + String(slaveId) + " with data: " + command);
    } else {
        Serial.printf("No node mapped for slave_id %d\n", slaveId);
    }
}

void retryMissingSlaves(unsigned long now) {
   
    bool missing = slaveIdToNodeId.find(currentSlave) == slaveIdToNodeId.end();
    bool timedOut = !missing && (now - lastSlaveSeen[currentSlave] > SLAVE_TIMEOUT);
    if (missing || timedOut) {
        String requestMsg = "SLAVE_ID_REQUEST:" + String(currentSlave);
        mesh.sendBroadcast(requestMsg);
        Serial.printf("Retrying discovery for slave_id %d\n", currentSlave);
    }
    currentSlave = (currentSlave == END_NODE) ? START_NODE : (currentSlave + 1);
    lastRetryMillis = now;
}
