SYSTEM_THREAD(ENABLED);


#include "bitset.h"

#define PRINT_DELAY 1000
unsigned long lastPrintTime = 0;

#define CAN_PRINT_MESSAGE_ID 0x204

SYSTEM_MODE(MANUAL);
CANChannel can(CAN_C4_C5);

//unsigned long lastPrintTime = 0;


void setup() {
    Serial.begin(9600);

    Particle.function("sendPrint", onTestPrint);

    can.begin(500000);
}

void loop() {
    unsigned long now = millis();

    if ((now - lastPrintTime) > 10000) {
        onTestPrint("1");
        lastPrintTime = millis();
    }

    //checkCAN();

    //checkButtons();
    if(can.available() <= 0) {
        return;
    }

    // we have messages
    CANMessage msg;
    if(!can.receive(msg)) {
        // there was a problem getting the message!
        return;
    }

    Serial.println("got can message, ID " + String(msg.id));



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


