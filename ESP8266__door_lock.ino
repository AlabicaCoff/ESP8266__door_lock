#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <Servo.h>

#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2SNoDAC.h"

#include "viola.h"

AudioGeneratorWAV *wav;
AudioFileSourcePROGMEM *file;
AudioOutputI2SNoDAC *out;

#define WIFI_SSID "Naphan 2.4 Ghz"
#define WIFI_PASSWORD "19112512"
#define HTTP_REST_PORT 8080

ESP8266WebServer httpRestServer(HTTP_REST_PORT);
IPAddress ip(192, 168, 1, 30);
IPAddress dns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

int ledStatus[2] = {12, 13};
int Speaker = 3;
int servoPin = 2;
int tryCount = 0;
int openButton = 4;
int closeButton = 5;
int irSensor = 14;
bool isUnlocked = true;
bool irTriggered = false;
Servo Servo1;

void openLock() {
  if (!isUnlocked) {
    int pos;
    Servo1.attach(servoPin);
    for (pos = 0; pos <= 180; pos += 1) {
      Servo1.write(pos);
      delay(15);
    }
    Servo1.detach();

    isUnlocked = true;
    irTriggered = false;

    file = new AudioFileSourcePROGMEM( viola, sizeof(viola) );
    out = new AudioOutputI2SNoDAC();
    wav = new AudioGeneratorWAV();
    wav->begin(file, out);
  }
}

void closeLock() {
  if (isUnlocked) {
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

    file = new AudioFileSourcePROGMEM( viola, sizeof(viola) );
    out = new AudioOutputI2SNoDAC();
    wav = new AudioGeneratorWAV();
    wav->begin(file, out);
  }
}

void openLockRemotely() {
  if (isUnlocked) {
    httpRestServer.send(500, "text/html", "Deadbolt is already unlocked");
  }
  openLock();
  httpRestServer.send(200, "text/html", "Deadbolt is now unlocked");
}

void closeLockRemotely() {
  if (!isUnlocked) {
    httpRestServer.send(500, "text/html", "Deadbolt is already locked");
  }
  closeLock();
  httpRestServer.send(200, "text/html", "Deadbolt is now locked");
}

void getLockStatus() {
  const String status = isUnlocked ? "no" : "yes";

  httpRestServer.send(200, "text/html", "Deadbolt is locked: " + status);
}

void restServerRouting() {
  httpRestServer.on("/", HTTP_GET, []() {
    httpRestServer.send(200, F("text/html"),
                        F("Welcome to the Smart Door!"));
  });
  httpRestServer.on(F("/openLock"), HTTP_GET, openLockRemotely);
  httpRestServer.on(F("/closeLock"), HTTP_GET, closeLockRemotely);
  httpRestServer.on(F("/getLockStatus"), HTTP_GET, getLockStatus);
}

void setup() {
  Serial.begin(115200);
  Servo1.attach(servoPin);
  Servo1.write(180);
  Servo1.detach();
  pinMode(openButton, INPUT);
  pinMode(closeButton, INPUT);
  pinMode(irSensor, INPUT_PULLUP);
  pinMode(ledStatus[0], OUTPUT);
  pinMode(ledStatus[1], OUTPUT);
  pinMode(Speaker, OUTPUT);

  WiFi.config(ip, dns, gateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Beginning WiFi Connection to: ");
  Serial.println(WIFI_SSID);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(++i);
    Serial.println(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);

  restServerRouting();
  httpRestServer.begin();
}

void loop() {
  if (wav) {
    if (!wav->loop()) {
      wav->stop();
      delete file;
      delete out;
      delete wav;
      wav = NULL;
      file = NULL;
      out = NULL;
    }
  }

  if (digitalRead(openButton))
  {
    Serial.println("openButton Pressed");
    openLock();
  } else if (digitalRead(closeButton)) {
    Serial.println("closeButton Pressed");
    closeLock();
  }
  if (WiFi.status() != WL_CONNECTED) {
    if (!isUnlocked) {
      Serial.println("WARNING: WiFi signal was lost!! Unlocking deadbolt for safety");
      openLock();
    }
  }

  if (digitalRead(irSensor) == HIGH) {
    irTriggered = true;
  }
  if (irTriggered) {
    Serial.println("IR sensor triggered");
    if (isUnlocked) {
      closeLock();
    }
    else if (!isUnlocked) {
      openLock();
    }

  }
  if (isUnlocked) {
    digitalWrite(ledStatus[0], HIGH);
    digitalWrite(ledStatus[1], LOW);
  }
  else {
    digitalWrite(ledStatus[0], LOW);
    digitalWrite(ledStatus[1], HIGH);
  }

  httpRestServer.handleClient();
}
