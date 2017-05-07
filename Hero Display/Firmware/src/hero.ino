/*
 * Format CAN data to display on TVs and process machine start / stop buttons
 */

#include "mf2017-can.h"
#include "socketcan.h"
#include <iostream>
#include <fstream>
using namespace std;

SYSTEM_THREAD(ENABLED);

/*
 * Pin config
 */

const auto MACHINE_START_PIN = D10;
const auto MACHINE_STOP_PIN = D11;

SocketCAN can("can0");
Communication comms(can);

void setup() {
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
#define JSON_SIGNAL(name) file << "\"" << #name << "\":" << comms.name << ",\n"

long exportLastTime = 0;
void exportDataToJSON() {
  auto now = millis();
  if (now - exportLastTime < 100) {
    return;
  }
  exportLastTime = now;

  ofstream file;
  file.open(dataFile);
  file << "{";
  JSON_SIGNAL(MachineStart);
  JSON_SIGNAL(MachineStop);
  JSON_SIGNAL(Input1Active);
  JSON_SIGNAL(GreenButtonPressed);
  JSON_SIGNAL(BlueButtonPressed);
  JSON_SIGNAL(RedButtonPressed);
  JSON_SIGNAL(Input2Active);
  JSON_SIGNAL(InputColorHue);
  JSON_SIGNAL(Input3Active);
  JSON_SIGNAL(InputCrankSpeed);
  JSON_SIGNAL(Input4Active);
  JSON_SIGNAL(LightsActive);
  file << "}";
  file.close();
}
