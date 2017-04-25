/*
 * Station for sending CAN messages to other stations
 */

#include "mf2017-can.h"

SYSTEM_THREAD(ENABLED);

BuiltinCAN can(CAN_D1_D2);
Communication comms(can);

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
  comms.Input3Active = digitalRead(SWITCH1_PIN);
  comms.Input2Active = digitalRead(SWITCH2_PIN);
  comms.receive();
  comms.transmit(MachineModules::Panel1);
  comms.transmit(MachineModules::Panel2);
  comms.transmit(MachineModules::Panel3);
}
