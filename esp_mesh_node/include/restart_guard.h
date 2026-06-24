#pragma once

#include <stdint.h>

void restart_guard_init();
void restart_guard_clear();
void restart_guard_note_activity();
bool restart_guard_is_lockout();
uint8_t restart_guard_get_count();
void restart_guard_request_idle_restart(const char *reason);
