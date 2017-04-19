/*
 * Station for sending CAN messages to other stations
 */

#include "mf2017-can.h"

SYSTEM_THREAD(ENABLED);

Communication comms;

// Switches
#define SWITCH1_PIN B2
#define SWITCH2_PIN B3

void setup() {
  pinMode(SWITCH1_PIN, INPUT_PULLDOWN);
  pinMode(SWITCH2_PIN, INPUT_PULLDOWN);
  Serial.begin();
  comms.begin();
}

void loop() {
  comms.ballEntering[0] = digitalRead(SWITCH1_PIN);
  comms.ballEntering[1] = digitalRead(SWITCH2_PIN);
  comms.receive();
  comms.transmit(MachineModules::Station1);
  comms.transmit(MachineModules::Station2);
}
