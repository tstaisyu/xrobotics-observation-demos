#include <M5Stack.h>

enum SystemState {
  NORMAL,
  TILTING,
  LATCHED_ALERT
};

enum ControlMode {
  AUTO,
  MANUAL
};

SystemState currentState = NORMAL;
ControlMode currentMode = AUTO;
SystemState prevState = NORMAL;

const float tiltThreshold = 0.50;
const unsigned long alertDelayMs = 2000;

unsigned long tiltStartTime = 0;

const char* stateToString(SystemState state) {
  switch (state) {
    case NORMAL:        return "NORMAL";
    case TILTING:       return "TILTING";
    case LATCHED_ALERT: return "LATCHED_ALERT";
    default:            return "UNKNOWN";
  }
}

const char* modeToString(ControlMode mode) {
  switch (mode) {
    case AUTO:   return "AUTO";
    case MANUAL: return "MANUAL";
    default:     return "UNKNOWN";
  }
}

void drawScreen(float ax, float ay, float az, bool isTilted, unsigned long tiltedMs) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("Latched Alert Demo");

  M5.Lcd.println();
  M5.Lcd.printf("MODE : %s\n", modeToString(currentMode));
  M5.Lcd.printf("STATE: %s\n", stateToString(currentState));

  M5.Lcd.println();
  M5.Lcd.printf("ax: %.2f\n", ax);
  M5.Lcd.printf("ay: %.2f\n", ay);
  M5.Lcd.printf("az: %.2f\n", az);

  M5.Lcd.println();
  M5.Lcd.printf("tilt: %s\n", isTilted ? "YES" : "NO");
  M5.Lcd.printf("hold: %lu ms\n", tiltedMs);

  M5.Lcd.println();
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.println("BtnA: AUTO/MANUAL");
  M5.Lcd.println("BtnB: RESET ALERT");

  M5.Lcd.println();
  switch (currentState) {
    case NORMAL:
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.println("STATUS NORMAL");
      break;
    case TILTING:
      M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.println("STATUS TILTING");
      break;
    case LATCHED_ALERT:
      M5.Lcd.setTextColor(RED);
      M5.Lcd.println("STATUS ALERT HOLD");
      break;
  }
}

void setup() {
  Serial.begin(115200);

  M5.begin();
  M5.IMU.Init();

  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    if (currentMode == AUTO) {
      currentMode = MANUAL;
      tiltStartTime = 0;
    } else {
      currentMode = AUTO;
      tiltStartTime = 0;
    }
  }

  if (M5.BtnB.wasPressed()) {
    currentState = NORMAL;
    tiltStartTime = 0;
  }

  float ax, ay, az;
  M5.IMU.getAccelData(&ax, &ay, &az);

  bool isTilted = abs(ax) > tiltThreshold;
  unsigned long now = millis();
  unsigned long tiltedMs = 0;

  if (currentMode == AUTO) {
    if (currentState == LATCHED_ALERT) {
      // アラート保持中は何もしない
      tiltedMs = 0;
    } else {
      if (!isTilted) {
        currentState = NORMAL;
        tiltStartTime = 0;
      } else {
        if (tiltStartTime == 0) {
          tiltStartTime = now;
        }

        tiltedMs = now - tiltStartTime;

        if (tiltedMs >= alertDelayMs) {
          currentState = LATCHED_ALERT;
          tiltStartTime = 0;
        } else {
          currentState = TILTING;
        }
      }
    }
  } else {
    // MANUAL中は自動判定しない
    tiltStartTime = 0;
    tiltedMs = 0;
  }

  drawScreen(ax, ay, az, isTilted, tiltedMs);

  if (currentState != prevState) {
    Serial.print(stateToString(prevState));
    Serial.print(" -> ");
    Serial.println(stateToString(currentState));

    prevState = currentState;
  }
 
  delay(100);
}