Servo servo;
const auto SERVO_PIN = TX;
uint8_t servoPos = 0;

void setup() {
  Particle.function("servo", updateServo);
  Particle.variable("servoPos", servoPos);
  servo.attach(SERVO_PIN);
  servo.write(servoPos);
}

void loop() {
  servo.write(servoPos);
}

int updateServo(String arg) {
  servoPos = arg.toInt();
  return servoPos;
}



