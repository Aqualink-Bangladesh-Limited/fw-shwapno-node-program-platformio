#include "debug_log.h"
#include "app_config.h"
#include <stdarg.h>
#include <string.h>

static DebugSendFn g_debugSender = nullptr;
static String g_logBuffer;
static const size_t LOG_BUFFER_LIMIT = DEBUG_LOG_BUFFER_BYTES;
static const size_t LOG_BUFFER_RESERVE = DEBUG_LOG_BUFFER_BYTES + 256;

static void ensureLogBufferCapacity()
{
  static bool reserved = false;
  if (reserved)
    return;
  g_logBuffer.reserve(LOG_BUFFER_RESERVE);
  reserved = true;
}

static void appendLogLine(const char *line)
{
  ensureLogBufferCapacity();

  if (g_logBuffer.length() > 0)
    g_logBuffer += '\n';

  g_logBuffer += line;

  while (g_logBuffer.length() > LOG_BUFFER_LIMIT)
  {
    int firstBreak = g_logBuffer.indexOf('\n');
    if (firstBreak < 0)
    {
      g_logBuffer.remove(0, g_logBuffer.length() / 2);
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

  if (g_debugSender)
  {
    g_debugSender(line);
  }
}

void debugLogPrintTail(Print *out, size_t maxChars)
{
  if (!out)
    return;
  const size_t len = g_logBuffer.length();
  if (len <= maxChars)
    out->print(g_logBuffer);
  else
    out->print(g_logBuffer.c_str() + (len - maxChars));
}

String debugLogGetBuffer()
{
  return g_logBuffer;
}

size_t debugLogGetBufferLength()
{
  return g_logBuffer.length();
}

void debugLogClearBuffer()
{
  g_logBuffer = "";
  ensureLogBufferCapacity();
}
