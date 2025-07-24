#include <Arduino.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <deque>
#include <vector>
#include <EEPROM.h>
#include "debug_print.h"

////***********Configure This Block***********//////////

const char* ssid = "Aqualink_Shwapno_Mirpur1";
const char* password = "aqualink@321";

////***********Configure This Block***********//////////

WiFiUDP udp;
int udpPort = 12345;
IPAddress udpAddress(255, 255, 255, 255);  // Broadcast address for all devices in the local network

#define RX_PIN 5
#define TX_PIN 4

#define LED_WIFI_STATUS 21
#define LED_RXTX_STATUS 14
#define LED_BLINK 11

#define WATCHDOG_TIMEOUT 30  // 30 seconds

unsigned long lastLedUpdateTime = 0;
const unsigned long ledUpdateInterval = 500;  // LED blink interval in milliseconds
bool ledWifiOn = false;
bool ledRxtxOn = false;

const int MAX_PACKET_SIZE = 256;
uint8_t udpBuffer[MAX_PACKET_SIZE];
uint8_t rxBuffer[MAX_PACKET_SIZE];
int rxBufferIndex = 0;

unsigned long lastReceivedTime = 0;
const unsigned long TIMEOUT = 10;

unsigned long lastBlinkTime = 0;          // To store the last time the LED was toggled
const unsigned long blinkInterval = 500;  // 500 milliseconds
bool ledState = false;

std::deque<std::vector<uint8_t>> packetQueue;
const int MAX_QUEUE_SIZE = 20;

unsigned long lastAttemptTime = 0;
unsigned long timeout = 60000;  // 1 minute timeout (in milliseconds)
int restartCount = 0;
const int maxRestarts = 100;

void connectToWiFi();
void processUDPAndSerialData();
void sendMessagesToRoot();
void sendTCPRequest(int nodeId, int requestId);
void sendMessagesFromQueue();
bool isValidPacket(uint8_t* packet, int len); 

void setup() {
  Serial.begin(115200);

  esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.println("Watchdog timer set for " + String(WATCHDOG_TIMEOUT) + " seconds");

  pinMode(LED_WIFI_STATUS, OUTPUT);
  pinMode(LED_RXTX_STATUS, OUTPUT);
  pinMode(LED_BLINK, OUTPUT);

  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  connectToWiFi();

  udp.begin(udpPort);

  Serial.println("UDP server is listening on port " + String(udpPort));
}

void loop() {
  esp_task_wdt_reset();

  unsigned long currentMillis = millis();

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_WIFI_STATUS, HIGH);
  } else {
    digitalWrite(LED_WIFI_STATUS, LOW);
    connectToWiFi();
  }

  processUDPAndSerialData();

  if (currentMillis - lastLedUpdateTime < 2000) {
    if (ledRxtxOn) {
      digitalWrite(LED_RXTX_STATUS, HIGH);  // Turn on RX/TX LED for 2 seconds
    }
  } else {
    digitalWrite(LED_RXTX_STATUS, LOW);
    ledRxtxOn = false;
  }

  sendMessagesFromQueue();
  //  sendMessagesToRoot();

  if (currentMillis - lastBlinkTime >= blinkInterval) {
    lastBlinkTime = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_BLINK, (ledState == 0) ? LOW : HIGH);
  }
}

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  lastAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");

    if (millis() - lastAttemptTime >= timeout) {
      Serial.println("\nFailed to connect to Wi-Fi in 1 minute. Restarting...");

      restartCount++;

      EEPROM.write(0, restartCount);
      EEPROM.commit();

      if (restartCount < maxRestarts) {
        Serial.println("ESP Restarting.....");
        ESP.restart();
      }
    }
  }
  Serial.println("Connected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  restartCount = 0;

  EEPROM.write(0, restartCount);
  EEPROM.commit();
}

void processUDPAndSerialData() {
  unsigned long currentMillis = millis();

  int packetSize = udp.parsePacket();
  if (packetSize) {
    // Retrieve the source IP and port from the incoming UDP packet
    udpAddress = udp.remoteIP();
    udpPort = udp.remotePort();

    Serial.print("Received data from IP: ");
    Serial.print(udpAddress);
    Serial.print(", Port: ");
    Serial.println(udpPort);

    int len = udp.read(udpBuffer, MAX_PACKET_SIZE);
    if (len > 0) {
      udpBuffer[len] = 0;  // Null-terminate the string
    }

    Serial.print("Received UDP packet: ");
    printPacket(udpBuffer, len);
    //Serial.println("Received UDP packet len: " + String(len));

    bool isTcpRequest = isValidPacket(udpBuffer, len);

    if (isTcpRequest) {
      if (packetQueue.size() >= MAX_QUEUE_SIZE) {
        packetQueue.pop_front();
      }

      std::vector<uint8_t> packet;
      for (int i = 0; i < 4; i++) {
        packet.push_back(udpAddress[i]);
      }

      // Add the 2 bytes of the UDP port to the packet (port is 2 bytes)
      packet.push_back(udpPort >> 8);    // High byte of the port
      packet.push_back(udpPort & 0xFF);  // Low byte of the port

      packet.insert(packet.end(), udpBuffer, udpBuffer + len);  // Add the UDP packet data to the vector
      packetQueue.push_back(packet);

      ledWifiOn = true;
      lastLedUpdateTime = currentMillis;
    } else {
      Serial.println("Packet discarded: Not a TCP request.");
    }
  }

  while (Serial2.available()) {
    uint8_t incomingByte = Serial2.read();
    lastReceivedTime = currentMillis;

    if (rxBufferIndex < MAX_PACKET_SIZE - 1) {
      rxBuffer[rxBufferIndex++] = incomingByte;
    } else {
      Serial.println("Buffer overflow, discarding data...");
      rxBufferIndex = 0;
    }
  }

  rxBuffer[rxBufferIndex] = '\0';

  // Timeout check: If no data has been received within the timeout period, discard the buffer
  if (millis() - lastReceivedTime > TIMEOUT) {
    if (rxBufferIndex > 0) {
      Serial.print("Received from RX/TX: ");
      printPacket(rxBuffer, rxBufferIndex);

      if (rxBufferIndex > 6) {
        // First 4 bytes are the destination IP address
        IPAddress destinationIP(rxBuffer[0], rxBuffer[1], rxBuffer[2], rxBuffer[3]);

        // Next 2 bytes are the UDP port (high byte first, then low byte)
        uint16_t destinationPort = (rxBuffer[4] << 8) | rxBuffer[5];

        // The rest of the data after the IP address
        uint8_t* packetData = rxBuffer + 6;        // Skip the first 4 bytes (IP address)
        int packetDataLength = rxBufferIndex - 6;  // Remaining data length

        Serial.print("Sending data to IP: ");
        Serial.print(destinationIP);
        Serial.print(", Port: ");
        Serial.println(udpPort);

        // Send the rest of the data to the extracted IP address and the predefined port
        udp.beginPacket(destinationIP, udpPort);
        udp.write(packetData, packetDataLength);
        udp.endPacket();

        ledRxtxOn = true;
        lastLedUpdateTime = currentMillis;  // Timestamp when LED was turned on
      } else {
        Serial.println("Packet length is less than 5 bytes, discarding data.");
      }

      rxBufferIndex = 0;
    }
  }
}

