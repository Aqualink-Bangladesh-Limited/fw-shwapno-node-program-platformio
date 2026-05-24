#pragma once

void restart_guard_init();
void restart_guard_clear();
void restart_guard_request(const char *reason);
bool restart_guard_is_halted();
