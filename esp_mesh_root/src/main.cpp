#include <Arduino.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <painlessMesh.h>
#include <HardwareSerial.h>
#include <deque>
#include <vector>
#include <map>
#include "app_config.h"
#include "debug_print.h"
#include "debug_log.h"
#include "led_handler.h"
#include "packet_conversion.h"
#include "packet_filter.h"
#include "root_modbus_handler.h"
#include "portal_handler.h"
#include "button_handler.h"
#include "mesh_handler.h"
#include "restart_guard.h"
#include "ota_rollback.h"

constexpr int MAX_PACKET_SIZE = 256;
constexpr int MAX_QUEUE_SIZE = 20;
constexpr unsigned long UART_TIMEOUT = 10;

painlessMesh mesh;

uint8_t rxBuffer[MAX_PACKET_SIZE];
int rxBufferIndex = 0;
unsigned long lastReceivedTime = 0;

std::deque<std::vector<uint8_t>> packetQueue;
std::map<int, uint32_t> slaveIdToNodeId;
std::map<int, unsigned long> lastSlaveSeen;

bool portalRequested = false;
unsigned long portalRequestTime = 0;
bool portalBootOnNextBoot = false;
uint8_t portalStoredDeviceId = ROOT_ID;
long mesh_rssi = 0;

constexpr unsigned long SLAVE_DISCOVERY_INTERVAL = 5000;
constexpr unsigned long SLAVE_TIMEOUT = 300000;
constexpr unsigned long DUMMY_BROADCAST_INTERVAL = 120000;
unsigned long lastRetryMillis = 0;
int currentSlave = START_NODE;

void handleMeshReceive(uint32_t from, String &msg);
void processUartInput();
void processPacketQueue();
void sendCommandToSlave(int slaveId, const String &command);
void retryMissingSlaves(unsigned long now);
static void initMeshRoot();

void setup()
{
  Serial.begin(115200);
  delay(500);

  esp_task_wdt_deinit();
  debugLog("boot: setup start");
  ota_rollback_early_check();
  restart_guard_init();

  Preferences prefs;
  if (prefs.begin("portal", false))
  {
    bool pb = prefs.getBool("portalBoot", false);
    if (pb)
    {
      portalBootOnNextBoot = true;
      uint32_t nid = prefs.getUInt("nodeId", ROOT_ID);
      portalStoredDeviceId = (uint8_t)(nid & 0xFF);
    }
    else if (ota_rollback_requires_portal_boot())
    {
      portalBootOnNextBoot = true;
      uint32_t nid = prefs.getUInt("nodeId", ROOT_ID);
      portalStoredDeviceId = (uint8_t)(nid & 0xFF);
      debugLog("boot: pending OTA verify, forcing portal");
    }
    prefs.end();
  }

  pinMode(LED_BLINK, OUTPUT);
  pinMode(LED_RXTX_STATUS, OUTPUT);
  pinMode(LED_MESH, OUTPUT);

  button_init();
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  if (portalBootOnNextBoot)
  {
    debugLog("boot: portal mode (post-OTA)");
    enterPortalMode(portalStoredDeviceId);
  }
  else
  {
    initMeshRoot();
    portal_init();
    rootInfo();
  }

  const unsigned long bootMs = millis();
  lastReceivedTime = bootMs;
  lastMeshReceivedTime = bootMs;

  esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
  esp_task_wdt_add(nullptr);
  debugLog("Watchdog timer set for %d seconds", WATCHDOG_TIMEOUT);
  debugLog("boot: setup done");
}

