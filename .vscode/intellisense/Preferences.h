#pragma once

#include "Arduino.h"

class Preferences {
 public:
  bool begin(const char* name, bool readOnly = false);
  void end();
  void clear();
  unsigned int getUInt(const char* key, unsigned int defaultValue = 0);
  size_t getBytesLength(const char* key);
  size_t getBytes(const char* key, void* buf, size_t maxLen);
  size_t putUInt(const char* key, unsigned int value);
  size_t putBytes(const char* key, const void* value, size_t len);
};
