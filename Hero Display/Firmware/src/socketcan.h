/*
 * Interface to a CAN bus on Linux.
 */

#pragma once

#include "mf2017-can.h"
#include <linux/can.h>

class SocketCAN : public CANInterface {
public:
  SocketCAN(const char* networkInterface);

  virtual void begin(unsigned long baudRate) override;
  virtual bool receive(CANMessage& message) override;
  virtual bool transmit(const CANMessage& message) override;

private:
  void bringCANInterfaceUp(unsigned long baudRate);
  void openSocket();

  can_frame messageParticleToLinux(const CANMessage& message);
  CANMessage messageLinuxToParticle(const can_frame &frame);

  void debugPrintMessage(const CANMessage& message);
  void perror(const char* message);

  const char* networkInterface;
  int sock; // SocketCAN socket
};