void loop()
{
  esp_task_wdt_reset();
  led_handler();
  button_task();
  portal_process_deferred_actions();

  if (isPortalActive())
  {
    portal_task();
  }
  else
  {
    mesh_handler_update();
    processUartInput();
    processPacketQueue();
    mesh_handler_update_rssi();

    static unsigned long lastNodePrint = 0;
    static unsigned long lastDiscoveryRetry = 0;
    static unsigned long lastDummyBroadcast = 0;
    const unsigned long now = millis();

    if (now - lastNodePrint >= 30000)
    {
      lastNodePrint = now;
      printConnectedNodes();
    }

    if (now - lastDiscoveryRetry >= SLAVE_DISCOVERY_INTERVAL)
    {
      lastDiscoveryRetry = now;
      retryMissingSlaves(now);
    }

    if (now - lastDummyBroadcast >= DUMMY_BROADCAST_INTERVAL)
    {
      lastDummyBroadcast = now;
      const String dummyMsg = "DUMMY_BROADCAST";
      mesh.sendBroadcast(dummyMsg);
      debugLog("Broadcasted dummy message to mesh: %s", dummyMsg.c_str());
    }

    if (now - lastReceivedTime > RESTART_TIMEOUT_MS)
      restart_guard_request_idle_restart("no UART for 15 min");
    else if (!slaveIdToNodeId.empty() &&
             now - lastMeshReceivedTime > RESTART_TIMEOUT_MS)
      restart_guard_request_idle_restart("no mesh RX for 15 min");

    if (portalRequested)
    {
      portalRequested = false;
      enterPortalMode(ROOT_ID);
    }
  }

  static unsigned long lastPrintTime = 0;
  const unsigned long printInterval = isPortalActive()
                                         ? DEBUG_PRINT_INTERVAL_PORTAL_MS
                                         : DEBUG_PRINT_INTERVAL_MESH_MS;
  const unsigned long now = millis();
  if (now - lastPrintTime >= printInterval)
  {
    const bool portalStatusReady = !isPortalActive() ||
                                   (now - portal_enteredAtMs() >= printInterval);
    if (portalStatusReady)
    {
      lastPrintTime = now;
      printDebugInfo();
    }
  }
}

static void initMeshRoot()
{
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  mesh.onReceive(handleMeshReceive);
  mesh_handler_init();
  debugLog("Mesh root initialized");
}

void handleMeshReceive(uint32_t from, String &msg)
{
  if (msg.startsWith("SLAVE_ID_RESPONSE:"))
  {
    const int slaveId = msg.substring(18).toInt();
    slaveIdToNodeId[slaveId] = from;
    lastSlaveSeen[slaveId] = millis();
    debugLog("Mapped slave_id %d to node %u", slaveId, from);
    return;
  }

  if (msg.isEmpty() || msg.length() > MAX_PACKET_SIZE)
    return;

  lastMeshReceivedTime = millis();

  if (packetQueue.size() >= MAX_QUEUE_SIZE)
    packetQueue.pop_front();

  uint8_t modbusPacket[MAX_PACKET_SIZE];
  const int modbusPacketSize = hexStringToByteArray(msg, modbusPacket);
  if (modbusPacketSize > 0)
  {
    const int slaveId = modbusPacket[6];
    lastSlaveSeen[slaveId] = millis();

    debugLog("Received message from node %u, slave id %d:", from, slaveId);
    printPacket(modbusPacket, modbusPacketSize);
    packetQueue.emplace_back(modbusPacket, modbusPacket + modbusPacketSize);
  }
}

