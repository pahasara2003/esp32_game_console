#include "buttons.h"
#include "tetris_input.h"
namespace { bool active = false; }
void tetrisInputEnd() { active = false; }
void tetrisInputRequireRelease() { buttonsRequireRelease(); }
void tetrisInputBegin() { active = true; buttonsRequireRelease(); }
uint8_t tetrisInputHeld() { return active ? buttonsHeld() : 0; }
uint8_t tetrisInputPoll() {
  if (!active) return 0;
  int command = buttonsNextPress();
  return command < 0 ? 0 : 1 << command;
}
