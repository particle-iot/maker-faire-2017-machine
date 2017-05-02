/*
 * Panel 3: Rotary encoder and ball wheel
 * 
 */

#include "mf2017-can.h"
#include "simple-storage.h"
#include "Encoder.h"
#include "beam-detector.h"

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

const auto LASER_ENABLE_PIN = D0;
const auto CAN_BUS_PINS = CAN_D1_D2;
const auto INPUT_CRANK_ENCODER_A_PIN = D3;
const auto INPUT_CRANK_ENCODER_B_PIN = D4;
const auto BALL_WHEEL_ENCODER_A_PIN = D5;
const auto BALL_WHEEL_ENCODER_B_PIN = D6;
const auto BALL_DETECTOR_1_PIN = A1;
const auto BALL_DETECTOR_2_PIN = A2;
const auto BALL_WHEEL_MOTOR_PWM_PIN = A5;

/*
 * CAN communication
 */
BuiltinCAN can(CAN_BUS_PINS);
Communication comms(can);

/*
 * Input crank encoder
 */
Encoder inputEncoder(INPUT_CRANK_ENCODER_A_PIN, INPUT_CRANK_ENCODER_B_PIN);

/*
 * Ball wheel encoder
 */
Encoder wheelEncoder(BALL_WHEEL_ENCODER_A_PIN, BALL_WHEEL_ENCODER_B_PIN);

/*
 * Ball detectors
 */
BeamDetector detector1(BALL_DETECTOR_1_PIN, LASER_ENABLE_PIN);
BeamDetector detector2(BALL_DETECTOR_2_PIN, LASER_ENABLE_PIN);

/*
 * Config stored in EEPROM
 */
struct Config {
  uint32_t app;
  uint32_t ballCount;
  uint16_t inputCrankPulsesPerRevolution;
  uint16_t ballWheelMotorFrequency;
} config;

Config defaultConfig = {
  /* app */ APP_CODE('M', 'F', '3', 0), // increment last digit to reset EEPROM on boot
  /* ballCount */ 0,
  /* inputCrankPulsesPerRevolution */ 600,
  /* ballWheelMotorFrequency */ 16000,
};

Storage<Config> storage(defaultConfig);

/*
 * SETUP
 */

void setup() {
  Serial.begin();
  loadStorage();
  setupComms();
  setupDetectors();
  setupMotor();
}

/*
 * LOOP
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

/*
 * Beam detectors
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

/*
 * Input crank
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
  inputCrankFactor = 1000.0 / (config.inputCrankPulsesPerRevolution * inputCrankInterval);
}

/*
 * Motor control
 */

uint16_t motorPWM = 0;

void setupMotor() {
  pinMode(BALL_WHEEL_MOTOR_PWM_PIN, OUTPUT);
}

void controlMotor() {
  // TODO

  analogWrite(BALL_WHEEL_MOTOR_PWM_PIN, runMachine ? motorPWM : 0, config.ballWheelMotorFrequency);
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

void transmitComms() {
  comms.Input3Active = (inputCrankSpeed > 0);
  comms.InputCrankSpeed = inputCrankSpeed;

  comms.transmit(MachineModules::Panel3);
}

/*
 * Debug printing
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

/*
 * Cloud functionality
 */
void registerCloud() {
  static bool registered = false;
  if (!registered && Particle.connected()) {
    registered = true;
    Particle.function("set", setConfigFromCloud);
  }
}

// Call Particle cloud function "set" with name=value to change a config value
int setConfigFromCloud(String arg) {
    int equalPos = arg.indexOf('=');
    if (equalPos < 0) {
        return -1;
    }
    String name = arg.substring(0, equalPos);
    String value = arg.substring(equalPos + 1);

    if (name.equals("ballCount")) {
      config.ballCount = value.toInt();
      ballCount = config.ballCount;
    } else if (name.equals("inputCrankPulsesPerRevolution")) {
      config.inputCrankPulsesPerRevolution = value.toInt();
      // recompute cached factor
      updateInputCrankFactor();
    } else if (name.equals("ballWheelMotorFrequency")) {
      config.ballWheelMotorFrequency = value.toInt();
    } else {
      return -2;
    }

    storage.store(config);
    return 0;
}

/*
 * Persistent storage
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
