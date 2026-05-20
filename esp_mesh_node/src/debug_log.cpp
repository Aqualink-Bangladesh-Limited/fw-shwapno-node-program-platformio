#include "debug_log.h"
#include <stdarg.h>
#include <string.h>

static DebugSendFn g_debugSender = nullptr;
static String g_logBuffer;
static const size_t LOG_BUFFER_LIMIT = 8192;

static void appendLogLine(const char *line)
{
  if (g_logBuffer.length() > 0)
  {
    g_logBuffer += '\n';
  }

  g_logBuffer += line;

  while (g_logBuffer.length() > LOG_BUFFER_LIMIT)
  {
    int firstBreak = g_logBuffer.indexOf('\n');
    if (firstBreak < 0)
    {
      g_logBuffer = g_logBuffer.substring(g_logBuffer.length() / 2);
      break;
    }

    g_logBuffer.remove(0, firstBreak + 1);
  }
}

void registerDebugSender(DebugSendFn fn)
{
  g_debugSender = fn;
}

void debugLog(const char *fmt, ...)
{
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  char line[320];
  snprintf(line, sizeof(line), "[%lu] %s", (unsigned long)millis(), buf);

  // Always print to Serial
  Serial.println(line);
  appendLogLine(line);

  // Forward to registered sender (e.g., websocket)
  if (g_debugSender)
  {
    g_debugSender(line);
  }
}

String debugLogGetBuffer()
{
  return g_logBuffer;
}

void debugLogClearBuffer()
{
  g_logBuffer = "";
}
