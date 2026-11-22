#include "game.h"
#include "buttons.h"
#include "birthday.h"
#include <Preferences.h>

Game currentGame;

void setup() {
  init_interface();
#ifndef BIRTHDAY_DEV
  Preferences startupPrefs;
  startupPrefs.begin("console", false);
  const char *buildId = __DATE__ " " __TIME__;
  bool birthdaySeen = startupPrefs.getString("bdayBuild", "") == buildId;
  startupPrefs.end();
  if (!birthdaySeen) {
    showBirthdayScene();
    startupPrefs.begin("console", false);
    startupPrefs.putString("bdayBuild", buildId);
    startupPrefs.end();
  }
  switchGame(0);  // home launcher
#endif
}

void loop() {
#ifdef BIRTHDAY_DEV
  static bool birthdayFinished = false;
  if (!birthdayFinished) {
    showBirthdayScene();
    birthdayFinished = true;
    switchGame(0);
  }
#endif
  if (Serial.available()) buttonsDebugKey(Serial.read());
  currentGame.gameLoop();
}
