#include "Hover.h"

#define STEP_CLOCK_DURATION 1

#define STEPPER_DIRECTION_PIN A5
#define STEPPER_STEP_PIN A4
#define STEPPER_EN_PIN A3

#define BUMP_BTN_PIN1 A0
#define BUMP_BTN_PIN2 A1

//TODO: What are these really?
#define BALL_PUMP_PIN D5
#define BALLS_FULL_PIN D6

#define STEPPER_DIRECTION_LEFT LOW
#define STEPPER_DIRECTION_RIGHT HIGH

//
//  Motor Stuff
//

enum  MotorAction { STOP = 0, LEFT, RIGHT, END };
void controlMotor(MotorAction action);

MotorAction _myMotorAction = MotorAction::STOP;
bool volatile _isMotorStepping = false;
bool volatile _isBumpSwitchOn = false;
bool volatile _isLeftBumpSwitchOn = false;
bool volatile _isRightBumpSwitchOn = false;
bool volatile _stepState = false;

//Timer stepperTimer(1, stepperCallback);
Timer bumperChecker(50, bumperCallback);


//
//  Hover Stuff
//

#define HOVER_TS_PIN D2
#define HOVER_RST_PIN D3

#define GESTUREMODE 0x00    //0x00 = disable gestures, 0x01 = enable gestures
#define TOUCHMODE 0x00      //0x00 = disable touch, 0x01 = enable touch
#define TAPMODE 0x01        //0x00 = disable taps, 0x01 = enable taps, 0x02 = single taps only, 0x03 = double taps only
#define POSITIONMODE 0x01   //0x00 = disable position tracking, 0x01 = enable position tracking

Hover hover = Hover(HOVER_TS_PIN, HOVER_RST_PIN, GESTUREMODE, TOUCHMODE, TAPMODE, POSITIONMODE );

unsigned long lastHoverCommand = 0;
unsigned long lastHoverCheck = 0;
bool state = false;

// TODO: how long should this run for?
#define DISPENSE_BALL_DURATION 3000
bool dispensingBall = false;
unsigned long startedDispensingBallTime = 0;

/*
    How to wire it up!

    Behaviors!

        move your hand left, the platform moves left
        right, right

        if you a stop switch, stop moving in that direction.

        if you touch the hover pad, release a ball


    TODOS;
        fire up the "ball pump pin when touched"
        fix the backoff / movement behavior Mohit added
        cleanup code a bit
*/


void setup() {
    Serial.begin(115200);
    hover.begin();

    pinMode(STEPPER_STEP_PIN, OUTPUT);
    pinMode(STEPPER_DIRECTION_PIN, OUTPUT);
    pinMode(STEPPER_EN_PIN, OUTPUT);

    pinMode(BUMP_BTN_PIN1, INPUT_PULLUP);
    pinMode(BUMP_BTN_PIN2, INPUT_PULLUP);

    pinMode(BALL_PUMP_PIN, OUTPUT);
    pinMode(BALLS_FULL_PIN, INPUT);

    pinMode(D7, OUTPUT);

    //Initialize default states
    digitalWrite(STEPPER_EN_PIN, HIGH); //disable stepper motor driver
    analogWrite(STEPPER_STEP_PIN, 128, 1200);

    // run me forever
    bumperChecker.start();
}

void loop() {
    // state = !state;
    // bool blink_state = false;
    hoverTick();

    dispenseBallTick();
}

void hoverTick() {

    // read the hover interface
    hover.readI2CData();

//            +----------------+
//            |                |
//            |                |
//            |     +----+     |
//   Left <-----+   |----|  +----->  Right!
//        <-----+   |----|  +----->
//            |     +----+     |
//            |                |
//            |                |
//            +----------------+


    // we want to update what we're doing every 50 milliseconds if we can, so it feels very responsive.

    // if we're in the "middle" don't move
    // if we haven't gotten an even in the last, uh, 100 milliseconds stop moving
    // if we've gotten new instructions since the last check, update.

    // unsigned long lastHoverCommand = 0;
    // unsigned long lastHoverCheck = 0;

    // only check every xx seconds
    unsigned long now = millis();
    if ((now - lastHoverCheck) < 25) {
        return;
    }

    lastHoverCheck = now;

    Position p = hover.getPosition();
    if (p.x==0 && p.y==0 && p.x==0) {
        // no input from user... stop the motor.
        controlMotor(MotorAction::STOP);
        return;
    }

    p.x = map(p.x, 0, 65535, 0, 100);
    p.y = map(p.y, 0, 65535, 0, 100);
    p.z = map(p.z, 0, 65535, 0, 100);

    // (move left) 0 ... 40-60 ... 100 (move right)
    //                  idle

    //  HERE!
    //
    //
    // TODO: if z is below some threshold, hold off and wait for clicks;


    const char *dir;
    if (p.x <= 40) {
        controlMotor(MotorAction::LEFT);
        dir = "left";
    }
    else if (p.x >= 60) {
        controlMotor(MotorAction::RIGHT);
        dir = "right";
    }
    else {
        dir = "stop";
        controlMotor(MotorAction::STOP);
    }

    // TODO: slow vs. fast regions?

    Serial.println(String::format("(%d, %d, %d) direction %s ", p.x, p.y, p.z, dir));

    Touch t = hover.getTouch();
    if ( t.touchID != 0) {
        //         Serial.print("Event: "); Serial.print(t.touchType); Serial.print("\t");
        //         Serial.print("Touch ID: "); Serial.print(t.touchID,HEX); Serial.print("\t");
        //         Serial.print("Value: "); Serial.print(t.touchValue,HEX); Serial.println("");
        dispenseBall();
    }
}

