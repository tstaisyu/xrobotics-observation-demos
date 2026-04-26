#include <Arduino.h>
#include <M5Stack.h>
#include <math.h>
#include <string.h>

const float kGravityBaseline = 1.0f;
const float kImpactThreshold = 0.35f;
const float kMediumThreshold = 0.70f;
const float kHighThreshold = 1.20f;
const unsigned long kImpactCooldownMs = 500;
const size_t kMaxEvents = 64;

struct ImpactEvent {
  unsigned long timestamp;
  float delta;
  const char* level;
};

enum DisplayMode {
  LIVE_MODE,
  LOG_MODE
};

ImpactEvent impactLog[kMaxEvents];
size_t impactCount = 0;
size_t logIndex = 0;

DisplayMode currentMode = LIVE_MODE;

unsigned long lastImpactTime = 0;
float latestDelta = 0.0f;
const char* latestLevel = "NONE";
unsigned long latestTimestamp = 0;

const char* levelForDelta(float delta) {
  if (delta >= kHighThreshold) {
    return "HIGH";
  }

  if (delta >= kMediumThreshold) {
    return "MEDIUM";
  }

  return "LOW";
}

void setLevelColor(const char* level) {
  if (strcmp(level, "HIGH") == 0) {
    M5.Lcd.setTextColor(RED);
  } else if (strcmp(level, "MEDIUM") == 0) {
    M5.Lcd.setTextColor(YELLOW);
  } else if (strcmp(level, "LOW") == 0) {
    M5.Lcd.setTextColor(GREEN);
  } else {
    M5.Lcd.setTextColor(CYAN);
  }
}

void drawLiveScreen() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("LIVE MODE");
  M5.Lcd.println();
  M5.Lcd.printf("COUNT: %u\n", (unsigned int)impactCount);
  setLevelColor(latestLevel);
  M5.Lcd.printf("LAST: %s\n", latestLevel);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.printf("DELTA: %.3f\n", latestDelta);
}

void drawLogScreen() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("LOG MODE");
  M5.Lcd.println();

  if (impactCount == 0) {
    M5.Lcd.println("NO LOGS");
    return;
  }

  const ImpactEvent& event = impactLog[logIndex];
  M5.Lcd.printf("LOG %u / %u\n", (unsigned int)(logIndex + 1), (unsigned int)impactCount);
  setLevelColor(event.level);
  M5.Lcd.printf("LEVEL: %s\n", event.level);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.printf("DELTA: %.3f\n", event.delta);
  M5.Lcd.printf("TIME: %lu\n", event.timestamp);
}

void drawScreen() {
  if (currentMode == LIVE_MODE) {
    drawLiveScreen();
  } else {
    drawLogScreen();
  }
}

void emitImpactEvent(const ImpactEvent& event) {
  Serial.print("{\"type\":\"IMPACT\",\"level\":\"");
  Serial.print(event.level);
  Serial.print("\",\"delta\":");
  Serial.print(event.delta, 3);
  Serial.print(",\"timestamp\":");
  Serial.print(event.timestamp);
  Serial.println("}");
}

void storeImpactEvent(float delta, const char* level, unsigned long timestampMs) {
  if (impactCount < kMaxEvents) {
    impactLog[impactCount].timestamp = timestampMs;
    impactLog[impactCount].delta = delta;
    impactLog[impactCount].level = level;
    impactCount++;
  }

  latestDelta = delta;
  latestLevel = level;
  latestTimestamp = timestampMs;

  if (impactCount > 0) {
    logIndex = impactCount - 1;
  }
}

void handleButtons() {
  if (M5.BtnB.wasPressed()) {
    currentMode = currentMode == LIVE_MODE ? LOG_MODE : LIVE_MODE;
    drawScreen();
  }

  if (currentMode != LOG_MODE || impactCount == 0) {
    return;
  }

  if (M5.BtnA.wasPressed() && logIndex > 0) {
    logIndex--;
    drawScreen();
  }

  if (M5.BtnC.wasPressed() && logIndex + 1 < impactCount) {
    logIndex++;
    drawScreen();
  }
}

void setup() {
  M5.begin();
  M5.IMU.Init();
  Serial.begin(115200);

  M5.Lcd.setRotation(1);
  drawScreen();
}

void loop() {
  M5.update();
  handleButtons();

  float ax, ay, az;
  M5.IMU.getAccelData(&ax, &ay, &az);

  float magnitude = sqrtf(ax * ax + ay * ay + az * az);
  float delta = fabsf(magnitude - kGravityBaseline);
  unsigned long now = millis();

  if (delta >= kImpactThreshold && now - lastImpactTime >= kImpactCooldownMs) {
    const char* level = levelForDelta(delta);
    ImpactEvent event = {now, delta, level};

    lastImpactTime = now;
    storeImpactEvent(delta, level, now);
    emitImpactEvent(event);
    drawScreen();
  }

  delay(100);
}
