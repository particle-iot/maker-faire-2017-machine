/*
 * Panel 2: Color wheel
 * 
 */

#include "mf2017-can.h"
#include "simple-storage.h"
#include "beam-detector.h"
#include "Adafruit_TCS34725.h"
#include "math.h"

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

/* Color sensor SDA D0 */
/* Color sensor SCL D1 */
const auto BALL_PUMP_PIN = D3;

const auto LASER_ENABLE_PIN = A0;
const auto BALL_DETECTOR_TOP_PIN = A1;
const auto BALL_DETECTOR_INPUT_PIN = A2;
const auto TOP_SERVO_PIN = A4;
const auto BOTTOM_SERVO_PIN = A5;
const auto MIDDLE_SERVO_PIN = WKP;

const auto CAN_BUS_PINS = CAN_C4_C5;

/*
 * CAN communication
 */
BuiltinCAN can(CAN_BUS_PINS);
Communication comms(can);

/*
 * Config stored in EEPROM
 */
struct Config {
  uint32_t app;
  uint32_t ballCount;
  uint16_t ballPumpStartDelay;
  uint16_t ballPumpStopDelay;
} config;

Config defaultConfig = {
  /* app */ APP_CODE('M', 'F', '2', 1), // increment last digit to reset EEPROM on boot
  /* ballCount */ 0,
  /* ballPumpStartDelay */ 500,
  /* ballPumpStopDelay */ 2000,
};

Storage<Config> storage(defaultConfig);

/*
 * SETUP
 */

void setup() {
  Serial.begin();
  loadStorage();
  setupComms();
  setupColorSensor();
  setupBallPump();
  setupDetectors();
  setupServos();
}

/*
 * LOOP
 */

void loop() {
  receiveComms();
  registerCloud();
  updateDetectors();
  updateColorSensor();
  controlHopper();
  doPanelControl();
  controlBallPump();
  controlTurbines();
  controlServos();
  printStatus();
  transmitComms();
  storeBallCount();
}

/*
 * Ball detectors
 */
BeamDetector detectorTop(BALL_DETECTOR_TOP_PIN, LASER_ENABLE_PIN);
BeamDetector detectorInput(BALL_DETECTOR_INPUT_PIN, LASER_ENABLE_PIN);

bool beamBrokenTop = false;
bool beamBrokenInput = false;
uint32_t ballCount = 0;

void setupDetectors() {
  detectorInput.begin(BeamDetector::POWER_OFF);
  detectorTop.begin(BeamDetector::POWER_OFF);

  // restore ballCounts from storage
  ballCount = config.ballCount;
}

void updateDetectors() {
  detectorTop.enable(runMachine);
  detectorInput.enable(runMachine);

  beamBrokenTop = detectorTop.beamBroken();
  beamBrokenInput = detectorInput.beamBroken();

  // Count balls
  static uint32_t oldDetectionCount = 0;
  uint32_t detectionCount = detectorTop.detectionCount();
  if (detectionCount != oldDetectionCount) {
    ballCount += detectionCount - oldDetectionCount;
  }
  oldDetectionCount = detectionCount;
}

/*
 * Color sensor
 */

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

void setupColorSensor() {
  tcs.begin();
  tcs.setInterrupt(false);
}

void updateColorSensor() {

}

/*
 * Ball pump
 */

const auto BALL_PUMP_RUN = HIGH;
const auto BALL_PUMP_STOP = LOW;

bool runBallPump = false;

enum BallPumpState_e {
  BALL_PUMP_IDLE,
  BALL_PUMP_RUNNING
} ballPumpState = BALL_PUMP_IDLE;

uint32_t lastInputDetectionTime = 0;
uint32_t lastInputGapTime = 0;
uint32_t ballPumpStartTime = 0;

void setupBallPump() {
  pinMode(BALL_PUMP_PIN, OUTPUT);
  digitalWrite(BALL_PUMP_PIN, BALL_PUMP_STOP);
}

void controlBallPump() {
  // Run the ball pump when there's no ball in front of the detector for a while
  auto now = millis();
  if (beamBrokenInput) {
    lastInputDetectionTime = now;
  } else {
    lastInputGapTime = now;
  }

  switch (ballPumpState) {
    case BALL_PUMP_IDLE:
      if (now - lastInputDetectionTime > config.ballPumpStartDelay) {
        ballPumpState = BALL_PUMP_RUNNING;
        ballPumpStartTime = now;
        runBallPump = true;
      }
      break;
    case BALL_PUMP_RUNNING:
      if (beamBrokenInput && now - lastInputGapTime > config.ballPumpStopDelay) {
        ballPumpState = BALL_PUMP_IDLE;
        runBallPump = false;
      }
      break;
  }

  digitalWrite(BALL_PUMP_PIN, runMachine && runBallPump ? BALL_PUMP_RUN : BALL_PUMP_STOP);
}

/*
 * Servo gates
 */

enum GatesState_e {
  ALL_GATES_CLOSED,
  TOP_GATE_OPEN,
  MIDDLE_GATE_OPEN,
  BOTTOM_GATE_OPEN
} gatesState = ALL_GATES_CLOSED;

Servo topServo;
Servo middleServo;
Servo bottomServo;

uint8_t topServoPos = 0;
uint8_t middleServoPos = 0;
uint8_t bottomServoPos = 0;
bool autoServo = true;

