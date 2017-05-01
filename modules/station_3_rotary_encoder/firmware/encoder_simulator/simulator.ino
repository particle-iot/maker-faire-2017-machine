SYSTEM_MODE(MANUAL);

const auto encoderA = A0;
const auto encoderB = A1;
const auto pulseInterval = 50;

void setup() {
  pinMode(D7, OUTPUT);
  pinMode(encoderA, OUTPUT);
  pinMode(encoderB, OUTPUT);
  digitalWrite(encoderA, LOW);
  digitalWrite(encoderB, LOW);

  noInterrupts();
  while(true) {
    for(int i = 0; i < 1000; i++) {
      digitalWrite(encoderA, HIGH);
      delayMicroseconds(pulseInterval);
      digitalWrite(encoderB, HIGH);
      delayMicroseconds(pulseInterval);
      digitalWrite(encoderA, LOW);
      delayMicroseconds(pulseInterval);
      digitalWrite(encoderB, LOW);
      delayMicroseconds(pulseInterval);
    }
    digitalWrite(D7, HIGH);
    delayMicroseconds(2000000);
    digitalWrite(D7, LOW);
  }
}
