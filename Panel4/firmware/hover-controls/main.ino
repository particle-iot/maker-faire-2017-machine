#include "Hover.h"


#include "can-lib/bitset.h"
#include "can-lib/can-interface.h"
#include "can-lib/mf2017-can.h"


#define STEP_CLOCK_DURATION 1

#define BUMP_BTN_PIN1 B0
#define BUMP_BTN_PIN2 B1

#define STEPPER_EN_PIN C0
#define STEPPER_DIRECTION_PIN C1
#define STEPPER_STEP_PIN D2

#define LASER_ENABLE A0
#define LASER_GATE1 A1
#define LASER_GATE2 A2
#define LASER_GATE3 A3

#define HOVER_TS_PIN D4
#define HOVER_RST_PIN D5

#define BALL_PUMP_PIN C3
#define BALLS_FULL_PIN C2

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


#define GESTUREMODE 0x00    //0x00 = disable gestures, 0x01 = enable gestures
#define TOUCHMODE 0x00      //0x00 = disable touch, 0x01 = enable touch
#define TAPMODE 0x01        //0x00 = disable taps, 0x01 = enable taps, 0x02 = single taps only, 0x03 = double taps only
#define POSITIONMODE 0x01   //0x00 = disable position tracking, 0x01 = enable position tracking

Hover hover = Hover(HOVER_TS_PIN, HOVER_RST_PIN, GESTUREMODE, TOUCHMODE, TAPMODE, POSITIONMODE );

unsigned long lastHoverCommand = 0;
unsigned long lastHoverCheck = 0;
bool state = false;

// TODO: how long should this run for?
#define DISPENSE_BALL_DURATION 1250
bool dispensingBall = false;
unsigned long startedDispensingBallTime = 0;

SYSTEM_MODE(MANUAL);



//CANChannel can(CAN_C4_C5);
BuiltinCAN can(CAN_C4_C5);
Communication comms = Communication(can);
#define CAN_PRINT_MESSAGE_ID 0x204




// laser gate stuff
#define PRINT_DELAY 1000
int counters[4] = {0,0,0,0};
unsigned long lastPrintTime = 0;

bool isMachineStopped = false;
bool blink_state = false;

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


    pinMode(LASER_ENABLE, OUTPUT);
    digitalWrite(LASER_ENABLE, HIGH);

    pinMode(LASER_GATE1, INPUT);
    pinMode(LASER_GATE2, INPUT);
    pinMode(LASER_GATE3, INPUT);

    pinMode(D7, OUTPUT);

    //Initialize default states
    digitalWrite(STEPPER_EN_PIN, HIGH); //disable stepper motor driver
    analogWrite(STEPPER_STEP_PIN, 128, 1200);

    // run me forever
    bumperChecker.start();

    //start up the can bus
    can.begin(500000);

    PMIC power;
    power.disableCharging();
}

void loop() {
    hoverTick();

    checkLaserGates();

    dispenseBallTick();

    transmitCANUpdates();

    if (isMachineStopped) {
        digitalWrite(D7, (blink_state) ? HIGH : LOW);
        blink_state = !blink_state;
        delay(10);
    }
}

unsigned long lastCanUpdateTime = 0;
#define CAN_UPDATE_FREQUENCY 250

