// Power the ball detector and read the analog value
#include "Particle.h"

SYSTEM_THREAD(ENABLED);

const auto LASER_PIN = D0;
const auto LIGHT_SENSOR_PIN = A0;
const auto LASER_ON = HIGH;
const auto LASER_OFF = LOW;

uint16_t lightLevel;

void setup() {
  Serial.begin();
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LASER_ON);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
}

bool registered = false;
void loop() {
  if (!registered && Particle.connected()) {
    Particle.variable("light", lightLevel);
  }

  lightLevel = analogRead(LIGHT_SENSOR_PIN);
  Serial.printlnf("%d,%d", millis(), lightLevel);
  delay(50);
}
