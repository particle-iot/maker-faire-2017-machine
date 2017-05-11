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
const auto INPUT_CRANK_ENCODER_B_PIN = D3;
const auto INPUT_CRANK_ENCODER_A_PIN = D4;
const auto BALL_WHEEL_ENCODER_B_PIN = D5;
const auto BALL_WHEEL_ENCODER_A_PIN = D6;
const auto SERVO_ENABLE_PIN = A0;
const auto BALL_DETECTOR_1_PIN = A1;
const auto BALL_DETECTOR_2_PIN = A2;
const auto BALL_WHEEL_KILL_SWITCH_PIN = A3;
const auto SERVO_1_PIN = A4;
const auto SERVO_2_PIN = A5;
const auto BALL_WHEEL_MOTOR_SPEED_PIN = DAC;
const auto BALL_DETECTOR_3_PIN = WKP;

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
BeamDetector detector3(BALL_DETECTOR_3_PIN, LASER_ENABLE_PIN);

/*
 * Servos
 */

Servo servo1;
Servo servo2;

/*
 * Config stored in EEPROM
 */
struct Config {
  uint32_t app;
  uint32_t ballCount1;
  uint32_t ballCount2;
  uint32_t ballCount3;
  uint16_t inputCrankPulsesPerRevolution;
  uint16_t inputCrankInterval;
  float ballWheelSpeedFactor;
  uint16_t ballWheelMinSpeed;
  uint16_t ballWheelMaxSpeed;
  uint16_t wheelPulsesPerBall;
  uint8_t servo1OpenPos;
  uint8_t servo1ClosedPos;
  uint8_t servo2OpenPos;
  uint8_t servo2ClosedPos;
} config;

Config defaultConfig = {
  /* app */ APP_CODE('M', 'F', '3', 3), // increment last digit to reset EEPROM on boot
  /* ballCount1 */ 0,
  /* ballCount2 */ 0,
  /* ballCount3 */ 0,
  /* inputCrankPulsesPerRevolution */ 600,
  /* inputCrankInterval */ 100,
  /* ballWheelSpeedFactor */ 600.0,
  /* ballWheelMinSpeed */ 200,
  /* ballWheelMaxSpeed */ 4000,
  /* wheelPulsesPerBall */ 40,
  /* servo1OpenPos */ 20,
  /* servo1ClosedPos */ 160,
  /* servo2OpenPos */ 20,
  /* servo2ClosedPos */ 160,
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
  setupServos();
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
  controlServos();
  printStatus();
  transmitComms();
  storeBallCount();
}

/*
 * Beam detectors
 */

bool beamBroken1 = false;
bool beamBroken2 = false;
bool beamBroken3 = false;
uint32_t ballCount1 = 0;
uint32_t ballCount2 = 0;
uint32_t ballCount3 = 0;

void setupDetectors() {
  detector1.begin(BeamDetector::POWER_OFF);
  detector2.begin(BeamDetector::POWER_OFF);
  detector3.begin(BeamDetector::POWER_OFF);

  // restore ballCounts from storage
  ballCount1 = config.ballCount1;
  ballCount2 = config.ballCount2;
  ballCount3 = config.ballCount3;
}

void updateDetectors() {
  detector1.enable(runMachine);
  detector2.enable(runMachine);
  detector3.enable(runMachine);

  beamBroken1 = detector1.beamBroken();
  beamBroken2 = detector2.beamBroken();
  beamBroken3 = detector3.beamBroken();

  // Count balls
  static uint32_t oldDetectionCount1 = 0;
  uint32_t detectionCount1 = detector1.detectionCount();
  if (detectionCount1 != oldDetectionCount1) {
    ballCount1 += detectionCount1 - oldDetectionCount1;
  }
  oldDetectionCount1 = detectionCount1;

  static uint32_t oldDetectionCount2 = 0;
  uint32_t detectionCount2 = detector2.detectionCount();
  if (detectionCount2 != oldDetectionCount2) {
    ballCount2 += detectionCount2 - oldDetectionCount2;
  }
  oldDetectionCount2 = detectionCount2;

  static uint32_t oldDetectionCount3 = 0;
  uint32_t detectionCount3 = detector3.detectionCount();
  if (detectionCount3 != oldDetectionCount3) {
    ballCount3 += detectionCount3 - oldDetectionCount3;
  }
  oldDetectionCount3 = detectionCount3;
}

