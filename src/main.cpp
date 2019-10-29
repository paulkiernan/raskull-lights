#include <Arduino.h>
#include "TeensyThreads.h"

#define HC12Serial Serial2

#ifdef PRIMARY
    const uint8_t CONTROLLER_ID = 0;
    const uint8_t LED_DELAY = 100;
#else
    const uint8_t CONTROLLER_ID = 1;
    const uint16_t LED_DELAY = 1000;
#endif

const uint8_t RF_CONTROL_PIN = 0;


void blinkthread() {
  /* Liveliness indicator
   * Scheduled thread for blinking the onboard LED at a regular
   * rate to inducate microcontroller liveliness
   */
  while (1) {
    digitalWriteFast(LED_BUILTIN, HIGH);
    threads.delay(LED_DELAY);
    digitalWriteFast(LED_BUILTIN, LOW);
    threads.delay(LED_DELAY);
    threads.yield();
  }
}

void setup() {
  delay(1000);  // Wait a second

  // Setup threads
  pinMode(LED_BUILTIN, OUTPUT);
  threads.addThread(blinkthread);

  // Setup RF module
  pinMode(RF_CONTROL_PIN, OUTPUT);
  digitalWrite(RF_CONTROL_PIN, HIGH);
  delay(80);               // HC-12 documented startup time
  HC12Serial.begin(9600);  // Serial port to HC12

  // Setup debug serial to computer
  Serial.begin(9600);      // Serial port to computer

}

void loop() {
}
