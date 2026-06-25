#include "portal_web.h"
#include "app_config.h"
#include "portal_handler.h"
#include "debug_log.h"
#include "ota_rollback.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <cstring>

static AsyncWebServer *server = nullptr;
static AsyncWebSocket *ws = nullptr;
static bool otaInProgress = false;
static bool otaFailed = false;
static size_t otaTotalBytes = 0;
static size_t otaWrittenBytes = 0;
static char otaMessage[64] = "Idle";

bool portalWeb_isOtaInProgress()
{
  return otaInProgress;
}

static void setOtaMessage(const char *msg)
{
  strncpy(otaMessage, msg, sizeof(otaMessage) - 1);
  otaMessage[sizeof(otaMessage) - 1] = '\0';
}

static void pushLogToWeb(const char *line)
{
  if (!line || otaInProgress || !ws || ws->count() == 0)
    return;
  ws->textAll(line);
}

static String escapeJson(const char *value)
{
  String escaped;
  if (!value)
    return escaped;
  escaped.reserve(strlen(value) + 16);

  for (size_t i = 0; value[i] != '\0'; i++)
  {
    char c = value[i];
    switch (c)
    {
    case '\\':
      escaped += "\\\\";
      break;
    case '"':
      escaped += "\\\"";
      break;
    case '\n':
      escaped += "\\n";
      break;
    case '\r':
      break;
    case '\t':
      escaped += "\\t";
      break;
    default:
      escaped += c;
      break;
    }
  }

  return escaped;
}

static String buildStatusJson()
{
  const unsigned pct = (otaTotalBytes > 0) ? (unsigned)((otaWrittenBytes * 100UL) / otaTotalBytes) : 0;
  const String msgEsc = escapeJson(otaMessage);
  const bool verifyHold = ota_rollback_is_verify_hold_active();
  const unsigned verifyLeft = (unsigned)ota_rollback_verify_hold_seconds_left();
  char buf[400];
  snprintf(buf, sizeof(buf),
           "{\"portalActive\":%s,\"otaInProgress\":%s,\"otaTotalBytes\":%u,"
           "\"otaWrittenBytes\":%u,\"otaPercent\":%u,\"otaMessage\":\"%s\","
           "\"logCountHint\":%u,\"otaVerifyHold\":%s,\"otaVerifySecondsLeft\":%u}",
           isPortalActive() ? "true" : "false",
           otaInProgress ? "true" : "false",
           (unsigned)otaTotalBytes,
           (unsigned)otaWrittenBytes,
           pct,
           msgEsc.c_str(),
           (unsigned)debugLogGetBufferLength(),
           verifyHold ? "true" : "false",
           verifyLeft);
  return String(buf);
}

static void sendVerifyHoldBlocked(AsyncWebServerRequest *request, const char *action)
{
  char body[120];
  snprintf(body, sizeof(body),
           "OTA verification in progress (%lus left). %s blocked until firmware is confirmed.",
           ota_rollback_verify_hold_seconds_left(), action);
  request->send(403, "text/plain", body);
}

