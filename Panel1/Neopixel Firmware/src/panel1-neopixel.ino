/*
 * Light the tracks for Panel 1 with Neopixel
 */

#include "neopixel.h"

SYSTEM_THREAD(ENABLED);

// Neopixel strips
const auto PIXEL_TYPE = SK6812RGBW;

const auto PIXEL_PIN_1 = A0;
const auto PIXEL_COUNT_1 = 60;

const auto PIXEL_PIN_2 = A1;
const auto PIXEL_COUNT_2 = 60;

const auto PIXEL_PIN_3 = A3;
const auto PIXEL_COUNT_3 = 60;

Adafruit_NeoPixel strip1(PIXEL_COUNT_1, PIXEL_PIN_1, PIXEL_TYPE);
Adafruit_NeoPixel strip2(PIXEL_COUNT_2, PIXEL_PIN_2, PIXEL_TYPE);
Adafruit_NeoPixel strip3(PIXEL_COUNT_3, PIXEL_PIN_3, PIXEL_TYPE);

uint32_t color1;
uint32_t color2;
uint32_t color3;

void setup() {
  color1 = normalizeColor(strip1.Color(0, 255, 0));
  color2 = normalizeColor(strip2.Color(0, 0, 255));
  color3 = normalizeColor(strip2.Color(255, 0, 0));

  strip1.begin();
  strip2.begin();
  strip3.begin();
}

void lightSolid(Adafruit_NeoPixel &strip, uint32_t color) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void loop() {
  lightSolid(strip1, color1);
  lightSolid(strip2, color2);
  lightSolid(strip3, color3);
  delay(100);
}

uint32_t normalizeColor(uint32_t c) {
  // Swap red and green for RGBW pixel
  uint8_t w = (c >> 24) & 0xff;
  uint8_t r = (c >> 16) & 0xff;
  uint8_t g = (c >> 8) & 0xff;
  uint8_t b = c & 0xff;
  return (w << 24) | (g << 16) | (r << 8) | b;
}
