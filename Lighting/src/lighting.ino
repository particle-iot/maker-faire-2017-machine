/*
 * Lighting behind the interaction surface
 */


#include "neopixel.h"
#include "mf2017-can.h"

SYSTEM_THREAD(ENABLED);

Communication comms;

// Neopixel strip
#define PIXEL_PIN A1
//#define PIXEL_COUNT 183
#define PIXEL_COUNT 56
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// Brightness
#define BRIGHTNESS_PIN A0

void setup() {
  Serial.begin();
  comms.begin();
  strip.begin();
  strip.show();
  startAttractMode();
}

void loop() {
  comms.receive();
  updateAttractMode();
  updateBrightness();
  display();
  delay(10);
}

enum {
  ATTRACT_MODE
} state = ATTRACT_MODE;

uint8_t attract[PIXEL_COUNT];
int attractPos;
int attractCount;

void startAttractMode() {
  for (int i = 0; i < PIXEL_COUNT; i++) {
    attract[i] = 0;
  }
  attractPos = 0;
  attractCount = 0;
}

void updateAttractMode() {
  if (attractCount >= PIXEL_COUNT) {
    //Serial.println("restarting");
    startAttractMode();
  }

  //Serial.printlnf("Update pos=%d count=%d", attractPos, attractCount);
  attract[attractPos] = 1;
  if (attractPos != 0) {
    attract[attractPos - 1] = 0;
  }
  attractPos++;

  if (attractPos >= PIXEL_COUNT - attractCount) {
    //Serial.println("Starting next round");
    attractPos = 0;
    attractCount++;
  }
}

void updateBrightness() {
  int brightness = analogRead(BRIGHTNESS_PIN) / 16 + 1;
  if (brightness > 255) {
    brightness = 255;
  }
  strip.setBrightness(brightness);
}

#define ATTRACT_COLOR 0xffffff
const uint32_t attractColors[] = {
  0xffffff,
  0xff0000,
  0x00ff00,
  0x0000ff
};
void display() {
  switch (state) {
    case ATTRACT_MODE:
      //Serial.println("Displaying");
      int n = (comms.ballEntering[0] ? 2 : 0) + (comms.ballEntering[1] ?  1 : 0);
      uint32_t c = attractColors[n];
      for (int i = 0; i < PIXEL_COUNT; i++) {
        strip.setPixelColor(i, attract[i] ? c : 0);
      }
      break;
  }
  strip.show();
}