void processUartInput()
{
  const unsigned long now = millis();
  while (Serial2.available())
  {
    const uint8_t incomingByte = Serial2.read();
    lastReceivedTime = now;
    if (rxBufferIndex < MAX_PACKET_SIZE - 1)
      rxBuffer[rxBufferIndex++] = incomingByte;
    else
    {
      debugLogPacket("UART overflow", rxBuffer, rxBufferIndex);
      rxBufferIndex = 0;
    }
  }

  if (now - lastReceivedTime > UART_TIMEOUT && rxBufferIndex > 0)
  {
    const int frameLen = rxBufferIndex;

    if (frameLen < 6)
    {
      debugLogPacket("UART ignored", rxBuffer, frameLen);
      rxBufferIndex = 0;
      return;
    }

    debugLogPacket("UART RX", rxBuffer, frameLen);

    if (isPortalTriggerForRoot(rxBuffer, rxBufferIndex))
    {
      debugLog("Portal trigger for root %u", (unsigned)ROOT_ID);
      portalRequested = true;
      portalRequestTime = now;
      rxBufferIndex = 0;
      ledRxtxOn = true;
      lastLedUpdateTime = now;
      return;
    }

    int mbapLen = 0;
    const uint8_t *mbap = getMbapPayload(rxBuffer, rxBufferIndex, &mbapLen);
    const bool hasRoutePrefix = (mbap != nullptr && mbap != rxBuffer);

    if (mbap != nullptr && isModbusRequestForRoot(mbap, mbapLen))
    {
      uint8_t modbusResponse[MAX_PACKET_SIZE];
      const int respLen = rootModbusHandler(mbap, mbapLen, modbusResponse, MAX_PACKET_SIZE);
      if (respLen > 0)
      {
        if (hasRoutePrefix)
          Serial2.write(rxBuffer, 6);
        Serial2.write(modbusResponse, respLen);
        debugLog("Root Modbus reply sent on UART");
        debugLogPacket("UART TX", modbusResponse, respLen);
        restart_guard_note_activity();
        ledRxtxOn = true;
        lastLedUpdateTime = now;
      }
      rxBufferIndex = 0;
      return;
    }

    if (mbap == nullptr)
    {
      debugLog("UART discarded: invalid MBAP (%d bytes)", frameLen);
      rxBufferIndex = 0;
      return;
    }

    /* Nodes expect [client IP:4][port:2][MBAP...]; forward full UART frame when prefixed */
    const int slaveId = hasRoutePrefix ? rxBuffer[12] : mbap[6];
    if (!(slaveId >= START_NODE && slaveId <= END_NODE))
    {
      debugLog("Out of range slave_id %d, ignoring packet.", slaveId);
      rxBufferIndex = 0;
      return;
    }

    const String hexMessage = hasRoutePrefix
                                  ? convertHexToString(rxBuffer, rxBufferIndex)
                                  : convertHexToString(const_cast<uint8_t *>(mbap), mbapLen);
    sendCommandToSlave(slaveId, hexMessage);
    restart_guard_note_activity();
    rxBufferIndex = 0;

    ledRxtxOn = true;
    lastLedUpdateTime = now;
  }
}

void processPacketQueue()
{
  static unsigned long lastSendTime = 0;
  constexpr unsigned long SEND_INTERVAL = 100;
  const unsigned long now = millis();

  if (now - lastSendTime >= SEND_INTERVAL && !packetQueue.empty())
  {
    lastSendTime = now;
    const auto &packet = packetQueue.front();
    for (uint8_t b : packet)
      Serial2.write(b);

    debugLog("Packet sent to master:");
    printPacket(packet.data(), static_cast<int>(packet.size()));
    packetQueue.pop_front();
  }
}

void sendCommandToSlave(int slaveId, const String &command)
{
  if (slaveIdToNodeId.find(slaveId) != slaveIdToNodeId.end())
  {
    mesh.sendSingle(slaveIdToNodeId[slaveId], command);
    debugLog("Sent command to slave_id %d (node %u)", slaveId, slaveIdToNodeId[slaveId]);
    debugLog("Send Request Packet to Node:%d with data: %s", slaveId, command.c_str());
  }
  else
  {
    debugLog("No node mapped for slave_id %d", slaveId);
  }
}

void retryMissingSlaves(unsigned long now)
{
  const bool missing = slaveIdToNodeId.find(currentSlave) == slaveIdToNodeId.end();
  const bool timedOut = !missing && (now - lastSlaveSeen[currentSlave] > SLAVE_TIMEOUT);
  if (missing || timedOut)
  {
    const String requestMsg = "SLAVE_ID_REQUEST:" + String(currentSlave);
    mesh.sendBroadcast(requestMsg);
    debugLog("Retrying discovery for slave_id %d", currentSlave);
  }
  currentSlave = (currentSlave == END_NODE) ? START_NODE : (currentSlave + 1);
  lastRetryMillis = now;
}
