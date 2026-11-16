#include "game.h"
#include "buttons.h"

Game currentGame;

void setup() {
  init_interface();
  switchGame(0);  // home launcher
}

void loop() {
  if (Serial.available()) buttonsDebugKey(Serial.read());
  currentGame.gameLoop();
}
