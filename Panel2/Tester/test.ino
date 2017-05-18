#include "mf2017-can.h"

SYSTEM_THREAD(ENABLED);

BuiltinCAN can(CAN_D1_D2);
Communication comms(can);

void setup() {
  Serial.begin();
  comms.begin();
}

void loop() {
  comms.receive();

  Serial.printlnf("rx=%d active=%d hue=%d count=%d",
    comms.Panel2StatusLastRx,
    comms.Input2Active,
    comms.InputColorHue,
    comms.BallCount2
  );
}
