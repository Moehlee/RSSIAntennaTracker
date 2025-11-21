#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Servo.h>

// PINS
constexpr uint8_t LCD_RS = A0;
constexpr uint8_t LCD_E = A1;
constexpr uint8_t LCD_D4 = A2;
constexpr uint8_t LCD_D5 = A3;
constexpr uint8_t LCD_D6 = A4;
constexpr uint8_t LCD_D7 = A5;

constexpr uint8_t ServoPanPin = PB8;
constexpr uint8_t ServoTiltPin = PB9;

constexpr uint8_t RSSI_ANT1_PIN = A6;
constexpr uint8_t RSSI_ANT2_PIN = A7;

constexpr uint8_t BUZZER_PIN = PA8;

#ifndef LED_BUILTIN
#define LED_BUILTIN PC13
#endif

Servo servoPan;
Servo servoTilt;
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

float panAngle  = 90.0;
float tiltAngle = 90.0;

float targetPan  = 90.0;
float targetTilt = 90.0;

#define RSSI_BUFFER_SIZE 5
int rssiBuffer[RSSI_BUFFER_SIZE] = {0};
int rssiIndex = 0;

bool tracking = false;

// Scan movement
int scanDir = 1;

const int SCAN_RANGE = 45;     // horizontal (PAN)
const int SCAN_STEP  = 2;      // etwas langsamer

const int SETTLE_TIME = 30;

const int RSSI_ON  = 320;
const int RSSI_OFF = 260;

const int PEAK_INTERVAL = 1300;   // Peak alle 1.3s

// Servo Limits
const int PAN_MIN  = 45;
const int PAN_MAX  = 135;
const int TILT_MIN = 85;       // leicht nach unten möglich
const int TILT_MAX = 130;      // nicht zu weit hoch

float KP = 0.16;
float KD = 0.05;

float lastPanError = 0;
float lastTiltError = 0;

unsigned long lastMove = 0;
unsigned long lastPeakSearch = 0;

int readRSSI() {
  int a1 = analogRead(RSSI_ANT1_PIN);
  int a2 = analogRead(RSSI_ANT2_PIN);
  return max(a1, a2);
}

int getFilteredRSSI(int raw) {
  rssiBuffer[rssiIndex] = raw;
  rssiIndex = (rssiIndex + 1) % RSSI_BUFFER_SIZE;

  int sum = 0;
  for (int i = 0; i < RSSI_BUFFER_SIZE; i++) sum += rssiBuffer[i];
  return sum / RSSI_BUFFER_SIZE;
}

void playSignalTone() {
  tone(BUZZER_PIN, 1000, 200); delay(250);
  tone(BUZZER_PIN, 1500, 200);
}

void playLostTone() {
  tone(BUZZER_PIN, 800, 300);
}

void moveServoSmooth(float &angle, float target, float &lastError, bool isTilt = false) {

  float error = target - angle;
  float derivative = error - lastError;

  float step = (KP * error + KD * derivative) * 0.30;  // WENIGER SPEED

  if (abs(error) > 1.0) angle += step;

  lastError = error;

  if (isTilt)
    angle = constrain(angle, TILT_MIN, TILT_MAX);
  else
    angle = constrain(angle, PAN_MIN, PAN_MAX);
}

int measureRSSIAtPan(float angle) {
  servoPan.write(angle);
  delay(120);
  return getFilteredRSSI(readRSSI());
}

int measureRSSIAtTilt(float angle) {
  servoTilt.write(angle);
  delay(120);
  return getFilteredRSSI(readRSSI());
}

void findBestPanPeak() {

  float center = panAngle;

  float left  = constrain(center - 8, PAN_MIN, PAN_MAX);
  float right = constrain(center + 8, PAN_MIN, PAN_MAX);

  int c = measureRSSIAtPan(center);
  int l = measureRSSIAtPan(left);
  int r = measureRSSIAtPan(right);

  if (l > c && l > r)      targetPan = left;
  else if (r > c)          targetPan = right;
  else                     targetPan = center;
}

void findBestTiltPeak() {

  float center = tiltAngle;

  float down = constrain(center - 6, TILT_MIN, TILT_MAX);
  float up   = constrain(center + 6, TILT_MIN, TILT_MAX);

  int c = measureRSSIAtTilt(center);
  int d = measureRSSIAtTilt(down);
  int u = measureRSSIAtTilt(up);

  if (d > c && d > u)      targetTilt = down;
  else if (u > c)          targetTilt = up;
  else                     targetTilt = center;
}

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  servoPan.attach(ServoPanPin);
  servoTilt.attach(ServoTiltPin);

  servoPan.write(panAngle);
  servoTilt.write(tiltAngle);

  lcd.begin(16, 2);
  lcd.print("Antenna Tracker");
  delay(1000);
  lcd.clear();

  lastMove = millis();
  lastPeakSearch = millis();
}

void loop() {

  int rawRSSI = readRSSI();
  int rssi = getFilteredRSSI(rawRSSI);

  if (millis() - lastMove > SETTLE_TIME) {

    if (tracking) {

      if (rssi < RSSI_OFF) {
        tracking = false;
        playLostTone();
      } else {
        if (rssi < RSSI_ON + 20){
          targetTilt = 90;
        }

        if (millis() - lastPeakSearch > PEAK_INTERVAL) {
          findBestPanPeak();
          findBestTiltPeak();
          lastPeakSearch = millis();
        }
      }

    } else {

      if (rssi > RSSI_ON) {
        tracking = true;
        playSignalTone();
      } else {
        targetPan += scanDir * SCAN_STEP;

        if (targetPan >= 90 + SCAN_RANGE) { scanDir = -1; targetPan = 90 + SCAN_RANGE; }
        if (targetPan <= 90 - SCAN_RANGE) { scanDir =  1; targetPan = 90 - SCAN_RANGE; }

        targetTilt = 90;
      }
    }

    lastMove = millis();
  }

  moveServoSmooth(panAngle,  targetPan,  lastPanError,  false);
  moveServoSmooth(tiltAngle, targetTilt, lastTiltError, true);

  servoPan.write(round(panAngle));
  servoTilt.write(round(tiltAngle));

  int displayRSSI = map(constrain(rssi, 150, 400), 150, 400, 0, 99);

  lcd.setCursor(0, 0);
  lcd.print("RSSI: ");
  lcd.print(displayRSSI);
  lcd.print("    ");

  lcd.setCursor(0, 1);
  lcd.print(tracking ? "TRACKING " : "SCANNING ");

  digitalWrite(LED_BUILTIN, tracking ? LOW : HIGH);

  delay(20);
}
