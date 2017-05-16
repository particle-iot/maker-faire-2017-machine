#include "neopixel.h"

SYSTEM_THREAD(ENABLED);
// Neopixel strip
const auto PIXEL_PIN = A0;
const auto PIXEL_COUNT = 8;
const auto PIXEL_TYPE = SK6812RGBW;
Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

void setup() {
  strip.begin();
  strip.show();
  for(int i = 0; i < PIXEL_COUNT; i++) {
    strip.setPixelColor(i, 255, 0, 0, 0);
  }
  strip.show();
}
