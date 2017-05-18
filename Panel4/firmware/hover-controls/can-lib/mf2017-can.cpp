/* particle-mf2017-can library by Julien Vanier <jvanier@gmail.com>
 */

#include "mf2017-can.h"
#include "bitset.h"

enum MessageIds {
  Panel1Status = 0x101,
  Panel1OutputStatus = 0x121,
  Panel2Status = 0x102,
  Panel3Status = 0x103,
  Panel3InputStatus = 0x113,
  Panel3OutputStatus = 0x123,
  Panel4Status = 0x104,
  Panel4OutputStatus = 0x124,
  LightsStatus = 0x110,
  DisplayStatus = 0x120,
  SupervisorControl = 0x030,
  Panel3Config = 0x500,
};

Communication::Communication(CANInterface &can) :
  can(can)
{
}

void Communication::begin() {
  can.begin(baudRate);
}

void Communication::receive() {
  CANMessage m;
  while (can.receive(m)) {
    decodeMessage(m);
  }
}

void Communication::decodeMessage(CANMessage m) {
  auto now = millis();
  switch (m.id) {
    case Panel1Status:
      Panel1StatusLastRx = now;
      Input1Active = getBit(m.data, 0, 0);
      Reservoir1Status = static_cast<ReservoirStatus>(getU8(m.data, 1));
      RedButtonPressed = getBit(m.data, 2, 0);
      GreenButtonPressed = getBit(m.data, 2, 1);
      BlueButtonPressed = getBit(m.data, 2, 2);
      BallCount1 = getU32(m.data, 4);
      break;
    case Panel1OutputStatus:
      Panel1OutputStatusLastRx = now;
      RedBallCount = getU16(m.data, 0);
      GreenBallCount = getU16(m.data, 2);
      BlueBallCount = getU16(m.data, 4);
      break;
    case Panel2Status:
      Panel2StatusLastRx = now;
      Input2Active = getBit(m.data, 0, 0);
      Reservoir2Status = static_cast<ReservoirStatus>(getU8(m.data, 1));
      InputColorHue = getU8(m.data, 2);
      BallCount2 = getU32(m.data, 4);
      break;
    case Panel3Status:
      Panel3StatusLastRx = now;
      Input3Active = getBit(m.data, 0, 0);
      Reservoir3Status = static_cast<ReservoirStatus>(getU8(m.data, 1));
      BallCount3 = getU32(m.data, 4);
      break;
    case Panel3InputStatus:
      Panel3InputStatusLastRx = now;
      InputCrankSpeed = getFloat(m.data, 0);
      break;
    case Panel3OutputStatus:
      Panel3OutputStatusLastRx = now;
      IntegrationCountA = getU16(m.data, 0);
      IntegrationCountB = getU16(m.data, 2);
      IntegrationCountC = getU16(m.data, 4);
      break;
    case Panel4Status:
      Panel4StatusLastRx = now;
      Input4Active = getBit(m.data, 0, 0);
      BallDropperLeftLimit = getBit(m.data, 0, 1);
      BallDropperRightLimit = getBit(m.data, 0, 2);
      PrintingPrizeA = getBit(m.data, 0, 3);
      PrintingPrizeB = getBit(m.data, 0, 4);
      PrintingPrizeC = getBit(m.data, 0, 5);
      Reservoir4Status = static_cast<ReservoirStatus>(getU8(m.data, 1));
      HoverPositionLR = getU8(m.data, 2);
      HoverPositionUD = getU8(m.data, 3);
      BallCount4 = getU32(m.data, 4);
      break;
    case Panel4OutputStatus:
      Panel4OutputStatusLastRx = now;
      PrizeCountA = getU16(m.data, 0);
      PrizeCountB = getU16(m.data, 2);
      PrizeCountC = getU16(m.data, 4);
      break;
    case LightsStatus:
      LightsStatusLastRx = now;
      LightsActive = getBit(m.data, 0, 0);
      break;
    case DisplayStatus:
      DisplayStatusLastRx = now;
      DisplayActive = getBit(m.data, 0, 0);
      break;
    case SupervisorControl:
      SupervisorControlLastRx = now;
      MachineStart = getBit(m.data, 0, 0);
      MachineStop = getBit(m.data, 0, 1);
      break;
    case Panel3Config:
      Panel3ConfigLastRx = now;
      InputCrankBallWheelSpeedRatio = getFloat(m.data, 0);
      break;
  }
}