/*
 * Input crank
 */

uint32_t inputCrankLastUpdate = 0;
long lastInputCrankPosition = 0;
float inputCrankSpeed = 0;
float inputCrankFactor = 0;

void readInputCrank() {
  auto now = millis();
  if (now - inputCrankLastUpdate < config.inputCrankInterval) {
    return;
  }
  inputCrankLastUpdate = now;

  long position = inputEncoder.read();
  long deltaPosition = position - lastInputCrankPosition;
  lastInputCrankPosition = position;

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
  //Serial.printlnf("pos=%d delta=%d speed=%f", position, deltaPosition, inputCrankSpeed);
}

void updateInputCrankFactor() {
  inputCrankFactor = 1000.0 / (config.inputCrankPulsesPerRevolution * config.inputCrankInterval);
}

/*
 * Motor control
 */

const auto MOTOR_RUN = LOW;
const auto MOTOR_KILL = HIGH;

// turn stall detection off during development if the wheel encoder is not connected
const auto ENABLE_STALL_DETECTION = true;

auto motorState = AUTO_START ? MOTOR_RUN : MOTOR_KILL;
uint32_t motorSpeed = 0;

uint32_t motorControlLastUpdate = 0;
const auto motorControlInterval = 100;
const auto motorStallMaxTime = 1000;
long wheelPosition = 0;
long oldWheelPosition = 0;
uint16_t motorStallCounter = 0;

void setupMotor() {
  pinMode(BALL_WHEEL_KILL_SWITCH_PIN, OUTPUT);
  digitalWrite(BALL_WHEEL_KILL_SWITCH_PIN, MOTOR_KILL);
  pinMode(BALL_WHEEL_MOTOR_SPEED_PIN, OUTPUT);
}

void controlMotor() {
  auto now = millis();
  if (now - motorControlLastUpdate < motorControlInterval) {
    return;
  }
  motorControlLastUpdate = now;

  // Scale input crank speed to a voltage in the 0-3.3V range to send to
  // the motor controller
  uint16_t maxSpeed = min(config.ballWheelMaxSpeed, 4095);
  motorSpeed = (uint32_t)(inputCrankSpeed * config.ballWheelSpeedFactor) + config.ballWheelMinSpeed;

  if (motorSpeed > maxSpeed) {
    motorSpeed = maxSpeed;
  }

  wheelPosition = wheelEncoder.read();

  // Safety setting of the motor
  // if the desired motor speed > 0 and there were no pulses on the encoder for 1 second,
  // trigger the motor controller kill switch until the machine start button is pressed again
  if (ENABLE_STALL_DETECTION && inputCrankSpeed > 0) {
    if (wheelPosition == oldWheelPosition) {
      motorStallCounter++;
    }
  } else {
    motorStallCounter = 0;
  }

  // Stop the motor when the machine stop button is pressed
  // Start the motor when the machine start button is pressed
  // Stop the motor when the motor is powered but not moving
  if (!runMachine) {
    motorState = MOTOR_KILL;
  } else if (comms.MachineStart) {
    motorState = MOTOR_RUN;
  } else if (motorStallCounter > motorStallMaxTime / motorControlInterval) {
    motorState = MOTOR_KILL;
  }

  oldWheelPosition = wheelPosition;

  digitalWrite(BALL_WHEEL_KILL_SWITCH_PIN, motorState);
  analogWrite(BALL_WHEEL_MOTOR_SPEED_PIN, runMachine ? motorSpeed : 0);
}

/*
 * Servos
 */

const auto SERVO_RUN = LOW;
const auto SERVO_KILL = HIGH;

uint8_t servo1Pos = 0;
uint8_t servo2Pos = 0;

