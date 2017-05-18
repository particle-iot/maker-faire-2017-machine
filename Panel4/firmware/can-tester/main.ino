SYSTEM_THREAD(ENABLED);


#include "bitset.h"

#define PRINT_DELAY 1000
unsigned long lastPrintTime = 0;

#define CAN_PRINT_MESSAGE_ID 0x204

//SYSTEM_MODE(MANUAL);
CANChannel can(CAN_D1_D2);



void setup() {
    Serial.begin(9600);

    Particle.function("sendPrint", onTestPrint);

    can.begin(100000);

}

void loop() {
    //unsigned long now = millis();

    //checkCAN();

    //checkButtons();
}



int onTestPrint(String cmd) {
    int index = cmd.toInt();
    Serial.print("sending can message...");

    CANMessage message;
    message.id = CAN_PRINT_MESSAGE_ID;
    message.len = 1;
    setU8(message.data, index, 0);
    can.transmit(message);

    Serial.println("done");
    return index;
}