static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='en'>
<head>
  <meta charset='utf-8'/>
  <meta name='viewport' content='width=device-width,initial-scale=1,viewport-fit=cover'/>
  <title>OTA Portal</title>
  <style>
    :root{color-scheme:dark;--bg:#07111f;--panel:#0d1727;--panel2:#111f33;--line:#20324c;--text:#e8eef7;--muted:#8da2bf;--accent:#5eead4;--accent2:#60a5fa;--warn:#f59e0b;--danger:#fb7185;--ok:#34d399;--shadow:0 20px 60px rgba(0,0,0,.35);}*{box-sizing:border-box}body{margin:0;font-family:Inter,Segoe UI,system-ui,sans-serif;background:radial-gradient(circle at top left,#12304f 0,#07111f 36%,#050b14 100%);color:var(--text);min-height:100vh}body:before{content:'';position:fixed;inset:0;background-image:linear-gradient(rgba(255,255,255,.025) 1px,transparent 1px),linear-gradient(90deg,rgba(255,255,255,.025) 1px,transparent 1px);background-size:36px 36px;mask-image:linear-gradient(180deg,rgba(0,0,0,.9),rgba(0,0,0,.2));pointer-events:none}.wrap{max-width:1200px;margin:0 auto;padding:24px}.hero{display:flex;gap:16px;align-items:flex-end;justify-content:space-between;flex-wrap:wrap;padding:24px 0 20px}.title h1{margin:0;font-size:clamp(30px,4vw,48px);line-height:1.03;letter-spacing:-.04em}.title p{margin:8px 0 0;color:var(--muted);max-width:760px}.badge{display:inline-flex;align-items:center;gap:8px;padding:10px 14px;border:1px solid rgba(255,255,255,.08);border-radius:999px;background:rgba(255,255,255,.03);backdrop-filter:blur(10px)}.badge i{width:8px;height:8px;border-radius:50%;background:var(--ok);box-shadow:0 0 0 6px rgba(52,211,153,.12)}.grid{display:grid;grid-template-columns:1.1fr .9fr;gap:16px}.card{background:linear-gradient(180deg,rgba(17,31,51,.92),rgba(10,18,31,.92));border:1px solid rgba(255,255,255,.08);border-radius:22px;box-shadow:var(--shadow);overflow:hidden}.card .hd{padding:18px 20px;border-bottom:1px solid rgba(255,255,255,.06);display:flex;justify-content:space-between;gap:12px;align-items:center}.card .hd h2{margin:0;font-size:16px;letter-spacing:.02em;text-transform:uppercase;color:#dce7f5}.card .hd span{color:var(--muted);font-size:13px}.card .bd{padding:20px}.stack{display:grid;gap:14px}.row{display:flex;gap:12px;flex-wrap:wrap}.stat{flex:1;min-width:160px;padding:14px 16px;border-radius:18px;background:rgba(255,255,255,.035);border:1px solid rgba(255,255,255,.06)}.stat .k{font-size:12px;color:var(--muted);text-transform:uppercase;letter-spacing:.08em}.stat .v{margin-top:6px;font-size:20px;font-weight:700}.bar{height:12px;border-radius:999px;background:#12233b;overflow:hidden;border:1px solid rgba(255,255,255,.06)}.bar > div{height:100%;width:0%;background:linear-gradient(90deg,var(--accent2),var(--accent));transition:width .25s ease}.controls{display:flex;gap:10px;flex-wrap:wrap}.btn{appearance:none;border:0;border-radius:14px;padding:12px 16px;font-weight:700;cursor:pointer;color:#06111a;background:linear-gradient(180deg,#d8fdf8,#8cf2e2);box-shadow:0 12px 28px rgba(94,234,212,.18)}.btn.secondary{background:rgba(255,255,255,.05);color:var(--text);border:1px solid rgba(255,255,255,.08);box-shadow:none}.btn.danger{background:linear-gradient(180deg,#fecdd3,#fb7185);color:#2b0a10}.btn:disabled{opacity:.55;cursor:not-allowed}.input{width:100%;padding:14px;border-radius:16px;background:#06101b;color:var(--text);border:1px solid rgba(255,255,255,.08)}.logbox{height:520px;overflow:auto;padding:16px;background:#040b14;border-top:1px solid rgba(255,255,255,.06);font-family:'SFMono-Regular',Consolas,monospace;font-size:12px;line-height:1.55;white-space:pre-wrap;word-break:break-word}.logline{padding:2px 0;border-bottom:1px solid rgba(255,255,255,.03)}.footer{padding:16px 20px;color:var(--muted);font-size:12px;border-top:1px solid rgba(255,255,255,.06)}.pill{padding:6px 10px;border-radius:999px;background:rgba(255,255,255,.06);font-size:12px;color:#d9e7f6}.pill.ok{background:rgba(52,211,153,.14);color:#99f6e4}.pill.warn{background:rgba(245,158,11,.16);color:#fde68a}.pill.danger{background:rgba(251,113,133,.16);color:#fecdd3}.muted{color:var(--muted)}@media (max-width: 960px){.grid{grid-template-columns:1fr}.logbox{height:360px}}
  </style>
</head>
<body>
  <div class='wrap'>
    <div class='hero'>
      <div class='title'>
        <h1>OTA Portal</h1>
        <p>Firmware upload and live device logs (WebSocket + 2s polling).</p>
      </div>
      <div class='badge'><i></i><span id='connText'>Connecting...</span></div>
    </div>

    <div class='grid'>
      <section class='card'>
        <div class='hd'><h2>Update Console</h2><span id='statusText'>Idle</span></div>
        <div class='bd'>
          <div class='stack'>
            <div class='row'>
              <div class='stat'><div class='k'>Portal</div><div class='v' id='portalState'>Unknown</div></div>
              <div class='stat'><div class='k'>OTA progress</div><div class='v' id='otaPercent'>0%</div></div>
              <div class='stat'><div class='k'>Transferred</div><div class='v' id='otaBytes'>0 / 0</div></div>
            </div>
            <div>
              <div class='bar'><div id='progressBar'></div></div>
              <div class='muted' style='margin-top:8px' id='otaMessage'>Waiting for action.</div>
            </div>
            <input id='file' class='input' type='file' accept='.bin,application/octet-stream'/>
            <div class='controls'>
              <button class='btn' id='uploadBtn' onclick='uploadFirmware()'>Upload firmware</button>
              <button class='btn secondary' onclick='refreshDashboard(true)'>Refresh now</button>
              <button class='btn danger' onclick='clearLogs()'>Clear logs</button>
              <button class='btn secondary' id='restartPortalBtn' onclick='restartPortal()'>Restart portal</button>
              <button class='btn secondary' id='exitPortalBtn' onclick='exitPortal()'>Exit portal</button>
            </div>
            <div class='muted'>Firmware upload is streamed directly to flash. The page keeps refreshing log state every 5 seconds so you can leave it open during long updates.</div>
          </div>
        </div>
      </section>

      <section class='card'>
        <div class='hd'><h2>Live Logs</h2><div class='row' style='align-items:center;gap:8px;margin:0'><span id='logSummary'>0 chars</span><button class='btn secondary' id='copyLogsBtn' onclick='copyLogs()' style='padding:8px 12px;font-size:12px'>Copy logs</button></div></div>
        <div class='logbox' id='logBox'></div>
        <div class='footer'>Live logs via WebSocket; full history refreshed every 2 seconds. Use <strong>Copy logs</strong> to share or <strong>Clear logs</strong> to reset.</div>
      </section>
    </div>
  </div>

  <script>
    let ws=null;
    let refreshTimer=null;
    let autoScroll=true;
    const logBox=document.getElementById('logBox');

    function setText(id,value){document.getElementById(id).textContent=value;}
    function appendLog(line){const row=document.createElement('div');row.className='logline';row.textContent=line;logBox.appendChild(row);while(logBox.children.length>400){logBox.removeChild(logBox.firstElementChild);}if(autoScroll){logBox.scrollTop=logBox.scrollHeight;}}
    function renderLogs(text){logBox.innerHTML='';if(!text){appendLog('No logs yet.');return;}text.split(/\n+/).forEach(line=>{if(line.trim().length)appendLog(line);});}
    function updateState(state){const verifyHold=!!state.otaVerifyHold;setText('portalState',verifyHold?('Verifying OTA ('+(state.otaVerifySecondsLeft||0)+'s)'):(state.portalActive?'Active':'Inactive'));setText('otaPercent',(state.otaPercent||0)+'%');setText('otaBytes',(state.otaWrittenBytes||0)+' / '+(state.otaTotalBytes||0));setText('otaMessage',verifyHold?('Confirming new firmware ('+(state.otaVerifySecondsLeft||0)+'s)...'):(state.otaMessage||'Idle'));setText('statusText',state.otaInProgress?'OTA in progress':(verifyHold?'Verifying firmware':'Ready'));document.getElementById('progressBar').style.width=(state.otaPercent||0)+'%';document.getElementById('progressBar').style.opacity=state.otaInProgress?1:.75;document.getElementById('uploadBtn').disabled=!!state.otaInProgress;const exitBtn=document.getElementById('exitPortalBtn');const restartBtn=document.getElementById('restartPortalBtn');if(exitBtn)exitBtn.disabled=verifyHold||!!state.otaInProgress;if(restartBtn)restartBtn.disabled=verifyHold||!!state.otaInProgress;document.getElementById('logSummary').textContent=(state.logCountHint||0)+' chars cached';}
    async function refreshDashboard(forceLogs){try{const status=await fetch('/status',{cache:'no-store'}).then(r=>r.json());updateState(status);if(forceLogs||!window._logLoaded){const logs=await fetch('/logs',{cache:'no-store'}).then(r=>r.text());renderLogs(logs);window._logLoaded=true;}}catch(e){setText('connText','Offline');}}
    async function clearLogs(){await fetch('/logs/clear',{method:'POST'});window._logLoaded=false;refreshDashboard(true);}
    function copyTextFallback(text){const ta=document.createElement('textarea');ta.value=text;ta.style.position='fixed';ta.style.left='-9999px';document.body.appendChild(ta);ta.focus();ta.select();try{document.execCommand('copy');return true;}catch(e){return false;}finally{document.body.removeChild(ta);}}
    async function copyLogs(){const btn=document.getElementById('copyLogsBtn');const prev=btn.textContent;btn.disabled=true;btn.textContent='Copying...';try{const logs=await fetch('/logs',{cache:'no-store'}).then(r=>r.text());if(!logs||!logs.trim()){alert('No logs to copy yet.');btn.textContent=prev;btn.disabled=false;return;}let ok=false;if(navigator.clipboard&&window.isSecureContext){await navigator.clipboard.writeText(logs);ok=true;}else{ok=copyTextFallback(logs);}btn.textContent=ok?'Copied!':'Copy failed';setTimeout(function(){btn.textContent=prev;btn.disabled=false;},1500);if(!ok)alert('Copy failed. Select logs manually from the box.');}catch(e){btn.textContent=prev;btn.disabled=false;alert('Copy failed.');}}
    async function restartPortal(){try{const r=await fetch('/restart');alert(await r.text());}catch(e){alert('Restart failed');}}
    async function exitPortal(){try{const r=await fetch('/exit');alert(await r.text());}catch(e){alert('Exit failed');}}
    function connectWs(){if(ws&&(ws.readyState===WebSocket.OPEN||ws.readyState===WebSocket.CONNECTING))return;try{ws=new WebSocket((location.protocol==='https:'?'wss://':'ws://')+location.host+'/debug');ws.onopen=function(){setText('connText','Live logs');};ws.onclose=function(){setText('connText','Polling only');ws=null;};ws.onerror=function(){setText('connText','Polling only');};ws.onmessage=function(e){appendLog(e.data);};}catch(e){setText('connText','Polling only');}}
    function uploadFirmware(){const file=document.getElementById('file').files[0];if(!file){alert('Pick a firmware file first.');return;}if(refreshTimer){clearInterval(refreshTimer);refreshTimer=null;}const fd=new FormData();fd.append('update',file,file.name);const xhr=new XMLHttpRequest();xhr.open('POST','/update',true);xhr.timeout=600000;xhr.upload.onprogress=function(e){if(e.lengthComputable){const pct=Math.round((e.loaded/e.total)*100);setText('otaPercent',pct+'%');setText('otaBytes',e.loaded+' / '+e.total);document.getElementById('progressBar').style.width=pct+'%';setText('otaMessage','Uploading firmware... '+pct+'%');setText('statusText','Uploading');setText('connText','Uploading...');}};xhr.onerror=function(){setText('otaMessage','Upload failed (network error)');setText('connText','Upload failed');refreshTimer=setInterval(function(){refreshDashboard(true);},2000);connectWs();};xhr.onreadystatechange=function(){if(xhr.readyState===4){setText('otaMessage',xhr.responseText||'Upload finished.');refreshDashboard(true);if(!refreshTimer){refreshTimer=setInterval(function(){refreshDashboard(true);},2000);}connectWs();}};xhr.send(fd);setText('statusText','Uploading');}
    logBox.addEventListener('scroll',function(){autoScroll=logBox.scrollTop+logBox.clientHeight>=logBox.scrollHeight-20;});
    connectWs();
    refreshDashboard(true);
    refreshTimer=setInterval(function(){refreshDashboard(true);},2000);
  </script>
</body>
</html>
)rawliteral";

