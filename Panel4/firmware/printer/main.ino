SYSTEM_THREAD(ENABLED);

// This #include statement was automatically added by the Particle IDE.
#include "lib/SdFat.h"

// This #include statement was automatically added by the Particle IDE.
#include "cdam_printer.h"
#include "cdam_const_config.h"
#define kIntervalPrinterMillis 100

#define PRINT_DELAY 1000
unsigned long lastPrintTime = 0;

int counters[4] = {0,0,0,0};

#define CS_PIN SS
SdFat SD;
File file;


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

    // buttons
    pinMode(D1, INPUT);
  	pinMode(D2, INPUT);
	pinMode(D3, INPUT);
	pinMode(D4, INPUT);

	//TODO: CAN-BUS PUBLISH COUNTS

}

void loop() {
    unsigned long now = millis();

    checkButtons();
}

void checkButtons() {
    int buttonDownValue = LOW;

    bool buttonOne = digitalRead(D1) == buttonDownValue;
    bool buttonTwo = digitalRead(D2) == buttonDownValue;
    bool buttonThree = digitalRead(D3) == buttonDownValue;
    bool buttonFour = digitalRead(D4) == buttonDownValue;

    int imageNumber = -1;

    if (buttonOne) {
        imageNumber = 1;
        counters[0]++;
    }
    else if (buttonTwo) {
        imageNumber = 2;
        counters[1]++;
    }
    else if (buttonThree) {
        imageNumber = 3;
        counters[2]++;
    }
    else if (buttonFour) {
        //imageNumber = 99;
        counters[3]++;
    }

    unsigned long now = millis();

    if ((imageNumber >= 0) && ((now - lastPrintTime) > PRINT_DELAY)) {
        lastPrintTime = now;

        if (imageNumber >= 0) {
//            Serial.println("the time is " + String(millis()));
//            Serial.println("pin D1 is " + (digitalRead(D1) ? String("HIGH") : String("LOW")));
//            Serial.println("pin D2 is " + (digitalRead(D2) ? String("HIGH") : String("LOW")));
//            Serial.println("pin D3 is " + (digitalRead(D3) ? String("HIGH") : String("LOW")));
//            Serial.println("pin D4 is " + (digitalRead(D4) ? String("HIGH") : String("LOW")));
//
////            Serial.println("analog D1 is " + String(analogRead(D1)));
////            Serial.println("analog D2 is " + String(analogRead(D2)));
////            Serial.println("analog D3 is " + String(analogRead(D3)));
////            Serial.println("analog D4 is " + String(analogRead(D4)));
//

            Serial.println("Printing image " + String(imageNumber));

            //printImageFile(190, 190, "particle_logo.raw");
            printImage(imageNumber);
        }
    }
}


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


