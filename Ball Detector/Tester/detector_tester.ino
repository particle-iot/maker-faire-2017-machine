// Power the ball detector and read the analog value
#include "beam-detector.h"

SYSTEM_THREAD(ENABLED);

const auto LASER_PIN = D0;
BeamDetector detectors[] = {
  BeamDetector(A1, LASER_PIN),
  BeamDetector(A2, LASER_PIN),
  BeamDetector(A3, LASER_PIN),
};

void setup() {
  Serial.begin();
  for (auto &detector : detectors) {
    detector.begin();
  }
}

void loop() {
  Serial.printf("%d", millis());
  for (auto &detector : detectors) {
    Serial.printf(",%d", detector.beamBroken());
  }
  for (auto &detector : detectors) {
    Serial.printf(",%d", detector.detectionCount());
  }
  Serial.println("");
}
