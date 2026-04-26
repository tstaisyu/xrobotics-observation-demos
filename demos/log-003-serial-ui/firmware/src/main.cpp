#include <Arduino.h>
#include <M5Stack.h>
#include <math.h>

enum SystemMode {
  MONITORING,
  PROTECT_MODE
};

SystemMode currentMode = MONITORING;

const float spikeThreshold = 0.35f;
const int protectSpikeLimit = 3;
const unsigned long spikeCooldownMs = 600;

float prevAx = 0.0f;
float prevAy = 0.0f;
float prevAz = 0.0f;
bool hasPrevAccel = false;

int spikeCount = 0;
unsigned long lastSpikeTime = 0;

const char* modeToString(SystemMode mode) {
  switch (mode) {
    case MONITORING: return "MONITORING";
    case PROTECT_MODE: return "PROTECT_MODE";
    default: return "UNKNOWN";
  }
}

void sendEvent(const char* eventType, float delta) {
  Serial.print("{\"event\":\"");
  Serial.print(eventType);
  Serial.print("\",\"mode\":\"");
  Serial.print(modeToString(currentMode));
  Serial.print("\",\"spikeCount\":");
  Serial.print(spikeCount);
  Serial.print(",\"delta\":");
  Serial.print(delta, 3);
  Serial.print(",\"timestamp\":");
  Serial.print(millis());
  Serial.println("}");
}

void setup() {
  M5.begin();
  M5.IMU.Init();
  Serial.begin(115200);

  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.println("SENSOR NODE");
  M5.Lcd.println("IMU ACTIVE");

  sendEvent("SYSTEM_START", 0.0f);
}

void loop() {
  M5.update();

  if (M5.BtnB.wasPressed()) {
    currentMode = MONITORING;
    spikeCount = 0;
    sendEvent("RESET", 0.0f);
  }

  float ax, ay, az;
  M5.IMU.getAccelData(&ax, &ay, &az);

  float delta = 0.0f;
  unsigned long now = millis();

  if (hasPrevAccel) {
    float dx = ax - prevAx;
    float dy = ay - prevAy;
    float dz = az - prevAz;
    delta = sqrtf(dx * dx + dy * dy + dz * dz);

    bool spikeDetected =
      delta >= spikeThreshold &&
      now - lastSpikeTime >= spikeCooldownMs;

    if (spikeDetected) {
      lastSpikeTime = now;

      if (currentMode == MONITORING) {
        spikeCount++;
        sendEvent("MOTION_SPIKE", delta);

        if (spikeCount >= protectSpikeLimit) {
          currentMode = PROTECT_MODE;
          sendEvent("ENTER_PROTECT_MODE", delta);
        }
      } else {
        sendEvent("PROTECT_MODE_ACTIVE", delta);
      }
    }
  } else {
    hasPrevAccel = true;
  }

  prevAx = ax;
  prevAy = ay;
  prevAz = az;
  delay(100);
}
