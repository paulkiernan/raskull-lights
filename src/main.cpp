#include <Arduino.h>
#include "TeensyThreads.h"

#include <RHMesh.h>
#include <RH_Serial.h>
#include "ArduinoLog.h"

#define HC12Serial Serial2

#ifdef PRIMARY
    const char CONTROLLER_ID = 0;
    const uint8_t TARGET_CONTROLLER_ID = 1;
    const uint8_t LED_DELAY = 100;
#else
    const char CONTROLLER_ID = 1;
    const uint8_t TARGET_CONTROLLER_ID = 0;
    const uint16_t LED_DELAY = 1000;
#endif

const uint8_t RF_CONTROL_PIN = 0;

// Mesh Network
RH_Serial g_rf_driver(HC12Serial);
RHMesh manager(g_rf_driver, CONTROLLER_ID);
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];   // Dont put this on the stack:

int iteration_count = 0;


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

    // Setup logging
    Serial.begin(9600);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.notice("***Begin Session***");

    // Setup threads
    pinMode(LED_BUILTIN, OUTPUT);
    threads.addThread(blinkthread);

    // Setup RF module
    pinMode(RF_CONTROL_PIN, OUTPUT);
    digitalWrite(RF_CONTROL_PIN, HIGH);
    delay(80);               // HC-12 documented startup time
    HC12Serial.begin(9600);  // Serial port to HC12

    if (!manager.init()) {
        Log.fatal("[FATAL] RF_Serial init failed" CR);
    } else {
        Log.verbose("[INFO] RF_Serial init for controller id complete!" CR);
    }
}


void loop() {
    iteration_count++;
    Log.verbose("[%l][%d] Iteration count: %d" CR, millis(), CONTROLLER_ID, iteration_count);
    delay(10);

#ifdef PRIMARY
    // Send a message to a rf22_mesh_server
    // A route to the destination will be automatically discovered.
    char data[50];
    sprintf(data, "And hello back to you from Mesh Server: %d", CONTROLLER_ID);
    if (manager.sendtoWait(data, sizeof(data), TARGET_CONTROLLER_ID) == RH_ROUTER_ERROR_NONE) {
        // It has been reliably delivered to the next node.
        // Now wait for a reply from the ultimate server
        uint8_t len = sizeof(buf);
        uint8_t from;
        if (manager.recvfromAckTimeout(buf, &len, 3000, &from)) {
            Log.verbose("got REPLY from : %X: %s" CR, from, (char*)buf);
        } else {
            Log.verbose("No reply, is the Mesh Server running?" CR);
        }
      }
    else {
        Log.verbose("sendtoWait failed. Are the intermediate mesh servers running?" CR);
    }
#else
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from)){
        Log.verbose("got REQUEST from : %X: %s" CR, from, (char*)buf);
        // Send a reply back to the originator client
        char data[50];
        sprintf(data, "Hello from Mesh Client: %d", CONTROLLER_ID);
        if (manager.sendtoWait(data, sizeof(data), from) != RH_ROUTER_ERROR_NONE) {
            Log.verbose("sendtoWait failed" CR);
        }
    }
#endif

}
