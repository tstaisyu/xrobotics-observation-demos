#include <Arduino.h>
#include <M5Stack.h>
#include <math.h>

enum MotionState {
  STABLE,
  MOTION_SPIKE
};

const float kSpikeThreshold = 0.35f;

MotionState currentState = STABLE;
MotionState previousState = STABLE;

float prevAx = 0.0f;
float prevAy = 0.0f;
float prevAz = 0.0f;
float currentDelta = 0.0f;
bool hasPreviousAccel = false;

const char* toStateString(MotionState state) {
  switch (state) {
    case STABLE:
      return "STABLE";
    case MOTION_SPIKE:
      return "MOTION_SPIKE";
    default:
      return "UNKNOWN";
  }
}

void drawScreen(float ax, float ay, float az, float delta, MotionState state) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("log-003 serial ui");
  M5.Lcd.println();
  M5.Lcd.printf("ax: %.2f\n", ax);
  M5.Lcd.printf("ay: %.2f\n", ay);
  M5.Lcd.printf("az: %.2f\n", az);
  M5.Lcd.println();
  M5.Lcd.printf("delta: %.3f\n", delta);
  M5.Lcd.printf("th: %.2f\n", kSpikeThreshold);
  M5.Lcd.println();

  if (state == MOTION_SPIKE) {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("STATE: MOTION_SPIKE");
  } else {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("STATE: STABLE");
  }
}

void emitStateChange(MotionState state, float delta, unsigned long timestampMs) {
  Serial.print("{\"state\":\"");
  Serial.print(toStateString(state));
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
  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
  M5.update();

  float ax, ay, az;
  M5.IMU.getAccelData(&ax, &ay, &az);

  unsigned long now = millis();

  if (hasPreviousAccel) {
    float dx = ax - prevAx;
    float dy = ay - prevAy;
    float dz = az - prevAz;
    currentDelta = static_cast<float>(sqrt(dx * dx + dy * dy + dz * dz));

    if (currentDelta >= kSpikeThreshold) {
      currentState = MOTION_SPIKE;
    } else {
      currentState = STABLE;
    }

    if (currentState != previousState) {
      emitStateChange(currentState, currentDelta, now);
      previousState = currentState;
    }
  } else {
    hasPreviousAccel = true;
    previousState = currentState;
  }

  prevAx = ax;
  prevAy = ay;
  prevAz = az;

  drawScreen(ax, ay, az, currentDelta, currentState);

  delay(100);
}
