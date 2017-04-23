/*
 * Lighting behind the interaction surface
 */


#include "neopixel.h"
#include "mf2017-can.h"
#include "hsv.h"
#include "simple-storage.h"
#include <algorithm>
using namespace std;

SYSTEM_THREAD(ENABLED);

BuiltinCAN can(CAN_D1_D2);
Communication comms(can);

// Neopixel strip
const auto PIXEL_PIN = A1;
//#define PIXEL_COUNT 183
const auto PIXEL_COUNT = 56;
const auto PIXEL_TYPE = WS2812B;

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// Brightness
const auto BRIGHTNESS_PIN = A0;

// Config stored in EEPROM
const auto PANEL_COUNT = 4;
struct Config {
  uint32_t app;
  uint8_t fillingDelay;
  uint8_t theaterChaseDelay;
  uint8_t theaterChaseDuration;

  uint8_t panelFirst[PANEL_COUNT];
  uint8_t panelSize[PANEL_COUNT];
  uint8_t activeTimeout;
  uint8_t colorFillDelay;
} config;

Config defaultConfig = {
  /* app */ APP_CODE('M', 'F', 'L', 2),
  /* fillingDelay */ 20,
  /* theaterChaseDelay */ 80,
  /* theaterChaseDuration */ 8,
  /* panelFirst */ { 0, 14, 28, 42 },
  /* panelCount */ { 14, 14, 14, 14 },
  /* activeTimeout */ 30,
  /* colorFillDelay */ 10,
};

Storage<Config> storage(defaultConfig);

void setup() {
  Serial.begin();
  storage.load(config);
  delay(100);
  comms.begin();
  strip.begin();
  strip.show();
  startAttractFilling();
}

void loop() {
  registerCloud();
  comms.receive();
  updateAttractMode();
  updateBrightness();

  // FIXME updatePanel1();
  updatePanel2();

  display();
}

void registerCloud() {
  static bool registered = false;
  if (!registered && Particle.connected()) {
    registered = true;
    Particle.function("set", updateConfig);
  }
}

/* Attract mode will fill the lights at the bottom of inactive panels
 * one dot at a time, then do a theater chase
 */
enum {
  FILLING,
  THEATER_CHASE
} attractState = FILLING;

uint8_t attract[PIXEL_COUNT];
unsigned attractPos;
unsigned attractCount;
long attractLastUpdate;
long attractStart;

uint8_t attractSize() {
  uint8_t s = 0;
  for (unsigned i = 0; i < PANEL_COUNT; i++) {
    s = max(config.panelSize[i], s);
  }
  return s;
}

void startAttractTheaterChase() {
  clearAttract();
  attractState = THEATER_CHASE;
}

void startAttractFilling() {
  clearAttract();
  attractState = FILLING;
}

void clearAttract() {
  for (unsigned i = 0, s = attractSize(); i < s; i++) {
    attract[i] = 0;
  }
  attractPos = 0;
  attractCount = 0;
  attractLastUpdate = 0;
  attractStart = millis();
}

void updateAttractMode() {
  switch (attractState) {
    case FILLING:
      updateAttractFilling();
      break;
    case THEATER_CHASE:
      updateAttractTheaterChase();
      break;
  }
}

void updateAttractFilling() {
  if (millis() - attractLastUpdate < config.fillingDelay) {
    return;
  }
  attractLastUpdate = millis();

  unsigned s = attractSize();

  attract[attractPos] = 1;
  if (attractPos != 0) {
    attract[attractPos - 1] = 0;
  }
  attractPos++;

  if (attractPos >= s - attractCount) {
    attractPos = 0;
    attractCount++;
  }

  if (attractCount >= s) {
    startAttractTheaterChase();
  }
}

void updateAttractTheaterChase() {
  if (millis() - attractLastUpdate < config.theaterChaseDelay) {
    return;
  }
  attractLastUpdate = millis();

  for (unsigned i = 0, s = attractSize(); i < s; i++) {
    unsigned n = (i - attractCount) % 4;
    attract[i] = (n < 2) ? 1 : 0;
  }
  attractCount++;

  if (millis() - attractStart > config.theaterChaseDuration * 1000) {
    startAttractFilling();
  }
}

