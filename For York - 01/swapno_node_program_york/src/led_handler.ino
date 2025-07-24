
#include "led_handler.h"

#define MAX_RESTART 10

// Define the default blink intervals
unsigned long meshblinkInterval = 500; // Default mesh blink interval (500ms)
unsigned long last_mesh_led_blink = 0; // Stores the last time LED was toggled
bool meshledState = 0;                 // Mesh LED state
int acLedState = 0;
int ledState = 0;
unsigned long last_led_blink = 0;   // Last time LED blink was triggered
unsigned long blinkInterval = 5000; // Blink interval for AC LED (5 seconds)
int restart_count = 0;

// Function to handle LED states
void led_handler()
{
  // Get current time
  unsigned long currentMillis = millis();

  // Handle AC LED based on the value of acState
  uint16_t acState = arr[1]; // Assuming `arr[1]` holds the AC state
  handleACLED(acState);

  // Handle mesh LED blinking interval based on the last received time
  updateMeshBlinkInterval(currentMillis);

  // Blink mesh LED based on calculated interval
  if (currentMillis - last_mesh_led_blink >= meshblinkInterval)
  {
    last_mesh_led_blink = currentMillis;
    meshledState = !meshledState;         // Toggle LED state
    digitalWrite(LED_MESH, meshledState); // Update mesh LED state
  }

  if (currentMillis - last_led_blink >= blinkInterval)
  {
    if (ledState)
    {
      blinkInterval = 5000;
    }
    else
    {
      blinkInterval = 1000;
    }

    last_led_blink = currentMillis;
    ledState = !ledState;              // Toggle AC LED state
    digitalWrite(LED_BLINK, ledState); // Update AC LED state
  }
}

// Handle AC LED state based on acState value
void handleACLED(uint16_t acState)
{
  switch (acState)
  {
  case 0:
    digitalWrite(LED_AC_STATUS, LOW); // Turn off AC LED
    break;
  case 1:
  case 2:
    digitalWrite(LED_AC_STATUS, HIGH); // Turn on AC LED
    break;
  case 3:
    blinkLED(); // Blink AC LED when acState is 3
    break;
  default:
    digitalWrite(LED_AC_STATUS, LOW); // Default case: turn off AC LED
    break;
  }
}

// Blink AC LED at a fixed rate when acState is 3
void blinkLED()
{
  unsigned long currentMillis = millis();
  static unsigned long last_blink = 0;
  // Toggle AC LED every interval milliseconds (e.g., 500ms)
  if (currentMillis - last_blink >= 500)
  {
    last_blink = currentMillis;
    acLedState = !acLedState;                // Toggle LED state
    digitalWrite(LED_AC_STATUS, acLedState); // Update AC LED state
  }
}

// Update mesh LED blink interval based on the last received time
void updateMeshBlinkInterval(unsigned long currentMillis)
{
  // Check if the last received time is within a 5-minute window
  if (currentMillis - lastMeshReceivedTime <= MESH_INTERVAL)
  {
    meshblinkInterval = 2000; // Blink mesh LED every 2 seconds if within 5 minutes

    restart_count = 0;
  }
  else
  {
    meshblinkInterval = 500; // Blink mesh LED every 500 milliseconds if more than 5 minutes have passed

    restart_count++;
    if (restart_count > MAX_RESTART)
    {
      Serial.println("ESP Restarting.....");
      ESP.restart();
    }
  }
}
