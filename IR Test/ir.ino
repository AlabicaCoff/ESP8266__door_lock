#define irSensor 15

#include <Servo.h>
int buzzer = 12;
int servoPin = 2;
bool isUnlocked;
bool irTriggered;
Servo Servo1;

void openLock() {
  if (!irTriggered && !isUnlocked) {
    return;
  }
  int pos;
  Servo1.attach(servoPin);
  for (pos = 0; pos <= 180; pos += 1) {
    Servo1.write(pos);
    delay(15);
  }
  Servo1.detach();
  isUnlocked = true;
  irTriggered = false;
}

void closeLock() {
  if (!irTriggered && isUnlocked) {
    return;
  }

  int pos;
  Servo1.attach(servoPin);

  for (pos = 180; pos >= 0; pos -= 1) {
    Servo1.write(pos);
    delay(15);
  }
  delay(1000);
  Servo1.detach();

  isUnlocked = false;
  irTriggered = false;
}

void setup() {
  Serial.begin(115200);
  pinMode(irSensor, INPUT);
  pinMode(buzzer, OUTPUT);
}

void loop() {
  digitalWrite(buzzer, HIGH);
}
