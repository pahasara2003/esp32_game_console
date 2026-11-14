#ifndef GAME_H
#define GAME_H

#include "config.h"
#include "buttons.h"
#include <functional>
#include <stdint.h>
#include <TFT_eSPI.h>
#include "Petme8x8.h"
#include "icons.h"

extern TFT_eSPI tft;


struct Vec2 {
  int x;
  int y;
};

extern const char* gameNames[];
extern const int GAME_COUNT;

class Game {
private:
  int index;
  String icon;

public:
  int highScores[5];
  const char* name;
  Game(){};
  Game(int i);
  std::function<void()> gameSetup;
  std::function<void()> gameLoop;

  void recordScore(int score);
};

extern Game currentGame;


void menuSetup();
void menuLoop();

void showSnakeMenu();
void initGame();
void generateFood();
void drawBlock(Vec2 pos, uint16_t color);
void drawBlock();
void handleInput();
void moveSnake();
bool checkSelfCollision(Vec2 head);
void drawScore();
void showSnakeGameOver();
void setupBuzzer();
void playFoodEatenSound();
void playNokiaSweepSound();
void initStars();
void updateStars();
void createFoodParticles(int x, int y, uint16_t color);
void updateParticles();
void drawSnakeHead();
void drawFood();
void restoreAfterPauseTetris();
void updatePauseMenu(int x);
void drawPauseMenu();

void drawBat();
void init_interface();
void audioReleaseForMusic();
void audioReclaimFromMusic();
void runMusicPlayer();
void breakoutSetup();
void breakoutLoop();
void runSetup();
void runLoop();
void galagaSetup();
void galagaLoop();
void puzzleSetup();
void puzzleLoop();
void ticTacToeSetup();
void ticTacToeLoop();
void pongSetup();
void pongLoop();
void minesweeperSetup();
void minesweeperLoop();
void lifeSetup();
void lifeLoop();
void mandelbrotSetup();
void mandelbrotLoop();
void lifeInputBegin();
void lifeInputEnd();
int lifeInputPoll(bool repeat = true);
void lifeInputRequireRelease();
void tetrisSetup();
void tetrisLoop();
void setupSnake();
void loopSnake();
void drawTransparentSpriteFast(int x, int y, int w, int h, const uint16_t *img, uint16_t transparentColor = 0xF81F);
void switchGame(int gameIndex);
void showGameHighScores();
void showResumeCountdown(void (*redrawScene)());
bool restoringCountdownDigit();
void resetAllGameScores();
int findCommand(bool repeat = true);
int getDirectHorizontalInput();
bool isDownHeld();

void playBootAnimation();
void playNavClick();
void playSelectTone();
void playTetrisDropSound();
void applyThemePreset(uint8_t index, bool persist);
void loadThemePreference();
const char* themeName(uint8_t index);
void setSoundEnabled(bool enabled, bool persist);
void setBuzzerTone(unsigned int frequency);

#endif
