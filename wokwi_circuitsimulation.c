//Smart Crutch Circuit on wokwi 
//embedded c
//AI assisted
//written on 9th FEB 2025

#include "HX711.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

// ---------- Main Loop Timing ----------
unsigned long lastLoopTime = 0;
const unsigned long LOOP_INTERVAL = 3000; // 3 seconds per cycle
unsigned long lastHXRead = 0;

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
bool systemOn = true;   // derived directly from slide switch

#define SOS_PIN 17  // PUSH BUTTON
#define CHARGE_DETECT_PIN 16 // PUSH BUTTON
#define POWER_SWITCH_PIN 4   // SLIDE SWITCH

#define HX_DOUT  21
#define HX_SCK   19

HX711 scale;

struct Vitals {
  float heartRate;
  float spo2;
  bool valid;
};

// ---------- Battery & Charging ----------
float batteryLevel = 100.0;   // percentage
bool charging = false;        // USB connected or not
unsigned long lastBatteryUpdate = 0;

void setup() {
  Serial.begin(115200);

  pinMode(SOS_PIN, INPUT_PULLUP);
  pinMode(CHARGE_DETECT_PIN, INPUT_PULLUP);
  pinMode(POWER_SWITCH_PIN, INPUT_PULLUP);

  Serial.println("Hello, ESP32!");

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

  // ----- ALWAYS ACTIVE -----
  systemOn = (digitalRead(POWER_SWITCH_PIN) == LOW);
  charging = (digitalRead(CHARGE_DETECT_PIN) == LOW);

  updateBattery();
  checkweight();   // HX711 must always be sampled

  // ----- SYSTEM OFF STATE -----
  if (!systemOn) {
    static bool shown = false;
    if (!shown) {
      Serial.println("🔌 SYSTEM OFF (Charging Active)");
      shown = true;
    }
    return;
  }
  static bool shown = false;
  shown = false;

  // ----- MAIN SYSTEM UPDATE -----
  if (now - lastLoopTime >= LOOP_INTERVAL) {
    lastLoopTime = now;

    Serial.println("\n==============================");
    Serial.println("🔁 System Update");

    checkSOS();
    checkIMU();
    checkVitals();

    Serial.print("Load: ");
    Serial.println(lastWeight);

    sendBLEPacket();
  }
}

// ---------- SOS ----------
void checkSOS() {
  static unsigned long lastPress = 0;
  if (digitalRead(SOS_PIN) == LOW) {
    if (millis() - lastPress > 500) {
      Serial.println("🚨 SOS PRESSED!");
      lastPress = millis();
    }
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
  float roll  = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;

  lastPitch = pitch;
  lastRoll = roll;
  lastAccMag = accMag;

  fallFlag = (accMag < 3 || accMag > 25);

  Serial.print("Pitch: ");
  Serial.print(pitch);
  Serial.print(" deg | Roll: ");
  Serial.print(roll);
  Serial.print(" deg | AccMag: ");
  Serial.println(accMag);

  if (fallFlag) {
    Serial.println("Possible fall (free fall / impact)");
  }
}

// ---------- MAX30102 (Mock) ----------
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
  Serial.print(" BPM | SpO2: ");
  Serial.print(v.spo2);
  Serial.println(" %");
}

// ---------- BATTERY ----------
void updateBattery() {
  unsigned long now = millis();
  if (now - lastBatteryUpdate < 1000) return;
  lastBatteryUpdate = now;

  if (charging) {
    batteryLevel += 0.3;
    if (batteryLevel > 100) batteryLevel = 100;
  } else {
    batteryLevel -= 0.05;
    if (batteryLevel < 0) batteryLevel = 0;
  }

  Serial.print("🔋 Battery: ");
  Serial.print(batteryLevel, 1);
  Serial.print("% ");
  Serial.println(charging ? "(Charging)" : "(Discharging)");
}

// ---------- BLE ----------
void sendBLEPacket() {
  CrutchPacket pkt;

  pkt.load = lastWeight;
  pkt.pitch = lastPitch;
  pkt.roll = lastRoll;
  pkt.accMag = lastAccMag;
  pkt.heartRate = lastHR;
  pkt.spo2 = lastSpO2;
  pkt.battery = batteryLevel;
  pkt.fallDetected = fallFlag;
  pkt.sos = (digitalRead(SOS_PIN) == LOW);

  Serial.println("📡 BLE Packet → Mobile App");
  Serial.print("{ ");
  Serial.print("\"load\": "); Serial.print(pkt.load); Serial.print(", ");
  Serial.print("\"pitch\": "); Serial.print(pkt.pitch); Serial.print(", ");
  Serial.print("\"roll\": "); Serial.print(pkt.roll); Serial.print(", ");
  Serial.print("\"accMag\": "); Serial.print(pkt.accMag); Serial.print(", ");
  Serial.print("\"HR\": "); Serial.print(pkt.heartRate); Serial.print(", ");
  Serial.print("\"SpO2\": "); Serial.print(pkt.spo2); Serial.print(", ");
  Serial.print("\"battery\": "); Serial.print(pkt.battery); Serial.print(", ");
  Serial.print("\"fall\": "); Serial.print(pkt.fallDetected); Serial.print(", ");
  Serial.print("\"sos\": "); Serial.print(pkt.sos);
  Serial.println(" }");
}
