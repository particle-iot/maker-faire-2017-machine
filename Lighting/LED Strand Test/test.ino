#include "neopixel.h"

SYSTEM_THREAD(ENABLED);
#define PIXEL_PIN A7
#define PIXEL_COUNT 600
#define PIXEL_TYPE SK6812RGBW 

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

void setup() {
  strip.begin();
}

void loop()
{
  allColor(strip.Color(0, 255, 0, 0));
  delay(2000);
  allColor(strip.Color(255, 0, 0, 0));
  delay(2000);
  allColor(strip.Color(0, 0, 255, 0));
  delay(2000);
  allColor(strip.Color(0, 0, 0, 255));
  delay(2000);
}

void allColor(uint32_t c) {
  for (int i = 0; i < PIXEL_COUNT; i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}
