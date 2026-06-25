#pragma once

#include <stdint.h>

void ota_rollback_early_check();
void ota_rollback_portal_task();
bool ota_rollback_is_verify_hold_active();
unsigned long ota_rollback_verify_hold_seconds_left();
bool ota_rollback_requires_portal_boot();
void ota_rollback_arm_verify_hold();
void ota_rollback_clear_portal_flags();
