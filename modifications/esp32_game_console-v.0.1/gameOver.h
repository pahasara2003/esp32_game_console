#include "TFT_eSPI.h"
// Stylish Game Over Menu Function
// Add this to your game code

#include "fonts.h"
bool showGameOver = true;
bool gameOverSelection = false;
// ===== Game Over Menu =====

void gameOverButtons() {
  int d = 30;
  tft.setTextFont(1);
  tft.fillRect(20, 140 + d, 130, 40, gameOverSelection ? TEXT_COLOR : TILE_COLOR);
  tft.setTextColor(!gameOverSelection ? TEXT_COLOR : TILE_COLOR);
  tft.drawString("New Game", 30, 150 + d);
  tft.fillRect(SCREEN_W - 150, 140 + d, 130, 40, !gameOverSelection ? TEXT_COLOR : TILE_COLOR);
  tft.setTextColor(gameOverSelection ? TEXT_COLOR : TILE_COLOR);
  tft.drawString("Main Menu", SCREEN_W - 140, 150 + d);
}

void showGameOverMenu() {

  if (showGameOver) {
    tft.fillScreen(TFT_YELLOW);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(40, 65);
    tft.println("GAME OVER!");
    showGameOver = false;

    tft.setTextFont(1);
    tft.setTextColor(TFT_BLACK);
    tft.drawString("Current ", 45, 85);
    tft.drawString("High ", SCREEN_W - 130, 85);
    tft.setTextFont(2);
    tft.setTextColor(TFT_BLUE);
    tft.drawString(String(score), 70, 115);

    preferences.begin("gameConsole", false);
    highscore = preferences.getInt((app + "_high").c_str(), 0);

    // Update high score if beaten
    if (score > highscore) {
      highscore = score;  // Update variable
      preferences.putInt((app + "_high").c_str(), score);
    }

    // Display high score (now it's always correct)
    tft.drawString(String(highscore), SCREEN_W - 100, 115);
    preferences.end();
    gameOverButtons();
  }

  if (command == 3) {
    gameOverSelection = true;
    gameOverButtons();
    command = -1;
  }
  if (command == 1) {
    gameOverSelection = false;
    gameOverButtons();
    command = -1;
  }
  if (command == 4) {
    preferences.begin("gameConsole", false);
    if (gameOverSelection) {
      preferences.putBool("getApp", true);
    } else {
      preferences.putBool("getApp", false);
    }
    preferences.end();
    esp_restart();
  }
}
