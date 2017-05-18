
SYSTEM_MODE(MANUAL);

void setup() {
    Serial.begin(9600);

    // ---------------------
    // TAKE ME TO YOUR LEADER
    // ---------------------
    ControlLights(true);
}

void loop() {

    // ---------------------
    // ADD ME TO YOUR LOOP
    // ---------------------
    BreatheTick();
}



// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
//  Fake Breathing Stuff (START)
//
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int breatheCounter = 0;
int breatheDirection = 1;
unsigned int lastTick = 0;
#define BREATHE_MIN 0
#define BREATHE_MAX 255
#define BREATHE_TICK_MS 10

void ControlLights(bool onOff) {
    RGB.control(onOff);
}


void BreatheTick() {
    unsigned long now = millis();
    if ((now - lastTick) < BREATHE_TICK_MS) {
        return;
    }
    lastTick = now;

    if (breatheCounter >= BREATHE_MAX) {
        breatheCounter = BREATHE_MAX;
        breatheDirection = -1;
    }
    else if (breatheCounter <= BREATHE_MIN) {
        breatheCounter = BREATHE_MIN;
        breatheDirection = 1;
    }

    RGB.color(0, breatheCounter, breatheCounter);
    breatheCounter = breatheCounter + breatheDirection;
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
//  Fake Breathing Stuff (END)
//
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