void dispenseBallTick() {
    unsigned long now = millis();

    if (dispensingBall) {
        if ((now - startedDispensingBallTime) > DISPENSE_BALL_DURATION) {
            dispensingBall = false;
            // stop pumping
            digitalWrite(BALL_PUMP_PIN, LOW);
            Serial.println("done dispensing ball...");
        }
    }
    else {
        // putting this here so we don't chain dispense'es
        if (digitalRead(BALLS_FULL_PIN) == HIGH) {
            dispenseBall();
        }
    }
}

void dispenseBall() {
    dispensingBall = true;
    startedDispensingBallTime = millis();

    // start pumping
    digitalWrite(BALL_PUMP_PIN, HIGH);
    Serial.println("Dispensing ball...");
}

/**
 * change what the motor is doing
 **/
void controlMotor(MotorAction action) {
    _myMotorAction = action;
    switch(_myMotorAction) {
        case MotorAction::LEFT:
            //digitalWrite(STEPPER_EN_PIN, LOW);
            if (!_isLeftBumpSwitchOn) {
                digitalWrite(STEPPER_DIRECTION_PIN, STEPPER_DIRECTION_LEFT);   //TODO: calibrate
                _isMotorStepping = true;
            }
            else {
                _isMotorStepping = false;
            }

            break;
        case MotorAction::RIGHT:
            //digitalWrite(STEPPER_EN_PIN, LOW);
            if (!_isRightBumpSwitchOn) {
                digitalWrite(STEPPER_DIRECTION_PIN, STEPPER_DIRECTION_RIGHT);   //TODO: calibrate
                 _isMotorStepping = true;
            }
            else {
                _isMotorStepping = false;
            }

            break;
        case MotorAction::STOP:
            //digitalWrite(STEPPER_EN_PIN, HIGH);
            _isMotorStepping = false;
            break;
        default:
            _isMotorStepping = false;
        break;
    }

    if (_isMotorStepping) {
        //stepperTimer.start();
        digitalWrite(STEPPER_EN_PIN, LOW);
    }
    else {
        //stepperTimer.stop();
        digitalWrite(STEPPER_EN_PIN, HIGH);
        //TODO: sleep the motor driver or something?
    }
}

// /**
//  * be the clock cycle you want to see in the world
//  **/
// void stepperCallback() {
//     if (_isMotorStepping && !_isBumpSwitchOn) {
//         digitalWrite(STEPPER_STEP_PIN, (_stepState) ? HIGH : LOW);
//         _stepState = !_stepState;
//     }
//     // TODO: else, sleep the motor driver or something?
// }

/**
 * is our bumper switch pressed?
 **/
void bumperCallback() {

    _isLeftBumpSwitchOn = digitalRead(BUMP_BTN_PIN2);
    _isRightBumpSwitchOn = digitalRead(BUMP_BTN_PIN1);
    //_isBumpSwitchOn = ((digitalRead(BUMP_BTN_PIN1) == HIGH) || (digitalRead(BUMP_BTN_PIN2) == HIGH));
    _isBumpSwitchOn = _isLeftBumpSwitchOn || _isRightBumpSwitchOn;


    if (_isLeftBumpSwitchOn && (_myMotorAction == MotorAction::LEFT)) {
        digitalWrite(STEPPER_EN_PIN, HIGH); //motor off
    }
    if (_isRightBumpSwitchOn && (_myMotorAction == MotorAction::RIGHT)) {
        digitalWrite(STEPPER_EN_PIN, HIGH); //motor off
    }

    if(_isBumpSwitchOn)
    {
        //digitalWrite(STEPPER_EN_PIN, HIGH); //disable stepper if switches pressed
        digitalWrite(D7,HIGH);
    }
    else {
        digitalWrite(D7,LOW);
    }
}

