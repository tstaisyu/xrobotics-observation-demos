#include <Arduino.h>
#include <M5Stack.h>
#include <math.h>
#include <stdlib.h>
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

ImpactEvent impactLog[kMaxEvents];
size_t impactCount = 0;

unsigned long lastImpactTime = 0;
char commandBuffer[32] = {0};
size_t commandLength = 0;

float latestDelta = 0.0f;
const char* latestLevel = "NONE";
unsigned long latestTimestamp = 0;

#if defined(__INTELLISENSE__)
struct MockSerialEx : MockSerial {
  int available();
  char read();
};
#define SERIAL_PORT_REF reinterpret_cast<MockSerialEx&>(Serial)
#else
#define SERIAL_PORT_REF Serial
#endif

const char* levelForDelta(float delta) {
  if (delta >= kHighThreshold) {
    return "HIGH";
  }

  if (delta >= kMediumThreshold) {
    return "MEDIUM";
  }

  return "LOW";
}

void drawScreen() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("LOCAL IMPACT LOG");
  M5.Lcd.println();

  if (strcmp(latestLevel, "HIGH") == 0) {
    M5.Lcd.setTextColor(RED);
  } else if (strcmp(latestLevel, "MEDIUM") == 0) {
    M5.Lcd.setTextColor(YELLOW);
  } else if (strcmp(latestLevel, "LOW") == 0) {
    M5.Lcd.setTextColor(GREEN);
  } else {
    M5.Lcd.setTextColor(CYAN);
  }

  M5.Lcd.printf("LEVEL: %s\n", latestLevel);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.printf("DELTA: %.3f\n", latestDelta);
  M5.Lcd.printf("COUNT: %u\n", (unsigned int)impactCount);
  M5.Lcd.printf("LAST: %lu\n", latestTimestamp);
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

void emitCount() {
  Serial.print("{\"type\":\"COUNT\",\"count\":");
  Serial.print((int)impactCount);
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
}

void listEvents() {
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
}

void clearEvents() {
  impactCount = 0;
  latestDelta = 0.0f;
  latestLevel = "NONE";
  latestTimestamp = 0;
  Serial.println("{\"type\":\"CLEARED\",\"count\":0}");
}

void handleCommand(const char* command) {
  if (strcmp(command, "LIST") == 0) {
    listEvents();
    return;
  }

  if (strcmp(command, "CLEAR") == 0) {
    clearEvents();
    return;
  }

  if (strcmp(command, "COUNT") == 0) {
    emitCount();
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

void setup() {
  M5.begin();
  M5.IMU.Init();
  Serial.begin(115200);

  M5.Lcd.setRotation(1);
  drawScreen();
}

void loop() {
  M5.update();
  readSerialCommands();

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

#undef SERIAL_PORT_REF
