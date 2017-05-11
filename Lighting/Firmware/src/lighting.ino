/*
 * Lighting behind the interaction surface
 */


#include "neopixel.h"
#include "mf2017-can.h"
#include "hsv.h"
#include "simple-storage.h"
#include <algorithm>
using namespace std;

// Define when using the 56 LED test strip instead of the full strip
//#define MINI_MODE

SYSTEM_THREAD(ENABLED);

BuiltinCAN can(CAN_D1_D2);
Communication comms(can);

// Neopixel strip
const auto PIXEL_PIN = A0;
#ifdef MINI_MODE
const auto PIXEL_COUNT = 56;
const auto PIXEL_TYPE = WS2812B;
#else
const auto PIXEL_COUNT = 300;
const auto PIXEL_TYPE = SK6812RGBW;
#endif

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// Config stored in EEPROM
const auto PANEL_COUNT = 4;
struct Config {
  uint32_t app;
  uint16_t fillingDelay;
  uint16_t theaterChaseDelay;
  uint16_t theaterChaseDuration;
  uint8_t panelFirst[PANEL_COUNT];
  uint8_t panelSize[PANEL_COUNT];
  uint16_t activeTimeout;
  uint16_t colorFillDelay;
  uint16_t colorFlowDelay;
  uint16_t bulletBaseDelay;
  uint16_t hueSliderDelay;
} config;

#ifdef MINI_MODE
Config defaultConfig = {
  /* app */ APP_CODE('M', 'F', 'L', 6),
  /* fillingDelay */ 20,
  /* theaterChaseDelay */ 80,
  /* theaterChaseDuration */ 8,
  /* panelFirst */ { 0, 14, 28, 42 },
  /* panelCount */ { 14, 14, 14, 14 },
  /* activeTimeout */ 30,
  /* colorFillDelay */ 100,
  /* colorFlowDelay */ 100,
  /* bulletBaseDelay */ 100,
  /* hueSliderDelay */ 20,
};
#else
Config defaultConfig = {
  /* app */ APP_CODE('M', 'F', 'L', 6),
  /* fillingDelay */ 5,
  /* theaterChaseDelay */ 40,
  /* theaterChaseDuration */ 8,
  /* panelFirst */ { 0, 75, 150, 225 },
  /* panelCount */ { 75, 75, 75, 75 },
  /* activeTimeout */ 15,
  /* colorFillDelay */ 20,
  /* colorFlowDelay */ 30,
  /* bulletBaseDelay */ 20,
  /* hueSliderDelay */ 20,
};
#endif

Storage<Config> storage(defaultConfig);

void setup() {
  Serial.begin();
  storage.load(config);
  comms.begin();
  strip.begin();
  strip.show();
  startAttractFilling();
}

