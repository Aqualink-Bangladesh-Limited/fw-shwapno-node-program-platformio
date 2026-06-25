#include "debug_print.h"
#include "debug_log.h"
#include "app_config.h"
#include "portal_handler.h"
#include "portal_web.h"
#include "mesh_handler.h"
#include "restart_guard.h"
#include "led_handler.h"
#include <Arduino.h>
#include <painlessMesh.h>
#include <WiFi.h>
#include <deque>
#include <map>
#include <list>
#include <vector>

extern painlessMesh mesh;
extern std::map<int, uint32_t> slaveIdToNodeId;
extern std::deque<std::vector<uint8_t>> packetQueue;
extern bool portalRequested;
extern bool portalBootOnNextBoot;
extern unsigned long lastReceivedTime;
extern int currentSlave;

static const char *boardVersionString()
{
#if defined(BOARD_VERSION_03)
  return "V03";
#elif defined(BOARD_VERSION_04)
  return "V04";
#else
  return "NONE";
#endif
}

static unsigned long secondsSince(unsigned long eventMs)
{
  return (millis() - eventMs) / 1000UL;
}

static const char *portalStateShort()
{
  if (isPortalActive())
    return "on";
  if (portalRequested)
    return "pending";
  return "off";
}

static uint16_t meshRssiRegisterValue()
{
  if (mesh_rssi != 0)
    return static_cast<uint16_t>(mesh_rssi * -1);
  return 0;
}

static int slaveIdForMeshNode(uint32_t meshNodeId)
{
  for (const auto &pair : slaveIdToNodeId)
  {
    if (pair.second == meshNodeId)
      return pair.first;
  }
  return -1;
}

static bool isMeshNodeOnline(uint32_t meshNodeId)
{
  if (!mesh_handler_is_started())
    return false;

  for (uint32_t node : mesh.getNodeList())
  {
    if (node == meshNodeId)
      return true;
  }
  return false;
}

static void logDeviceIdentity()
{
  debugLog("Boot: mesh root %s %s", boardVersionString(), FIRMWARE_VERSION);
  debugLog("Root ID %u, slaves %d-%d", (unsigned)ROOT_ID, START_NODE, END_NODE);
}

static void logNodeMap()
{
  if (slaveIdToNodeId.empty())
  {
    debugLog("Node map: none");
    return;
  }

  for (const auto &pair : slaveIdToNodeId)
  {
    const bool online = isMeshNodeOnline(pair.second);
    debugLog("Node map: nodeId %d -> meshId %u (%s)",
             pair.first, pair.second, online ? "online" : "offline");
  }
}

static void logUnpairedPeers()
{
  if (!mesh_handler_is_started())
    return;

  const std::list<uint32_t> nodeList = mesh.getNodeList();
  for (uint32_t meshNodeId : nodeList)
  {
    if (slaveIdForMeshNode(meshNodeId) < 0)
      debugLog("Unpaired peer: meshId %u", meshNodeId);
  }
}

static void formatPortalSsid(char *buf, size_t len)
{
  snprintf(buf, len, "OTA-ROOT-%u", (unsigned)ROOT_ID);
}

static void logPortalApDetails(const char *portalSsid)
{
  debugLog("Portal: on");
  debugLog("AP SSID %s, password %s", portalSsid, PORTAL_PASSWORD);
  debugLog("Open http://%d.%d.%d.%d/ for OTA", PORTAL_AP_IP_1, PORTAL_AP_IP_2,
           PORTAL_AP_IP_3, PORTAL_AP_IP_4);
  debugLog("Exit in web UI or %lu min idle (paused during OTA)",
           (unsigned long)(PORTAL_TIMEOUT_MS / 60000UL));
}

bool debugShouldLogPacketDetails()
{
  return !isPortalActive();
}

void printPacket(uint8_t *packet, int packetSize)
{
  printPacket(static_cast<const uint8_t *>(packet), packetSize);
}

static void appendPacketHex(String &line, const uint8_t *packet, int packetSize)
{
  for (int i = 0; i < packetSize; i++)
  {
    char hexBuf[4];
    snprintf(hexBuf, sizeof(hexBuf), "%02X", packet[i]);
    line += hexBuf;
    if (i < packetSize - 1)
      line += ' ';
  }
}

void printPacket(const uint8_t *packet, int packetSize)
{
  if (!debugShouldLogPacketDetails() || !packet || packetSize <= 0)
    return;

  String line;
  line.reserve(static_cast<size_t>(packetSize) * 3 + 8);
  appendPacketHex(line, packet, packetSize);
  debugLogText(line);
}

void printPacket(const std::vector<uint8_t> &packet)
{
  printPacket(packet.data(), static_cast<int>(packet.size()));
}

void debugLogPacket(const char *prefix, const uint8_t *packet, int packetSize)
{
  if (!debugShouldLogPacketDetails() || !prefix || !packet || packetSize <= 0)
    return;

  String line(prefix);
  line += " (";
  line.reserve(line.length() + (static_cast<size_t>(packetSize) * 3) + 8);
  appendPacketHex(line, packet, packetSize);
  line += ')';

  debugLogText(line);
}

void debugLogPacket(const char *prefix, const std::vector<uint8_t> &packet)
{
  debugLogPacket(prefix, packet.data(), static_cast<int>(packet.size()));
}

