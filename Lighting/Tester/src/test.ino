/*
 * Station for sending CAN messages to other stations
 */

#include "mf2017-can.h"

SYSTEM_THREAD(ENABLED);

BuiltinCAN can(CAN_D1_D2);
Communication comms(can);

enum ButtonColor {
  BUTTON_RED,
  BUTTON_GREEN,
  BUTTON_BLUE
};
enum LimitSwitch {
  LIMIT_LEFT,
  LIMIT_RIGHT
};
void updateButton(ButtonColor button);
void updateLimit(LimitSwitch limit);

void setup() {
  Serial.begin();
  comms.begin();
}

void loop() {
  clearInput();
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case '\r': case '\n': printHelp(); break;

      case ',': updateButton(BUTTON_RED); break;
      case '.': updateButton(BUTTON_GREEN); break;
      case '/': updateButton(BUTTON_BLUE); break;

      case 'q': updateHue(0); break;
      case 'w': updateHue(1); break;
      case 'e': updateHue(2); break;
      case 'r': updateHue(3); break;
      case 't': updateHue(4); break;
      case 'y': updateHue(5); break;
      case 'u': updateHue(6); break;
      case 'i': updateHue(7); break;
      case 'o': updateHue(8); break;
      case 'p': updateHue(9); break;

      case '0': updateCrankSpeed(0); break;
      case '1': updateCrankSpeed(1); break;
      case '2': updateCrankSpeed(2); break;
      case '3': updateCrankSpeed(3); break;
      case '4': updateCrankSpeed(4); break;
      case '5': updateCrankSpeed(5); break;
      case '6': updateCrankSpeed(6); break;
      case '7': updateCrankSpeed(7); break;
      case '8': updateCrankSpeed(8); break;
      case '9': updateCrankSpeed(9); break;

      case 'a': updateHandLR(0); break;
      case 's': updateHandLR(1); break;
      case 'd': updateHandLR(2); break;
      case 'f': updateHandLR(3); break;
      case 'g': updateHandLR(4); break;
      case 'h': updateHandLR(5); break;
      case 'j': updateHandLR(6); break;
      case 'k': updateHandLR(7); break;
      case 'l': updateHandLR(8); break;

      case 'z': updateHandUD(0); break;
      case 'x': updateHandUD(1); break;
      case 'c': updateHandUD(2); break;
      case 'v': updateHandUD(3); break;
      case 'b': updateHandUD(4); break;
      case 'n': updateHandUD(5); break;
      case 'm': updateHandUD(6); break;

      case ';': updateLimit(LIMIT_LEFT); break;
      case '\'': updateLimit(LIMIT_RIGHT); break;

    }
  }

  comms.receive();
  displayStartStop();
  comms.transmit(MachineModules::Panel1);
  comms.transmit(MachineModules::Panel2);
  comms.transmit(MachineModules::Panel3);
  comms.transmit(MachineModules::Panel4);

  delay(100);
}

void printHelp() {
  Serial.print(
    "Panel lights tester\r\n"
    "Type one of these characters to simulate interaction with a panel\r\n"
    "\r\n"
    "Panel 1: , for red, . for green, / for blue\r\n"
    "Panel 2: q to p for hue\r\n"
    "Panel 3: 0 to 9 for input crank speed\r\n"
    "Panel 4: a to l for hand left/right, z to m for hand up/down,\r\n"
    "         ; and ' for limit switches\r\n"
  );
}

void displayStartStop() {
  static bool start = false;
  static bool stop = false;
  if (comms.MachineStart && !start) {
    Serial.println("START!");
  }
  if (comms.MachineStop && !stop) {
    Serial.println("STOP!");
  }
  start = comms.MachineStart;
  stop = comms.MachineStop;
}

void clearInput() {
  comms.Input1Active = false;
  comms.RedButtonPressed = false;
  comms.GreenButtonPressed = false;
  comms.BlueButtonPressed = false;

  comms.Input2Active = false;

  // comms.Input3Active is deactivated with the 0 key

  // comms.Input4Active is deactivated with the z key
}

void updateButton(ButtonColor button) {
  comms.Input1Active = true;
  switch (button) {
    case BUTTON_RED:
      comms.RedButtonPressed = true;
      comms.RedBallCount++;
      break;
    case BUTTON_GREEN:
      comms.GreenButtonPressed = true;
      comms.GreenBallCount++;
      break;
    case BUTTON_BLUE:
      comms.BlueButtonPressed = true;
      comms.BlueBallCount++;
      break;
  }
}

void updateHue(unsigned h) {
  comms.Input2Active = true;
  comms.InputColorHue = 25 * h;
  comms.BallCount2++;
}

void updateCrankSpeed(unsigned s) {
  comms.Input3Active = (s > 0);
  comms.InputCrankSpeed = s / 3.0;
  comms.IntegrationCountA++;
  comms.IntegrationCountB++;
  comms.IntegrationCountC++;
}

void updateHandLR(int pos) {
  comms.HoverPositionLR = (uint8_t)(pos * 255 / 8.0);
  comms.PrizeCountA++;
  comms.PrizeCountB++;
  comms.PrizeCountC++;
}

void updateHandUD(int pos) {
  comms.Input4Active = (pos > 0);
  comms.HoverPositionUD = (uint8_t)(pos * 255 / 8.0);
}

void updateLimit(LimitSwitch limit) {
  switch (limit) {
    case LIMIT_LEFT:
      comms.BallDropperLeftLimit = !comms.BallDropperLeftLimit;
      break;
    case LIMIT_RIGHT:
      comms.BallDropperRightLimit = !comms.BallDropperRightLimit;
      break;
  }
}