void loop() {
  registerCloud();
  comms.receive();
  updateAttractMode();
  // TODO: handle brightness through CAN
  //updateBrightness();

  updatePanel1();
  updatePanel2();
  updatePanel3();
  updatePanel4();

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
    unsigned n = (i - attractCount) % 5;
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
long panelLastUpdate[PANEL_COUNT] = { 0 };
uint32_t panelPixels[PIXEL_COUNT] = { 0 };

// Set all pixels of a panel to off (black)
void clearPanel(unsigned panel) {
  unsigned first = config.panelFirst[panel];
  for (unsigned i = first, n = first + config.panelSize[panel]; i < n; i++) {
    panelPixels[i] = 0;
  }
}

// Shift all pixels of a panel forward
void shiftPanel(unsigned panel) {
  unsigned first = config.panelFirst[panel];
  for (unsigned i = first + config.panelSize[panel] - 1; i > first; i--) {
    panelPixels[i] = panelPixels[i - 1];
  }
}

/* Panel 1 interaction: Color fill with the color of the button pressed
 * then breathe
 */

const auto RED_COLOR = 0xff0000;
const auto GREEN_COLOR = 0x00ff00;
const auto BLUE_COLOR = 0x0000ff;
const auto PANEL1_MAX_BRIGHTNESS = 255;

uint32_t panel1Color;
uint8_t panel1Brightness;
bool panel1RedPressed;
bool panel1GreenPressed;
bool panel1BluePressed;

void updatePanel1() {
  if (comms.Input1Active) {
    // panel just became active
    if (!panelActive[PANEL1]) {
      panelActive[PANEL1] = true;
      clearPanel(PANEL1);
      panel1RedPressed = false;
      panel1GreenPressed = false;
      panel1BluePressed = false;
    }
    panelLastActive[PANEL1] = millis();
  }

  if (!panelActive[PANEL1]) {
    return;
  }

  if (millis() - panelLastUpdate[PANEL1] < config.colorFillDelay) {
    return;
  }
  panelLastUpdate[PANEL1] = millis();

  if (comms.RedButtonPressed && !panel1RedPressed) {
    panel1Color = RED_COLOR;
    panel1Brightness = PANEL1_MAX_BRIGHTNESS;
  }
  panel1RedPressed = comms.RedButtonPressed;

  if (comms.GreenButtonPressed && !panel1GreenPressed) {
    panel1Color = GREEN_COLOR;
    panel1Brightness = PANEL1_MAX_BRIGHTNESS;
  }
  panel1GreenPressed = comms.GreenButtonPressed;

  if (comms.BlueButtonPressed && !panel1BluePressed) {
    panel1Color = BLUE_COLOR;
    panel1Brightness = PANEL1_MAX_BRIGHTNESS;
  }
  panel1BluePressed = comms.BlueButtonPressed;

  // Color fill

  // shift every pixel one forward
  shiftPanel(PANEL1);

  // shift in active color
  unsigned first = config.panelFirst[PANEL1];
  uint32_t c = ((panel1Color * panel1Brightness) >> 8) & panel1Color;
  panelPixels[first] = c;

  // Reduce brightness of color for next shift in
  if (panel1Brightness > 1) {
    panel1Brightness -= 2;
  }

  if (millis() - panelLastActive[PANEL1] > config.activeTimeout * 1000) {
    panelActive[PANEL1] = false;
  }
}

/* Panel 2 interaction: flow current color
 */

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

  if (millis() - panelLastUpdate[PANEL2] < config.colorFlowDelay) {
    return;
  }
  panelLastUpdate[PANEL2] = millis();

  // Color flow

  // shift every pixel one forward
  shiftPanel(PANEL2);

  // Compute RGB of input hue
  RgbColor rgb = HsvToRgb(HsvColor(comms.InputColorHue, 255, 255));
  uint32_t c = strip.Color(rgb.r, rgb.g, rgb.b);
  unsigned first = config.panelFirst[PANEL2];
  panelPixels[first] = c;

  if (millis() - panelLastActive[PANEL2] > config.activeTimeout * 1000) {
    panelActive[PANEL2] = false;
  }
}

/* Panel 3 interaction: rainbow bullet
 */

#ifdef MINI_MODE
const auto bulletWidth = 2;
const uint8_t bulletValue[bulletWidth + 1] = { 255, 128, 64 };
#else
const auto bulletWidth = 4;
const uint8_t bulletValue[bulletWidth + 1] = { 255, 255, 180, 128, 64 };
#endif
uint8_t bulletHue = 0;
unsigned bulletPos = bulletWidth;

void updatePanel3() {
  if (comms.Input3Active) {
    // panel just became active
    if (!panelActive[PANEL3]) {
      panelActive[PANEL3] = true;
    }
    panelLastActive[PANEL3] = millis();
  }

  if (!panelActive[PANEL3]) {
    return;
  }

  long bulletDelay = config.bulletBaseDelay;
  if (comms.InputCrankSpeed > 0.1) {
    bulletDelay = (long) (bulletDelay / comms.InputCrankSpeed);
  }
  //Serial.printlnf("crank=%f base=%d delay=%d", comms.InputCrankSpeed, config.bulletBaseDelay, bulletDelay);

  if (millis() - panelLastUpdate[PANEL3] < bulletDelay) {
    return;
  }
  panelLastUpdate[PANEL3] = millis();

  // Draw a bullet that moves across the panel faster the faster the
  // user spins the crank. The bullet shifts colors as it moves
  if (comms.InputCrankSpeed > 0) {
    bulletHue++;
    bulletPos++;
  }
  if (bulletPos >= config.panelSize[PANEL3] - bulletWidth) {
    bulletPos = bulletWidth;
  }

  clearPanel(PANEL3);
  unsigned first = config.panelFirst[PANEL3];
  for (int i = -bulletWidth; i <= bulletWidth; i++) {
    // The bullet is all one hue, but the value decreases at the extremities
    uint8_t value = bulletValue[i < 0 ? -i : i];
    RgbColor rgb = HsvToRgb(HsvColor(bulletHue, 255, value));
    uint32_t c = strip.Color(rgb.r, rgb.g, rgb.b);
    panelPixels[first + bulletPos + i] = c;
  }

  if (millis() - panelLastActive[PANEL3] > config.activeTimeout * 1000) {
    panelActive[PANEL3] = false;
  }
}

/* Panel 4 interaction: hue slider
 */