static const char PORTAL_HOME_URL[] = "http://192.168.4.1/";

static void attachCaptivePortalRoutes(AsyncWebServer *srv)
{
  auto redirectHome = [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    request->redirect(PORTAL_HOME_URL);
  };

  const char *captivePaths[] = {
      "/generate_204",
      "/gen_204",
      "/hotspot-detect.html",
      "/library/test/success.html",
      "/connecttest.txt",
      "/ncsi.txt",
      "/redirect",
      "/wpad.dat",
      "/fwlink",
      "/success.txt",
      "/canonical.html",
  };

  for (const char *path : captivePaths)
  {
    srv->on(path, HTTP_GET, redirectHome);
    srv->on(path, HTTP_HEAD, redirectHome);
  }

  srv->onNotFound([](AsyncWebServerRequest *request) {
    portal_touchActivity();
    if (request->method() == HTTP_OPTIONS)
    {
      request->send(200);
      return;
    }
    if (request->method() == HTTP_GET || request->method() == HTTP_HEAD)
      request->redirect(PORTAL_HOME_URL);
    else
      request->send(404, "text/plain", "Not found");
  });
}

void portalWeb_init()
{
  // placeholder
}

void portalWeb_start()
{
  if (server)
    return;

  otaInProgress = false;
  otaFailed = false;
  otaTotalBytes = 0;
  otaWrittenBytes = 0;
  setOtaMessage("Idle");

  server = new AsyncWebServer(80);
  ws = new AsyncWebSocket("/debug");
  ws->onEvent([](AsyncWebSocket * /*server*/, AsyncWebSocketClient *client, AwsEventType type,
                 void * /*arg*/, uint8_t * /*data*/, size_t /*len*/) {
    if (type == WS_EVT_CONNECT)
    {
      portal_touchActivity();
      const String logs = debugLogGetBuffer();
      if (logs.length() > 0)
        client->text(logs);
      debugLog("WebSocket client connected: %u", client->id());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
      portal_touchActivity();
      debugLog("WebSocket client disconnected: %u", client->id());
    }
    else if (type == WS_EVT_DATA || type == WS_EVT_PONG)
    {
      portal_touchActivity();
    }
  });
  server->addHandler(ws);

  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    request->send(request->beginResponse(200, "text/html", PORTAL_HTML));
  });

  server->on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    if (otaInProgress)
    {
      request->send(200, "application/json",
                    "{\"portalActive\":true,\"otaInProgress\":true,\"otaMessage\":\"Upload in progress\"}");
      return;
    }
    request->send(200, "application/json", buildStatusJson());
  });

  server->on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    if (otaInProgress)
    {
      request->send(200, "text/plain", "OTA upload in progress...\n");
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("text/plain");
    debugLogPrintTail(response, DEBUG_LOG_BUFFER_BYTES);
    request->send(response);
  });

  server->on("/logs/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    debugLogClearBuffer();
    setOtaMessage("Logs cleared");
    request->send(200, "text/plain", "Logs cleared");
  });

  server->on("/ping", HTTP_GET, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    request->send(200, "text/plain", "ok");
  });

  server->on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    if (ota_rollback_is_verify_hold_active())
    {
      debugLog("portal: restart blocked, OTA verify %lus remaining",
               ota_rollback_verify_hold_seconds_left());
      sendVerifyHoldBlocked(request, "Restart");
      return;
    }
    Preferences prefs;
    if (prefs.begin("portal", false))
    {
      prefs.putBool("portalBoot", true);
      prefs.putUInt("nodeId", NODE_ID);
      prefs.end();
    }
    request->send(200, "text/plain", "Restarting into portal mode...");
    portal_schedule_ota_reboot();
  });

  server->on("/exit", HTTP_GET, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    if (ota_rollback_is_verify_hold_active())
    {
      debugLog("portal: exit blocked, OTA verify %lus remaining",
               ota_rollback_verify_hold_seconds_left());
      sendVerifyHoldBlocked(request, "Exit");
      return;
    }
    request->send(200, "text/plain", "Rejoining mesh. Restarting...");
    portal_schedule_exit();
  });

  /* Multipart OTA upload (field name must be "update") - ESPAsyncWebServer OTA pattern */
  server->on(
      "/update", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        if (otaFailed || Update.hasError())
        {
          otaInProgress = false;
          otaFailed = false;
          setOtaMessage("Update failed");
          debugLog("OTA failed");
          request->send(500, "text/plain", "Update failed");
          return;
        }

        if (otaWrittenBytes == 0)
        {
          setOtaMessage("Update failed");
          debugLog("OTA failed: no data written");
          request->send(400, "text/plain", "No firmware data received");
          return;
        }

        otaInProgress = false;
        debugLog("OTA complete, written=%u", (unsigned)otaWrittenBytes);
        setOtaMessage("OTA complete. Rebooting...");
        ota_rollback_arm_verify_hold();
        Preferences prefs;
        if (prefs.begin("portal", false))
        {
          prefs.putBool("portalBoot", true);
          prefs.putUInt("nodeId", NODE_ID);
          prefs.end();
        }
        request->send(200, "text/plain", "Flash complete, rebooting...");
        portal_schedule_ota_reboot();
      },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        if (otaFailed)
          return;

        if (index == 0)
        {
          otaInProgress = true;
          portal_touchActivity();
          debugLog("OTA upload active - idle timeout paused");
          otaWrittenBytes = 0;
          otaTotalBytes = 0;
          otaFailed = false;
          setOtaMessage("Starting OTA...");
          debugLog("OTA start: %s", filename.c_str());
          const size_t maxSize = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
          if (!Update.begin(maxSize))
          {
            otaFailed = true;
            otaInProgress = false;
            setOtaMessage("Update.begin failed");
            debugLog("Update.begin failed");
            Update.printError(Serial);
            return;
          }
        }

        esp_task_wdt_reset();
        yield();

        if (Update.write(data, len) != len)
        {
          otaFailed = true;
          otaInProgress = false;
          setOtaMessage("Update write failed");
          debugLog("Update write failed");
          Update.printError(Serial);
          return;
        }

        otaWrittenBytes += len;
        portal_touchActivity();

        if (otaTotalBytes == 0 && request->contentLength() > 0)
          otaTotalBytes = request->contentLength();

        if (otaTotalBytes > 0 && (otaWrittenBytes % 65536) < (size_t)len)
        {
          snprintf(otaMessage, sizeof(otaMessage), "Uploading... %u%%",
                   (unsigned)((otaWrittenBytes * 100UL) / otaTotalBytes));
        }

        if (final)
        {
          otaTotalBytes = index + len;
          if (Update.end(true))
          {
            snprintf(otaMessage, sizeof(otaMessage), "Finishing OTA (%u bytes)", (unsigned)(index + len));
            debugLog("OTA flash end OK, %u bytes", (unsigned)(index + len));
          }
          else
          {
            otaFailed = true;
            otaInProgress = false;
            setOtaMessage("Update.end failed");
            debugLog("Update.end failed");
            Update.printError(Serial);
          }
        }
      });

  attachCaptivePortalRoutes(server);

  server->begin();

  registerDebugSender(pushLogToWeb);

  debugLog("Portal web server started (free heap %u)", (unsigned)ESP.getFreeHeap());
}

void portalWeb_stop()
{
  registerDebugSender(nullptr);

  if (ws)
  {
    ws->closeAll();
  }

  if (server)
  {
    server->end();
    delete server;
    server = nullptr;
  }

  if (ws)
  {
    delete ws;
    ws = nullptr;
  }

  Serial.println("[portal] web server stopped");
}
