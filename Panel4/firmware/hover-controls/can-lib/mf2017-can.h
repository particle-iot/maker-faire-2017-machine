/* mf2017-can library by Julien Vanier <jvanier@gmail.com>
 */

#pragma once

#include "Particle.h"
#include "can-interface.h"

enum class MachineModules {
  Panel1,
  Panel2,
  Panel3,
  Panel4,
  Panel5,
  Lights,
  Display,
  Supervisor,
};

enum class ReservoirStatus : uint8_t {
  OK,
  Full,
  Empty
};

class Communication {
private:
  CANInterface &can;

public:
  constexpr static long baudRate = 500000;

  Communication(CANInterface &can);
  void begin();
  void receive();
  void transmit(MachineModules module);

public:
  // All the variables shared between the modules

  // Panel1
  bool Input1Active;
  ReservoirStatus Reservoir1Status;
  bool GreenButtonPressed;
  bool BlueButtonPressed;
  bool RedButtonPressed;
  uint32_t BallCount1;

  long Panel1StatusLastTx;
  long Panel1StatusLastRx;

  uint16_t RedBallCount;
  uint16_t GreenBallCount;
  uint16_t BlueBallCount;

  long Panel1OutputStatusLastTx;
  long Panel1OutputStatusLastRx;

  // Panel2
  bool Input2Active;
  ReservoirStatus Reservoir2Status;
  uint8_t InputColorHue;
  uint32_t BallCount2;

  long Panel2StatusLastTx;
  long Panel2StatusLastRx;

  // Panel3
  bool Input3Active;
  ReservoirStatus Reservoir3Status;
  uint32_t BallCount3;

  long Panel3StatusLastTx;
  long Panel3StatusLastRx;

  float InputCrankSpeed;

  long Panel3InputStatusLastTx;
  long Panel3InputStatusLastRx;

  uint16_t IntegrationCountA;
  uint16_t IntegrationCountB;
  uint16_t IntegrationCountC;

  long Panel3OutputStatusLastTx;
  long Panel3OutputStatusLastRx;

  // Panel4
  bool Input4Active;
  ReservoirStatus Reservoir4Status;
  bool BallDropperLeftLimit;
  bool BallDropperRightLimit;
  bool PrintingPrizeA;
  bool PrintingPrizeB;
  bool PrintingPrizeC;
  uint8_t HoverPositionLR;
  uint8_t HoverPositionUD;
  uint32_t BallCount4;

  long Panel4StatusLastTx;
  long Panel4StatusLastRx;

  uint16_t PrizeCountA;
  uint16_t PrizeCountB;
  uint16_t PrizeCountC;

  long Panel4OutputStatusLastTx;
  long Panel4OutputStatusLastRx;

  // Lights
  bool LightsActive;

  long LightsStatusLastTx;
  long LightsStatusLastRx;

  // Display
  bool DisplayActive;

  long DisplayStatusLastTx;
  long DisplayStatusLastRx;

  // Supervisor
  bool MachineStart;
  bool MachineStop;

  long SupervisorControlLastTx;
  long SupervisorControlLastRx;
  
  // machine config
  float InputCrankBallWheelSpeedRatio;

  long Panel3ConfigLastTx;
  long Panel3ConfigLastRx;

private:
  void decodeMessage(CANMessage m);
};