void Communication::transmit(MachineModules module) {
  auto now = millis();
  switch(module) {
    case MachineModules::Panel1:
      if (now - Panel1StatusLastTx >= 100) {
        Panel1StatusLastTx = now;
        CANMessage m;
        m.id = Panel1Status;
        m.len = 8;
        setBit(m.data, Input1Active, 0, 0);
        setU8(m.data, static_cast<uint8_t>(Reservoir1Status), 1);
        setBit(m.data, RedButtonPressed, 2, 0);
        setBit(m.data, GreenButtonPressed, 2, 1);
        setBit(m.data, BlueButtonPressed, 2, 2);
        setU32(m.data, BallCount1, 4);
        can.transmit(m);
      }
      if (now - Panel1OutputStatusLastTx >= 100) {
        Panel1OutputStatusLastTx = now;
        CANMessage m;
        m.id = Panel1OutputStatus;
        m.len = 8;
        setU16(m.data, RedBallCount, 0);
        setU16(m.data, GreenBallCount, 2);
        setU16(m.data, BlueBallCount, 4);
        can.transmit(m);
      }

      break;

    case MachineModules::Panel2:
      if (now - Panel2StatusLastTx >= 100) {
        Panel2StatusLastTx = now;
        CANMessage m;
        m.id = Panel2Status;
        m.len = 8;
        setBit(m.data, Input2Active, 0, 0);
        setU8(m.data, static_cast<uint8_t>(Reservoir2Status), 1);
        setU8(m.data, InputColorHue, 2);
        setU32(m.data, BallCount2, 4);
        can.transmit(m);
      }

      break;

    case MachineModules::Panel3:
      if (now - Panel3StatusLastTx >= 100) {
        Panel3StatusLastTx = now;
        CANMessage m;
        m.id = Panel3Status;
        m.len = 8;
        setBit(m.data, Input3Active, 0, 0);
        setU8(m.data, static_cast<uint8_t>(Reservoir3Status), 1);
        setU32(m.data, BallCount3, 4);
        can.transmit(m);
      }
      if (now - Panel3InputStatusLastTx >= 100) {
        Panel3InputStatusLastTx = now;
        CANMessage m;
        m.id = Panel3InputStatus;
        m.len = 8;
        setFloat(m.data, InputCrankSpeed, 0);
        can.transmit(m);
      }
      if (now - Panel3OutputStatusLastTx >= 100) {
        Panel3OutputStatusLastTx = now;
        CANMessage m;
        m.id = Panel3OutputStatus;
        m.len = 8;
        setU16(m.data, IntegrationCountA, 0);
        setU16(m.data, IntegrationCountB, 2);
        setU16(m.data, IntegrationCountC, 4);
        can.transmit(m);
      }

      break;

    case MachineModules::Panel4:
      if (now - Panel4StatusLastTx >= 100) {
        Panel4StatusLastTx = now;
        CANMessage m;
        m.id = Panel4Status;
        m.len = 8;
        setBit(m.data, Input4Active, 0, 0);
        setBit(m.data, BallDropperLeftLimit, 0, 1);
        setBit(m.data, BallDropperRightLimit, 0, 2);
        setBit(m.data, PrintingPrizeA, 0, 3);
        setBit(m.data, PrintingPrizeB, 0, 4);
        setBit(m.data, PrintingPrizeC, 0, 5);
        setU8(m.data, static_cast<uint8_t>(Reservoir4Status), 1);
        setU8(m.data, HoverPositionLR, 2);
        setU8(m.data, HoverPositionUD, 3);
        setU32(m.data, BallCount4, 4);
        can.transmit(m);
      }

      break;

    case MachineModules::Lights:
      if (now - LightsStatusLastTx >= 100) {
        LightsStatusLastTx = now;
        CANMessage m;
        m.id = LightsStatus;
        m.len = 1;
        setBit(m.data, LightsActive, 0, 0);
        can.transmit(m);
      }

      break;

    case MachineModules::Display:
      if (now - DisplayStatusLastTx >= 100) {
        DisplayStatusLastTx = now;
        CANMessage m;
        m.id = DisplayStatus;
        m.len = 1;
        setBit(m.data, DisplayActive, 0, 0);
        can.transmit(m);
      }

      break;

    case MachineModules::Supervisor:
      if (now - SupervisorControlLastTx >= 100) {
        SupervisorControlLastTx = now;
        CANMessage m;
        m.id = SupervisorControl;
        m.len = 1;
        setBit(m.data, MachineStart, 0, 0);
        setBit(m.data, MachineStop, 0, 1);
        can.transmit(m);
      }

      if (now - Panel3ConfigLastTx >= 1000) {
        Panel3ConfigLastTx = now;
        CANMessage m;
        m.id = Panel3Config;
        m.len = 8;
        setFloat(m.data, InputCrankBallWheelSpeedRatio, 0);
        can.transmit(m);
      }

      break;
  }
}
