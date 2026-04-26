#include <Arduino.h>
#include <M5Stack.h>
#include <math.h>

const float kGravityBaseline = 1.0f;
const float kImpactThreshold = 0.35f;
const float kMediumThreshold = 0.70f;
const float kHighThreshold = 1.20f;
const unsigned long kImpactCooldownMs = 500;

float prevDelta = 0.0f;
unsigned long lastImpactTime = 0;

const char* levelForDelta(float delta) {
  if (delta >= kHighThreshold) {
    return "HIGH";
  }

  if (delta >= kMediumThreshold) {
    return "MEDIUM";
  }

  return "LOW";
}

void drawFixedScreen() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.println("IMPACT LOGGER");
  M5.Lcd.println("IMU ACTIVE");
  M5.Lcd.println();
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("SERIAL JSON OUT");
}

void emitImpactEvent(const char* level, float delta, unsigned long timestampMs) {
  Serial.print("{\"type\":\"IMPACT\",\"level\":\"");
  Serial.print(level);
  Serial.print("\",\"delta\":");
  Serial.print(delta, 3);
  Serial.print(",\"timestamp\":");
  Serial.print(timestampMs);
  Serial.println("}");
}

void setup() {
  M5.begin();
  M5.IMU.Init();
  Serial.begin(115200);

  M5.Lcd.setRotation(1);
  drawFixedScreen();
}

void loop() {
  M5.update();

  float ax, ay, az;
  M5.IMU.getAccelData(&ax, &ay, &az);

  float magnitude = sqrtf(ax * ax + ay * ay + az * az);
  float delta = fabsf(magnitude - kGravityBaseline);
  unsigned long now = millis();

  if (delta >= kImpactThreshold && now - lastImpactTime >= kImpactCooldownMs) {
    lastImpactTime = now;
    prevDelta = delta;
    emitImpactEvent(levelForDelta(delta), delta, now);
  }

  delay(100);
}
