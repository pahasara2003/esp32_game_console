#pragma once
#include <stdint.h>

// Commands are the same portrait directions used by the launcher.
class BirthdayCode {
 public:
  bool press(int command, uint32_t now) {
    if (position && uint32_t(now - lastPress) > 4000) position = 0;
    if (command < 0) return false;
    if (command == position) {
      lastPress = now;
      if (++position == 4) { position = 0; return true; }
    } else {
      position = command == 0 ? 1 : 0;
      lastPress = now;
    }
    return false;
  }
 private:
  uint8_t position = 0;
  uint32_t lastPress = 0;
};
