#include <Arduino.h>
#include <M5Stack.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum SystemMode {
  MONITORING,
  PROTECT_MODE
};

SystemMode currentMode = MONITORING;

const float spikeThreshold = 0.35f;
int protectSpikeLimit = 3;
const unsigned long spikeCooldownMs = 600;

float prevAx = 0.0f;
float prevAy = 0.0f;
float prevAz = 0.0f;
bool hasPrevAccel = false;

int spikeCount = 0;
unsigned long lastSpikeTime = 0;
char inputBuffer[64] = {0};
size_t inputLength = 0;

#if defined(__INTELLISENSE__)
struct MockSerialEx : MockSerial {
  int available();
  char read();
};
#define SERIAL_PORT_REF reinterpret_cast<MockSerialEx&>(Serial)
#else
#define SERIAL_PORT_REF Serial
#endif

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
  Serial.print(",\"protectSpikeLimit\":");
  Serial.print(protectSpikeLimit);
  Serial.print(",\"delta\":");
  Serial.print(delta, 3);
  Serial.print(",\"timestamp\":");
  Serial.print(millis());
  Serial.println("}");
}

void drawFixedScreen() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.println("SENSOR NODE");
  M5.Lcd.println("IMU ACTIVE");
  M5.Lcd.println();
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.println("RULE ADJUSTABLE");
}

void handleSerialCommand(const char* line) {
  if (strncmp(line, "LIMIT:", 6) == 0) {
    int newLimit = atoi(line + 6);

    if (newLimit >= 1 && newLimit <= 20) {
      protectSpikeLimit = newLimit;
      spikeCount = 0;
      currentMode = MONITORING;
      sendEvent("RULE_UPDATED", 0.0f);
    }
  }

  if (strcmp(line, "RESET") == 0) {
    currentMode = MONITORING;
    spikeCount = 0;
    sendEvent("RESET", 0.0f);
  }
}

void readSerialCommands() {
  while (SERIAL_PORT_REF.available() > 0) {
    char c = SERIAL_PORT_REF.read();

    if (c == '\n') {
      inputBuffer[inputLength] = '\0';
      handleSerialCommand(inputBuffer);
      inputLength = 0;
      inputBuffer[0] = '\0';
    } else if (c != '\r' && inputLength < sizeof(inputBuffer) - 1) {
      inputBuffer[inputLength++] = c;
      inputBuffer[inputLength] = '\0';
    }
  }
}

void setup() {
  M5.begin();
  M5.IMU.Init();
  Serial.begin(115200);

  M5.Lcd.setRotation(1);
  drawFixedScreen();

  sendEvent("SYSTEM_START", 0.0f);
}

void loop() {
  M5.update();
  readSerialCommands();

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

#undef SERIAL_PORT_REF
