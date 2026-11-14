#include "game.h"
#include "buttons.h"
#include "rally.h"

Game currentGame;

void setup() {
  init_interface();
  rallySetup();  // Temporary direct boot for rally development
}

void loop() {
  if (Serial.available()) buttonsDebugKey(Serial.read());
  rallyLoop();
}
