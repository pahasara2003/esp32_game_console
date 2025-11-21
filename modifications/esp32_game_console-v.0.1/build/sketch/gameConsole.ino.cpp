#include <Arduino.h>
#line 1 "/home/pahasara/Arduino/gameConsole/gameConsole.ino"
#include <Preferences.h>
#include "config.h"
#include "sounds.h"
String app;
bool getApp;

Preferences preferences;
#include "gameOver.h"

#include "runGame/runGame.h"
#include "menu.h"
#include "snakeGame.h"
#include "tetris.h"
#include "breakout.h"
#include "galaga.h"


#line 18 "/home/pahasara/Arduino/gameConsole/gameConsole.ino"
void setup();
#line 53 "/home/pahasara/Arduino/gameConsole/gameConsole.ino"
void loop();
#line 18 "/home/pahasara/Arduino/gameConsole/gameConsole.ino"
void setup() {
  preferences.begin("gameConsole", false);

  if(preferences.getBool("firstBoot",false)){
      preferences.putString("app", "about");
      preferences.putBool("getApp", true);
  }

  getApp = preferences.getBool("getApp", false);
  if (!getApp) {
    app = "menu";
  }
  else
    app = preferences.getString("app", "menu");
  preferences.putBool("getApp", false);


  preferences.end();
  beginKeyboard();
  pinMode(5, INPUT_PULLDOWN);
  pinMode(DISPLAY_PIN, OUTPUT);

  digitalWrite(DISPLAY_PIN,HIGH);



  if (app == "run") run_setup();
  else if (app == "snake") setupSnake();
  else if (app == "tetris") setupTetris();
  else if (app == "breakout") breakout_setup();
  else if (app == "galaga") galaga_setup();

  else menu_setup();
}

void loop() {
  unsigned long now = millis();
  
  // Single sleep check - removed duplicate
  if (now - last_action >= 60000) {
    goToSleep();
  }

  if (now - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = now;

    if(now - lastSound >= 150){
        if(app != "about")
          noTone(BUZZER_PIN);
        lastSound = now;
    }

    for (int i = 0; i < NUM_BUTTONS; i++) {
      int16_t raw = ads.readADC_SingleEnded(i);
      float voltage = raw * 0.000125;
      bool pressed = voltage > 1.5;
      if (voltage > 1.5) {
        command = i;
        UPDATE_INTERVAL = 180;
        last_action = now;
        tone(BUZZER_PIN, 300);
      } else {
        UPDATE_INTERVAL = 30;
      }
    }
    
    if (digitalRead(5) == HIGH) {
      command = 4;
      UPDATE_INTERVAL = 180;
      tone(BUZZER_PIN, 600);
      last_action = now;
    } else {
      UPDATE_INTERVAL = 50;
    }
  }


  if (app == "run") run_loop();
  if (app == "snake") loopSnake();
  if (app == "tetris") loopTetris();
  if (app == "menu") menu_loop();
  if (app == "breakout") breakout_loop();
  if (app == "galaga") galaga_loop();

}
