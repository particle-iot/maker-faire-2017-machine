/*
 * Panel 1: 3 Big Buttons
 * 
 */

#include "mf2017-can.h"
#include "simple-storage.h"
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

const auto CAN_BUS_PINS = CAN_D1_D2;
const auto HOPPER_PIN = D3;
const auto TURBINES_PIN = D4;
const auto BALL_PUMP_PIN = D5;
// D6 spare relay shield pin

const auto INPUT_BALL_DETECTOR_PIN = A0;
const auto BALL_DETECTOR_1_PIN = A1;
const auto BALL_DETECTOR_2_PIN = A2;
const auto BALL_DETECTOR_3_PIN = A3;
const auto BUTTON_RED_PIN = A4;
const auto BUTTON_GREEN_PIN = A5;
const auto BUTTON_BLUE_PIN = A6;
const auto LASER_ENABLE_PIN = A7;
const auto SERVO_PIN = TX;

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
  uint32_t ballCount1;
  uint32_t ballCount2;
  uint32_t ballCount3;
  uint8_t servoPos1;
  uint8_t servoPos2;
  uint8_t servoPos3;
  uint16_t trackSelectTime;
  uint16_t ballPumpRunTime;
  uint16_t hopperStartDelay;
  uint16_t hopperStopDelay;
  uint16_t hopperMaxRunTime;
  uint16_t waitTime;
} config;

Config defaultConfig = {
  /* app */ APP_CODE('M', 'F', '1', 1), // increment last digit to reset EEPROM on boot
  /* ballCount1 */ 0,
  /* ballCount2 */ 0,
  /* ballCount3 */ 0,
  /* servoPos1 */ 60,
  /* servoPos2 */ 80,
  /* servoPos3 */ 100,
  /* trackSelectTime */ 500,
  /* ballPumpRunTime */ 3000,
  /* hopperStartDelay */ 2000,
  /* hopperStopDelay */ 1000,
  /* hopperMaxRunTime */ 5000,
  /* waitTime */ 2000,
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
  setupButtons();
  setupBallPump();
  setupTurbines();
  setupHopper();
  setupServos();
}

/*
 * LOOP
 */

