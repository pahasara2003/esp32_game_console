#include "game.h"
#include "birthday.h"

// TEMPORARY loudness test: loops the birthday melody forever.
// Restore the normal start-up when done.
struct TestNote { uint16_t frequency, beats; };
static const TestNote TEST_SONG[] = {
  {262,1},{262,1},{294,2},{262,2},{349,2},{330,4},
  {262,1},{262,1},{294,2},{262,2},{392,2},{349,4},
  {262,1},{262,1},{523,2},{440,2},{349,2},{330,2},{294,4},
  {466,1},{466,1},{440,2},{349,2},{392,2},{349,4}
};

Game currentGame;

void setup() {
  init_interface();
  for (;;) {
    for (const TestNote &n : TEST_SONG) {
      setBuzzerTone(n.frequency * 2);
      delay(n.beats * 155UL * 8 / 10);
      setBuzzerTone(0);
      delay(n.beats * 155UL * 2 / 10);
    }
    delay(800);
  }
}

void loop() {
  currentGame.gameLoop();
}