void printMeshInfo()
{
  debugLog("Mesh: %s / %s, port %u, ch %u",
           MESH_PREFIX, MESH_PASSWORD, (unsigned)MESH_PORT, (unsigned)MESH_CHANNEL);
}

static void appendMeshTreeIndent(String &out, unsigned depth)
{
  for (unsigned i = 0; i < depth; ++i)
    out += "  ";
}

static void appendMeshTreeJson(String &out, const painlessmesh::protocol::NodeTree &tree,
                               unsigned depth)
{
  appendMeshTreeIndent(out, depth);
  out += "{\n";

  appendMeshTreeIndent(out, depth + 1);
  out += "\"meshId\": ";
  out += tree.nodeId;

  if (tree.root)
  {
    out += ",\n";
    appendMeshTreeIndent(out, depth + 1);
    out += "\"rootId\": ";
    out += static_cast<unsigned>(ROOT_ID);
    out += ",\n";
    appendMeshTreeIndent(out, depth + 1);
    out += "\"root\": true";
  }
  else
  {
    const int routingNodeId = slaveIdForMeshNode(tree.nodeId);
    if (routingNodeId >= 0)
    {
      out += ",\n";
      appendMeshTreeIndent(out, depth + 1);
      out += "\"nodeId\": ";
      out += routingNodeId;
    }

    out += ",\n";
    appendMeshTreeIndent(out, depth + 1);
    out += "\"online\": ";
    out += isMeshNodeOnline(tree.nodeId) ? "true" : "false";
  }

  if (!tree.subs.empty())
  {
    out += ",\n";
    appendMeshTreeIndent(out, depth + 1);
    out += "\"subs\": [\n";

    size_t idx = 0;
    const size_t count = tree.subs.size();
    for (const auto &sub : tree.subs)
    {
      appendMeshTreeJson(out, sub, depth + 2);
      if (++idx < count)
        out += ",\n";
      else
        out += '\n';
    }

    appendMeshTreeIndent(out, depth + 1);
    out += ']';
  }

  out += '\n';
  appendMeshTreeIndent(out, depth);
  out += '}';
}

void printMeshLayout()
{
  if (!mesh_handler_is_started())
  {
    debugLogText("{}");
    return;
  }

  String json;
  json.reserve(512);
  appendMeshTreeJson(json, mesh.asNodeTree(), 0);
  debugLogText(json);
}

void printConnectedNodes()
{
  printMeshLayout();
}

void rootInfo()
{
  logDeviceIdentity();
  printMeshInfo();
  debugLog("Portal: off (UART FC 0x41 to root %u, or GPIO%d 5s)",
           (unsigned)ROOT_ID, PORTAL_BUTTON_PIN);
}

void portalInfo()
{
  char portalSsid[32];
  formatPortalSsid(portalSsid, sizeof(portalSsid));

  debugLog("Portal opened (mesh was running)");
  logDeviceIdentity();
  printMeshInfo();
  logNodeMap();
  logPortalApDetails(portalSsid);
}

static void logPeriodicSummary()
{
  debugLog("Status: %s %s | uptime %lus | heap %u",
           boardVersionString(), FIRMWARE_VERSION,
           (unsigned long)(millis() / 1000UL), (unsigned)ESP.getFreeHeap());
}

static void printMeshPeriodicStatus()
{
  const unsigned long uartIdleSec = secondsSince(lastReceivedTime);
  const unsigned long meshIdleSec = secondsSince(lastMeshReceivedTime);

  debugLog("Mode: mesh, portal %s", portalStateShort());
  debugLog("Root ID %u, slaves %d-%d", (unsigned)ROOT_ID, START_NODE, END_NODE);

  if (mesh_handler_is_started())
    debugLog("Mesh node %u | %s", mesh.getNodeId(), MESH_PREFIX);
  else
    debugLog("Mesh stack: stopped");

  debugLog("RSSI %ld dBm, reg 0x%04X=%u | UART idle %lus | mesh RX %lus",
           mesh_rssi, (unsigned)ROOT_REG_MESH_RSSI, (unsigned)meshRssiRegisterValue(),
           uartIdleSec, meshIdleSec);

  logNodeMap();
  logUnpairedPeers();

  debugLog("Idle restart %u/%u%s",
           (unsigned)restart_guard_get_count(),
           (unsigned)MAX_CONSECUTIVE_IDLE_RESTARTS,
           restart_guard_is_lockout() ? " LOCKOUT" : "");

  if (packetQueue.size() > 0)
    debugLog("Reply queue: %u", (unsigned)packetQueue.size());
}

static void printPortalPeriodicStatus()
{
  char portalSsid[32];
  formatPortalSsid(portalSsid, sizeof(portalSsid));

  debugLog("Mode: portal | AP %s | clients %u | OTA %s",
           portalSsid, (unsigned)WiFi.softAPgetStationNum(),
           portalWeb_isOtaInProgress() ? "yes" : "no");
}

void printDebugInfo()
{
  if (isPortalActive())
  {
    logPeriodicSummary();
    printPortalPeriodicStatus();
    return;
  }

  logPeriodicSummary();
  printMeshPeriodicStatus();
}