void sendMessagesToRoot() {
  unsigned long currentMillis = millis();
  static unsigned long lastSendTime = 0;
  const unsigned long interval = 100;
  static int currentNode = 1;
  static int requestId = 1;

  // Check if interval has passed since the last request
  if (currentMillis - lastSendTime >= interval) {
    lastSendTime = currentMillis;

    // Send a message to the mesh
    sendTCPRequest(currentNode, requestId);

    currentNode++;
    requestId++;

    if (currentNode > 6) {
      currentNode = 1;
    }
  }
}

void sendTCPRequest(int nodeId, int requestId) {
  // Modbus TCP packet components
  uint16_t transactionId = requestId;
  uint16_t protocolId = 0x0400;        // Protocol ID for Modbus TCP
  uint16_t unitId = nodeId;            // Slave ID (unit identifier)
  uint8_t functionCode = 0x10;         // Function code for Write Multiple Registers
  uint16_t startingRegister = 0x0400;  // Starting register
  uint16_t numRegisters = 3;           // Number of registers to write
  uint16_t data[] = { 1, 25, 1 };      // Values to write to the registers

  // Calculate the total length of the message (including headers and data)
  uint16_t dataLength = numRegisters * 2;  // Each register is 2 bytes
  uint16_t length = 6 + dataLength + 1;    // 6 bytes for header and 1 byte for number of bytes to write

  // Construct the Modbus TCP packet
  std::vector<uint8_t> packet(13 + dataLength);  // 13 bytes for header and dynamic size for data

  // Transaction Identifier (2 bytes)
  packet[0] = (transactionId >> 8) & 0xFF;
  packet[1] = transactionId & 0xFF;

  // Protocol Identifier (2 bytes)
  packet[2] = (protocolId >> 8) & 0xFF;
  packet[3] = protocolId & 0xFF;

  // Length (2 bytes)
  packet[4] = (length >> 8) & 0xFF;
  packet[5] = length & 0xFF;

  // Unit Identifier (1 byte)
  packet[6] = unitId;

  // Function Code (1 byte)
  packet[7] = functionCode;

  // Starting Register (2 bytes)
  packet[8] = (startingRegister >> 8) & 0xFF;
  packet[9] = startingRegister & 0xFF;

  // Number of Registers (2 bytes)
  packet[10] = (numRegisters >> 8) & 0xFF;
  packet[11] = numRegisters & 0xFF;

  // Byte Count (1 byte) - Number of bytes of data to follow (2 bytes per register)
  packet[12] = numRegisters * 2;  // 2 bytes per register

  for (int i = 0; i < numRegisters; i++) {
    packet[13 + i * 2] = (data[i] >> 8) & 0xFF;
    packet[14 + i * 2] = data[i] & 0xFF;
  }

  // Send the constructed packet to the mesh
  // String message = "Modbus TCP Write Request to Node " + String(nodeId) + " Request " + String(requestId);
  // Serial.println("Message: " + message);

  if (packetQueue.size() >= MAX_QUEUE_SIZE) {
    packetQueue.pop_front();  // Remove the oldest packet if queue is full
  }

  packetQueue.push_back(packet);
}

void sendMessagesFromQueue() {
  unsigned long currentMillis = millis();
  static unsigned long lastSendTime = 0;
  const unsigned long interval = 100;  // Send every 100ms

  if (currentMillis - lastSendTime >= interval) {
    lastSendTime = currentMillis;

    if (!packetQueue.empty()) {
      std::vector<uint8_t> packet = packetQueue.front();
      packetQueue.pop_front();

      Serial2.write(packet.data(), packet.size());

      Serial.print("Sending packet to Root: ");
      printPacket(packet.data(), packet.size());
    }
  }
}

bool isValidPacket(uint8_t* packet, int len) {
  if (len < 6) return false;
  uint16_t length = (packet[4] >> 8) | packet[5];
  uint16_t expectedLength = 6 + length;
  //Serial.println("Expected Length: " + String(expectedLength));
  return (len == expectedLength);
}
