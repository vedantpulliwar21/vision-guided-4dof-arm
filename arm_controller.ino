// Arduino controller for the vision-guided 4-DOF arm.
// - Receives "base,shoulder,elbow" angles from vision/tracker.py over USB serial
// - Drives 4 servos through a PCA9685 driver board (channels 0, 4, 8, 12)
// - Closes the gripper when the HC-SR04 sees an object within 3 cm
// - Sending "R" releases the object and resumes tracking
//
// Library needed: Adafruit PWM Servo Driver (Sketch > Include Library > Manage Libraries)

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();  // default I2C address 0x40

// HC-SR04 pins
const int TRIG = 9;
const int ECHO = 10;
const float GRAB_CM = 3.0;     // claw size

// PCA9685 channels (CONFIRM which joint is on which channel)
const int BASE = 0;
const int SHOULDER = 4;
const int ELBOW = 8;
const int GRIPPER = 12;

// Gripper angles: tune these to your claw
const int GRIP_OPEN = 30;
const int GRIP_CLOSED = 100;

bool holding = false;

void setAngle(int channel, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, 110, 500);  // ~0.5 to 2.4 ms at 50 Hz
  pwm.setPWM(channel, 0, pulse);
}

float readDistanceCm() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long t = pulseIn(ECHO, HIGH, 25000);  // ~4 m timeout
  if (t == 0) return 999;               // no echo
  return t * 0.0343 / 2.0;
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);

  // Home position
  setAngle(BASE, 90);
  setAngle(SHOULDER, 90);
  setAngle(ELBOW, 90);
  setAngle(GRIPPER, GRIP_OPEN);
  Serial.println("READY");
}

void loop() {
  // 1. Commands from Python
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line == "R") {
      holding = false;
      setAngle(GRIPPER, GRIP_OPEN);
      Serial.println("RELEASED");
    } else if (!holding) {  // arm stays still while holding
      int b, s, e;
      if (sscanf(line.c_str(), "%d,%d,%d", &b, &s, &e) == 3) {
        setAngle(BASE, b);
        setAngle(SHOULDER, s);
        setAngle(ELBOW, e);
      }
    }
  }

  // 2. Proximity-triggered grasp
  if (!holding) {
    float d = readDistanceCm();
    if (d < GRAB_CM) {
      setAngle(GRIPPER, GRIP_CLOSED);
      holding = true;
      Serial.println("GRABBED");
    }
  }

  delay(20);
}
