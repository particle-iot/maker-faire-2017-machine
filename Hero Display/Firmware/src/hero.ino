/*
 * Format CAN data to display on TVs and process machine start / stop buttons
 */

#include "mf2017-can.h"
#include "socketcan.h"
#include <stdio.h>

SYSTEM_THREAD(ENABLED);

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

  static long t = 0;
  if (millis() - t > 1000) {
    t = millis();
    Serial.printlnf("%f", comms.InputCrankSpeed);
  }
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
  Serial.println("setupComms start");
  comms.begin();
  Serial.println("setupComms end");
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
float junk;
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
  JSON_SIGNAL_INT(GreenButtonPressed);
  JSON_SIGNAL_INT(BlueButtonPressed);
  JSON_SIGNAL_INT(RedButtonPressed);
  JSON_SIGNAL_INT(Input2Active);
  JSON_SIGNAL_INT(InputColorHue);
  JSON_SIGNAL_INT(Input3Active);
  JSON_SIGNAL_FLOAT(InputCrankSpeed);
  JSON_SIGNAL_INT(Input4Active);
  JSON_SIGNAL_INT(LightsActive);
  fprintf(file, "}");
  fclose(file);
}