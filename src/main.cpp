#include <Arduino.h>
#include "TeensyThreads.h"

#define HC12Serial Serial2

volatile int blinkcode = 0;
int count = 0;

int SET_PIN = 0;

int incomingByte = 0; // for incoming serial data
int outgoingByte = 0; // for incoming serial data


void blinkthread() {
  /* Liveliness indicator
   * Scheduled thread for blinking the onboard LED at a regular
   * rate to inducate microcontroller liveliness
   */
  while (1) {
    digitalWriteFast(LED_BUILTIN, HIGH);
    threads.delay(150);
    digitalWriteFast(LED_BUILTIN, LOW);
    threads.delay(150);
    threads.yield();
  }
}

void setup() {
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  threads.addThread(blinkthread);
  pinMode(SET_PIN, OUTPUT);

  Serial.begin(9600);      // Serial port to computer

  digitalWrite(SET_PIN, HIGH);
  delay(80);
  HC12Serial.begin(9600);  // Serial port to HC12

  //HC12Serial.write("

}

void loop() {
  if (HC12Serial.available()) {       // If HC-12 has data
    incomingByte = Serial.read();
      Serial.print("(debug) Receiving Byte: ");
      Serial.println(incomingByte);
      Serial.write(HC12Serial.read());  // Send the data to Serial monitor
    }
  if (Serial.available()) {           // If Serial monitor has data
  outgoingByte = Serial.read();
    Serial.print("(debug) Transmitting Byte: ");
    Serial.println(outgoingByte);
    HC12Serial.write(outgoingByte);      // Send that data to HC-12
  }
}