uint8_t leftHue = 0;
uint8_t rightHue = 0;

void updatePanel4() {
  if (comms.Input4Active) {
    // panel just became active
    if (!panelActive[PANEL4]) {
      panelActive[PANEL4] = true;
    }
    panelLastActive[PANEL4] = millis();
  }

  if (!panelActive[PANEL4]) {
    return;
  }

  if (millis() - panelLastUpdate[PANEL4] < config.hueSliderDelay) {
    return;
  }
  panelLastUpdate[PANEL4] = millis();

  // Change left and right hue depending on the joysticks.
  // Linearly interpolate between the 2 hues
  leftHue = comms.HoverPositionLR;
  rightHue = comms.HoverPositionUD;

  unsigned first = config.panelFirst[PANEL4];
  unsigned size = config.panelSize[PANEL4];

  // Find the increment to be able to linearly interpolate between the 2 hues.
  // The basic formula is delta = (rightHue - leftHue) / (size - 1)
  // Take special care of negative numbers because these calculations
  // are done using integer arithmetic.
  uint16_t deltaHue;
  int8_t diff = rightHue - leftHue;
  if (diff >= 0) {
    deltaHue = (diff << 8) / (size - 1);
  } else {
    deltaHue = -((-diff << 8) / (size - 1));
  }
  uint16_t accumulatedHue = leftHue << 8;
  for (unsigned i = first, n = first + size; i < n; i++) {
    RgbColor rgb = HsvToRgb(HsvColor(accumulatedHue >> 8, 255, 255));
    uint32_t c = strip.Color(rgb.r, rgb.g, rgb.b);
    panelPixels[i] = c;
    accumulatedHue += deltaHue;
  }

  if (millis() - panelLastActive[PANEL4] > config.activeTimeout * 1000) {
    panelActive[PANEL4] = false;
  }
}

#ifdef MINI_MODE
const auto ATTRACT_COLOR = 0xffffff;
#else
const auto ATTRACT_COLOR = 0xff000000; // use white pixels
#endif

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
  strip.show();
}

uint32_t normalizeColor(uint32_t c) {
#ifdef MINI_MODE
  return c;
#else
  // Swap red and green for RGBW pixel
  uint8_t w = (c >> 24) & 0xff;
  uint8_t r = (c >> 16) & 0xff;
  uint8_t g = (c >> 8) & 0xff;
  uint8_t b = c & 0xff;
  return (w << 24) | (g << 16) | (r << 8) | b;
#endif
}

// Fill the lights of panel with the current active mode pattern
void displayActive(unsigned panel) {
  unsigned first = config.panelFirst[panel];
  for (unsigned i = first, n = first + config.panelSize[panel]; i < n; i++) {
    strip.setPixelColor(i, normalizeColor(panelPixels[i]));
  }
}

// Fill the lights of panel with the current attract mode pattern
void displayAttract(unsigned panel) {
  unsigned s = config.panelSize[panel];
  for (unsigned i = 0, panelPos = config.panelFirst[panel], attractPos = attractSize() - s;
      i < s;
      i++, panelPos++, attractPos++) {
    strip.setPixelColor(panelPos, attract[attractPos] ? normalizeColor(ATTRACT_COLOR) : 0);
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
    } else if (name.equals("panel1First")) {
        config.panelFirst[PANEL1] = value.toInt();
    } else if (name.equals("panel2First")) {
        config.panelFirst[PANEL2] = value.toInt();
    } else if (name.equals("panel3First")) {
        config.panelFirst[PANEL3] = value.toInt();
    } else if (name.equals("panel4First")) {
        config.panelFirst[PANEL4] = value.toInt();
    } else if (name.equals("panel1Size")) {
        config.panelSize[PANEL1] = value.toInt();
    } else if (name.equals("panel2Size")) {
        config.panelSize[PANEL2] = value.toInt();
    } else if (name.equals("panel3Size")) {
        config.panelSize[PANEL3] = value.toInt();
    } else if (name.equals("panel4Size")) {
        config.panelSize[PANEL4] = value.toInt();
    } else if (name.equals("activeTimeout")) {
        config.activeTimeout = value.toInt();
    } else if (name.equals("colorFillDelay")) {
        config.colorFillDelay = value.toInt();
    } else if (name.equals("colorFlowDelay")) {
        config.colorFlowDelay = value.toInt();
    } else if (name.equals("bulletBaseDelay")) {
        config.bulletBaseDelay = value.toInt();
    } else if (name.equals("hueSliderDelay")) {
        config.hueSliderDelay = value.toInt();
    } else {
        return -2;
    }

    storage.store(config);
    return 0;
}

