/*
 * Panel 3: Rotary encoder and ball wheel
 * 
 * Pin usage:
 * D0: Ball detector laser enable
 * D1, D2: CAN bus
 * D3, D4: Input crank encoder (may be changed)
 * D5, D6: Ball wheel encoder (may be changed)
 * A1: Ball detector 1
 * A2: Ball detector 2
 */

#include "mf2017-can.h"
#include "simple-storage.h"
#include "Encoder.h"
#include "beam-detector.h"

SYSTEM_THREAD(ENABLED);

/* Define AUTO_START to true during testing and false during the event
 * It decides whether to wait for the MachineStart flag to turn on the
 * mechanisms or auto-start everything at boot.
 */
const bool AUTO_START = true;

// The main flag to enable or disable mechanisms
bool runMachine = AUTO_START;

/* CAN communication
 */
BuiltinCAN can(CAN_D1_D2);
Communication comms(can);

/* Input crank encoder
 */
Encoder inputEncoder(D3, D4);

/* Ball wheel encoder
 */
Encoder wheelEncoder(D5, D6);

/* Ball detectors
 */
BeamDetector detector1(A1, D0);
BeamDetector detector2(A2, D0);

/* Config stored in EEPROM
 */
struct Config {
  uint32_t app;
  uint32_t ballCount;
  uint16_t inputCrankPulsePerRevolution;
} config;

Config defaultConfig = {
  /* app */ APP_CODE('M', 'F', '3', 0), // increment last digit to reset EEPROM on boot
  /* ballCount */ 0,
  /* inputCrankPulsePerRevolution */ 600,
};

Storage<Config> storage(defaultConfig);

/* SETUP
 */

void setup() {
  Serial.begin();
  loadStorage();
  setupComms();
  setupDetectors();
}

/* LOOP
 */

void loop() {
  receiveComms();
  registerCloud();
  updateDetectors();
  readInputCrank();
  controlMotor();
  printStatus();
  transmitComms();
  storeBallCount();
}

/* Beam detectors
 */

bool beamBroken1 = false;
bool beamBroken2 = false;
uint32_t ballCount = 0;

void setupDetectors() {
  detector1.begin(BeamDetector::POWER_OFF);
  detector2.begin(BeamDetector::POWER_OFF);

  // restore ballCount from storage
  ballCount = config.ballCount;
}

void updateDetectors() {
  detector1.enable(runMachine);
  detector2.enable(runMachine);

  beamBroken1 = detector1.beamBroken();
  beamBroken2 = detector2.beamBroken();

  // Count balls using detector 1 only
  static uint32_t oldDetectionCount = 0;
  uint32_t detectionCount = detector1.detectionCount();
  if (detectionCount != oldDetectionCount) {
    ballCount += detectionCount - oldDetectionCount;
  }
}

/* Input crank
 */

uint32_t inputCrankLastUpdate = 0;
const auto inputCrankInterval = 100;
long lastInputCrankPosition = 0;
float inputCrankSpeed = 0;
float inputCrankFactor = 0;

void readInputCrank() {
  if (millis() - inputCrankLastUpdate < inputCrankInterval) {
    return;
  }

  long position = inputEncoder.read();
  long deltaPosition = position - lastInputCrankPosition;

  /* Convert pulses since the last interval to speed
   * speed_revolution_per_second = (pulses / pulses_per_revolution) / interval_ms * 1000 * ms_per_second
   */
  if (inputCrankFactor == 0) {
    // cache inputCrankFactor since it doesn't change often
    updateInputCrankFactor();
  }
  if (deltaPosition > 0) {
    inputCrankSpeed = deltaPosition * inputCrankFactor;
  } else {
    inputCrankSpeed = 0;
  }
}

void updateInputCrankFactor() {
  inputCrankFactor = 1000.0 / (config.inputCrankPulsePerRevolution * inputCrankInterval);
}

/* Motor control
 */

void controlMotor() {
  // TODO
}

/* CAN communication
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

void transmitComms() {
  comms.Input3Active = (inputCrankSpeed > 0);
  comms.InputCrankSpeed = inputCrankSpeed;

  comms.transmit(MachineModules::Panel3);
}

/* Debug printing
 */

uint32_t printLastUpdate = 0;
const auto printInterval = 100;

void printStatus() {
  if (millis() - printLastUpdate < printInterval) {
    return;
  }

  Serial.printlnf(
    "input=%f d1=%d d2=%d balls=%d",
    inputCrankSpeed,
    beamBroken1,
    beamBroken2,
    ballCount
  );
}

/* Cloud functionality
 */
void registerCloud() {
  static bool registered = false;
  if (!registered && Particle.connected()) {
    registered = true;
    Particle.function("set", updateConfig);
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

    if (name.equals("ballCount")) {
      config.ballCount = value.toInt();
      ballCount = config.ballCount;
    } else if (name.equals("inputCrankPulsePerRevolution")) {
      config.inputCrankPulsePerRevolution = value.toInt();
      // recompute cached factor
      updateInputCrankFactor();
    } else {
      return -2;
    }

    storage.store(config);
    return 0;
}

/* Storage
 */

void loadStorage() {
  storage.load(config);
}

// Write ball count to EEPROM every N balls
void storeBallCount() {
  static const auto STORE_BALLS_DELTA = 100;
  if (ballCount - config.ballCount > STORE_BALLS_DELTA) {
    config.ballCount = ballCount;
    storage.store(config);
  }
}