void loop() {
  receiveComms();
  registerCloud();
  updateDetectors();
  updateButtons();
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
BeamDetector detector1(BALL_DETECTOR_1_PIN, LASER_ENABLE_PIN);
BeamDetector detector2(BALL_DETECTOR_2_PIN, LASER_ENABLE_PIN);
BeamDetector detector3(BALL_DETECTOR_3_PIN, LASER_ENABLE_PIN);
BeamDetector inputDetector(INPUT_BALL_DETECTOR_PIN, LASER_ENABLE_PIN);

bool beamBroken1 = false;
bool beamBroken2 = false;
bool beamBroken3 = false;
bool beamBrokenInput = false;
uint32_t ballCount1 = 0;
uint32_t ballCount2 = 0;
uint32_t ballCount3 = 0;

void setupDetectors() {
  detector1.begin(BeamDetector::POWER_OFF);
  detector2.begin(BeamDetector::POWER_OFF);
  detector3.begin(BeamDetector::POWER_OFF);
  inputDetector.begin(BeamDetector::POWER_OFF);

  // restore ballCounts from storage
  ballCount1 = config.ballCount1;
  ballCount2 = config.ballCount2;
  ballCount3 = config.ballCount3;
}

void updateDetectors() {
  detector1.enable(runMachine);
  detector2.enable(runMachine);
  detector3.enable(runMachine);
  inputDetector.enable(runMachine);

  beamBroken1 = detector1.beamBroken();
  beamBroken2 = detector2.beamBroken();
  beamBroken3 = detector3.beamBroken();
  beamBrokenInput = inputDetector.beamBroken();

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
 * Big buttons!
 */
bool redButtonPressed = false;
bool greenButtonPressed = false;
bool blueButtonPressed = false;

const auto BUTTON_PRESSED = LOW;

void setupButtons() {
  const auto mode = INPUT_PULLUP;
  pinMode(BUTTON_RED_PIN, mode);
  pinMode(BUTTON_GREEN_PIN, mode);
  pinMode(BUTTON_BLUE_PIN, mode);
}

void updateButtons() {
  redButtonPressed = digitalRead(BUTTON_RED_PIN) == BUTTON_PRESSED;
  greenButtonPressed = digitalRead(BUTTON_GREEN_PIN) == BUTTON_PRESSED;
  blueButtonPressed = digitalRead(BUTTON_BLUE_PIN) == BUTTON_PRESSED;
}

/*
 * Ball pump
 */

const auto BALL_PUMP_RUN = HIGH;
const auto BALL_PUMP_STOP = LOW;

bool runBallPump = false;

void setupBallPump() {
  pinMode(BALL_PUMP_PIN, OUTPUT);
  digitalWrite(BALL_PUMP_PIN, BALL_PUMP_STOP);
}

void controlBallPump() {
  digitalWrite(BALL_PUMP_PIN, runMachine && runBallPump ? BALL_PUMP_RUN : BALL_PUMP_STOP);
}

/*
 * Wind turbines
 */

const auto TURBINES_RUN = HIGH;
const auto TURBINES_STOP = LOW;

bool runTurbines = false;

void setupTurbines() {
  pinMode(TURBINES_PIN, OUTPUT);
  digitalWrite(TURBINES_PIN, TURBINES_STOP);
}

void controlTurbines() {
  digitalWrite(TURBINES_PIN, runMachine && runTurbines ? TURBINES_RUN : TURBINES_STOP);
}

/*
 * Ball hopper
 */

const auto HOPPER_STOP = LOW;
const auto HOPPER_RUN = HIGH;

bool runHopper = false;

enum HopperState_e {
  HOPPER_IDLE,
  HOPPER_RUNNING,
  HOPPER_TIMEOUT
} hopperState = HOPPER_IDLE;

uint32_t lastInputDetectionTime = 0;
uint32_t lastInputGapTime = 0;
uint32_t hopperStartTime = 0;

void setupHopper() {
  pinMode(HOPPER_PIN, OUTPUT);
  digitalWrite(HOPPER_PIN, HOPPER_STOP);
}

void controlHopper() {
  // Run the hopper when there's no ball in front of the detector for a while
  auto now = millis();
  if (beamBrokenInput) {
    lastInputDetectionTime = now;
  } else {
    lastInputGapTime = now;
  }

  switch (hopperState) {
    case HOPPER_IDLE:
      if (now - lastInputDetectionTime > config.hopperStartDelay) {
        hopperState = HOPPER_RUNNING;
        hopperStartTime = now;
        runHopper = true;
      }
      break;
    case HOPPER_RUNNING:
      if (beamBrokenInput && now - lastInputGapTime > config.hopperStopDelay) {
        hopperState = HOPPER_IDLE;
        runHopper = false;
      //} else if (now - hopperStartTime > config.hopperMaxRunTime) {
      //  hopperState = HOPPER_TIMEOUT;
      }
      break;

    // FIXME: timeout was meant for case where the input is full but the
    // beam is not broken. Disabled because there's no good way to get
    // out of a timeout
    //case HOPPER_TIMEOUT:
    //  if (beamBrokenInput) {
    //    hopperState = HOPPER_IDLE;
    //  }
    //  break;
  }

  digitalWrite(HOPPER_PIN, runMachine && runHopper ? HOPPER_RUN : HOPPER_STOP);
}

/*
 * Servos
 */
Servo servo;

uint8_t servoPos = 0;
bool autoServo = true;

enum TrackSelectorState_e {
  TRACK_1,
  TRACK_2,
  TRACK_3
} trackSelectorState = TRACK_1;

void setupServos() {
  servoPos = config.servoPos1;
  servo.attach(SERVO_PIN);
  servo.write(servoPos);
}

void controlServos() {
  // Update the position of the servo from the desired active track
  if (autoServo) {
    switch (trackSelectorState) {
      case TRACK_1:
        servoPos = config.servoPos1;
        break;
      case TRACK_2:
        servoPos = config.servoPos2;
        break;
      case TRACK_3:
        servoPos = config.servoPos3;
        break;
    }
  }

  if (runMachine) {
    servo.write(servoPos);
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
  comms.Input1Active = redButtonPressed || greenButtonPressed || blueButtonPressed;
  comms.RedButtonPressed = redButtonPressed;
  comms.GreenButtonPressed = greenButtonPressed;
  comms.BlueButtonPressed = blueButtonPressed;

  comms.BallCount3 = ballCount1 + ballCount2 + ballCount3;
  comms.RedBallCount = ballCount1;
  comms.GreenBallCount = ballCount2;
  comms.BlueBallCount = ballCount3;

  comms.transmit(MachineModules::Panel1);
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
    Particle.function("servo", overrideServoPos);
    Particle.function("pump", overrideBallPump);
    Particle.function("hopper", overrideHopper);
    Particle.function("turbines", overrideTurbines);
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
    } else if (name.equals("servoPos1")) {
      config.servoPos1 = value.toInt();
    } else if (name.equals("servoPos2")) {
      config.servoPos2 = value.toInt();
    } else if (name.equals("servoPos3")) {
      config.servoPos3 = value.toInt();
    } else if (name.equals("trackSelectTime")) {
      config.trackSelectTime = value.toInt();
    } else if (name.equals("ballPumpRunTime")) {
      config.ballPumpRunTime = value.toInt();
    } else if (name.equals("hopperStartDelay")) {
      config.hopperStartDelay = value.toInt();
    } else if (name.equals("hopperStopDelay")) {
      config.hopperStopDelay = value.toInt();
    } else if (name.equals("hopperMaxRunTime")) {
      config.hopperMaxRunTime = value.toInt();
    } else if (name.equals("waitTime")) {
      config.waitTime = value.toInt();
    } else {
      return -2;
    }

    storage.store(config);
    return 0;
}

int overrideServoPos(String arg) {
  servoPos = arg.toInt();
  autoServo = false;
  return 0;
}

int overrideBallPump(String arg) {
  runBallPump = arg.toInt();
  return 0;
}

int overrideHopper(String arg) {
  runHopper = arg.toInt();
  return 0;
}

int overrideTurbines(String arg) {
  runTurbines = arg.toInt();
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
