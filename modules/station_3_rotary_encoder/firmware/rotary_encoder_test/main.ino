void doEncoderA();
void doEncoderB();

// int encoderA = A7;
// int encoderB = A6;
int encoderA = D1;
int encoderB = D2;

volatile bool A_set = false;
volatile bool B_set = false;
volatile int encoderPos = 0;

int prevPos = 0;
int value = 0;

// Direct memory acess to pin for highest access speed
STM32_Pin_Info* PIN_MAP2 = HAL_Pin_Map();
#define digitalReadFast(_pin) (PIN_MAP2[_pin].gpio_peripheral->IDR & PIN_MAP2[_pin].gpio_pin)

void setup()
{
    Serial.begin(9600);
    delay(1000);
    Serial.println(encoderPos);
    pinMode(encoderA, INPUT_PULLUP);
    pinMode(encoderB, INPUT_PULLUP);
    attachInterrupt(encoderA, doEncoderA, CHANGE);
    attachInterrupt(encoderB, doEncoderB, CHANGE);
}

void loop()
{
    // copy to local var to avoid multiple access to global that changes in interrupt
    int pos = encoderPos;
    if (prevPos != pos)
    {
        prevPos = pos;
        Serial.println(pos);
    }
    delay(50);
}
//----------------------------------------------------------------

void doEncoderA()
{
    if (digitalReadFast(encoderA) != A_set)
    { // debounce once more
        A_set = !A_set;
        // adjust counter + if A leads B
        if (A_set && !B_set)
        {
            encoderPos += 1;
        }
    }
}

// Interrupt on B changing state, same as A above
void doEncoderB()
{
    if (digitalReadFast(encoderB) != B_set)
    {
        B_set = !B_set;
        //  adjust counter - 1 if B leads A
        if (B_set && !A_set)
        {
            encoderPos -= 1;
        }
    }
}
