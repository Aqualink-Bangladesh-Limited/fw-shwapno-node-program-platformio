#include <Arduino.h>
#include <esp_task_wdt.h>
#include <painlessMesh.h>
#include <HardwareSerial.h>
#include <deque>
#include <vector>
#include "debug_print.h"
#include "app_config.h"
#include "led_handler.h"
#include "packet_conversion.h"

painlessMesh mesh;

const int MAX_PACKET_SIZE = 256;
uint8_t rxBuffer[MAX_PACKET_SIZE];
int rxBufferIndex = 0;
unsigned long lastReceivedTime = 0;
const unsigned long TIMEOUT = 10;
unsigned long lastMeshReceivedTime = 0;

std::deque<std::vector<uint8_t>> packetQueue;
const int MAX_QUEUE_SIZE = 20;

void processIncomingData();
void sendMessagesFromQueue();

void receivedCallback(uint32_t from, String& msg) {

  lastMeshReceivedTime = millis();

  Serial.print("Received message from node ");
  Serial.print(from);
  Serial.print(": ");
  Serial.println(msg);

  if (msg.length() > 0) {
    int responseLength = msg.length();

    if (responseLength > MAX_PACKET_SIZE) {
      Serial.println("Received packet exceeds max size, discarding...");
      return;
    }

    if (packetQueue.size() >= MAX_QUEUE_SIZE) {
      packetQueue.pop_front();
    }

    uint8_t modbusPacket[256];
    int modbusPacketSize = hexStringToByteArray(msg, modbusPacket);

    if (modbusPacketSize > 0) {
      std::vector<uint8_t> packet(modbusPacket, modbusPacket + modbusPacketSize);
      packetQueue.push_back(packet);
    }
  }
}

void setup() {
  Serial.begin(115200);

  esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.println("Watchdog timer set for " + String(WATCHDOG_TIMEOUT) + " seconds");

  pinMode(LED_BLINK, OUTPUT);
  pinMode(LED_MESH, OUTPUT);

  // Set up mesh network
  //mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  mesh.onReceive(receivedCallback);

  printMeshInfo();


  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  esp_task_wdt_reset();

  mesh.update();

  processIncomingData();

  static unsigned long lastTime = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - lastTime >= 30000) {
    lastTime = currentMillis;
    printConnectedNodes();
  }

  sendMessagesFromQueue();
  // Send messages to the mesh network periodically
  //  sendMessagesToMesh();

  led_handler();
}

void processIncomingData() {
  unsigned long currentMillis = millis();

  while (Serial2.available()) {
    uint8_t incomingByte = Serial2.read();
    lastReceivedTime = millis();

    if (rxBufferIndex < MAX_PACKET_SIZE - 1) {
      rxBuffer[rxBufferIndex++] = incomingByte;
    } else {
      Serial.println("Buffer overflow, discarding incomplete packet...");
      printPacket(rxBuffer, rxBufferIndex);

      rxBufferIndex = 0;
    }
  }
  // Timeout check: If we don't receive any data within the TIMEOUT period, treat as a new packet
  if (currentMillis - lastReceivedTime > TIMEOUT) {
    if (rxBufferIndex > 0) {
      Serial.print("Received UART data: ");
      printPacket(rxBuffer, rxBufferIndex);

      String hexMessage = convertHexToString(rxBuffer, rxBufferIndex);
      mesh.sendBroadcast(hexMessage);  // Broadcast hex packet as a string
      Serial.println("Broadcasting TCP Packet");
    }
    rxBufferIndex = 0;
  }
}

void sendMessagesFromQueue() {
  unsigned long currentMillis = millis();
  static unsigned long lastSendTime = 0;
  const unsigned long interval = 100;  // Send every 100ms

  // Check if 100ms has passed since the last send
  if (currentMillis - lastSendTime >= interval) {
    lastSendTime = currentMillis;

    if (!packetQueue.empty()) {
      std::vector<uint8_t> packet = packetQueue.front();
      packetQueue.pop_front();  // Remove the sent packet from the queue

      // Send the packet via Serial2
      for (int i = 0; i < packet.size(); i++) {
        Serial2.write(packet[i]);
      }

      Serial.print("Packet Send to Master: ");
      printPacket(packet.data(), packet.size());
    }
  }
}
