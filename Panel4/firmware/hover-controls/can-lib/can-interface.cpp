// Interface for CAN functionality to allow a different driver on the
// Photon and Raspberry Pi

#include "can-interface.h"

// CANChannel built in to the Photon and Electron firmware
#if PLATFORM_ID == 6 || PLATFORM_ID == 10

BuiltinCAN::BuiltinCAN(HAL_CAN_Channel channel) :
  can(channel)
{
}

void BuiltinCAN::begin(unsigned long baudRate) {
  can.begin(baudRate);
}

bool BuiltinCAN::receive(CANMessage &message) {
  return can.receive(message);
}

bool BuiltinCAN::transmit(const CANMessage &message) {
  return can.transmit(message);
}

#endif
