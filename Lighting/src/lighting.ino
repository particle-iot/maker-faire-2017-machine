/*
 * Lighting behind the interaction surface
 */


#include "neopixel.h"
#include "mf2017-can.h"
#include "simple-storage.h"
#include <algorithm>
using namespace std;

SYSTEM_THREAD(ENABLED);

BuiltinCAN can(CAN_D1_D2);
Communication comms(can);

// Neopixel strip
#define PIXEL_PIN A1
//#define PIXEL_COUNT 183
#define PIXEL_COUNT 56
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// Brightness
#define BRIGHTNESS_PIN A0

// Config stored in EEPROM
#define PANEL_COUNT 4
struct Config {
  uint32_t app;
  uint8_t fillingDelay;
  uint8_t theaterChaseDelay;

  uint8_t panelFirst[PANEL_COUNT];
  uint8_t panelSize[PANEL_COUNT];
} config;

Config defaultConfig = {
  /* app */ APP_CODE('M', 'F', 'L', 0),
  /* fillingDelay */ 10,
  /* theaterChaseDelay */ 250,
  /* panelFirst */ { 0, 14, 28, 42 },
  /* panelCount */ { 14, 14, 14, 14 },
};

Storage<Config> storage(defaultConfig);

void setup() {
  Serial.begin();
  storage.load(config);
  delay(100);
  comms.begin();
  strip.begin();
  strip.show();
  startAttractMode();
}

void loop() {
  registerCloud();
  comms.receive();
  updateAttractMode();
  updateBrightness();
  display();
  delay(10);
}

void registerCloud() {
  static bool registered = false;
  if (!registered && Particle.connected()) {
    registered = true;
    Particle.function("set", updateConfig);
  }
}

/* Attract mode will fill the lights at the bottom of inactive panels
 * one dot at a time, then do a theater chase */
enum {
  FILLING,
  THEATER_CHASE
} attractState = FILLING;

uint8_t attract[PIXEL_COUNT];
int attractPos;
int attractCount;

uint8_t attractSize() {
  uint8_t s = 0;
  for (unsigned i = 0; i < PANEL_COUNT; i++) {
    s = max(config.panelSize[i], s);
  }
  return s;
}

void startAttractMode() {
  for (unsigned i = 0, s = attractSize(); i < s; i++) {
    attract[i] = 0;
  }
  attractPos = 0;
  attractCount = 0;
}

void updateAttractMode() {
  unsigned s = attractSize();
  if (attractCount >= s) {
    startAttractMode();
  }

  attract[attractPos] = 1;
  if (attractPos != 0) {
    attract[attractPos - 1] = 0;
  }
  attractPos++;

  if (attractPos >= s - attractCount) {
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
  for (unsigned panel = 0; panel < PANEL_COUNT; panel++) {
    displayAttract(panel);
  }
  strip.show();
}

// Fill the lights of panel with the current attract mode pattern
void displayAttract(unsigned panel) {
  unsigned s = config.panelSize[panel];
  for (unsigned i = 0, panelPos = config.panelFirst[panel], attractPos = attractSize() - s;
      i < s;
      i++, panelPos++, attractPos++) {
    strip.setPixelColor(panelPos, attract[attractPos] ? ATTRACT_COLOR : 0);
  }
}

// Call Particle cloud function "set" with name=value to update a config value
int updateConfig(String arg) {
    int equalPos = arg.indexOf('=');
    if (equalPos < 0) {
        return -1;
    }
    String name = arg.substring(0, equalPos);
    String value = arg.substring(equalPos + 1);

    if (name.equals("fillingDelay")) {
        config.fillingDelay = value.toInt();
    } else if (name.equals("theaterChaseDelay")) {
        config.theaterChaseDelay = value.toInt();
    } else {
        return -2;
    }

    storage.store(config);
    return 0;
}

