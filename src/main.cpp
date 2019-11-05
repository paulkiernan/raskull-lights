#include <Arduino.h>
#include "TeensyThreads.h"

#include <RHMesh.h>
#include <RH_Serial.h>
#include "ArduinoLog.h"
#include "freeram.h"

#define HC12Serial Serial1

#ifdef PRIMARY
    const uint8_t LED_DELAY = 100;
    const uint8_t nodeId = 1;
#else
    const uint16_t LED_DELAY = 1000;
    const uint8_t nodeId = 2;
#endif

#define RH_HAVE_SERIAL
#define N_NODES 2
int iteration_count = 0;
const uint8_t RF_CONTROL_PIN = 2;

// Mesh Network
RH_Serial g_rf_driver(HC12Serial);
RHMesh *manager;
char buf[RH_MESH_MAX_MESSAGE_LEN];   // Dont put this on the stack:


uint8_t routes[N_NODES]; // full routing table for mesh
int16_t rssi[N_NODES];   // signal strength info


const __FlashStringHelper* getErrorString(uint8_t error) {
  switch(error) {
    case 1: return F("invalid length");
    break;
    case 2: return F("no route");
    break;
    case 3: return F("timeout");
    break;
    case 4: return F("no reply");
    break;
    case 5: return F("unable to deliver");
    break;
  }
  return F("unknown");
}


void updateRoutingTable() {
  for(uint8_t n=1;n<=N_NODES;n++) {
    RHRouter::RoutingTableEntry *route = manager->getRouteTo(n);
    if (n == nodeId) {
      routes[n-1] = 255; // self
    } else {
      routes[n-1] = route->next_hop;
      if (routes[n-1] == 0) {
        // if we have no route to the node, reset the received signal strength
        rssi[n-1] = 0;
      }
    }
  }
}


// Create a JSON string with the routing info to each node
void getRouteInfoString(char *p, size_t len) {
  p[0] = '\0';
  strcat(p, "[");
  for(uint8_t n=1;n<=N_NODES;n++) {
    strcat(p, "{\"n\":");
    sprintf(p+strlen(p), "%d", routes[n-1]);
    strcat(p, ",");
    strcat(p, "\"r\":");
    sprintf(p+strlen(p), "%d", rssi[n-1]);
    strcat(p, "}");
    if (n<N_NODES) {
      strcat(p, ",");
    }
  }
  strcat(p, "]");
}


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
    randomSeed(analogRead(0));
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


    //nodeId = EEPROM.read(0);
    if (nodeId > 10) {
        Log.notice("EEPROM nodeId invalid: %d" CR, nodeId);
    }
    Log.notice("initializing node: %d" CR, nodeId);

    manager = new RHMesh(g_rf_driver, nodeId);
    delay(100);
    if (!manager->init()) {
        Log.fatal("[FATAL] RF_Serial init failed" CR);
    }
    Log.verbose("[INFO] RF_Serial init for controller id complete!" CR);

    for(uint8_t n=1;n<=N_NODES;n++) {
        routes[n-1] = 0;
        rssi[n-1] = 0;
    }

    Log.verbose("Free Memory= %d" CR, freeRam());
}


void loop() {
    iteration_count++;
    Log.verbose(
        "[%l][%d] Iteration count: %d" CR,
        millis(),
        nodeId,
        iteration_count
    );

    for(uint8_t n=1;n<=N_NODES;n++) {
        if (n == nodeId) continue; // self

        updateRoutingTable();
        getRouteInfoString(buf, RH_MESH_MAX_MESSAGE_LEN);

        Log.verbose("Sending Message %d->%d : %s" CR, nodeId, n, buf);

        // send an acknowledged message to the target node
        uint8_t error = manager->sendtoWait((uint8_t *)buf, strlen(buf), n);
        if (error != RH_ROUTER_ERROR_NONE) {
            Log.error("Encountered Error: %S" CR, getErrorString(error));
        } else {
            Log.verbose("OK" CR);
            // we received an acknowledgement from the next hop for the node we
            // tried to send to.
            RHRouter::RoutingTableEntry *route = manager->getRouteTo(n);
            if (route->next_hop != 0) {
                rssi[route->next_hop-1] = g_rf_driver.lastRssi();
            }
        }

        // listen for incoming messages. Wait a random amount of time before we transmit
        // again to the next node
        unsigned long nextTransmit = millis() + random(3000, 5000);
        while (nextTransmit > millis()) {
            int waitTime = nextTransmit - millis();
            uint8_t len = sizeof(buf);
            uint8_t from;
            if (manager->recvfromAckTimeout((uint8_t *)buf, &len, waitTime, &from)) {
                buf[len] = '\0'; // null terminate string
                Log.verbose("Got message: %d-> %s" CR, from, buf);
                // we received data from node 'from', but it may have actually come from an intermediate node
                RHRouter::RoutingTableEntry *route = manager->getRouteTo(from);
                if (route->next_hop != 0) {
                    rssi[route->next_hop-1] = g_rf_driver.lastRssi();
                }
            }
        }
    }

}
