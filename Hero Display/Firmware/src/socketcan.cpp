// socket.h must be included before Particle.h to avoid double define in hal/inc/socket_hal.h
#include <sys/socket.h>
#include "Particle.h"

#include "socketcan.h"

#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/can.h>

SocketCAN::SocketCAN(const char *networkInterface)
  : networkInterface(networkInterface)
{
}

void SocketCAN::begin(unsigned long baudRate)
{
  bringCANInterfaceUp(baudRate);
  openSocket();
}

void SocketCAN::bringCANInterfaceUp(unsigned long baudRate) {
  String cmd = String::format("/sbin/ip link set can0 up type can bitrate %d", baudRate);

  auto proc = Process::run(cmd.c_str());
  proc.wait();
}

void SocketCAN::openSocket() {
	// open socket
	if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("socket");
		return;
	}

    // convert can0 to network interface index
	struct ifreq ifr;
	strncpy(ifr.ifr_name, networkInterface, IFNAMSIZ - 1);

	ifr.ifr_name[IFNAMSIZ - 1] = '\0';
	ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
    if (!ifr.ifr_ifindex) {
		perror("if_nametoindex");
        sock = -1;
		return;
	}

    // set socket to non-blocking
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
    
    // bind socket to the network interface
	struct sockaddr_can addr;
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
        sock = -1;
		return;
	}
}

bool SocketCAN::receive(CANMessage &message)
{
  if (sock < 0) {
    return false;
  }
  can_frame frame;
  
  if (read(sock, &frame, sizeof(frame)) > 0) {
    message = messageLinuxToParticle(frame);
    Serial.printf("Received %02x ", message.id);
    for (int i = 0; i < message.len; i++) {
      Serial.printf("%02x", message.data[i]);
    }
    Serial.println("");
    return true;
  }

  return false;
}

bool SocketCAN::transmit(const CANMessage &message)
{
  if (sock < 0) {
    return false;
  }

  can_frame frame = messageParticleToLinux(message);
  write(sock, &frame, sizeof(frame));
  return true;
}

/* Translate CAN frame from Particle to Linux format */
can_frame SocketCAN::messageParticleToLinux(const CANMessage &message)
{
  can_frame frame = { 0 };
  frame.can_id = message.id;
  frame.can_dlc = message.len;
  for (int i = 0; i < message.len; i++) {
    frame.data[i] = message.data[i];
  }
  return frame;
}

CANMessage SocketCAN::messageLinuxToParticle(const can_frame &frame)
{
  CANMessage message;
  message.id = frame.can_id;
  message.len = frame.can_dlc;
  for (int i = 0; i < message.len; i++) {
    message.data[i] = frame.data[i];
  }
  return message;
}

void SocketCAN::perror(const char *message)
{
  fprintf(stderr, "%s\n", message);
}
