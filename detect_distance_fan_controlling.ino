
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo radarServo;

// Sensor and servo
const int servoPin = 6;
const int trigPin = 9;
const int echoPin = 10;

// L293D motor driver
const int enablePin = 5;
const int motorIN1 = 7;
const int motorIN2 = 8;

// Settings
const float maxDistance = 20.0;
const float minDistance = 2.0;
const unsigned long lostDelay = 1500;
const int scanStep = 2;

int angle = 0;
int direction = 1;

bool detected = false;
unsigned long lastSeen = 0;

int fanPercent = 0;
float distance = -1;

float readDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration =
    pulseIn(echoPin, HIGH, 12000);

  if (duration == 0) return -1;

  return duration * 0.0343 / 2.0;
}

void lcdLine(int row, String message) {
  lcd.setCursor(0, row);

  while (message.length() < 16) {
    message += " ";
  }

  lcd.print(message.substring(0, 16));
}

int calculateFan(float d) {
  if (d < minDistance || d > maxDistance) {
    return 0;
  }

  float percent =
    40.0 + (d - minDistance) * 60.0 /
    (maxDistance - minDistance);

  return constrain((int)(percent + 0.5), 40, 100);
}

void setFan(int percent) {
  percent = constrain(percent, 0, 100);

  digitalWrite(motorIN1, HIGH);
  digitalWrite(motorIN2, LOW);

  int pwm = map(percent, 0, 100, 0, 255);
  analogWrite(enablePin, pwm);
}

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(enablePin, OUTPUT);
  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);

  analogWrite(enablePin, 0);
  digitalWrite(motorIN1, LOW);
  digitalWrite(motorIN2, LOW);

  radarServo.attach(servoPin);
  radarServo.write(angle);

  lcd.init();
  lcd.backlight();

  lcdLine(0, "SMART COOLING");
  lcdLine(1, "Initializing...");
  delay(1500);
}

void loop() {
  distance = readDistance();

  bool inRange =
    distance >= minDistance &&
    distance <= maxDistance;

  if (inRange) {
    detected = true;
    lastSeen = millis();
    fanPercent = calculateFan(distance);
  }
  else if (detected) {
    fanPercent = 0;

    if (millis() - lastSeen >= lostDelay) {
      detected = false;
    }
  }
  else {
    fanPercent = 0;
  }

  // Update fan
  setFan(fanPercent);

  // Update LCD
  if (inRange) {
    lcdLine(0,
      "Dist:" + String(distance, 1) + "cm");

    lcdLine(1,
      "Fan:" + String(fanPercent) +
      "% A:" + String(angle));
  }
  else if (detected) {
    lcdLine(0, "OBJECT LOST");
    lcdLine(1, "Fan:0% Waiting");
  }
  else {
    lcdLine(0, "SCANNING...");
    lcdLine(1, "No object F:0%");
  }

  // Serial Monitor
  Serial.print("Angle: ");
  Serial.print(angle);
  Serial.print(" Distance: ");
  Serial.print(distance);
  Serial.print(" Fan: ");
  Serial.print(fanPercent);
  Serial.println("%");

  // Servo movement
  if (!detected) {
    angle += direction * scanStep;

    if (angle >= 180) {
      angle = 180;
      direction = -1;
    }

    if (angle <= 0) {
      angle = 0;
      direction = 1;
    }

    radarServo.write(angle);
  }

  delay(70);
}
