/*
 * Interface to a CAN bus on Linux.
 */

#pragma once

#include "mf2017-can.h"

class SocketCAN : public CANInterface {
public:
  SocketCAN(const char *networkInterface);

  virtual void begin(unsigned long baudRate) override;
  virtual bool available() override;
  virtual bool receive(CANMessage &message) override;
  virtual bool transmit(const CANMessage &message) override;
};
