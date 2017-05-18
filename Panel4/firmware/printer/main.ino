SYSTEM_THREAD(ENABLED);

#include "bitset.h"

#include "lib/SdFat.h"
#include "cdam_printer.h"
#include "cdam_const_config.h"
#define kIntervalPrinterMillis 100


#define PRINT_DELAY 1000
unsigned long lastPrintTime = 0;

#define CAN_PRINT_MESSAGE_ID 0x204


#define CS_PIN SS
SdFat SD;
File file;


SYSTEM_MODE(MANUAL);
CANChannel can(CAN_D1_D2);


using namespace cdam;

Printer _printer = Printer();

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);

    Particle.function("testPrint", onTestPrint);

	_printer.initialize();
	_printer.begin(kPrinterHeatingTime);
	_printer.setTimes(7000, 300);
	_printer.setASB(true);
	_printer.active = false;


    // Initialize the SD.
      if (!SD.begin(CS_PIN)) {
        Serial.print("failed to init SD card");
        Particle.publish("printer", "sd card init failed");
      }

//    // buttons
//  pinMode(D1, INPUT);
//  pinMode(D2, INPUT);
//	pinMode(D3, INPUT);
//	pinMode(D4, INPUT);

	//TODO: CAN-BUS PUBLISH COUNTS

    can.begin(500000);


    PMIC power;
    power.disableCharging();
}

void loop() {
    //unsigned long now = millis();

    checkCAN();

    //checkButtons();
}

void checkCAN() {
    if(can.available() <= 0) {
        return;
    }

    // we have messages

    CANMessage msg;
    if(!can.receive(msg)) {
        // there was a problem getting the message!
        return;
    }

    Serial.println(String::format("CAN Msg: %d, Length: %d ", msg.id, msg.len));

//    // every time we get message id 100, lets blink our LED.
//    if (msg.id == 0x100) {
//        led_state = !led_state;
//        digitalWrite(D7, (led_state) ? HIGH:LOW);
//    }

    // todo: drop other pending prints?

    if (msg.id == CAN_PRINT_MESSAGE_ID) {
        // got a print command
        int index = getU8(msg.data, 0);
        if (index > 0) {
            Serial.println("Printing image " + String(index));
            printImage(index);
        }
    }
}

//void checkButtons() {
//    int buttonDownValue = LOW;
//
//    bool buttonOne = digitalRead(D1) == buttonDownValue;
//    bool buttonTwo = digitalRead(D2) == buttonDownValue;
//    bool buttonThree = digitalRead(D3) == buttonDownValue;
//    bool buttonFour = digitalRead(D4) == buttonDownValue;
//
//    int imageNumber = -1;
//
//    if (buttonOne) {
//        imageNumber = 1;
//        counters[0]++;
//    }
//    else if (buttonTwo) {
//        imageNumber = 2;
//        counters[1]++;
//    }
//    else if (buttonThree) {
//        imageNumber = 3;
//        counters[2]++;
//    }
//    else if (buttonFour) {
//        //imageNumber = 99;
//        counters[3]++;
//    }
//
//    unsigned long now = millis();
//
//    if ((imageNumber >= 0) && ((now - lastPrintTime) > PRINT_DELAY)) {
//        lastPrintTime = now;
//
//        if (imageNumber >= 0) {
//            Serial.println("Printing image " + String(imageNumber));
//
//            //printImageFile(190, 190, "particle_logo.raw");
//            printImage(imageNumber);
//        }
//    }
//}


void printImage(int imageNumber) {

    switch(imageNumber) {
        case 1:
            //printImageFile(202, 380, "candy_image.raw");
            printImageFile(271, 393, "p_candy.raw");
        break;

        case 2:
            //printImageFile(380, 518, "coupon.raw");
            //printImageFile(200, 264, "coupon2.raw");
            printImageFile(271, 393, "p_coupon.raw");
        break;

        case 3:
            //printImageFile(229, 380, "money.raw");
            printImageFile(271, 393, "p_money.raw");
        break;

        case 4:
            printImageFile(190, 190, "particle_logo.raw");
        break;

        case 5: printImageFile(271, 393, "beachball.raw"); break;
        case 6: printImageFile(271, 393, "silly.raw"); break;
        case 7: printImageFile(271, 393, "nyancat.raw"); break;

        // //TODO: obviously remove this before production
//        case 99:
//             printImageFile(204, 247, "etjgJ2D.raw");
//        break;
    }
}

void printImageFile(int width, int height, String filename) {
    File imageFile = SD.open(filename.c_str(), FILE_READ);
      if (!imageFile.isOpen()) {
        Serial.println("unable to open " + filename);
        _printer.write("failed to open file", 19);
        _printer.feed(3);

        //TODO: print files I can see?
      }
      else {
          _printer.feed(2);
          _printer.printBitmap(width, height, dynamic_cast<Stream*>(&imageFile));
          _printer.feed(4);
      }
}

int onTestPrint(String cmd) {
    int thing = cmd.toInt();
    printImage(thing);
}