/* Are panels being interacted with?
 */
enum {
  PANEL1,
  PANEL2,
  PANEL3,
  PANEL4
};
bool panelActive[PANEL_COUNT] = { 0 };
long panelLastActive[PANEL_COUNT] = { 0 };
uint32_t panelPixels[PIXEL_COUNT] = { 0 };

void clearPanel(unsigned panel) {
  unsigned first = config.panelFirst[panel];
  for (unsigned i = first, n = first + config.panelSize[panel]; i < n; i++) {
    panelPixels[i] = 0;
  }
}

/* Panel 1 interaction: Color fill with the color of the button pressed
 * then breathe
 */

const auto RED_COLOR = 0xff0000;
const auto GREEN_COLOR = 0x00ff00;
const auto BLUE_COLOR = 0x0000ff;

unsigned panel1Count;
uint32_t panel1Color;

void updatePanel1() {
  if (comms.Input1Active && !panelActive[PANEL1]) {
    panelActive[PANEL1] = true;
    if (comms.RedButtonPressed) {
      panel1Color = RED_COLOR;
    } else if (comms.GreenButtonPressed) {
      panel1Color = GREEN_COLOR;
    } else if (comms.BlueButtonPressed) {
      panel1Color = BLUE_COLOR;
    }
  }
}

/* Panel 2 interaction: flow current color
 */

long panel2LastUpdate = 0;

void updatePanel2() {
  if (comms.Input2Active) {
    // panel just became active
    if (!panelActive[PANEL2]) {
      panelActive[PANEL2] = true;
      clearPanel(PANEL2);
    }
    panelLastActive[PANEL2] = millis();
  }

  if (!panelActive[PANEL2]) {
    return;
  }

  if (millis() - panel2LastUpdate < config.colorFillDelay) {
    return;
  }
  panel2LastUpdate = millis();

  // FIXME: HACK!
  unsigned x = analogRead(BRIGHTNESS_PIN) / 16 + 1;
  if (x > 255) {
    x = 255;
  }
  comms.InputColorHue = x;

  // Color flow
  if (panelActive[PANEL2]) {
    // shift every pixel one forward
    unsigned first = config.panelFirst[PANEL2];
    for (unsigned i = first + config.panelSize[PANEL2] - 1; i > first; i--) {
      panelPixels[i] = panelPixels[i - 1];
    }

    // Compute RGB of input hue
    RgbColor inputColor = HsvToRgb(HsvColor(comms.InputColorHue, 255, 255));
    uint32_t c = strip.Color(inputColor.r, inputColor.g, inputColor.b);
    panelPixels[first] = c;
  }

  if (millis() - panelLastActive[PANEL2] > config.activeTimeout * 1000) {
    panelActive[PANEL2] = false;
  }
}

void updateBrightness() {
  //unsigned brightness = analogRead(BRIGHTNESS_PIN) / 16 + 1;
  //if (brightness > 255) {
  //  brightness = 255;
  //}
  unsigned brightness = 16;
  strip.setBrightness(brightness);
}

const auto ATTRACT_COLOR = 0xffffff;

long displayLastUpdate = 0;
const auto DISPLAY_INTERVAL = 10;

void display() {
  if (millis() - displayLastUpdate < DISPLAY_INTERVAL) {
    return;
  }
  displayLastUpdate = millis();

  for (unsigned panel = 0; panel < PANEL_COUNT; panel++) {
    if (panelActive[panel]) {
      displayActive(panel);
    } else {
      displayAttract(panel);
    }
  }
  strip.show();
}

// Fill the lights of panel with the current active mode pattern
void displayActive(unsigned panel) {
  unsigned first = config.panelFirst[panel];
  for (unsigned i = first, n = first + config.panelSize[panel]; i < n; i++) {
    strip.setPixelColor(i, panelPixels[i]);
  }
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
    } else if (name.equals("theaterChaseDuration")) {
        config.theaterChaseDuration = value.toInt();
    } else if (name.equals("activeTimeout")) {
        config.activeTimeout = value.toInt();
    } else if (name.equals("colorFillDelay")) {
        config.colorFillDelay = value.toInt();
    } else {
        return -2;
    }

    storage.store(config);
    return 0;
}

