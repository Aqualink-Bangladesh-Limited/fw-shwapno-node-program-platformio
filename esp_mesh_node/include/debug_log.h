#pragma once
#include <Arduino.h>

typedef void (*DebugSendFn)(const char *line);

void debugLog(const char *fmt, ...);
void registerDebugSender(DebugSendFn fn);
String debugLogGetBuffer();
void debugLogClearBuffer();
