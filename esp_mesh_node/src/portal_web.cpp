#include "portal_web.h"
#include "app_config.h"
#include "portal_handler.h"
#include "debug_log.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_task_wdt.h>

static AsyncWebServer *server = nullptr;
static AsyncWebSocket *ws = nullptr;
static bool otaInProgress = false;
static size_t otaTotalBytes = 0;
static size_t otaWrittenBytes = 0;
static String otaMessage = "Idle";

static void notifyAllClients(const char *line)
{
  if (!ws)
    return;
  ws->textAll(line);
}

static String escapeJson(const String &value)
{
  String escaped;
  escaped.reserve(value.length() + 16);

  for (size_t i = 0; i < value.length(); i++)
  {
    char c = value.charAt(i);
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
  String json = "{";
  json += "\"portalActive\":";
  json += isPortalActive() ? "true" : "false";
  json += ",\"otaInProgress\":";
  json += otaInProgress ? "true" : "false";
  json += ",\"otaTotalBytes\":";
  json += String((unsigned long)otaTotalBytes);
  json += ",\"otaWrittenBytes\":";
  json += String((unsigned long)otaWrittenBytes);
  json += ",\"otaPercent\":";
  if (otaTotalBytes > 0)
  {
    json += String((unsigned long)((otaWrittenBytes * 100UL) / otaTotalBytes));
  }
  else
  {
    json += "0";
  }
  json += ",\"otaMessage\":\"";
  json += escapeJson(otaMessage);
  json += "\",\"logCountHint\":";
  String logs = debugLogGetBuffer();
  json += String((unsigned long)logs.length());
  json += "}";
  return json;
}

static String buildPortalPage()
{
  return R"rawliteral(
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
        <p>Clean status view for firmware upload, live logs, and progress tracking. The dashboard refreshes itself every 5 seconds while keeping a live WebSocket stream open.</p>
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
              <button class='btn secondary' onclick='location.href="/exit"'>Exit portal</button>
            </div>
            <div class='muted'>Firmware upload is streamed directly to flash. The page keeps refreshing log state every 5 seconds so you can leave it open during long updates.</div>
          </div>
        </div>
      </section>

      <section class='card'>
        <div class='hd'><h2>Live Logs</h2><span id='logSummary'>0 chars</span></div>
        <div class='logbox' id='logBox'></div>
        <div class='footer'>Recent logs are cached on-device and pushed live over WebSocket. Use <strong>Clear logs</strong> to reset the view.</div>
      </section>
    </div>
  </div>

  <script>
    let ws=null;
    let refreshTimer=null;
    let autoScroll=true;
    const logBox=document.getElementById('logBox');

    function setText(id,value){document.getElementById(id).textContent=value;}
    function appendLog(line){const row=document.createElement('div');row.className='logline';row.textContent=line;logBox.appendChild(row);while(logBox.children.length>600){logBox.removeChild(logBox.firstElementChild);}if(autoScroll){logBox.scrollTop=logBox.scrollHeight;}}
    function renderLogs(text){logBox.innerHTML='';if(!text){appendLog('No logs yet.');return;}text.split(/\n+/).forEach(line=>{if(line.trim().length)appendLog(line);});}
    function updateState(state){setText('portalState',state.portalActive?'Active':'Inactive');setText('otaPercent',(state.otaPercent||0)+'%');setText('otaBytes',(state.otaWrittenBytes||0)+' / '+(state.otaTotalBytes||0));setText('otaMessage',state.otaMessage||'Idle');setText('statusText',state.otaInProgress?'OTA in progress':'Ready');document.getElementById('progressBar').style.width=(state.otaPercent||0)+'%';document.getElementById('progressBar').style.opacity=state.otaInProgress?1:.75;document.getElementById('uploadBtn').disabled=!!state.otaInProgress;document.getElementById('logSummary').textContent=(state.logCountHint||0)+' chars cached';}
    async function refreshDashboard(forceLogs){try{const status=await fetch('/status',{cache:'no-store'}).then(r=>r.json());updateState(status);if(forceLogs||!window._logLoaded){const logs=await fetch('/logs',{cache:'no-store'}).then(r=>r.text());renderLogs(logs);window._logLoaded=true;}}catch(e){setText('connText','Offline');}}
    async function clearLogs(){await fetch('/logs/clear',{method:'POST'});window._logLoaded=false;refreshDashboard(true);}
    function uploadFirmware(){const file=document.getElementById('file').files[0];if(!file){alert('Pick a firmware file first.');return;}const xhr=new XMLHttpRequest();xhr.open('POST','/update',true);xhr.upload.onprogress=function(e){if(e.lengthComputable){const pct=Math.round((e.loaded/e.total)*100);setText('otaPercent',pct+'%');setText('otaBytes',e.loaded+' / '+e.total);document.getElementById('progressBar').style.width=pct+'%';setText('otaMessage','Uploading firmware... '+pct+'%');setText('statusText','Uploading');}};xhr.onreadystatechange=function(){if(xhr.readyState===4){setText('otaMessage',xhr.responseText||'Upload finished.');refreshDashboard(true);}};xhr.send(file);setText('statusText','Uploading');setText('connText','Uploading...');}
    function connectWs(){try{ws&&ws.close();ws=new WebSocket((location.protocol==='https:'?'wss://':'ws://')+location.host+'/debug');ws.onopen=function(){setText('connText','Live connected');};ws.onclose=function(){setText('connText','Reconnecting...');setTimeout(connectWs,1500);};ws.onerror=function(){setText('connText','Reconnect pending');};ws.onmessage=function(e){appendLog(e.data);};}catch(e){setText('connText','Live unavailable');}}
    logBox.addEventListener('scroll',function(){autoScroll=logBox.scrollTop+logBox.clientHeight>=logBox.scrollHeight-20;});
    connectWs();
    refreshDashboard(true);
    refreshTimer=setInterval(function(){refreshDashboard(false);},5000);
  </script>
</body>
</html>
)rawliteral";
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
  otaTotalBytes = 0;
  otaWrittenBytes = 0;
  otaMessage = "Idle";

  server = new AsyncWebServer(80);
  ws = new AsyncWebSocket("/debug");

  ws->onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT)
    {
      portal_touchActivity();
      debugLog("WebSocket client connected: %u", client->id());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
      debugLog("WebSocket client disconnected: %u", client->id());
    }
  });

  server->addHandler(ws);

  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    request->send(200, "text/html", buildPortalPage());
  });

  server->on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    request->send(200, "application/json", buildStatusJson());
  });

  server->on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    request->send(200, "text/plain", debugLogGetBuffer());
  });

  server->on("/logs/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    debugLogClearBuffer();
    otaMessage = "Logs cleared";
    request->send(200, "text/plain", "Logs cleared");
  });

  server->on("/ping", HTTP_GET, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    request->send(200, "text/plain", "ok");
  });

  server->on("/exit", HTTP_GET, [](AsyncWebServerRequest *request) {
    portal_touchActivity();
    request->send(200, "text/plain", "Rejoining mesh...");
    // small delay then exit
    delay(200);
    exitPortalMode();
  });

  // Raw binary upload handler
  server->on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Response is sent from the body callback after Update.end()
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0)
    {
      portal_touchActivity();
      // start update
      otaInProgress = true;
      otaTotalBytes = total;
      otaWrittenBytes = 0;
      otaMessage = "Starting OTA...";
      debugLog("OTA start, size=%u", (unsigned)total);
      if (!Update.begin(total))
      {
        debugLog("Update.begin failed");
        otaMessage = "Update.begin failed";
        otaInProgress = false;
        request->send(500, "text/plain", "Update begin failed");
        return;
      }
    }

    // write chunk
    if (otaInProgress)
    {
      portal_touchActivity();
      // feed watchdog during long uploads
      esp_task_wdt_reset();
      size_t written = Update.write(data, len);
      if (written != len)
      {
        debugLog("Update write failed");
        otaMessage = "Update write failed";
        otaInProgress = false;
        request->send(500, "text/plain", "Update write failed");
        return;
      }
      otaWrittenBytes += written;
      otaMessage = String("Uploading firmware... ") + String((otaTotalBytes > 0) ? (unsigned long)((otaWrittenBytes * 100UL) / otaTotalBytes) : 0) + "%";
    }

    if (index + len == total)
    {
      // finalize
      if (Update.end(true))
      {
        debugLog("OTA complete, written=%u", (unsigned)Update.size());
        otaMessage = "OTA complete. Rebooting...";
        otaInProgress = false;
        // set NVS flag so next boot returns to portal mode for verification
        Preferences prefs;
        if (prefs.begin("portal", false))
        {
          prefs.putBool("portalBoot", true);
          prefs.putUInt("nodeId", NODE_ID);
          prefs.end();
        }
        request->send(200, "text/plain", "Flash complete, rebooting...");
        delay(200);
        // ESP.restart(); // debug: keep disabled while tracing reboot loop
      }
      else
      {
        debugLog("Update end failed");
        otaMessage = "Update end failed";
        otaInProgress = false;
        request->send(500, "text/plain", "Update failed at end");
      }
    }
  });

  server->begin();

  // register debug sender
  registerDebugSender(notifyAllClients);

  debugLog("Portal web server started");
}

void portalWeb_stop()
{
  if (!server)
    return;

  server->end();
  delete server;
  server = nullptr;

  if (ws)
  {
    delete ws;
    ws = nullptr;
  }

  registerDebugSender(nullptr);
  debugLog("Portal web server stopped");
}
