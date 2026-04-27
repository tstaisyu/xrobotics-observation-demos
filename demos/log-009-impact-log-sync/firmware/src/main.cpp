#include <Arduino.h>
#include <M5Stack.h>
#include <Preferences.h>
#include <math.h>
#include <string.h>

const float kGravityBaseline = 1.0f;
const float kImpactThreshold = 0.35f;
const float kMediumThreshold = 0.70f;
const float kHighThreshold = 1.20f;
const unsigned long kImpactCooldownMs = 500;
const unsigned long kClearHoldMs = 1500;
const size_t kMaxEvents = 64;

struct ImpactEvent {
  unsigned long timestamp;
  float delta;
  char level[8];
};

enum DisplayMode {
  LIVE_MODE,
  LOG_MODE
};

Preferences preferences;
ImpactEvent impactLog[kMaxEvents];
size_t impactCount = 0;
size_t logIndex = 0;

DisplayMode currentMode = LIVE_MODE;

unsigned long lastImpactTime = 0;
float latestDelta = 0.0f;
char latestLevel[8] = "NONE";
unsigned long latestTimestamp = 0;
bool clearHandled = false;
char commandBuffer[32] = {0};
size_t commandLength = 0;

#if defined(__INTELLISENSE__)
struct MockSerialEx : MockSerial {
  int available();
  char read();
};
#define SERIAL_PORT_REF reinterpret_cast<MockSerialEx&>(Serial)
#else
#define SERIAL_PORT_REF Serial
#endif

void clearStoredLogs();
void sendClearDone();

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

void drawLogsClearedScreen() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.println("LOGS CLEARED");
  M5.Lcd.println();
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("COUNT: 0");
}

void refreshLatestFromLog() {
  if (impactCount == 0) {
    latestDelta = 0.0f;
    strcpy(latestLevel, "NONE");
    latestTimestamp = 0;
    logIndex = 0;
    return;
  }

  const ImpactEvent& event = impactLog[impactCount - 1];
  latestDelta = event.delta;
  strcpy(latestLevel, event.level);
  latestTimestamp = event.timestamp;

  if (logIndex >= impactCount) {
    logIndex = impactCount - 1;
  }
}

void savePersistentLog() {
  preferences.putUInt("count", (unsigned int)impactCount);
  preferences.putBytes("events", impactLog, impactCount * sizeof(ImpactEvent));
}

void loadPersistentLog() {
  unsigned int storedCount = preferences.getUInt("count", 0);

  if (storedCount > kMaxEvents) {
    storedCount = kMaxEvents;
  }

  impactCount = storedCount;

  if (impactCount > 0) {
    preferences.getBytes("events", impactLog, impactCount * sizeof(ImpactEvent));
  }

  refreshLatestFromLog();
}

void clearStoredLogs() {
  impactCount = 0;
  memset(impactLog, 0, sizeof(impactLog));
  preferences.clear();
  refreshLatestFromLog();
  currentMode = LIVE_MODE;
  drawLogsClearedScreen();
}

void sendClearDone() {
  Serial.println("CLEAR_DONE");
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

void emitSyncDump() {
  Serial.println("SYNC_START");

  for (size_t i = 0; i < impactCount; ++i) {
    Serial.print("{\"index\":");
    Serial.print((int)i);
    Serial.print(",\"type\":\"IMPACT\",\"level\":\"");
    Serial.print(impactLog[i].level);
    Serial.print("\",\"delta\":");
    Serial.print(impactLog[i].delta, 3);
    Serial.print(",\"timestamp\":");
    Serial.print(impactLog[i].timestamp);
    Serial.println("}");
  }

  Serial.println("SYNC_END");
}

void storeImpactEvent(float delta, const char* level, unsigned long timestampMs) {
  if (impactCount < kMaxEvents) {
    impactLog[impactCount].timestamp = timestampMs;
    impactLog[impactCount].delta = delta;
    strncpy(impactLog[impactCount].level, level, sizeof(impactLog[impactCount].level) - 1);
    impactLog[impactCount].level[sizeof(impactLog[impactCount].level) - 1] = '\0';
    impactCount++;
  }

  latestDelta = delta;
  strncpy(latestLevel, level, sizeof(latestLevel) - 1);
  latestLevel[sizeof(latestLevel) - 1] = '\0';
  latestTimestamp = timestampMs;

  if (impactCount > 0) {
    logIndex = impactCount - 1;
  }

  savePersistentLog();
}

void handleCommand(const char* command) {
  if (strcmp(command, "SYNC_REQUEST") == 0) {
    emitSyncDump();
    return;
  }

  if (strcmp(command, "CLEAR_LOGS") == 0) {
    clearStoredLogs();
    sendClearDone();
  }
}

void readSerialCommands() {
  while (SERIAL_PORT_REF.available() > 0) {
    char c = SERIAL_PORT_REF.read();

    if (c == '\n') {
      commandBuffer[commandLength] = '\0';
      handleCommand(commandBuffer);
      commandLength = 0;
      commandBuffer[0] = '\0';
    } else if (c != '\r' && commandLength < sizeof(commandBuffer) - 1) {
      commandBuffer[commandLength++] = c;
      commandBuffer[commandLength] = '\0';
    }
  }
}

void handleButtons() {
  if (M5.BtnB.wasPressed()) {
    currentMode = currentMode == LIVE_MODE ? LOG_MODE : LIVE_MODE;
    drawScreen();
  }

  if (currentMode == LIVE_MODE) {
    if (M5.BtnC.pressedFor(kClearHoldMs) && !clearHandled) {
      clearHandled = true;
      clearStoredLogs();
    }

    if (!M5.BtnC.isPressed()) {
      clearHandled = false;
    }

    return;
  }

  if (impactCount == 0) {
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

  preferences.begin("impactlog", false);
  loadPersistentLog();

  M5.Lcd.setRotation(1);
  drawScreen();
}

void loop() {
  M5.update();
  handleButtons();
  readSerialCommands();

  float ax, ay, az;
  M5.IMU.getAccelData(&ax, &ay, &az);

  float magnitude = sqrtf(ax * ax + ay * ay + az * az);
  float delta = fabsf(magnitude - kGravityBaseline);
  unsigned long now = millis();

  if (delta >= kImpactThreshold && now - lastImpactTime >= kImpactCooldownMs) {
    const char* level = levelForDelta(delta);
    ImpactEvent event = {now, delta, {0}};

    strncpy(event.level, level, sizeof(event.level) - 1);
    event.level[sizeof(event.level) - 1] = '\0';

    lastImpactTime = now;
    storeImpactEvent(delta, level, now);
    emitImpactEvent(event);
    drawScreen();
  }

  delay(100);
}

#undef SERIAL_PORT_REF
