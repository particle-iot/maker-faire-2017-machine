// Interface for CAN functionality to allow a different driver on the
// Photon and Raspberry Pi

#pragma once

#include "Particle.h"

class CANInterface {
public:
  virtual void begin(unsigned long baudRate) = 0;
  virtual bool receive(CANMessage &message) = 0;
  virtual bool transmit(const CANMessage &message) = 0;
};

// CANChannel built in to the Photon and Electron firmware
#if PLATFORM_ID == 6 || PLATFORM_ID == 10

class BuiltinCAN : public CANInterface {
public:
  BuiltinCAN(HAL_CAN_Channel channel = CAN_D1_D2);
  virtual void begin(unsigned long baudRate) override;
  virtual bool receive(CANMessage &message) override;
  virtual bool transmit(const CANMessage &message) override;

protected:
  CANChannel can;
};

#endif
