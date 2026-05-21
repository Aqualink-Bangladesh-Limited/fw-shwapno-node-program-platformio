#pragma once
#include <Arduino.h>
#include <Print.h>

typedef void (*DebugSendFn)(const char *line);

void debugLog(const char *fmt, ...);
void registerDebugSender(DebugSendFn fn);
void debugLogPrintTail(Print *out, size_t maxChars);
String debugLogGetBuffer();
size_t debugLogGetBufferLength();
void debugLogClearBuffer();