void transmitCANUpdates() {
    unsigned long now = millis();
    if (( now - lastCanUpdateTime ) < CAN_UPDATE_FREQUENCY) {
        return;
    }
    lastCanUpdateTime = now;

    comms.receive();

    if (comms.MachineStart) {
        isMachineStopped = false;
    }
    else if (comms.MachineStop) {
        isMachineStopped = true;
    }


    //case Panel4Status:

    comms.BallDropperLeftLimit = _isLeftBumpSwitchOn;//getBit(m.data, 0, 1);
    comms.BallDropperRightLimit = _isRightBumpSwitchOn;//getBit(m.data, 0, 2);

    // these are set by the print routines
    //    comms.PrintingPrizeA
    //    comms.PrintingPrizeB
    //    comms.PrintingPrizeC

    // these are set by the hover routines
    //    comms.HoverPositionLR = getU8(m.data, 2);
    //    comms.HoverPositionUD = getU8(m.data, 3);


    // what are these?
    //    comms.Reservoir4Status = static_cast<ReservoirStatus>(getU8(m.data, 1));
    //    comms.Input4Active = getBit(m.data, 0, 0);
    //    comms.BallCount4 = getU32(m.data, 4);


    //case Panel4OutputStatus:
    comms.PrizeCountA = counters[0];
    comms.PrizeCountB = counters[1];
    comms.PrizeCountC = counters[2];

    comms.transmit(MachineModules::Panel4);


    // reset momentary elements
    comms.PrintingPrizeA = false;
    comms.PrintingPrizeB = false;
    comms.PrintingPrizeC = false;
}


void checkLaserGates() {
    int buttonDownValue = LOW;

    bool buttonOne = digitalRead(LASER_GATE1) == buttonDownValue;
    bool buttonTwo = digitalRead(LASER_GATE2) == buttonDownValue;
    bool buttonThree = digitalRead(LASER_GATE3) == buttonDownValue;

    int imageNumber = -1;

    //
    //  Count the balls
    //

    if (buttonOne) {
        imageNumber = 5;
        counters[0]++;
        comms.PrintingPrizeA = true;
    }
    else if (buttonTwo) {
        imageNumber = 6;
        counters[1]++;
        comms.PrintingPrizeB = true;
    }
    else if (buttonThree) {
        imageNumber = 2;
        counters[2]++;
        comms.PrintingPrizeC = true;
    }

    //
    //  If enough time has passed (print the images)
    //

    unsigned long now = millis();
    if ((imageNumber >= 0) && ((now - lastPrintTime) > PRINT_DELAY)) {
        lastPrintTime = now;

        if (imageNumber >= 0) {
            Serial.println("1,2,3 " + String(buttonOne) + "," + String(buttonTwo) + "," + String(buttonThree));

            Serial.println("Printing image " + String(imageNumber));
            sendPrintCommand(imageNumber);
        }
    }
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

    // don't print to serial as much, it's expensive
    if ((now - lastHoverCheck) < 250) {
        Serial.println(String::format("(%d, %d, %d) direction %s ", p.x, p.y, p.z, dir));
    }
    comms.HoverPositionLR = p.x;//getU8(m.data, 2);
    comms.HoverPositionUD = p.z;//getU8(m.data, 3);

    if ((p.z == 0) && ( _myMotorAction == MotorAction::STOP)) {
        dispenseBall();
    }

    Touch t = hover.getTouch();
    if ( t.touchID != 0) {
        Serial.print("Touch Event: "); Serial.print(t.touchType); Serial.print("\t");
        Serial.print("Touch ID: "); Serial.print(t.touchID,HEX); Serial.print("\t");
        Serial.print("Value: "); Serial.print(t.touchValue,HEX); Serial.println("");
        dispenseBall();
    }

    lastHoverCheck = now;
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
//        if (digitalRead(BALLS_FULL_PIN) == HIGH) {
//            //Serial.println("Balls full pin is high");
//            dispenseBall();
//        }
    }
}

/**
    Kicks off a ball being dispensed.
*/
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
                Serial.println("want left but bumper is on");
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
                Serial.println("want right but bumper is on");
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

    if (isMachineStopped) {
        _isMotorStepping = false;
        Serial.println("Would move except CAN BUS MOTOR STOPPED");
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




    if (isMachineStopped) {
        digitalWrite(STEPPER_EN_PIN, HIGH); //motor off
    }
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


void sendPrintCommand(int index) {
    CANMessage message;
    message.id = CAN_PRINT_MESSAGE_ID;
    message.len = 1;
    setU8(message.data, index, 0);
    can.transmit(message);
}


