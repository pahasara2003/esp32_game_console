#include <Arduino.h>
#include "buttons.h"
#include "game.h"
namespace {
bool active = false;
int lastDirection = -1;
unsigned long lastRepeat = 0;
}
void lifeInputRequireRelease() { buttonsRequireRelease(); lastDirection = -1; }
void lifeInputEnd() { active = false; }
void lifeInputBegin() { active = true; lifeInputRequireRelease(); }
int lifeInputPoll(bool repeat) {
  if (!active) return -1;
  int command = buttonsNextPress();
  if (command >= 0 && command < 4) {
    lastDirection = command;
    lastRepeat = millis();
    return command;
  }
  if (command == 4) return 4;
  uint8_t held = buttonsHeld() & 15;
  if (lastDirection >= 0 && !(held & (1 << lastDirection))) lastDirection = -1;
  if (repeat && lastDirection >= 0 && millis() - lastRepeat >= 140) {
    lastRepeat = millis();
    return lastDirection;
  }
  return -1;
}