void setupServos() {
  topServoPos = config.topServoClosedPos;
  middleServoPos = config.middleServoClosedPos;
  bottomServoPos = config.bottomServoClosedPos;

  topServo.attach(TOP_SERVO_PIN);
  middleServo.attach(MIDDLE_SERVO_PIN);
  bottomServo.attach(BOTTOM_SERVO_PIN);

  topServo.write(topServoPos);
  middleServo.write(middleServoPos);
  bottomServo.write(bottomServoPos);
}

void controlServos() {
  // Update the position of the servo from the desired active track
  if (autoServo) {
    switch (gatesState) {
      case ALL_GATES_CLOSED:
        topServoPos = config.topServoClosedPos;
        middleServoPos = config.middleServoClosedPos;
        bottomServoPos = config.bottomServoClosedPos;
        break;
      case TOP_GATE_OPEN:
        topServoPos = config.topServoOpenPos;
        middleServoPos = config.middleServoClosedPos;
        bottomServoPos = config.bottomServoClosedPos;
        break;
      case MIDDLE_GATE_OPEN:
        topServoPos = config.topServoOpenPos;
        middleServoPos = config.middleServoOpenPos;
        bottomServoPos = config.bottomServoClosedPos;
        break;
      case BOTTOM_GATE_OPEN:
        topServoPos = config.topServoOpenPos;
        middleServoPos = config.middleServoOpenPos;
        bottomServoPos = config.bottomServoOpenPos;
        break;
    }
  }

  if (runMachine) {
    topServo.write(topServoPos);
    middleServo.write(middleServoPos);
    bottomServo.write(bottomServoPos);
  }
}

/*
 * Main panel control
 */

enum PanelState_e {
  PANEL_IDLE,
  PANEL_SELECT_TRACK,
  PANEL_RUN_PUMP,
  PANEL_WAIT,
} panelState = PANEL_IDLE;

uint32_t trackSelectStartTime = 0;
uint32_t ballPumpStartTime = 0;
uint32_t waitStartTime = 0;

void doPanelControl() {
  auto now = millis();

  switch (panelState) {
    case PANEL_IDLE:
      if (redButtonPressed || greenButtonPressed || blueButtonPressed) {
        panelState = PANEL_SELECT_TRACK;
        trackSelectStartTime = now;
        if (blueButtonPressed) {
          trackSelectorState = TRACK_1;
          runTurbines = true;
        } else if (redButtonPressed) {
          trackSelectorState = TRACK_2;
        } else if (greenButtonPressed) {
          trackSelectorState = TRACK_3;
        }
      }
      break;

    case PANEL_SELECT_TRACK:
      if (now - trackSelectStartTime > config.trackSelectTime) {
        panelState = PANEL_RUN_PUMP;
        ballPumpStartTime = now;
        runBallPump = true;
      }
      break;

    case PANEL_RUN_PUMP:
      if (now - ballPumpStartTime > config.ballPumpRunTime) {
        panelState = PANEL_WAIT;
        runBallPump = false;
      }
      break;

    case PANEL_WAIT:
      if (now - waitStartTime > config.waitTime) {
        panelState = PANEL_IDLE;
      }
      break;
  }

  if (runTurbines && beamBroken1) {
    runTurbines = false;
  }
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
  comms.Input2Active = false /* TODO */;
  comms.RedButtonPressed = redButtonPressed;
  comms.GreenButtonPressed = greenButtonPressed;
  comms.BlueButtonPressed = blueButtonPressed;

  comms.BallCount3 = ballCount1 + ballCount2 + ballCount3;
  comms.RedBallCount = ballCount1;
  comms.GreenBallCount = ballCount2;
  comms.BlueBallCount = ballCount3;

  comms.transmit(MachineModules::Panel2);
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
    "rgb=%d/%d/%d st=%d hop=%d pump=%d turb=%d servo=%d beams=%d/%d/%d/%d balls=%d/%d/%d",
    redButtonPressed,
    greenButtonPressed,
    blueButtonPressed,
    panelState,
    runHopper,
    runBallPump,
    runTurbines,
    servoPos,
    beamBroken1,
    beamBroken2,
    beamBroken3,
    beamBrokenInput,
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
    Particle.function("servo1", overrideServo1Pos);
    Particle.function("servo2", overrideServo2Pos);
    Particle.function("servo3", overrideServo3Pos);
    Particle.function("pump", overrideBallPump);
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
    } else if (name.equals("ballPumpStartDelay")) {
      config.ballPumpStartDelay = value.toInt();
    } else if (name.equals("ballPumpStopDelay")) {
      config.ballPumpStopDelay = value.toInt();
    } else {
      return -2;
    }

    storage.store(config);
    return 0;
}

int overrideServo1Pos(String arg) {
  topServoPos = arg.toInt();
  autoServo = false;
  return 0;
}

int overrideServo2Pos(String arg) {
  middleServoPos = arg.toInt();
  autoServo = false;
  return 0;
}

int overrideServo3Pos(String arg) {
  bottomServoPos = arg.toInt();
  autoServo = false;
  return 0;
}

int overrideBallPump(String arg) {
  runBallPump = arg.toInt();
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
  if (ballCount - config.ballCount >= STORE_BALLS_DELTA) {
    config.ballCount = ballCount;
    storage.store(config);
  }
}
