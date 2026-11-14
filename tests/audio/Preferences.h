#pragma once
#include <stdint.h>
// Storage is stubbed; these tests exercise synthesis, not ESP32 NVS.
struct Preferences {
  void begin(const char *, bool) {}
  void end() {}
  uint8_t getUChar(const char *, uint8_t fallback) { return fallback; }
  bool getBool(const char *, bool fallback) { return fallback; }
  void putUChar(const char *, uint8_t) {}
  void putBool(const char *, bool) {}
};
