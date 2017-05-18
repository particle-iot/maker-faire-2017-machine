/*
 * Light the tracks for Panel 1 with Neopixel
 */

#include "neopixel.h"
#include "mf2017-can.h"

SYSTEM_THREAD(ENABLED);

/* Define AUTO_START to true during testing and false during the event.
 * It decides whether to wait for the MachineStart flag to turn on the
 * mechanisms or auto-start everything at boot.
 */
const bool AUTO_START = true;

// The main flag to enable or disable mechanisms
bool runMachine = AUTO_START;

/*
 * Pin usage:
 */
const auto CAN_BUS_PINS = CAN_D1_D2;

// Neopixel strips
const auto PIXEL_TYPE = SK6812RGBW;

const auto PIXEL_PIN_1 = A0;
const auto PIXEL_COUNT_1 = 60;

const auto PIXEL_PIN_2 = A1;
const auto PIXEL_COUNT_2 = 60;

const auto PIXEL_PIN_3 = A3;
const auto PIXEL_COUNT_3 = 60;

// keeps track of how much time we've been running the animation
uint32_t animation_timer1;
uint32_t animation_timer2;
uint32_t animation_timer3;

bool animateStrip1 = true;
bool animateStrip2 = false;
bool animateStrip3 = false;

const auto ANIMATION_TIME_MAX = 4000;

/*
 * CAN communication
 */
BuiltinCAN can(CAN_BUS_PINS);
Communication comms(can);

/*
 * NeoPixel
 */
Adafruit_NeoPixel strip1(PIXEL_COUNT_1, PIXEL_PIN_1, PIXEL_TYPE);
Adafruit_NeoPixel strip2(PIXEL_COUNT_2, PIXEL_PIN_2, PIXEL_TYPE);
Adafruit_NeoPixel strip3(PIXEL_COUNT_3, PIXEL_PIN_3, PIXEL_TYPE);

uint32_t color1;
uint32_t color2;
uint32_t color3;
uint32_t color4;

void setup() {
  setupComms();

  color1 = normalizeColor(strip1.Color(0, 255, 0));
  color2 = normalizeColor(strip2.Color(0, 0, 255));
  color3 = normalizeColor(strip2.Color(255, 0, 0));
  color4 = normalizeColor(strip2.Color(0, 0, 0));

  strip1.begin();
  strip2.begin();
  strip3.begin();

  lightSolid(strip1, color1);
  lightSolid(strip2, color2);
  lightSolid(strip3, color3);
}

void lightSolid(Adafruit_NeoPixel &strip, uint32_t color) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

// Theatre-style crawling lights.
void lightTheater(Adafruit_NeoPixel &strip, uint32_t c, uint8_t wait) {
  for (int j=0; j<1; j++) { // do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c); // turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0); // turn every third pixel off
      }
    }
  }
}

// Fill the dots one after the other with a color
void colorWipe(Adafruit_NeoPixel &strip, uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void loop() {
  // receiveComms();
  // updateBallCount();
  // updateLeds();

  // colorWipe(strip1, color1, 50);
  // colorWipe(strip1, color4, 50);
  // colorWipe(strip2, color2, 50);
  // colorWipe(strip2, color4, 50);
  // colorWipe(strip3, color3, 50);
  // colorWipe(strip3, color4, 50);

  // lightTheater(strip1, color1, 100);
  // lightTheater(strip2, color2, 100);
  // lightTheater(strip2, color2, 100);
  // Serial.printlnf("comms.[Red|Blue|Green]BallCount: %d %d %d", comms.RedBallCount, comms.BlueBallCount, comms.GreenBallCount);
  // delay(100);
}

/*
 * CAN communication
 */

void setupComms() {
  comms.begin();
}

void receiveComms() {
  comms.receive();
  if (comms.MachineStop) {
    runMachine = false;
  } else if (comms.MachineStart) {
    runMachine = true;
  }
}

void updateBallCount() {
  // Count balls
  static uint32_t oldBallCount1 = 0;
  uint32_t ballCount1 = comms.RedBallCount;
  if (ballCount1 != oldBallCount1) {
    ballCount1 += ballCount1 - oldBallCount1;
    animateStrip1 = true;
    animation_timer1 = millis();
  }
  oldBallCount1 = ballCount1;

  static uint32_t oldBallCount2 = 0;
  uint32_t ballCount2 = comms.GreenBallCount;
  if (ballCount2 != oldBallCount2) {
    ballCount2 += ballCount2 - oldBallCount2;
    animateStrip2 = true;
    animation_timer2 = millis();
  }
  oldBallCount2 = ballCount2;

  static uint32_t oldBallCount3 = 0;
  uint32_t ballCount3 = comms.BlueBallCount;
  if (ballCount3 != oldBallCount3) {
    ballCount3 += ballCount3 - oldBallCount3;
    animateStrip3 = true;
    animation_timer3 = millis();
  }
  oldBallCount3 = ballCount3;
}

void updateLeds() {
  if (animateStrip1)
  {
    if (millis() - animation_timer1 < ANIMATION_TIME_MAX) {
      lightTheater(strip1, color1, 50);
    }
    else {
      animateStrip1 = false;
    }
  }
  else {
    lightSolid(strip1, color1);
  }

  if (animateStrip2)
  {
    if (millis() - animation_timer2 < ANIMATION_TIME_MAX) {
      lightTheater(strip2, color2, 50);
    }
    else {
      animateStrip2 = false;
    }
  }
  else {
    lightSolid(strip2, color2);
  }

  if (animateStrip3)
  {
    if (millis() - animation_timer3 < ANIMATION_TIME_MAX) {
      lightTheater(strip3, color3, 50);
    }
    else {
      animateStrip3 = false;
    }
  }
  else {
    lightSolid(strip3, color3);
  }
}

uint32_t normalizeColor(uint32_t c) {
  // Swap red and green for RGBW pixel
  uint8_t w = (c >> 24) & 0xff;
  uint8_t r = (c >> 16) & 0xff;
  uint8_t g = (c >> 8) & 0xff;
  uint8_t b = c & 0xff;
  return (w << 24) | (g << 16) | (r << 8) | b;
}
