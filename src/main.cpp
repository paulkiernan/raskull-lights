#include <Arduino.h>
#include "TeensyThreads.h"

#include <RHMesh.h>
#include <RH_Serial.h>

#define HC12Serial Serial2
#define RH_MESH_MAX_MESSAGE_LEN 50

#ifdef PRIMARY
    const uint8_t CONTROLLER_ID = 0;
    const uint8_t LED_DELAY = 100;
    uint8_t data[] = "And hello back to you from server0";
#else
    const uint8_t CONTROLLER_ID = 1;
    const uint16_t LED_DELAY = 1000;
    uint8_t data[] = "And hello back to you from server1";
#endif

const uint8_t RF_CONTROL_PIN = 0;

// Mesh Network
RH_Serial g_rf_driver(HC12Serial);
RHMesh manager(g_rf_driver, CONTROLLER_ID);
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];   // Dont put this on the stack:


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

    if (!manager.init()) {
        Serial.println("[FATAL] RF_Serial init failed");
    }
}


void loop() {

    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from)){
        Serial.print("got request from : 0x");
        Serial.print(from, HEX);
        Serial.print(": ");
        Serial.println((char*)buf);
        // Send a reply back to the originator client
        if (manager.sendtoWait(data, sizeof(data), from) != RH_ROUTER_ERROR_NONE)
            Serial.println("sendtoWait failed");
    }
}
