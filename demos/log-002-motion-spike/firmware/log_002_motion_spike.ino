#include <M5Stack.h>

enum SystemState {
  STABLE,
  MOTION_SPIKE
};

SystemState currentState = STABLE;
SystemState prevState = STABLE;

const float spikeThreshold = 0.35;              // 急変判定しきい値
const unsigned long spikeHoldMs = 800;          // SPIKE表示を保持する時間

float prevAx = 0.0;
float prevAy = 0.0;
float prevAz = 0.0;

bool hasPrevAccel = false;
unsigned long spikeStartTime = 0;

const char* stateToString(SystemState state) {
  switch (state) {
    case STABLE:       return "STABLE";
    case MOTION_SPIKE: return "MOTION_SPIKE";
    default:           return "UNKNOWN";
  }
}

void drawScreen(float ax, float ay, float az, float delta) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("Motion Spike Demo");

  M5.Lcd.println();
  M5.Lcd.printf("ax: %.2f\n", ax);
  M5.Lcd.printf("ay: %.2f\n", ay);
  M5.Lcd.printf("az: %.2f\n", az);

  M5.Lcd.println();
  M5.Lcd.printf("delta: %.2f\n", delta);
  M5.Lcd.printf("threshold: %.2f\n", spikeThreshold);

  M5.Lcd.println();
  if (currentState == MOTION_SPIKE) {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("STATE: MOTION_SPIKE");
  } else {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("STATE: STABLE");
  }
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
  float delta = 0.0;

  if (hasPrevAccel) {
    float dx = ax - prevAx;
    float dy = ay - prevAy;
    float dz = az - prevAz;

    delta = sqrt(dx * dx + dy * dy + dz * dz);

    if (delta >= spikeThreshold) {
      currentState = MOTION_SPIKE;
      spikeStartTime = now;
    }
  } else {
    hasPrevAccel = true;
  }

  if (currentState == MOTION_SPIKE) {
    if (now - spikeStartTime >= spikeHoldMs) {
      currentState = STABLE;
    }
  }

  if (currentState != prevState) {
    Serial.print(stateToString(prevState));
    Serial.print(" -> ");
    Serial.print(stateToString(currentState));
    Serial.print(" | delta=");
    Serial.println(delta, 3);

    prevState = currentState;
  }

  prevAx = ax;
  prevAy = ay;
  prevAz = az;

  drawScreen(ax, ay, az, delta);

  delay(100);
}