#pragma once

bool bridge_is_started();

void bridge_init();
void bridge_wifi_update();
void bridge_process_udp_serial();
void bridge_send_queue();
void bridge_check_idle_restart();
void bridge_stop();

unsigned long bridge_last_udp_activity();
unsigned long bridge_last_rxtx_activity();
bool bridge_led_udp_on();
bool bridge_led_rxtx_on();
void bridge_refresh_led_flags();
