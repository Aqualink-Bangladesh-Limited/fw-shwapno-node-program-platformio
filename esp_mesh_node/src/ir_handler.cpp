#include "app_config.h"
#include "ir_handler.h"
#include <IRsend.h>
#include "debug_log.h"

IRsend irsend(IR_PIN);

void ac_on();
void ac_off();
void ac_temperature_set(uint16_t temp);

void ir_init()
{
  irsend.begin();
}

void ir_handler()
{
  // state = 0 : No packet received
  // state = 1 : On packet received
  // state = 2 : Only Temperature Signal will Received
  // state = 3 : Off packet received

  uint16_t ac_state = arr[1];
  if (ac_state == 1)
  {
    ac_on();
  }
  else if (ac_state == 2)
  {
    ac_temperature_set(arr[0]);
  }
  else if (ac_state == 3)
  {
    ac_off();
  }
}

void ac_on()
{
  debugLog("AC On Function Called");

#if defined(SAMSUNG_01)
  irsend.sendRaw(ac_on_raw, irOnRawLength, 38);
#else
  irsend.sendRaw(ac_on_raw, irRawLength, 38);
#endif
}

void ac_off()
{
  debugLog("AC Off Function Called");

#if defined(SAMSUNG_01)
  irsend.sendRaw(ac_off_raw, irOffRawLength, 38);
#else
  irsend.sendRaw(ac_off_raw, irRawLength, 38);
#endif
}

void ac_temperature_set(uint16_t temp)
{
  debugLog("AC Temperature Set Function Called");
  if (temp >= 18 && temp <= 30)
  {
    irsend.sendRaw(temp_array[temp - 18], irRawLength, 38);
  }
}