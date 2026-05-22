#include "HX711.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

// ---------- Main Loop Timing ----------
unsigned long lastLoopTime = 0;
const unsigned long LOOP_INTERVAL = 3000;
unsigned long lastHXRead = 0;

// ---------- LED / BUZZER TIMING ----------
unsigned long lastBlink = 0;
bool ledState = false;

// ---------- BLE Data Packet ----------
struct CrutchPacket {
  float load;
  float pitch;
  float roll;
  float accMag;
  float heartRate;
  float spo2;
  float battery;
  bool fallDetected;
  bool sos;
};

// ---------- Latest Sensor Values ----------
float lastWeight = 0;
float lastPitch = 0;
float lastRoll = 0;
float lastAccMag = 0;
float lastHR = 0;
float lastSpO2 = 0;
bool fallFlag = false;

// ---------- System Power ----------
bool systemOn = true;

#define SOS_PIN 17
#define CHARGE_DETECT_PIN 16
#define POWER_SWITCH_PIN 4

// ---------- ALERT OUTPUT ----------
#define POWER_LED 13
#define SOS_LED 12
#define BUZZER 14

#define HX_DOUT 21
#define HX_SCK 19

HX711 scale;

struct Vitals {
  float heartRate;
  float spo2;
  bool valid;
};

// ---------- Battery & Charging ----------
float batteryLevel = 100.0;
bool charging = false;
unsigned long lastBatteryUpdate = 0;

void setup() {
  Serial.begin(115200);

  pinMode(SOS_PIN, INPUT_PULLUP);
  pinMode(CHARGE_DETECT_PIN, INPUT_PULLUP);
  pinMode(POWER_SWITCH_PIN, INPUT_PULLUP);

  pinMode(POWER_LED, OUTPUT);
  pinMode(SOS_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  scale.begin(HX_DOUT, HX_SCK);
  scale.set_scale(420.0);
  scale.tare();

  Wire.begin(25, 26);

  if (!mpu.begin()) {
    Serial.println("❌ MPU6050 not found");
    while (1) delay(10);
  }

  Serial.println("✅ MPU6050 ready");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  randomSeed(analogRead(0));
}

void loop() {

  unsigned long now = millis();

  // ----- POWER STATE -----
  systemOn = (digitalRead(POWER_SWITCH_PIN) == LOW);
  charging = (digitalRead(CHARGE_DETECT_PIN) == LOW);

  digitalWrite(POWER_LED, systemOn);

  updateBattery();
  checkweight();

  // ----- SYSTEM OFF -----
  if (!systemOn) {
    digitalWrite(SOS_LED, LOW);
    digitalWrite(BUZZER, LOW);
    return;
  }

  // ----- SOS ALERT -----
  bool sos = digitalRead(SOS_PIN) == LOW;

  if (sos) {

    if (now - lastBlink > 300) {
      lastBlink = now;
      ledState = !ledState;

      digitalWrite(SOS_LED, ledState);
      digitalWrite(BUZZER, ledState);
    }

  } else {

    digitalWrite(SOS_LED, LOW);
    digitalWrite(BUZZER, LOW);

  }

  // ----- MAIN SYSTEM UPDATE -----
  if (now - lastLoopTime >= LOOP_INTERVAL) {

    lastLoopTime = now;

    Serial.println("\n==============================");
    Serial.println("🔁 System Update");

    checkIMU();
    checkVitals();

    Serial.print("Load: ");
    Serial.println(lastWeight);

    sendBLEPacket();
  }
}

// ---------- HX711 ----------
void checkweight() {
  if (millis() - lastHXRead < 100) return;
  lastHXRead = millis();

  if (scale.is_ready()) {
    lastWeight = scale.get_units(3);
  }
}

// ---------- IMU ----------
void checkIMU() {

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;

  float accMag = sqrt(ax * ax + ay * ay + az * az);

  float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
  float roll = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;

  lastPitch = pitch;
  lastRoll = roll;
  lastAccMag = accMag;

  fallFlag = (accMag < 3 || accMag > 25);

  Serial.print("Pitch: ");
  Serial.print(pitch);
  Serial.print(" Roll: ");
  Serial.print(roll);
  Serial.print(" Acc: ");
  Serial.println(accMag);

  if (fallFlag) {
    Serial.println("Possible fall detected");
  }
}

// ---------- MAX30102 MOCK ----------
Vitals readMAX30102() {
  Vitals v;
  v.heartRate = random(65, 90);
  v.spo2 = random(95, 100);
  v.valid = true;
  return v;
}

void checkVitals() {

  Vitals v = readMAX30102();

  lastHR = v.heartRate;
  lastSpO2 = v.spo2;

  Serial.print("HR: ");
  Serial.print(v.heartRate);
  Serial.print(" BPM SpO2: ");
  Serial.println(v.spo2);
}

// ---------- BATTERY ----------
void updateBattery() {

  unsigned long now = millis();

  if (now - lastBatteryUpdate < 1000) return;

  lastBatteryUpdate = now;

  if (charging)
    batteryLevel += 0.3;
  else
    batteryLevel -= 0.05;

  batteryLevel = constrain(batteryLevel, 0, 100);

  Serial.print("Battery: ");
  Serial.print(batteryLevel);
  Serial.println("%");
}

// ---------- BLE ----------
void sendBLEPacket() {

  Serial.print("{");
  Serial.print("\"pitch\":"); Serial.print(lastPitch); Serial.print(",");
  Serial.print("\"roll\":"); Serial.print(lastRoll); Serial.print(",");
  Serial.print("\"acc\":"); Serial.print(lastAccMag); Serial.print(",");
  Serial.print("\"sos\":"); Serial.print(digitalRead(SOS_PIN)==LOW);
  Serial.println("}");
}