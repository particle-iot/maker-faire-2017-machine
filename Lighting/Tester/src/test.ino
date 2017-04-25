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
void updateButton(ButtonColor button);

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

      case 'a': updateJoystick(1, 0); break;
      case 'z': updateJoystick(-1, 0); break;
      case 's': updateJoystick(0, 1); break;
      case 'x': updateJoystick(0, -1); break;

    }
  }

  comms.receive();
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
    "Panel 4: a and z for left joystick, s and x for right joystick\r\n"
  );
}

void clearInput() {
  comms.Input1Active = false;
  comms.RedButtonPressed = false;
  comms.GreenButtonPressed = false;
  comms.BlueButtonPressed = false;

  comms.Input2Active = false;

  // comms.Input3Active is deactivated with the 0 key

  comms.Input4Active = false;
  comms.LeftJoystickUp    = false;
  comms.LeftJoystickDown  = false;
  comms.RightJoystickUp   = false;
  comms.RightJoystickDown = false;
}

void updateButton(ButtonColor button) {
  comms.Input1Active = true;
  switch (button) {
    case BUTTON_RED:
      comms.RedButtonPressed = true;
      break;
    case BUTTON_GREEN:
      comms.GreenButtonPressed = true;
      break;
    case BUTTON_BLUE:
      comms.BlueButtonPressed = true;
      break;
  }
}

void updateHue(unsigned h) {
  comms.Input2Active = true;
  comms.InputColorHue = 25 * h;
}

void updateCrankSpeed(unsigned s) {
  comms.Input3Active = (s > 0);
  comms.InputCrankSpeed = s / 3.0;
}

void updateJoystick(int l, int r) {
  comms.Input4Active = true;
  comms.LeftJoystickUp    = (l == 1);
  comms.LeftJoystickDown  = (l == -1);
  comms.RightJoystickUp   = (r == 1);
  comms.RightJoystickDown = (r == -1);
}

