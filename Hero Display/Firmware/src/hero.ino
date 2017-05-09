/*
 * Format CAN data to display on TVs and process machine start / stop buttons
 */

#include "mf2017-can.h"
#include "socketcan.h"
#include <stdio.h>

// System threading is not stable yet on the Pi. It appears to deadlock,
// so use manual mode instead
//SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

/*
 * Pin config
 */

const auto MACHINE_START_PIN = D10;
const auto MACHINE_STOP_PIN = D11;

SocketCAN can("can0");
Communication comms(can);

void setup() {
  Serial.begin(9600);
  setupComms();
  setupButtons();
}

void loop() {
  receiveComms();
  processButtons();
  exportDataToJSON();
  transmitComms();
}

/*
 * Machine start and stop buttons
 */

const auto BUTTON_PRESSED = LOW;

void setupButtons() {
  pinMode(MACHINE_START_PIN, INPUT);
  pinMode(MACHINE_STOP_PIN, INPUT);
}

void processButtons() {
  comms.MachineStart = digitalRead(MACHINE_START_PIN) == BUTTON_PRESSED;
  comms.MachineStop = digitalRead(MACHINE_STOP_PIN) == BUTTON_PRESSED;
}

/*
 * CAN communication
 */

void setupComms() {
  comms.begin();
}

void receiveComms() {
  comms.receive();
}

void transmitComms() {
  comms.DisplayActive = true;
  comms.transmit(MachineModules::Supervisor);
  comms.transmit(MachineModules::Display);
}

/*
 * Make CAN data available to JSON every 100ms
 */

const auto dataFile = "/home/pi/website/data.json";
#define JSON_SIGNAL_INT(name) fprintf(file, "  \"%s\": %d,\n", #name, comms.name)
#define JSON_SIGNAL_FLOAT(name) fprintf(file, "  \"%s\": %f,\n", #name, comms.name)

long exportLastTime = 0;
void exportDataToJSON() {
  auto now = millis();
  if (now - exportLastTime < 100) {
    return;
  }
  exportLastTime = now;

  FILE *file = fopen(dataFile, "w");
  fprintf(file, "{\n");
  JSON_SIGNAL_INT(MachineStart);
  JSON_SIGNAL_INT(MachineStop);
  JSON_SIGNAL_INT(Input1Active);
  JSON_SIGNAL_INT(BallCount1);
  JSON_SIGNAL_INT(GreenButtonPressed);
  JSON_SIGNAL_INT(BlueButtonPressed);
  JSON_SIGNAL_INT(RedButtonPressed);
  JSON_SIGNAL_INT(Input2Active);
  JSON_SIGNAL_INT(BallCount2);
  JSON_SIGNAL_INT(InputColorHue);
  JSON_SIGNAL_INT(Input3Active);
  JSON_SIGNAL_INT(BallCount3);
  JSON_SIGNAL_FLOAT(InputCrankSpeed);
  JSON_SIGNAL_INT(Input4Active);
  JSON_SIGNAL_INT(BallCount4);
  JSON_SIGNAL_INT(HoverPositionLR);
  JSON_SIGNAL_INT(HoverPositionUD);
  JSON_SIGNAL_INT(LightsActive);
  // Shows if the firmware is running, and terminates the JSON properly
  // (trailing commas are not allowed in JSON...)
  fprintf(file, "  \"millis\": %d\n", millis());
  fprintf(file, "}");
  fclose(file);
}
