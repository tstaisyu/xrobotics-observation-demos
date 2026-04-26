#pragma once

#include <cmath>
#include <cstring>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;

class String {
 public:
  String();
  String(const char*);
  void trim();
  bool startsWith(const char*) const;
  String substring(int) const;
  int toInt() const;
  String& operator=(const char*);
  String& operator+=(char);
  bool operator==(const char*) const;
};

unsigned long millis();
void delay(unsigned long ms);

using std::strcmp;
using std::sqrt;