enum {
  DOORS_CLOSED,
  DOOR1_OPEN,
  DOOR2_OPEN,
} doorState = DOORS_CLOSED;

void setupServos() {
  servo1.attach(SERVO_1_PIN);
  servo2.attach(SERVO_2_PIN);
  pinMode(SERVO_ENABLE_PIN, OUTPUT);
  digitalWrite(SERVO_ENABLE_PIN, SERVO_KILL);
}

void controlServos() {
  // Change the servo position for every ball by counting the number of
  // pulses on the ball wheel
  switch (doorState) {
    case DOOR1_OPEN:
      servo1Pos = config.servo1OpenPos;
      servo2Pos = config.servo2ClosedPos;
      break;
    case DOOR2_OPEN:
      servo1Pos = config.servo1ClosedPos;
      servo2Pos = config.servo2OpenPos;
      break;
    default:
      servo1Pos = config.servo1ClosedPos;
      servo2Pos = config.servo2ClosedPos;
      break;
  }

  servo1.write(servo1Pos);
  servo2.write(servo2Pos);
  digitalWrite(SERVO_ENABLE_PIN, runMachine ? SERVO_RUN : SERVO_KILL);
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
  auto now = millis();
  if (now - printLastUpdate < printInterval) {
    return;
  }
  printLastUpdate = now;

  Serial.printlnf(
    "crank=%f wheel=%d motor=%d/%d stall=%d beams=%d/%d/%d balls=%d/%d/%d",
    inputCrankSpeed,
    wheelPosition,
    motorSpeed,
    motorState,
    motorStallCounter,
    beamBroken1,
    beamBroken2,
    beamBroken3,
    ballCount1,
    ballCount2,
    ballCount3
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
    Particle.function("door", changeDoorState);
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

    if (name.equals("ballCount1")) {
      config.ballCount1 = value.toInt();
      ballCount1 = config.ballCount1;
    } else if (name.equals("ballCount2")) {
      config.ballCount2 = value.toInt();
      ballCount2 = config.ballCount2;
    } else if (name.equals("ballCount3")) {
      config.ballCount3 = value.toInt();
      ballCount3 = config.ballCount3;
    } else if (name.equals("inputCrankPulsesPerRevolution")) {
      config.inputCrankPulsesPerRevolution = value.toInt();
      // recompute cached factor
      updateInputCrankFactor();
    } else if (name.equals("inputCrankInterval")) {
      config.inputCrankInterval = value.toInt();
      // recompute cached factor
      updateInputCrankFactor();
    } else if (name.equals("ballWheelSpeedFactor")) {
      config.ballWheelSpeedFactor = value.toFloat();
    } else if (name.equals("ballWheelMinSpeed")) {
      config.ballWheelMinSpeed = value.toInt();
    } else if (name.equals("ballWheelMaxSpeed")) {
      config.ballWheelMaxSpeed = value.toInt();
    } else if (name.equals("wheelPulsesPerBall")) {
      config.wheelPulsesPerBall = value.toInt();
    } else if (name.equals("servo1OpenPos")) {
      config.servo1OpenPos = value.toInt();
    } else if (name.equals("servo1ClosedPos")) {
      config.servo1ClosedPos = value.toInt();
    } else if (name.equals("servo2OpenPos")) {
      config.servo2OpenPos = value.toInt();
    } else if (name.equals("servo2ClosedPos")) {
      config.servo2ClosedPos = value.toInt();
    } else {
      return -2;
    }

    storage.store(config);
    return 0;
}

int changeDoorState(String arg) {
  doorState = arg.toInt();
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
  static const auto STORE_BALLS_DELTA = 20;
  if (ballCount1 - config.ballCount1 >= STORE_BALLS_DELTA) {
    config.ballCount1 = ballCount1;
    storage.store(config);
  }
  if (ballCount2 - config.ballCount2 >= STORE_BALLS_DELTA) {
    config.ballCount2 = ballCount2;
    storage.store(config);
  }
  if (ballCount3 - config.ballCount3 >= STORE_BALLS_DELTA) {
    config.ballCount3 = ballCount3;
    storage.store(config);
  }
}
