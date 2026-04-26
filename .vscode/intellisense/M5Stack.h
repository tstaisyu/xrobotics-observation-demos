#pragma once

#include "Arduino.h"

static const int BLACK = 0;
static const int WHITE = 1;
static const int RED = 2;
static const int GREEN = 3;
static const int CYAN = 4;

struct MockLcd {
  void fillScreen(int color);
  void setCursor(int x, int y);
  void setTextSize(int size);
  void setTextColor(int color);
  void setRotation(int rotation);
  void println(const char* text = "");
  void printf(const char* format, ...);
};

struct MockIMU {
  void Init();
  void getAccelData(float* ax, float* ay, float* az);
};

struct MockSerial {
  void begin(unsigned long baud);
  void print(const char* text);
  void print(int value);
  void print(unsigned long value);
  void print(float value, int digits = 2);
  void println(const char* text);
};

struct MockButton {
  bool wasPressed();
};

struct MockM5Class {
  MockLcd Lcd;
  MockIMU IMU;
  MockButton BtnB;
  void begin();
  void update();
};

extern MockM5Class M5;
extern MockSerial Serial;
