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
  uint16_t gateDelay;
  uint16_t interactionTimeout;
  uint16_t topServoOpenPos;
  uint16_t topServoClosedPos;
  uint16_t middleServoOpenPos;
  uint16_t middleServoClosedPos;
  uint16_t bottomServoOpenPos;
  uint16_t bottomServoClosedPos;
} config;

Config defaultConfig = {
  /* app */ APP_CODE('M', 'F', '2', 0), // increment last digit to reset EEPROM on boot
  /* ballCount */ 0,
  /* ballPumpStartDelay */ 500,
  /* ballPumpStopDelay */ 2000,
  /* gateDelay */ 750,
  /* interactionTimeout */ 5000,
  /* topServoOpenPos */ 45,
  /* topServoClosedPos */ 89,
  /* middleServoOpenPos */ 45,
  /* middleServoClosedPos */ 89,
  /* bottomServoOpenPos */ 135,
  /* bottomServoClosedPos */ 91,
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
  doPanelControl();
  controlBallPump();
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

uint16_t red = 0;
uint16_t green = 0;
uint16_t blue = 0;
uint16_t clear = 0;

uint32_t colorSensorLastUpdate = 0;
uint32_t colorSensorInterval = 100;

void setupColorSensor() {
  tcs.begin();
  tcs.setInterrupt(false);
}

void updateColorSensor() {
  auto now = millis();
  if (now - colorSensorLastUpdate < colorSensorInterval) {
    return;
  }
  colorSensorLastUpdate = now;
  
  tcs.getRawData(&red, &green, &blue, &clear);
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
  WAIT_FOR_COLOR_1,
  COLOR_1_MATCHED,
  WAIT_FOR_COLOR_2,
  COLOR_2_MATCHED,
  WAIT_FOR_COLOR_3,
  COLOR_3_MATCHED
} panelState = WAIT_FOR_COLOR_1;

uint32_t panelTime = 0;
bool panelActive = false;
uint16_t matchedColor = 0;
#define ORANGE_HUE 35
#define CYAN_HUE 180
#define MAGENTA_HUE 300

void doPanelControl() {
  auto now = millis();

  switch (panelState) {
    case WAIT_FOR_COLOR_1:
      // match orange
      if (red > 2 * blue && green > blue) {
        panelState = COLOR_1_MATCHED;
        gatesState = TOP_GATE_OPEN;
        matchedColor = ORANGE_HUE;
        panelTime = now;
      }
      break;
    case COLOR_1_MATCHED:
      if (now - panelTime > config.gateDelay) {
        panelState = WAIT_FOR_COLOR_2;
        gatesState = ALL_GATES_CLOSED;
      }
      break;
    case WAIT_FOR_COLOR_2:
      // match cyan
      if (blue > 2*red && green > red) {
        panelState = COLOR_2_MATCHED;
        gatesState = MIDDLE_GATE_OPEN;
        matchedColor = CYAN_HUE;
        panelTime = now;
      }
      break;
    case COLOR_2_MATCHED:
      if (now - panelTime > config.gateDelay) {
        panelState = WAIT_FOR_COLOR_3;
        gatesState = ALL_GATES_CLOSED;
      }
      break;
    case WAIT_FOR_COLOR_3:
      // match magenta
      if (red > green && blue > green) {
        panelState = COLOR_3_MATCHED;
        gatesState = BOTTOM_GATE_OPEN;
        matchedColor = MAGENTA_HUE;
        panelTime = now;
      }
      break;
    case COLOR_3_MATCHED:
      if (now - panelTime > config.gateDelay) {
        panelState = WAIT_FOR_COLOR_1;
        gatesState = ALL_GATES_CLOSED;
      }
      break;
  }

  panelActive = now - panelTime < config.interactionTimeout;
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
  comms.Input2Active = panelActive;
  comms.InputColorHue = matchedColor;
  comms.BallCount2 = ballCount;

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
    "rgb=%d/%d/%d st=%d pump=%d servos=%d/%d/%d beams=%d/%d balls=%d active=%d",
    red,
    green,
    blue,
    panelState,
    runBallPump,
    topServoPos,
    middleServoPos,
    bottomServoPos,
    beamBrokenTop,
    beamBrokenInput,
    ballCount,
    panelActive
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
    } else if (name.equals("gateDelay")) {
      config.gateDelay = value.toInt();
    } else if (name.equals("interactionTimeout")) {
      config.interactionTimeout = value.toInt();
    } else if (name.equals("topServoOpenPos")) {
      config.topServoOpenPos = value.toInt();
    } else if (name.equals("topServoClosedPos")) {
      config.topServoClosedPos = value.toInt();
    } else if (name.equals("middleServoOpenPos")) {
      config.middleServoOpenPos = value.toInt();
    } else if (name.equals("middleServoClosedPos")) {
      config.middleServoClosedPos = value.toInt();
    } else if (name.equals("bottomServoOpenPos")) {
      config.bottomServoOpenPos = value.toInt();
    } else if (name.equals("bottomServoClosedPos")) {
      config.bottomServoClosedPos = value.toInt();
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
