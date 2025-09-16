#include "app_config.h"
#include "ir_handler.h"
#include <IRsend.h>

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
  Serial.println("AC On Function Called");
#if defined(MIDEA_03)
  irsend.sendRaw(ac_on_raw[0], irRawLength, 38);
  delay(1000);
  irsend.sendRaw(ac_on_raw[1], irRawLength, 38);
#else
  irsend.sendRaw(ac_on_raw, irRawLength, 38);

#endif
}

void ac_off()
{
  Serial.println("AC Off Function Called");
#if defined(MIDEA_03)
  irsend.sendRaw(ac_off_raw[0], irRawLength, 38);
  delay(1000);
  irsend.sendRaw(ac_off_raw[1], irRawLength, 38);
#else
  irsend.sendRaw(ac_off_raw, irRawLength, 38);
#endif
}

void ac_temperature_set(uint16_t temp)
{
  Serial.println("AC Temperature Set Function Called");
  if (temp >= 18 && temp <= 30)
  {
#if defined(MIDEA_03)
    irsend.sendRaw(temp_array[temp - 18][0], irRawLength, 38);
    delay(1000);
    irsend.sendRaw(temp_array[temp - 18][1], irRawLength, 38);
#else
    irsend.sendRaw(temp_array[temp - 18], irRawLength, 38);
#endif
  }
}