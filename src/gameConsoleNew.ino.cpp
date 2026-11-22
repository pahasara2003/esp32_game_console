# 1 "/tmp/tmps6hn34zi"
#include <Arduino.h>
# 1 "/home/pahasara/gameConsole/src/gameConsoleNew.ino"
#include "game.h"
#include "buttons.h"
#include "birthday.h"
#include <Preferences.h>

Game currentGame;
void setup();
void loop();
static uint16_t mixBreakoutColor(uint16_t a, uint16_t b, int amount);
static uint16_t fadeBreakout(uint16_t color, int amount);
static void initBreakoutBackdropGrid();
static void soundPaddleHit();
static void soundWallHit();
static void soundBrickCrack();
static void soundLostBall();
static void soundPowerup();
static void dirtyBreakout(int y, int h);
static void spawnRing(int cx, int cy, uint16_t color);
static void spawnPopup(int cx, int cy, int pts, uint16_t color);
static void spawnParticles(int cx, int cy, uint16_t color);
static void dirtyBreakoutObjects();
static void updateBreakoutEffects();
template<class Surface>
static void drawTinyDigit(Surface &canvas, int x, int y, int d, uint16_t col);
template<class Surface>
static void composeBreakout(Surface &canvas, int top, int bottom);
static void drawBreakoutHeader();
static void updateScoreAndLivesHUD();
static void drawBreakoutBottomBar();
static void spawnPowerup(int x, int y);
static void updatePowerup();
static void resetBallOnPaddle();
static void initBreakoutLevel(int lvl);
static void checkBrickCollisions();
static void updateBallAndPhysics();
static void handleBreakoutInput();
static void showBreakoutStartScreen();
static void drawBreakoutPauseOption(int item);
static void drawBreakoutPauseMenu();
static void handleBreakoutPause();
static void drawBreakoutGameOver();
static void handleBreakoutGameOver();
static void drawBreakoutLevelComplete();
static void handleBreakoutLevelComplete();
void breakoutSetup();
void breakoutLoop();
static void initGalagaBackdropGrid();
static uint16_t galagaBackdropColor(int x, int y);
static void restoreGalagaBackdropRect(int x, int y, int w, int h);
static void drawGalagaBackdrop();
static void updateGalagaSound();
static void startGalagaSound(int effect);
static void soundPlayerLaser();
static void soundAlienHit();
static void soundPlayerExplode();
static void initStarfield();
static void updateStarfield();
static void drawGalagaHeader();
static void updateGalagaScoreHUD();
static void drawGalagaBottomBar();
static void drawShip(int px, int py);
static void eraseShip(int px, int py);
static bool prepareAlienSprite(int type);
static void drawAlienSprite(int x, int y, int type);
static void eraseAlienSprite(int x, int y);
static void moveAlienSprite(int oldX, int oldY, int x, int y, int type);
static void drawGalagaLaser(int index);
static void drawAlienBullet(int index);
static void eraseGalagaPops();
static void drawGalagaPops();
static void firePlayerLaser();
static void alienShoot(int ax, int ay);
static void updateBullets();
static void initGalagaWave(int wave);
static void updateAliens();
static void handleGalagaInput();
static void drawGalagaPauseOption(int item);
static void drawGalagaPauseMenu();
static void redrawGalagaPlayfield();
static void handleGalagaPause();
static void showGalagaStartScreen();
static void drawGalagaGameOver();
static void handleGalagaGameOver();
static void drawGalagaWaveClearOption(int item);
static void drawGalagaWaveClear();
static void handleGalagaWaveClear();
void galagaSetup();
void galagaLoop();
const char* themeName(uint8_t index);
void applyThemePreset(uint8_t index, bool persist);
void loadThemePreference();
void setDisplayBrightness(uint8_t brightness, bool persist);
void setBatterySaver(bool enabled);
bool restoringCountdownDigit();
void showGameHighScores();
void resetAllGameScores();
void switchGame(int gameIndex);
void drawTransparentSpriteFast(int x, int y, int w, int h, const uint16_t *img, uint16_t transparentColor);
static void i2sToneTask(void *);
static void initI2SAudio();
void audioReleaseForMusic();
void audioReclaimFromMusic();
void setBuzzerTone(unsigned int frequency);
void setSoundEnabled(bool enabled, bool persist);
void playToneDirect(unsigned int freq, unsigned long durationMs);
void playNavClick();
void playSelectTone();
static void stopNokiaMusic();
void playNokiaSweepSound();
static void serviceNokiaMusic();
void playTetrisDropSound();
void drawBootHand(int tip, bool fromLeft, uint16_t handColor);
void drawBootSpark(int x, int y, uint16_t color);
uint16_t bootRgb565(uint8_t r, uint8_t g, uint8_t b);
void fillBootGradient(int x, int y, int w, int h,
                      uint8_t r1, uint8_t g1, uint8_t b1,
                      uint8_t r2, uint8_t g2, uint8_t b2);
static uint16_t bootLerp(uint16_t a, uint16_t b, float t);
static float bootEase(int frame, int from, int len);
static void bootDrawIcon(TFT_eSprite *spr, int cx, int cy, int size, float alpha);
void playBootAnimation();
void init_interface();
bool isDownHeld();
int findCommand(bool repeat);
void PauseMenuControls();
void drawPauseMenu(void);
void showGameOver();
void drawDefaultBg();
void drawDefaultBg2();
int getDirectHorizontalInput();
static uint16_t homeBlend(uint16_t a, uint16_t b, int amount);
static uint16_t homeSwap(uint16_t color);
static int roundRectOutside4(int x, int y, int left, int top, int w, int h, int r);
static bool prepareTileEffects();
void getTileCoordinates(int index, int &x, int &y);
static bool prepareLauncherTile();
static void drawLauncherLabel(const char *label);
static void buildLabelMask(int index, uint8_t *mask);
void drawAppTile(int index, bool selected);
void drawPageDots();
void drawLauncherTitle();
void redrawVisibleTiles();
static void composeSlidingPage(uint16_t *strip, int stripY, int lines, int page,
                               int offset, int selected, const uint8_t *labels);
void slideLauncherPage(int oldPage, int newPage, int oldSelected);
static void drawAboutInformation();
void showAboutScreen();
void playSoundTestScreen();
static int readSettingsCommand();
static String titleCase(const char *name);
void drawSettingsRow(int row, bool selected);
static void drawSettingsHeader();
static void drawSettingsPalette();
static void drawSettingsFooter();
static void refreshSettingsTheme(int selectedRow);
void drawSettingsScreen(int selectedRow);
static void drawResetScoresConfirmation(int choice);
static bool confirmResetScores();
void showSettingsScreen();
void showNotice(const char* name);
void menuSetup();
void menuLoop();
static void resetInput();
static int input();
static void header(const char *name, const char *detail);
static void footer(const char *top, const char *bottom);
static void intro(const char *name, const char *line1, const char *line2,
                  const char *line3);
static void pause(const char *);
static void result(const char *outcome, const char *detail);
static void enterPause();
static int winner();
static int solve(int turn, int depth);
template<class Surface>
static void paintTile(Surface &canvas, int i, int x, int y);
static void drawTile(int i);
static void draw();
static void finish();
static int computerTurn();
void ticTacToeSetup();
void ticTacToeLoop();
static void dirty(int y, int h);
static void dirtyObjects();
static void impact(int x, int y);
template<class Surface>
static void compose(Surface &canvas, int top, int bottom);
static void drawHeader();
static void drawFooter();
static void serve();
static void draw();
static void drawMotion();
static void movePaddle(int direction);
static void showIntro();
static void drawPauseOption(int item);
static void drawPongPauseMenu();
static void enterPause();
static bool handlePause(int cmd);
static void drawGameOver();
static void end();
void pongSetup();
void pongLoop();
static void plant(int safe);
static void uncover(int index);
static void drawTile(int i);
static void drawDirty();
static void draw();
static void finish(bool won);
void minesweeperSetup();
void minesweeperLoop();
static bool useNextRunnerTimePixel(int x, int y);
static uint16_t blendRun565(uint16_t first, uint16_t second, uint8_t amount);
static uint16_t runnerTimeColor(const uint16_t *palette);
static void syncRunnerDayCycle();
static uint16_t runnerBackdropAt(int x, int y);
static void restoreRunnerBackdropRect(int x, int y, int w, int h);
static void drawRunnerBackdrop();
static void drawRunCloudClipped(uint8_t cloudIndex);
static void updateRunnerDayCycleBackdrop();
static void initRunClouds();
static void drawRunClouds();
static void updateRunClouds();
static void soundJump();
static void soundCrash();
static void soundMilestone();
static void drawRunnerHeader();
static void updateRunnerScoreHUD();
static void drawRunner(int px, int py, int prevPy, int anim, bool inAir);
static void eraseRunner(int px, int py);
static void drawRunnerDeathFrame(int py, int frameIndex);
static void playRunnerDeathAnimation(int py);
static void addRunPickup(float x, int y, bool shield);
static void spawnRunObstacle();
static void collectRunCoin(float x, int y);
static void updateRunCollectedCoins(uint32_t elapsed);
static void burstRunObstacle(int index);
static void updateRunDebris(uint32_t elapsed);
static void cacheRunnerObstacleColors();
static uint16_t runnerObstaclePixel(int index, int xx, int yy);
static void drawRunnerObstacle(int index, int x, int y);
static void updateAndDrawObstacles();
static void drawRunPickups();
static void updateRunPickups();
static void initRunnerGame();
static void drawRunnerChoice();
static void showRunnerStartScreen();
static void drawRunnerPauseOption(int item);
static void drawRunnerPauseMenu();
static void handleRunnerPause();
static void drawRunnerGameOver();
static void handleRunnerGameOver();
void runSetup();
void runLoop();
static int puzzleRow(int index);
static int puzzleCol(int index);
static int puzzleColorBand(int value, int size);
static uint16_t puzzleBandColor(int band);
static void setPuzzleGeometry();
static void drawPuzzleGoalPreview();
static void puzzleTilePosition(int index, int &x, int &y);
static void restorePuzzleBackdrop(int x, int y, int w, int h);
template<class Surface>
static void drawPuzzleTileAt(Surface &canvas, int x, int y, uint8_t value, bool highlighted);
static void drawPuzzleTile(int index);
static void animatePuzzleSlide(int fromIndex, int toIndex, uint8_t value);
static void drawPuzzleMoves();
static void drawPuzzleHeader();
static void drawPuzzleBottomBar();
static void drawPuzzleBoard();
static bool slidePuzzleGap(int command, bool countMove);
static bool puzzleIsSolved();
static void shufflePuzzle();
static int puzzleSerialCommand();
static void drawPuzzleStartChoices();
static void drawPuzzleStartScreen();
static void drawPuzzlePause();
static void drawPuzzleComplete();
static void startPuzzle();
void puzzleSetup();
void puzzleLoop();
static bool snakeWallAt(int x, int y);
static void drawSnakeBrick(int x, int y, int size);
static uint16_t snakeMix(uint16_t a, uint16_t b, int amount);
static void playBiteSound();
static void playDeathSound();
static void drawGrassyCell(int gx, int gy);
static void renderFullGrassBoard();
static void drawModernHeader();
static void updateScoreHUD();
static void drawBottomControlBar();
static int snakeNeighborDirection(Vec2 from, Vec2 to);
void drawSnakeHead();
static void drawSnakeSegment(int index);
static void drawApple();
static void restorePlayfieldRect(int rx, int ry, int rw, int rh);
static void drawTinyDigit(TFT_eSprite &sp, int x, int y, int d, uint16_t col);
static void eraseScorePopup();
static void spawnScorePopup(int gx, int gy, int val);
static void renderScorePopupStep();
static bool cellHasSnakeOrApple(int gx, int gy);
static void createAppleParticles(int gx, int gy);
void updateParticles();
void generateFood();
bool checkSelfCollision(Vec2 head);
void handleInput();
void moveSnake();
static void renderIntroGrassPreview();
static void renderIntroSelectorRow();
void showSnakeMenu();
void initGame();
static void redrawFullSnakeScene();
static void drawSnakePauseOption(int item);
static void drawSnakePauseMenu();
static void handleSnakePause();
static void drawSnakeGameOverScreen();
static void handleSnakeGameOver();
void setupSnake();
void loopSnake();
static uint16_t tetrisMix(uint16_t a, uint16_t b, int amount);
static uint16_t brickTone(int row, int k);
static uint16_t brickColorAt(int x, int y);
static void drawTetrisBricks(int cx, int cy, int cw, int ch);
void drawTetrisHeader();
void drawTetrisPlayfieldFrame();
int bodySize();
int getEncode(int x, int y);
void drawRect(int x, int y, uint32_t color);
void drawRect(int x, int y);
void redrawChangedCells();
void drawBody();
void drawNext();
void redrawRowsAbove(int cleared_row);
void drawMenu(void);
void drawTetrisScore();
void updateScore();
void updateTime();
void addToDead();
bool checkCollision(int dx, int dy);
bool newTetro();
void tryRotate();
void getCommand();
void restoreAfterPauseTetris();
void drawTetrisPauseOption(int item);
void drawTetrisPauseMenu();
void handleTetrisPause();
void drawTetrisGameOver();
void handleTetrisGameOver();
void tetrisLoop();
#line 8 "/home/pahasara/gameConsole/src/gameConsoleNew.ino"
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
  switchGame(0);
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
# 1 "/home/pahasara/gameConsole/src/breakout.ino"
extern TFT_eSPI tft;
#include "game.h"
#include "ui_kit.h"
#include <Arduino.h>
# 17 "/home/pahasara/gameConsole/src/breakout.ino"
constexpr int BO_WIDTH = 240;
constexpr int BO_HEIGHT = 320;
constexpr int BO_PLAYFIELD_TOP = 48;
constexpr int BO_PLAYFIELD_BOTTOM = 290;





constexpr int BRICK_ROWS = 6;
constexpr int BRICK_COLS = 6;
constexpr int BRICK_WIDTH = 36;
constexpr int BRICK_HEIGHT = 11;
constexpr int BRICK_GAP_X = 2;
constexpr int BRICK_GAP_Y = 3;
constexpr int BRICK_START_X = 7;
constexpr int BRICK_START_Y = 56;


constexpr int BASE_PADDLE_WIDTH = 46;
constexpr int PADDLE_HEIGHT = 8;
constexpr int PADDLE_Y = 276;
constexpr int PADDLE_SPEED = 4;
constexpr int BALL_SIZE = 6;
constexpr float BALL_SPEED_INIT = 2.25f;


#define BO_BG UI_BG
#define BO_CARD UI_PANEL
#define BO_SURFACE UI_SURFACE
#define BO_WHITE UI_TEXT
#define BO_MUTED UI_MUTED
constexpr uint16_t BO_AMBER = 0xFE04;
constexpr uint16_t BO_CYAN_ACCENT = 0x36FF;
constexpr uint16_t BO_EMERALD = 0x26E9;
constexpr uint16_t BO_CORAL = 0xFA8B;
constexpr uint16_t BO_HEADER_START = 0xFD20;
constexpr uint16_t BO_HEADER_END = 0xFFE0;
constexpr uint16_t BO_PADDLE = 0xFFFF;
constexpr uint16_t BO_BALL = 0xFFE8;

static uint16_t mixBreakoutColor(uint16_t a, uint16_t b, int amount) {
  int r = (((a >> 11) & 31) * (255 - amount) + ((b >> 11) & 31) * amount) / 255;
  int g = (((a >> 5) & 63) * (255 - amount) + ((b >> 5) & 63) * amount) / 255;
  int bl = ((a & 31) * (255 - amount) + (b & 31) * amount) / 255;
  return (uint16_t)((r << 11) | (g << 5) | bl);
}

static uint16_t fadeBreakout(uint16_t color, int amount) {
  return (((color >> 11) * amount / 255) << 11) |
         ((((color >> 5) & 63) * amount / 255) << 5) |
         ((color & 31) * amount / 255);
}



static const uint16_t BO_ROW_PALETTE[BRICK_ROWS] = {
  0xF98B,
  0xFD24,
  0xFE64,
  0x268C,
  0x263F,
  0xA33F
};

static uint16_t breakoutBrickColor(uint8_t row, uint8_t shade = 0) {
  uint16_t base = BO_ROW_PALETTE[row % BRICK_ROWS];
  if (shade == 0) return base;
  return mixBreakoutColor(base, 0xFFFF, min(90, int(shade) * 22));
}

static const int ROW_POINTS[BRICK_ROWS] = { 70, 60, 50, 40, 30, 20 };
static int breakoutLevel = 1;


constexpr int BO_GRID_SIZE = 12;
constexpr int BO_GRID_COLS = BO_WIDTH / BO_GRID_SIZE;
constexpr int BO_GRID_ROWS = (BO_PLAYFIELD_BOTTOM - BO_PLAYFIELD_TOP + 1 + BO_GRID_SIZE - 1) / BO_GRID_SIZE;
static uint8_t boBackdropGrid[BO_GRID_COLS][BO_GRID_ROWS];
static uint16_t boRowBg[BO_PLAYFIELD_BOTTOM - BO_PLAYFIELD_TOP + 1];
static uint16_t boRowGrid[BO_PLAYFIELD_BOTTOM - BO_PLAYFIELD_TOP + 1];

struct BreakoutStar {
  uint8_t x;
  uint16_t y;
  uint8_t bright;
};
constexpr int BO_STAR_COUNT = 22;
static BreakoutStar boStars[BO_STAR_COUNT];

static void initBreakoutBackdropGrid() {
  for (int x = 0; x < BO_GRID_COLS; x++)
    for (int y = 0; y < BO_GRID_ROWS; y++)
      boBackdropGrid[x][y] = (x * 7 + y * 13 + x * y) % 4;


  constexpr uint16_t TOP_COL = 0x1045;
  constexpr uint16_t MID_COL = 0x0887;
  constexpr uint16_t BOT_COL = 0x0928;
  const int totalH = BO_PLAYFIELD_BOTTOM - BO_PLAYFIELD_TOP + 1;
  for (int i = 0; i < totalH; ++i) {
    uint16_t bg = (i < totalH / 2)
        ? mixBreakoutColor(TOP_COL, MID_COL, i * 255 / (totalH / 2))
        : mixBreakoutColor(MID_COL, BOT_COL, (i - totalH / 2) * 255 / max(1, totalH - totalH / 2 - 1));
    boRowBg[i] = bg;
    boRowGrid[i] = mixBreakoutColor(bg, 0x3A52, 46);
  }

  for (int i = 0; i < BO_STAR_COUNT; ++i) {
    boStars[i].x = 8 + (i * 37 + (i & 3) * 19) % (BO_WIDTH - 16);
    boStars[i].y = BO_PLAYFIELD_TOP + 6 + (i * 53 + i * i * 7) % (totalH - 20);
    boStars[i].bright = i % 3;
  }
}




static uint8_t bricks[BRICK_ROWS][BRICK_COLS];
static uint8_t brickHits[BRICK_ROWS][BRICK_COLS];
static uint8_t boBrickShade[BRICK_ROWS][BRICK_COLS];

static int paddleX = (BO_WIDTH - BASE_PADDLE_WIDTH) / 2;
static int paddleWidth = BASE_PADDLE_WIDTH;
static int breakoutLastDirection = 0;

static float ballX = BO_WIDTH / 2.0f;
static float ballY = PADDLE_Y - BALL_SIZE - 2;
static float ballVelX = BALL_SPEED_INIT * 0.65f;
static float ballVelY = -BALL_SPEED_INIT;

static bool ballLaunched = false;
static int lives = 3;
static int pauseSelection = 0;
static const char *const BO_PAUSE_ITEMS[] = {"Resume", "Restart", "High scores", "Main menu"};
static bool pauseReady = false;


struct BreakoutPowerup {
  float x, y;
  int type;
  bool active;
};
static BreakoutPowerup powerup;


struct BreakoutParticle {
  float x, y;
  float vx, vy;
  uint16_t color;
  int life;
};
constexpr int BO_MAX_PARTICLES = 24;
static BreakoutParticle boParticles[BO_MAX_PARTICLES];
static int boParticleCount = 0;

struct BreakoutRing {
  int x, y;
  int radius;
  int life;
  uint16_t color;
};
constexpr int BO_MAX_RINGS = 3;
static BreakoutRing boRings[BO_MAX_RINGS];

struct BreakoutPopup {
  int x;
  float y;
  int value;
  uint16_t color;
  int life;
};
static BreakoutPopup boPopup = {0, 0, 0, 0, 0};

static int boCeilingFlash = 0;
static int boLeftFlash = 0;
static int boRightFlash = 0;
static int boSideFlashY = 0;
constexpr int BO_SIDE_FLASH_HALF = 20;




static uint8_t audioBrickCombo = 0;

static void soundPaddleHit() {
  audioBrickCombo = 0;
  audio::play(audio::Cue::Paddle);
}

static void soundWallHit() {
  audio::play(audio::Cue::Wall);
}

static void soundBrickHit(uint16_t freq = 1319) {
  audioBrickCombo = min(12, int(audioBrickCombo) + 1);
  audio::play(audio::Cue::Brick, freq + audioBrickCombo * 45);
  if (audioBrickCombo % 4 == 0) audio::play(audio::Cue::Sparkle);
}

static void soundBrickCrack() {
  audio::play(audio::Cue::Lock);
}

static void soundLostBall() {
  audioBrickCombo = 0;
  audio::play(audio::Cue::Hit);
}

static void soundPowerup() {
  audio::play(audio::Cue::Powerup);
}




constexpr int BO_BAND_HEIGHT = 16;
constexpr int BO_BANDS = (BO_PLAYFIELD_BOTTOM - BO_PLAYFIELD_TOP + BO_BAND_HEIGHT) / BO_BAND_HEIGHT;
static uint16_t boDirtyBands = 0;
static_assert(BO_BANDS <= 16, "Dirty bands must fit the mask");

struct BreakoutTrail { float x, y; };
static BreakoutTrail boTrail[8];
static int boTrailCount = 0;
static int boPaddlePulse = 0;

static void dirtyBreakout(int y, int h) {
  int first = max(y, BO_PLAYFIELD_TOP);
  int last = min(y + h - 1, BO_PLAYFIELD_BOTTOM);
  if (first > last) return;
  for (int i = (first - BO_PLAYFIELD_TOP) / BO_BAND_HEIGHT;
       i <= (last - BO_PLAYFIELD_TOP) / BO_BAND_HEIGHT; ++i)
    boDirtyBands |= uint16_t(1) << i;
}

static void spawnRing(int cx, int cy, uint16_t color) {
  for (int i = 0; i < BO_MAX_RINGS; ++i) {
    if (boRings[i].life <= 0) {
      boRings[i] = {cx, cy, 3, 7, color};
      return;
    }
  }
  boRings[0] = {cx, cy, 3, 7, color};
}

static void spawnPopup(int cx, int cy, int pts, uint16_t color) {
  boPopup = {constrain(cx, 18, BO_WIDTH - 18), (float)cy, pts, color, 14};
}

static void spawnParticles(int cx, int cy, uint16_t color) {
  int spawned = 0;
  for (int i = 0; i < BO_MAX_PARTICLES && spawned < 12; ++i) {
    if (boParticles[i].life > 0) continue;
    float angle = (spawned / 12.0f) * 6.283f + (random(0, 10) * 0.05f);
    float speed = random(11, 28) / 10.0f;
    boParticles[i].x = cx;
    boParticles[i].y = cy;
    boParticles[i].vx = cosf(angle) * speed;
    boParticles[i].vy = sinf(angle) * speed - 0.35f;
    boParticles[i].color = (spawned % 3 == 0) ? 0xFFFF : color;
    boParticles[i].life = random(7, 14);
    if (i >= boParticleCount) boParticleCount = i + 1;
    ++spawned;
  }
  spawnRing(cx, cy, color);
}

static void dirtyBreakoutObjects() {
  dirtyBreakout((int)ballY - 6, 13);
  dirtyBreakout(PADDLE_Y - 4, PADDLE_HEIGHT + 8);
  if (powerup.active) dirtyBreakout((int)powerup.y - 8, 17);
  for (int i = 0; i < boTrailCount; ++i) dirtyBreakout((int)boTrail[i].y - 3, 7);
  for (int i = 0; i < boParticleCount; ++i)
    if (boParticles[i].life > 0) dirtyBreakout((int)boParticles[i].y - 2, 5);
  for (int i = 0; i < BO_MAX_RINGS; ++i)
    if (boRings[i].life > 0)
      dirtyBreakout(boRings[i].y - boRings[i].radius - 2, (boRings[i].radius + 2) * 2 + 1);
  if (boPopup.life > 0) dirtyBreakout((int)boPopup.y - 8, 20);
  if (boCeilingFlash > 0) dirtyBreakout(BO_PLAYFIELD_TOP, 4);
  if (boLeftFlash > 0 || boRightFlash > 0)
    dirtyBreakout(boSideFlashY - BO_SIDE_FLASH_HALF, BO_SIDE_FLASH_HALF * 2 + 1);
}

static void updateBreakoutEffects() {
  if (boPaddlePulse > 0) --boPaddlePulse;
  if (boCeilingFlash > 0) --boCeilingFlash;
  if (boLeftFlash > 0) --boLeftFlash;
  if (boRightFlash > 0) --boRightFlash;

  if (ballLaunched) {
    for (int i = min(boTrailCount, 7); i > 0; --i) boTrail[i] = boTrail[i - 1];
    boTrail[0] = {ballX, ballY};
    boTrailCount = min(8, boTrailCount + 1);
  } else {
    boTrailCount = 0;
  }

  int activeMax = 0;
  for (int i = 0; i < boParticleCount; ++i) {
    auto &p = boParticles[i];
    if (p.life <= 0) continue;
    p.x += p.vx;
    p.y += p.vy;
    p.vy += 0.12f;
    --p.life;
    if (p.life > 0) activeMax = i + 1;
  }
  boParticleCount = activeMax;

  for (int i = 0; i < BO_MAX_RINGS; ++i) {
    if (boRings[i].life > 0) {
      boRings[i].radius += 2;
      --boRings[i].life;
    }
  }

  if (boPopup.life > 0) {
    boPopup.y -= 0.7f;
    --boPopup.life;
  }
}


template<class Surface>
static void drawTinyDigit(Surface &canvas, int x, int y, int d, uint16_t col) {
  static const uint8_t glyphs[10][5] = {
    {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1},
    {7,4,7,1,7}, {7,4,7,5,7}, {7,1,2,2,2}, {7,5,7,5,7}, {7,5,7,1,7}
  };
  if (d < 0 || d > 9) return;
  for (int r = 0; r < 5; ++r)
    for (int c = 0; c < 3; ++c)
      if (glyphs[d][r] & (4 >> c)) canvas.fillRect(x + c * 2, y + r * 2, 2, 2, col);
}


template<class Surface>
static void composeBreakout(Surface &canvas, int top, int bottom) {

  for (int y = top; y <= bottom; ++y) {
    int ry = y - BO_PLAYFIELD_TOP;
    bool isHGrid = (ry % BO_GRID_SIZE) == 0;
    canvas.drawFastHLine(0, y, BO_WIDTH, isHGrid ? boRowGrid[ry] : boRowBg[ry]);
    if (!isHGrid) {
      uint16_t gc = boRowGrid[ry];
      for (int x = BO_GRID_SIZE; x < BO_WIDTH; x += BO_GRID_SIZE)
        canvas.drawPixel(x, y, gc);
    }
  }


  for (int i = 0; i < BO_STAR_COUNT; ++i) {
    int sy = boStars[i].y;
    if (sy < top - 1 || sy > bottom + 1) continue;
    int sx = boStars[i].x;
    uint16_t sc = boStars[i].bright == 2 ? 0x9E7F : (boStars[i].bright == 1 ? 0x5B94 : 0x39E9);
    canvas.drawPixel(sx, sy, sc);
    if (boStars[i].bright == 2) {
      canvas.drawPixel(sx - 1, sy, 0x39E9);
      canvas.drawPixel(sx + 1, sy, 0x39E9);
      canvas.drawPixel(sx, sy - 1, 0x39E9);
      canvas.drawPixel(sx, sy + 1, 0x39E9);
    }
  }


  if (boCeilingFlash > 0 && top <= BO_PLAYFIELD_TOP + 2) {
    uint16_t fc = fadeBreakout(BO_CYAN_ACCENT, boCeilingFlash * 50);
    canvas.drawFastHLine(0, BO_PLAYFIELD_TOP, BO_WIDTH, fc);
    canvas.drawFastHLine(0, BO_PLAYFIELD_TOP + 1, BO_WIDTH, fadeBreakout(fc, 130));
  }
  int sfTop = max(top, boSideFlashY - BO_SIDE_FLASH_HALF);
  int sfH = min(bottom, boSideFlashY + BO_SIDE_FLASH_HALF) - sfTop + 1;
  if (boLeftFlash > 0 && sfH > 0) {
    uint16_t fc = fadeBreakout(BO_CYAN_ACCENT, boLeftFlash * 50);
    canvas.drawFastVLine(0, sfTop, sfH, fc);
    canvas.drawFastVLine(1, sfTop, sfH, fadeBreakout(fc, 130));
  }
  if (boRightFlash > 0 && sfH > 0) {
    uint16_t fc = fadeBreakout(BO_CYAN_ACCENT, boRightFlash * 50);
    canvas.drawFastVLine(BO_WIDTH - 1, sfTop, sfH, fc);
    canvas.drawFastVLine(BO_WIDTH - 2, sfTop, sfH, fadeBreakout(fc, 130));
  }


  for (int r = 0; r < BRICK_ROWS; ++r) {
    int y = BRICK_START_Y + r * (BRICK_HEIGHT + BRICK_GAP_Y);
    if (y > bottom || y + BRICK_HEIGHT <= top) continue;
    for (int c = 0; c < BRICK_COLS; ++c) {
      uint8_t state = bricks[r][c];
      if (!state) continue;
      int x = BRICK_START_X + c * (BRICK_WIDTH + BRICK_GAP_X);
      uint16_t base, hi, lo;
      if (state == 2) {

        bool cracked = brickHits[r][c] > 0;
        base = cracked ? 0x7BEF : 0xCE79;
        hi = cracked ? 0xBDF7 : 0xFFFF;
        lo = cracked ? 0x4228 : 0x6B6D;
      } else {
        base = breakoutBrickColor(r, boBrickShade[r][c]);
        hi = mixBreakoutColor(base, 0xFFFF, 125);
        lo = fadeBreakout(base, 135);
      }
      canvas.fillRoundRect(x, y, BRICK_WIDTH, BRICK_HEIGHT, 3, base);
      canvas.drawFastHLine(x + 2, y + 1, BRICK_WIDTH - 4, hi);
      canvas.drawFastHLine(x + 2, y + BRICK_HEIGHT - 1, BRICK_WIDTH - 4, lo);

      canvas.drawFastHLine(x + 4, y + 2, 6, 0xFFFF);
      if (state == 2 && brickHits[r][c] > 0) {

        int mx = x + BRICK_WIDTH / 2;
        canvas.drawLine(mx - 4, y + 2, mx, y + 6, 0x2124);
        canvas.drawLine(mx, y + 6, mx + 4, y + BRICK_HEIGHT - 2, 0x2124);
      }
    }
  }


  for (int i = 0; i < BO_MAX_RINGS; ++i) {
    if (boRings[i].life > 0) {
      uint16_t rc = fadeBreakout(boRings[i].color, min(255, boRings[i].life * 38));
      canvas.drawCircle(boRings[i].x, boRings[i].y, boRings[i].radius, rc);
    }
  }


  for (int i = 0; i < boParticleCount; ++i) {
    auto &p = boParticles[i];
    if (p.life > 0) {
      uint16_t pc = fadeBreakout(p.color, min(255, p.life * 26));
      canvas.fillCircle((int)p.x, (int)p.y, p.life > 7 ? 1 : 0, pc);
    }
  }


  if (boPopup.life > 0) {
    int py = (int)boPopup.y;
    if (py + 10 >= top && py - 2 <= bottom) {
      uint16_t pc = fadeBreakout(boPopup.color, min(255, boPopup.life * 28));
      int digits = boPopup.value >= 100 ? 3 : 2;
      int px = boPopup.x - (digits * 8 + 6) / 2;

      canvas.fillRect(px, py + 4, 6, 2, pc);
      canvas.fillRect(px + 2, py + 2, 2, 6, pc);
      int v = boPopup.value;
      int x = px + 8;
      if (v >= 100) {
        drawTinyDigit(canvas, x, py, (v / 100) % 10, pc);
        x += 8;
      }
      drawTinyDigit(canvas, x, py, (v / 10) % 10, pc);
      drawTinyDigit(canvas, x + 8, py, v % 10, pc);
    }
  }


  for (int i = boTrailCount - 1; i >= 0; --i) {
    uint16_t tc = mixBreakoutColor(BO_HEADER_START, BO_CYAN_ACCENT, i * 255 / 7);
    canvas.fillCircle((int)boTrail[i].x, (int)boTrail[i].y, i < 3 ? 2 : 1,
                      fadeBreakout(tc, 165 - i * 18));
  }


  if (powerup.active) {
    int x = (int)powerup.x, y = (int)powerup.y;
    uint16_t color = powerup.type == 1 ? BO_CYAN_ACCENT
                   : (powerup.type == 2 ? BO_AMBER : BO_CORAL);
    canvas.drawCircle(x, y, 7, fadeBreakout(color, 110));
    canvas.fillCircle(x, y, 5, color);
    canvas.drawPixel(x - 2, y - 2, 0xFFFF);
    if (powerup.type == 1) {
      canvas.drawFastHLine(x - 3, y, 7, ui::INK_DARK);
      canvas.drawPixel(x - 2, y - 1, ui::INK_DARK);
      canvas.drawPixel(x + 2, y - 1, ui::INK_DARK);
    } else if (powerup.type == 2) {
      canvas.drawFastHLine(x - 2, y, 5, ui::INK_DARK);
    } else {
      canvas.drawFastHLine(x - 2, y, 5, ui::INK_DARK);
      canvas.drawFastVLine(x, y - 2, 5, ui::INK_DARK);
    }
  }


  if (boPaddlePulse) {
    canvas.drawRoundRect(paddleX - 2, PADDLE_Y - 2,
                         paddleWidth + 4, PADDLE_HEIGHT + 4, 5,
                         fadeBreakout(BO_CYAN_ACCENT, boPaddlePulse * 25));
  }
  canvas.fillRoundRect(paddleX, PADDLE_Y, paddleWidth, PADDLE_HEIGHT, 4, 0xDEFB);
  canvas.fillRoundRect(paddleX + 1, PADDLE_Y + 1, paddleWidth - 2, PADDLE_HEIGHT - 2, 3, BO_PADDLE);

  uint16_t capCol = paddleWidth > BASE_PADDLE_WIDTH ? BO_CYAN_ACCENT : BO_HEADER_START;
  canvas.fillRoundRect(paddleX + 1, PADDLE_Y + 1, 6, PADDLE_HEIGHT - 2, 2, capCol);
  canvas.fillRoundRect(paddleX + paddleWidth - 7, PADDLE_Y + 1, 6, PADDLE_HEIGHT - 2, 2, capCol);
  canvas.drawFastHLine(paddleX + 9, PADDLE_Y + PADDLE_HEIGHT - 2, paddleWidth - 18, 0x9CF3);


  canvas.fillCircle((int)ballX, (int)ballY, BALL_SIZE / 2 + 2, fadeBreakout(BO_HEADER_START, 75));
  canvas.fillCircle((int)ballX, (int)ballY, BALL_SIZE / 2, BO_BALL);
  canvas.drawPixel((int)ballX - 1, (int)ballY - 1, 0xFFFF);
}

static void renderBreakoutFrame(bool full = false) {
  if (full) boDirtyBands = (uint16_t(1) << BO_BANDS) - 1;
  if (!boDirtyBands) return;
  TFT_eSprite band(&tft);
  band.setColorDepth(16);
  if (!band.createSprite(BO_WIDTH, BO_BAND_HEIGHT)) {
    int x = tft.getViewportX(), y = tft.getViewportY();
    int w = tft.getViewportWidth(), h = tft.getViewportHeight();
    int top = max(y, BO_PLAYFIELD_TOP);
    int bottom = min(y + h - 1, BO_PLAYFIELD_BOTTOM);
    tft.setViewport(x, top, w, bottom - top + 1, false);
    composeBreakout(tft, BO_PLAYFIELD_TOP, BO_PLAYFIELD_BOTTOM);
    tft.setViewport(x, y, w, h, false);
    boDirtyBands = 0;
    return;
  }
  for (int i = 0; i < BO_BANDS; ++i) {
    if (!(boDirtyBands & (uint16_t(1) << i))) continue;
    int y = BO_PLAYFIELD_TOP + i * BO_BAND_HEIGHT;
    int h = min(BO_BAND_HEIGHT, BO_PLAYFIELD_BOTTOM - y + 1);
    band.setViewport(0, 0, BO_WIDTH, h);
    band.setOrigin(0, -y);
    composeBreakout(band, y, y + h - 1);
    band.pushSprite(0, y, 0, 0, BO_WIDTH, h);
  }
  band.deleteSprite();
  boDirtyBands = 0;
}




static void drawBreakoutHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(224, 42)) return;
  header.fillSprite(BO_BG);
  header.fillRoundRect(0, 0, 224, 42, 12, ui::CARD);

  header.setTextDatum(TL_DATUM);
  header.setTextColor(ui::LABEL, ui::CARD);
  header.drawString("Breakout", 12, 5, 2);

  if (audioBrickCombo >= 2) {
    String comboTxt = "x" + String(audioBrickCombo);
    int cw = header.textWidth(comboTxt, 2) + 12;
    header.fillRoundRect(12, 22, cw, 16, 8, BO_CYAN_ACCENT);
    header.setTextDatum(MC_DATUM);
    header.setTextColor(ui::INK_DARK, BO_CYAN_ACCENT);
    header.drawString(comboTxt, 12 + cw / 2, 30, 2);
  } else {
    header.setTextDatum(TL_DATUM);
    header.setTextColor(ui::SECONDARY, ui::CARD);
    header.drawString("Level " + String(breakoutLevel), 12, 23, 2);
  }

  header.setTextDatum(TR_DATUM);
  header.setTextColor(BO_HEADER_START, ui::CARD);
  header.drawString("Score", 156, 2, 2);
  header.setTextColor(ui::SECONDARY, ui::CARD);
  header.drawString("Lives", 212, 2, 2);

  header.setTextColor(ui::LABEL, ui::CARD);
  String points(score);
  int scoreFont = header.textWidth(points, 4) <= 76 ? 4 : 2;
  header.drawString(points, 156, scoreFont == 4 ? 16 : 23, scoreFont);

  for (int i = 0; i < 3; i++) {
    int x = 171 + i * 14;
    uint16_t heartColor = i < lives ? BO_CORAL : ui::SEPARATOR;
    header.fillCircle(x + 3, 26, 3, heartColor);
    header.fillCircle(x + 9, 26, 3, heartColor);
    header.fillTriangle(x, 26, x + 12, 26, x + 6, 35, heartColor);
  }
  header.pushSprite(8, 3);
  header.deleteSprite();
}

static void updateScoreAndLivesHUD() {
  drawBreakoutHeader();
}

static void drawBreakoutBottomBar() {
  tft.fillRect(0, 294, 240, 26, BO_BG);
  ui::hint(306, ballLaunched ? "Shoot to pause" : "Up to launch", BO_BG);
}




static void spawnPowerup(int x, int y) {
  if (powerup.active) return;
  powerup.x = x + BRICK_WIDTH / 2.0f;
  powerup.y = y;
  powerup.type = random(1, 4);
  powerup.active = true;
}

static void updatePowerup() {
  if (!powerup.active) return;

  powerup.y += 1.15f;


  if (powerup.y >= PADDLE_Y - 4 && powerup.y <= PADDLE_Y + PADDLE_HEIGHT &&
      powerup.x >= paddleX - 4 && powerup.x <= paddleX + paddleWidth + 4) {
    int px = (int)powerup.x;
    int ptype = powerup.type;
    powerup.active = false;
    soundPowerup();
    if (ptype == 1) {
      paddleWidth = 64;
      paddleX = constrain(paddleX, 4, BO_WIDTH - paddleWidth - 4);
      boPaddlePulse = 10;
      score += 50;
      spawnParticles(px, PADDLE_Y, BO_CYAN_ACCENT);
      spawnPopup(px, PADDLE_Y - 8, 50, BO_CYAN_ACCENT);
    } else if (ptype == 2) {
      ballVelX *= 0.78f;
      ballVelY *= 0.78f;
      if (abs(ballVelY) < 1.65f) ballVelY = (ballVelY < 0 ? -1.65f : 1.65f);
      score += 40;
      spawnParticles(px, PADDLE_Y, BO_AMBER);
      spawnPopup(px, PADDLE_Y - 8, 40, BO_AMBER);
    } else if (ptype == 3) {
      if (lives < 3) {
        lives++;
        audio::play(audio::Cue::Life);
      }
      score += 100;
      spawnParticles(px, PADDLE_Y, BO_CORAL);
      spawnPopup(px, PADDLE_Y - 8, 100, BO_CORAL);
    }
    updateScoreAndLivesHUD();
    return;
  }


  if (powerup.y > BO_PLAYFIELD_BOTTOM) {
    powerup.active = false;
    return;
  }
}

static void resetBallOnPaddle() {
  ballX = paddleX + paddleWidth / 2.0f;
  ballY = PADDLE_Y - BALL_SIZE / 2.0f - 2;
  float dirX = (random(2) == 0 ? 1.0f : -1.0f);
  ballVelX = BALL_SPEED_INIT * 0.65f * dirX;
  ballVelY = -BALL_SPEED_INIT;
  ballLaunched = false;
  drawBreakoutBottomBar();
}




static void initBreakoutLevel(int lvl) {
  if (lvl == 1) audio::restartMusic();
  breakoutLevel = lvl;
  initBreakoutBackdropGrid();
  paddleWidth = BASE_PADDLE_WIDTH;
  paddleX = (BO_WIDTH - paddleWidth) / 2;
  breakoutLastDirection = 0;
  powerup.active = false;
  boParticleCount = boTrailCount = boPaddlePulse = 0;
  boCeilingFlash = boLeftFlash = boRightFlash = 0;
  boPopup.life = 0;
  for (int i = 0; i < BO_MAX_PARTICLES; ++i) boParticles[i].life = 0;
  for (int i = 0; i < BO_MAX_RINGS; ++i) boRings[i].life = 0;
  audioBrickCombo = 0;


  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      if (lvl == 1) {
        bricks[r][c] = 1;
      } else if (lvl == 2) {
        bricks[r][c] = (r == 0) ? 2 : 1;
      } else {
        bricks[r][c] = ((r + c) % 3 == 0) ? 2 : 1;
      }
      brickHits[r][c] = 0;
      boBrickShade[r][c] = (c == 2 || c == 3) ? 2 : ((c == 1 || c == 4) ? 1 : 0);
    }
  }

  tft.fillScreen(BO_BG);


  tft.drawFastHLine(0, BO_PLAYFIELD_TOP - 1, BO_WIDTH, ui::SEPARATOR);
  tft.drawFastHLine(0, BO_PLAYFIELD_BOTTOM + 1, BO_WIDTH, ui::SEPARATOR);

  drawBreakoutHeader();
  drawBreakoutBottomBar();
  resetBallOnPaddle();
  renderBreakoutFrame(true);
}




static void checkBrickCollisions() {
  int remaining = 0;

  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      if (bricks[r][c] == 0) continue;
      remaining++;

      int bx = BRICK_START_X + c * (BRICK_WIDTH + BRICK_GAP_X);
      int by = BRICK_START_Y + r * (BRICK_HEIGHT + BRICK_GAP_Y);


      if (ballX + BALL_SIZE / 2.0f >= bx &&
          ballX - BALL_SIZE / 2.0f <= bx + BRICK_WIDTH &&
          ballY + BALL_SIZE / 2.0f >= by &&
          ballY - BALL_SIZE / 2.0f <= by + BRICK_HEIGHT) {

        float diffX = ballX - (bx + BRICK_WIDTH / 2.0f);
        float diffY = ballY - (by + BRICK_HEIGHT / 2.0f);

        if (abs(diffX) / BRICK_WIDTH > abs(diffY) / BRICK_HEIGHT) {
          ballVelX = (diffX > 0) ? abs(ballVelX) : -abs(ballVelX);
        } else {
          ballVelY = (diffY > 0) ? abs(ballVelY) : -abs(ballVelY);
        }

        int cx = bx + BRICK_WIDTH / 2;
        int cy = by + BRICK_HEIGHT / 2;


        if (bricks[r][c] == 2) {
          brickHits[r][c]++;
          if (brickHits[r][c] >= 2) {
            bricks[r][c] = 0;
            int bonus = 80 + min(6, int(audioBrickCombo)) * 10;
            score += bonus;
            dirtyBreakout(by, BRICK_HEIGHT);
            spawnParticles(cx, cy, 0xFFFF);
            spawnPopup(cx, cy, bonus, BO_HEADER_END);
            soundBrickHit(1568);
            if (random(10) > 6) spawnPowerup(bx, by);
            remaining--;
          } else {
            dirtyBreakout(by, BRICK_HEIGHT);
            spawnRing(cx, cy, 0xCE79);
            soundBrickCrack();
          }
        } else {
          bricks[r][c] = 0;
          int pts = ROW_POINTS[r] + (audioBrickCombo >= 2 ? min(5, int(audioBrickCombo) - 1) * 10 : 0);
          score += pts;
          uint16_t bCol = breakoutBrickColor(r, boBrickShade[r][c]);
          dirtyBreakout(by, BRICK_HEIGHT);
          spawnParticles(cx, cy, bCol);
          spawnPopup(cx, cy, pts, bCol);

          static const uint16_t rowPitch[BRICK_ROWS] = {1568, 1319, 1175, 1047, 880, 784};
          soundBrickHit(rowPitch[r]);
          if (random(10) > 7) spawnPowerup(bx, by);
          remaining--;
        }

        updateScoreAndLivesHUD();
        if (remaining == 0) {
          audio::play(audio::Cue::Win);
          gameState = 3;
          menu_drawn = false;
        }
        return;
      }
    }
  }

  if (remaining == 0) {
    audio::play(audio::Cue::Win);
    gameState = 3;
    menu_drawn = false;
  }
}

static void updateBallAndPhysics() {
  ballX += ballVelX;
  ballY += ballVelY;


  if (ballX - BALL_SIZE / 2.0f <= 1) {
    ballX = 1 + BALL_SIZE / 2.0f;
    ballVelX = abs(ballVelX);
    boLeftFlash = 5;
    boSideFlashY = (int)ballY;
    soundWallHit();
  } else if (ballX + BALL_SIZE / 2.0f >= BO_WIDTH - 2) {
    ballX = BO_WIDTH - 2 - BALL_SIZE / 2.0f;
    ballVelX = -abs(ballVelX);
    boRightFlash = 5;
    boSideFlashY = (int)ballY;
    soundWallHit();
  }


  if (ballY - BALL_SIZE / 2.0f <= BO_PLAYFIELD_TOP) {
    ballY = BO_PLAYFIELD_TOP + BALL_SIZE / 2.0f;
    ballVelY = abs(ballVelY);
    boCeilingFlash = 5;
    soundWallHit();
  }


  if (ballVelY > 0 &&
      ballY + BALL_SIZE / 2.0f >= PADDLE_Y &&
      ballY - BALL_SIZE / 2.0f <= PADDLE_Y + PADDLE_HEIGHT &&
      ballX + BALL_SIZE / 2.0f >= paddleX - 1 &&
      ballX - BALL_SIZE / 2.0f <= paddleX + paddleWidth + 1) {

    ballY = PADDLE_Y - BALL_SIZE / 2.0f - 1;
    ballVelY = -abs(ballVelY);


    float offset = (ballX - (paddleX + paddleWidth / 2.0f)) / (paddleWidth / 2.0f);
    ballVelX = offset * (BALL_SPEED_INIT * 1.3f);
    if (abs(ballVelX) < 0.6f) ballVelX = (ballVelX >= 0) ? 0.8f : -0.8f;

    boPaddlePulse = 10;
    spawnRing((int)ballX, PADDLE_Y, BO_CYAN_ACCENT);
    bool hadCombo = audioBrickCombo >= 2;
    soundPaddleHit();
    if (hadCombo) updateScoreAndLivesHUD();
  }


  if (ballY > BO_PLAYFIELD_BOTTOM) {
    soundLostBall();
    lives--;
    if (lives > 0) audio::play(audio::Cue::Lose);
    updateScoreAndLivesHUD();

    if (lives <= 0) {
      audio::play(audio::Cue::GameOver);
      gameState = 2;
      menu_drawn = false;
      return;
    } else {
      paddleWidth = BASE_PADDLE_WIDTH;
      boTrailCount = 0;
      resetBallOnPaddle();
    }
  }


  checkBrickCollisions();
}

static void handleBreakoutInput() {

  bool launchPressed = false;
  int pressedDirection = 0;
  bool shootPressed = false;
  int command;
  while ((command = buttonsNextPress()) >= 0) {
    if (command == 0) launchPressed = true;
    else if (command == 1) pressedDirection = 1;
    else if (command == 3) pressedDirection = -1;
    else if (command == 4) shootPressed = true;
  }

  if (shootPressed) {
    audio::pauseMusic(true); gameState = 1;
    menu_drawn = false;
    pauseSelection = 0;
    pauseReady = false;
    return;
  }


  uint8_t held = buttonsHeld();
  bool rightHeld = (held & 2) != 0;
  bool leftHeld = (held & 8) != 0;
  if (pressedDirection != 0) breakoutLastDirection = pressedDirection;
  int direction = pressedDirection;
  if (direction == 0) {
    if (rightHeld && leftHeld) direction = breakoutLastDirection;
    else if (rightHeld) direction = 1;
    else if (leftHeld) direction = -1;
  }
  paddleX = constrain(paddleX + direction * PADDLE_SPEED,
                      4, BO_WIDTH - paddleWidth - 4);

  if (!ballLaunched) {
    ballX = paddleX + paddleWidth / 2.0f;
    ballY = PADDLE_Y - BALL_SIZE / 2.0f - 2;
  }
  if (launchPressed && !ballLaunched) {
    ballLaunched = true;
    boPaddlePulse = 8;
    audio::play(audio::Cue::Jump);
    drawBreakoutBottomBar();
  }
}




static void showBreakoutStartScreen() {
  gameState = -1;
  tft.setRotation(2);
  tft.fillScreen(BO_BG);
  drawBreakoutHeader();
  static const ui::Control controls[] = {
    {"Left / Right", "Move"}, {"Up", "Launch"}, {"Shoot", "Pause"}};
  ui::startCard(52, "Breakout", "Clear every brick", controls, 3,
                currentGame.highScores[0], BO_HEADER_START, nullptr, "Press Shoot to start");
}

static void drawBreakoutPauseOption(int item) {
  ui::pauseItem(BO_PAUSE_ITEMS, 4, item, pauseSelection, BO_HEADER_START);
}

static void drawBreakoutPauseMenu() {
  buttonsRequireRelease();
  ui::pauseSheet("Breakout", BO_PAUSE_ITEMS, 4, pauseSelection, BO_HEADER_START);
}

static void handleBreakoutPause() {
  if (!pauseReady) {
    if (readShoot() == LOW) pauseReady = true;
    return;
  }

  int cmd = findCommand();
  if (cmd == 0 || cmd == 2) {
    int old = pauseSelection;
    pauseSelection = (pauseSelection + (cmd == 0 ? 3 : 1)) % 4;
    drawBreakoutPauseOption(old);
    drawBreakoutPauseOption(pauseSelection);
    playNavClick();
  } else if (cmd == 4) {
    playSelectTone();
    if (pauseSelection == 0) {
      showResumeCountdown([]() {
        if (!restoringCountdownDigit()) tft.fillScreen(BO_BG);
        tft.drawFastHLine(0, BO_PLAYFIELD_TOP - 1, BO_WIDTH, ui::SEPARATOR);
        tft.drawFastHLine(0, BO_PLAYFIELD_BOTTOM + 1, BO_WIDTH, ui::SEPARATOR);
        drawBreakoutHeader();
        drawBreakoutBottomBar();
        renderBreakoutFrame(true);
      });
      gameState = 0;
      menu_drawn = false;
    } else if (pauseSelection == 1) {
      score = 0;
      lives = 3;
      initBreakoutLevel(1);
      gameState = 0;
      menu_drawn = false;
    } else if (pauseSelection == 2) {
      showGameHighScores();
      tft.fillScreen(BO_BG);
      tft.drawFastHLine(0, BO_PLAYFIELD_TOP - 1, BO_WIDTH, ui::SEPARATOR);
      tft.drawFastHLine(0, BO_PLAYFIELD_BOTTOM + 1, BO_WIDTH, ui::SEPARATOR);
      drawBreakoutHeader();
      drawBreakoutBottomBar();
      renderBreakoutFrame(true);
      drawBreakoutPauseMenu();
      pauseReady = false;
    } else {
      switchGame(0);
    }
  }
}

static void drawBreakoutGameOver() {
  currentGame.recordScore(score);
  ui::gameOverSheet("Breakout", score, currentGame.highScores[0], BO_HEADER_START);
}

static void handleBreakoutGameOver() {
  int cmd = findCommand();
  if (cmd == 4) {
    playSelectTone();
    score = 0;
    lives = 3;
    initBreakoutLevel(1);
    gameState = 0;
    menu_drawn = false;
  } else if (cmd == 0) {
    playSelectTone();
    switchGame(0);
  }
}

static void drawBreakoutLevelComplete() {
  int bonus = lives * 200;
  ui::card(24, 66, 192, 186);
  ui::heading(40, 80, "Breakout", "Stage clear");
  ui::chip(196 - tft.textWidth("Level " + String(breakoutLevel), 2) - 16, 76,
           ("Level " + String(breakoutLevel)).c_str(), BO_CYAN_ACCENT);
  ui::stat(40, 132, "Score", String(score));
  ui::stat(128, 132, "Bonus", "+" + String(bonus), BO_HEADER_START);
  ui::gradientPill(40, 188, 160, 30, "Next stage", BO_HEADER_START, BO_HEADER_END);
  ui::hint(234, "Press Shoot to continue", ui::CARD);
}

static void handleBreakoutLevelComplete() {
  int cmd = findCommand();
  if (cmd == 4) {
    score += lives * 200;
    playSelectTone();
    initBreakoutLevel(breakoutLevel + 1);
    gameState = 0;
    menu_drawn = false;
  }
}




void breakoutSetup() {
  tft.setRotation(2);
  score = 0;
  lives = 3;
  showBreakoutStartScreen();
}

void breakoutLoop() {
  audio::gameState(gameState);
  if (gameState == 0) {
    uint32_t frameStart = millis();
    dirtyBreakoutObjects();
    handleBreakoutInput();
    if (gameState != 0) return;

    updateBreakoutEffects();
    if (ballLaunched) {
      updateBallAndPhysics();
    }
    updatePowerup();
    dirtyBreakoutObjects();
    renderBreakoutFrame();


    uint32_t spent = millis() - frameStart;
    if (spent < 16) delay(16 - spent);
    return;
  } else if (gameState == 1) {
    if (!menu_drawn) {
      drawBreakoutPauseMenu();
      menu_drawn = true;
    }
    handleBreakoutPause();
  } else if (gameState == 2) {
    if (!menu_drawn) {
      drawBreakoutGameOver();
      menu_drawn = true;
    }
    handleBreakoutGameOver();
  } else if (gameState == 3) {
    if (!menu_drawn) {
      drawBreakoutLevelComplete();
      menu_drawn = true;
    }
    handleBreakoutLevelComplete();
  } else if (gameState == -1) {
    int cmd = findCommand();
    if (cmd == 4) {
      playSelectTone();
      score = 0;
      lives = 3;
      initBreakoutLevel(1);
      gameState = 0;
    }
  }
}
# 1 "/home/pahasara/gameConsole/src/galaga.ino"
#include "game.h"
#include "ui_kit.h"
#include "arcade_style.h"
#include <Arduino.h>
# 16 "/home/pahasara/gameConsole/src/galaga.ino"
constexpr int GALA_W = 240;
constexpr int GALA_H = 320;
constexpr int GALA_PLAYFIELD_TOP = 48;
constexpr int GALA_PLAYFIELD_BOTTOM = 290;

constexpr int SHIP_W = 18;
constexpr int SHIP_H = 16;
constexpr int SHIP_Y = 268;
constexpr int SHIP_SPEED = 5;
constexpr int GALA_LASER_WIDTH = 4;
constexpr int GALA_LASER_HEIGHT = 12;
constexpr uint16_t GALA_LASER_COLOR = 0x47F5;

constexpr int MAX_PLAYER_BULLETS = 4;
constexpr int MAX_ALIEN_BULLETS = 6;
constexpr int ALIEN_ROWS = 3;
constexpr int ALIEN_COLS = 5;
constexpr int TOTAL_ALIENS = ALIEN_ROWS * ALIEN_COLS;


#define GALA_BG UI_BG
#define GALA_CARD UI_PANEL
#define GALA_SURFACE UI_SURFACE
#define GALA_WHITE UI_TEXT
#define GALA_MUTED UI_MUTED
#define GALA_CYAN UI_CYAN
constexpr uint16_t GALA_GREEN = 0x268C;
constexpr uint16_t GALA_AMBER = 0xFE64;
constexpr uint16_t GALA_RED = 0xF98B;
constexpr uint16_t GALA_PURPLE = 0xA33F;
constexpr uint16_t GALA_DARK_TEXT = 0x2087;
constexpr uint16_t GALA_FRAME = 0x416D;
constexpr float GALA_ENEMY_MOTION_SCALE = 0.20f;




struct GalaBullet {
  float x, y;
  float vy;
  bool active;
};

struct GalaAlien {
  float x, y;
  float formX, formY;
  int type;
  bool active;
  bool diving;
  float diveAngle;
  float diveSpeed;
};

static GalaBullet pBullets[MAX_PLAYER_BULLETS];
static GalaBullet aBullets[MAX_ALIEN_BULLETS];
static GalaAlien aliens[TOTAL_ALIENS];

static int shipX = (GALA_W - SHIP_W) / 2;
static int prevShipX = shipX;
static int galaLastDirection = 0;
static int galaLives = 3;
static int galaWave = 1;
static unsigned long lastPlayerShot = 0;
static unsigned long lastAlienShot = 0;

static float fleetOffsetX = 0;
static float fleetDir = 1.0f;
static unsigned long lastFleetMove = 0;

static int galaPauseSelection = 0;
static const char *const GALA_PAUSE_ITEMS[] = {"Resume", "Restart", "High scores", "Main menu"};
static int galaWaveClearSelection = 0;
static bool galaPauseReady = false;


struct GalaStar {
  float x, y;
  float speed;
  uint16_t color;
};
static GalaStar stars[25];

static void initGalagaBackdropGrid() {}

static uint16_t galagaBackdropColor(int x, int y) {
  return arcadeStyle::backdrop(x, y);
}

static void restoreGalagaBackdropRect(int x, int y, int w, int h) {
  int x0 = max(0, x), x1 = min(GALA_W, x + w);
  int y0 = max(GALA_PLAYFIELD_TOP, y), y1 = min(GALA_PLAYFIELD_BOTTOM + 1, y + h);
  if (x1 <= x0 || y1 <= y0) return;
  static uint16_t row[GALA_W];
  tft.setSwapBytes(true);
  for (int yy = y0; yy < y1; yy++) {
    for (int xx = x0; xx < x1; xx++) row[xx - x0] = galagaBackdropColor(xx, yy);
    tft.pushImage(x0, yy, x1 - x0, 1, row);
  }
  tft.setSwapBytes(false);
}

static void drawGalagaBackdrop() {
  restoreGalagaBackdropRect(0, GALA_PLAYFIELD_TOP, GALA_W,
                            GALA_PLAYFIELD_BOTTOM - GALA_PLAYFIELD_TOP + 1);
}




static int galaSoundEffect = 0;
static unsigned long galaSoundStarted = 0;

static void updateGalagaSound() {

}

static void startGalagaSound(int effect) {
  audio::play(effect == 1 ? audio::Cue::Laser :
              effect == 2 ? audio::Cue::Hit : audio::Cue::Death);
}

static void soundPlayerLaser() {
  startGalagaSound(1);
}

static void soundAlienHit() {
  startGalagaSound(2);
}

static void soundPlayerExplode() {
  audio::play(galaLives <= 1 ? audio::Cue::GameOver : audio::Cue::Lose);
}




static void initStarfield() {
  for (int i = 0; i < 25; i++) {
    stars[i].x = random(0, GALA_W);
    stars[i].y = random(GALA_PLAYFIELD_TOP + 5, GALA_PLAYFIELD_BOTTOM - 5);
    stars[i].speed = random(10, 30) / 10.0f;
    int b = random(0, 3);
    stars[i].color = (b == 0) ? UI_MUTED : (b == 1 ? UI_SURFACE : 0xFFFF);
  }
}

static void updateStarfield() {
  for (int i = 0; i < 25; i++) {
    restoreGalagaBackdropRect((int)stars[i].x, (int)stars[i].y, 2, 2);
    stars[i].y += stars[i].speed;
    if (stars[i].y > GALA_PLAYFIELD_BOTTOM) {
      stars[i].y = GALA_PLAYFIELD_TOP + 2;
      stars[i].x = random(0, GALA_W);
    }
    tft.drawPixel((int)stars[i].x, (int)stars[i].y, stars[i].color);
  }
}




static void drawGalagaHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(224, 38)) return;
  header.fillSprite(GALA_BG);
  header.fillRoundRect(0, 0, 224, 38, 12, ui::CARD);
  header.setTextDatum(ML_DATUM);
  header.setTextColor(ui::LABEL);
  header.drawString("Galaga", 12, 8, 2);
  header.setTextColor(ui::SECONDARY);
  header.drawString("Wave " + String(galaWave), 12, 27, 2);
  header.setTextDatum(TR_DATUM);
  header.setTextColor(0xFD20);
  header.drawString("Score", 150, 3, 2);
  header.setTextColor(ui::SECONDARY);
  header.drawString("Ships", 212, 3, 2);
  header.setTextColor(ui::LABEL);
  header.drawString(String(score), 150, 19, 2);
  for (int i = 0; i < 3; i++) {
    int x = 180 + i * 12;
    header.fillTriangle(x, 31, x + 4, 23, x + 8, 31, i < galaLives ? GALA_PURPLE : 0x4208);
  }
  header.pushSprite(8, 5);
  header.deleteSprite();
}

static void updateGalagaScoreHUD() {
  drawGalagaHeader();
}

static void drawGalagaBottomBar() {
  tft.fillRect(0, 294, 240, 26, GALA_BG);
  String hint = "Wave " + String(galaWave) + "   Up to fire";
  ui::hint(306, hint.c_str(), GALA_BG);
}




static TFT_eSprite galaShipArt(&tft);
static TFT_eSprite galaEnemyArt(&tft);
static TFT_eSprite galaEnemyMove(&tft);
static unsigned long galaShipAnimationAt = 0;
static unsigned long galaEnemyAnimationAt = 0;

static void drawShip(int px, int py) {
  if (!galaShipArt.created() && !galaShipArt.createSprite(18, 22)) return;
  for (int y = 0; y < 22; ++y)
    for (int x = 0; x < 18; ++x)
      galaShipArt.drawPixel(x, y, galagaBackdropColor(px + x, py + y));
  int phase = (millis() / 80) % 3;
  int flameTip = 19 + phase;
  galaShipArt.fillTriangle(6, 15, 11, 15, 9, flameTip, GALA_RED);
  galaShipArt.fillTriangle(7, 16, 10, 16, 9, flameTip - 1, 0xFD20);
  galaShipArt.drawFastVLine(9, 16, 2 + phase, 0xFFE0);

  galaShipArt.fillTriangle(8, 0, 3, 15, 14, 15, 0x2948);
  galaShipArt.fillTriangle(4, 6, 0, 14, 6, 13, 0xFFFF);
  galaShipArt.fillTriangle(13, 6, 17, 14, 11, 13, 0xFFFF);
  galaShipArt.drawFastHLine(0, 14, 5, GALA_RED);
  galaShipArt.drawFastHLine(13, 14, 5, GALA_RED);
  galaShipArt.fillTriangle(8, 1, 5, 13, 12, 13, 0xCE59);
  galaShipArt.drawFastVLine(8, 2, 11, 0xFFFF);
  galaShipArt.fillRoundRect(7, 5, 4, 6, 1, 0x02B5);
  galaShipArt.drawFastVLine(8, 5, 4, 0x07FF);
  galaShipArt.drawPixel(8, 5, 0xFFFF);
  galaShipArt.fillRect(7, 13, 4, 3, 0x528A);
  galaShipArt.drawFastHLine(7, 15, 4, 0x9CF3);
  galaShipArt.drawPixel(2, 12, phase == 0 ? 0x07FF : 0x0410);
  galaShipArt.drawPixel(15, 12, phase == 0 ? 0x07FF : 0x0410);
  galaShipArt.pushSprite(px, py);
  galaShipAnimationAt = millis();
}

static void eraseShip(int px, int py) {
  restoreGalagaBackdropRect(px, py, SHIP_W, 22);
}

static bool prepareAlienSprite(int type) {
  if (!galaEnemyArt.created() && !galaEnemyArt.createSprite(16, 12)) return false;
  constexpr uint16_t transparent = 0xF81F;
  galaEnemyArt.fillSprite(transparent);
  int phase = ((millis() / 140) + type) & 1;
  uint16_t body = type == 2 ? GALA_PURPLE : (type == 1 ? GALA_GREEN : GALA_RED);
  galaEnemyArt.fillCircle(3, 5 + phase, 3, body);
  galaEnemyArt.fillCircle(12, 5 + phase, 3, body);
  galaEnemyArt.fillRoundRect(3, 1, 10, 10, 4, body);
  galaEnemyArt.drawFastHLine(6, 2, 4, 0xFFFF);
  galaEnemyArt.drawPixel(5, 5, GALA_DARK_TEXT);
  galaEnemyArt.drawPixel(10, 5, GALA_DARK_TEXT);
  galaEnemyArt.drawPixel(6, 7, GALA_DARK_TEXT);
  galaEnemyArt.drawFastHLine(7, 8, 2, GALA_DARK_TEXT);
  galaEnemyArt.drawPixel(9, 7, GALA_DARK_TEXT);
  if (type == 2) galaEnemyArt.drawFastHLine(6, 0, 4, GALA_AMBER);
  return true;
}

static void drawAlienSprite(int x, int y, int type) {
  if (!prepareAlienSprite(type)) return;
  constexpr uint16_t transparent = 0xF81F;
  int visibleHeight = min(12, GALA_PLAYFIELD_BOTTOM + 1 - y);
  if (visibleHeight > 0) {
    int vx = tft.getViewportX(), vy = tft.getViewportY();
    int vw = tft.getViewportWidth(), vh = tft.getViewportHeight();
    int top = max(vy, GALA_PLAYFIELD_TOP);
    int bottom = min(vy + vh, GALA_PLAYFIELD_BOTTOM + 1);
    if (top < bottom) {
      tft.setViewport(vx, top, vw, bottom - top, false);
      galaEnemyArt.pushSprite(x, y, transparent);
    }
    tft.setViewport(vx, vy, vw, vh, false);
  }
}

static void eraseAlienSprite(int x, int y) {
  restoreGalagaBackdropRect(x, y, 16, min(12, GALA_PLAYFIELD_BOTTOM + 1 - y));
}


static void moveAlienSprite(int oldX, int oldY, int x, int y, int type) {
  int left = min(oldX, x), top = min(oldY, y);
  int width = max(oldX, x) - left + 16, height = max(oldY, y) - top + 12;
  if (width > 24 || height > 24 ||
      (!galaEnemyMove.created() && !galaEnemyMove.createSprite(24, 24)) ||
      !prepareAlienSprite(type)) {
    eraseAlienSprite(oldX, oldY);
    drawAlienSprite(x, y, type);
    return;
  }
  for (int yy = 0; yy < height; ++yy)
    for (int xx = 0; xx < width; ++xx)
      galaEnemyMove.drawPixel(xx, yy, galagaBackdropColor(left + xx, top + yy));
  galaEnemyArt.pushToSprite(&galaEnemyMove, x - left, y - top, 0xF81F);
  int vx = tft.getViewportX(), vy = tft.getViewportY();
  int vw = tft.getViewportWidth(), vh = tft.getViewportHeight();
  int clipTop = max(vy, GALA_PLAYFIELD_TOP);
  int clipBottom = min(vy + vh, GALA_PLAYFIELD_BOTTOM + 1);
  if (clipTop < clipBottom) {
    tft.setViewport(vx, clipTop, vw, clipBottom - clipTop, false);
    galaEnemyMove.pushSprite(left, top, 0, 0, width, height);
  }
  tft.setViewport(vx, vy, vw, vh, false);
}

static void drawGalagaLaser(int index) {
  if (!pBullets[index].active) return;
  int x = (int)pBullets[index].x - GALA_LASER_WIDTH / 2;
  int y = (int)pBullets[index].y;
  for (int i = 3; i >= 0; --i) {
    int cy = y + GALA_LASER_HEIGHT + 3 + i * 4;
    int radius = i < 2 ? 2 : 1;
    if (cy - radius < GALA_PLAYFIELD_TOP || cy + radius >= SHIP_Y) continue;
    uint16_t color = arcadeStyle::mix(0xFD20, 0x36FF, i * 255 / 3);
    tft.fillCircle(x + GALA_LASER_WIDTH / 2 + ((i + millis() / 80) % 3) - 1,
                   cy, radius, arcadeStyle::fade(color, 150 - i * 30));
  }
  tft.fillRect(x, y, GALA_LASER_WIDTH, GALA_LASER_HEIGHT, GALA_LASER_COLOR);
  tft.drawFastVLine(x + 1, y + 1, 8, (millis() / 60) & 1 ? 0xFCB4 : 0xFFFF);
  tft.drawFastHLine(x + 1, y, 2, 0xFFFF);
  tft.drawFastHLine(x, y + GALA_LASER_HEIGHT - 1, GALA_LASER_WIDTH, GALA_CYAN);
}

static void drawAlienBullet(int index) {
  if (!aBullets[index].active) return;

  bool yellow = (((millis() / 180) + index) & 1) != 0;
  tft.fillCircle((int)aBullets[index].x, (int)aBullets[index].y, 2,
                 yellow ? 0xFFE0 : GALA_RED);
}


struct GalaPop { int x; float y; int points; uint16_t color; unsigned long born; bool active; };
static GalaPop galaPops[4] = {};
static int galaPopNext = 0;
static void eraseGalagaPops() {
  for (auto &p : galaPops)
    if (p.active) restoreGalagaBackdropRect(p.x - 16, (int)p.y, 32, 11);
}
static void drawGalagaPops() {
  for (auto &p : galaPops) {
    if (!p.active) continue;
    unsigned long age = millis() - p.born;
    if (age >= 600) { p.active = false; continue; }
    p.y = max((float)(GALA_PLAYFIELD_TOP + 8), p.y - 0.7f);
    int py = (int)p.y;
    uint16_t color = arcadeStyle::fade(p.color, max(0, 255 - (int)age * 255 / 600));
    int digits = p.points >= 100 ? 3 : 2;
    int px = p.x - (digits * 8 + 6) / 2;
    tft.fillRect(px, py + 4, 6, 2, color);
    tft.fillRect(px + 2, py + 2, 2, 6, color);
    int x = px + 8;
    if (p.points >= 100) {
      drawTinyDigit(tft, x, py, (p.points / 100) % 10, color);
      x += 8;
    }
    drawTinyDigit(tft, x, py, (p.points / 10) % 10, color);
    drawTinyDigit(tft, x + 8, py, p.points % 10, color);
  }
}




static void firePlayerLaser() {

  if (lastPlayerShot != 0 && millis() - lastPlayerShot < 120) return;
  for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
    if (!pBullets[i].active) {
      pBullets[i].x = shipX + SHIP_W / 2.0f;
      pBullets[i].y = SHIP_Y - GALA_LASER_HEIGHT - 2;
      pBullets[i].vy = -8.5f;
      pBullets[i].active = true;
      lastPlayerShot = millis();
      soundPlayerLaser();
      break;
    }
  }
}

static void alienShoot(int ax, int ay) {
  if (millis() - lastAlienShot < 1500) return;
  for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
    if (!aBullets[i].active) {
      aBullets[i].x = ax + 8;
      aBullets[i].y = ay + 12;
      aBullets[i].vy = 1.6f;
      aBullets[i].active = true;
      lastAlienShot = millis();
      break;
    }
  }
}

static void updateBullets() {

  for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
    if (pBullets[i].active) {
      int oldY = (int)pBullets[i].y;
      restoreGalagaBackdropRect((int)pBullets[i].x - 4, oldY, 9,
                               min(GALA_LASER_HEIGHT + 21, SHIP_Y - oldY));
      pBullets[i].y += pBullets[i].vy;

      if (pBullets[i].y < GALA_PLAYFIELD_TOP + 2) {
        pBullets[i].active = false;
        continue;
      }


      bool hit = false;
      for (int a = 0; a < TOTAL_ALIENS; a++) {
        if (aliens[a].active) {
          int ax = (int)aliens[a].x;
          int ay = (int)aliens[a].y;
          if (pBullets[i].x + GALA_LASER_WIDTH / 2 > ax &&
              pBullets[i].x - GALA_LASER_WIDTH / 2 < ax + 16 &&
              pBullets[i].y + GALA_LASER_HEIGHT > ay && pBullets[i].y < ay + 12) {
            aliens[a].active = false;
            eraseAlienSprite(ax, ay);
            pBullets[i].active = false;
            hit = true;
            soundAlienHit();

            int points = (aliens[a].type == 2) ? 150 : (aliens[a].type == 1 ? 80 : 40);
            score += points;
            galaPops[galaPopNext] = {constrain(ax + 8, 22, 218), (float)constrain(ay, 56, 260), points,
                                     aliens[a].type == 2 ? GALA_PURPLE : (aliens[a].type == 1 ? GALA_GREEN : GALA_RED), millis(), true};
            galaPopNext = (galaPopNext + 1) % 4;
            updateGalagaScoreHUD();
            break;
          }
        }
      }

      if (!hit && pBullets[i].active) {
        drawGalagaLaser(i);
      }
    }
  }


  for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
    if (aBullets[i].active) {
      restoreGalagaBackdropRect((int)aBullets[i].x - 2, (int)aBullets[i].y - 2, 5, 5);
      aBullets[i].y += aBullets[i].vy;

      if (aBullets[i].y > GALA_PLAYFIELD_BOTTOM - 2) {
        aBullets[i].active = false;
        continue;
      }


      if (aBullets[i].x >= shipX + 2 && aBullets[i].x <= shipX + SHIP_W - 2 &&
          aBullets[i].y >= SHIP_Y && aBullets[i].y <= SHIP_Y + SHIP_H) {
        aBullets[i].active = false;
        soundPlayerExplode();
        galaLives--;
        updateGalagaScoreHUD();

        if (galaLives <= 0) {
          gameState = 2;
          menu_drawn = false;
          return;
        } else {
          eraseShip(shipX, SHIP_Y);
          shipX = (GALA_W - SHIP_W) / 2;
          prevShipX = shipX;
          drawShip(shipX, SHIP_Y);
        }
      } else {
        drawAlienBullet(i);
      }
    }
  }
}




static void initGalagaWave(int wave) {
  if (wave == 1) audio::restartMusic();
  audio::play(audio::Cue::Wave);
  galaWave = wave;
  for (auto &p : galaPops) p.active = false;
  fleetOffsetX = 0;
  fleetDir = 1.0f;
  lastFleetMove = millis();

  int startX = 26;
  int startY = 64;
  int colW = 38;
  int rowH = 22;

  for (int r = 0; r < ALIEN_ROWS; r++) {
    for (int c = 0; c < ALIEN_COLS; c++) {
      int idx = r * ALIEN_COLS + c;
      aliens[idx].formX = startX + c * colW;
      aliens[idx].formY = startY + r * rowH;
      aliens[idx].x = aliens[idx].formX;
      aliens[idx].y = aliens[idx].formY;
      aliens[idx].type = (r == 0) ? 2 : (r == 1 ? 1 : 0);
      aliens[idx].active = true;
      aliens[idx].diving = false;
    }
  }

  for (int i = 0; i < MAX_PLAYER_BULLETS; i++) pBullets[i].active = false;
  for (int i = 0; i < MAX_ALIEN_BULLETS; i++) aBullets[i].active = false;

  tft.fillScreen(GALA_BG);
  initGalagaBackdropGrid();
  initStarfield();
  drawGalagaBackdrop();
  tft.drawFastHLine(0, GALA_PLAYFIELD_TOP - 1, GALA_W, GALA_SURFACE);
  tft.drawFastHLine(0, GALA_PLAYFIELD_BOTTOM + 1, GALA_W, GALA_SURFACE);

  drawGalagaHeader();
  drawGalagaBottomBar();

  drawShip(shipX, SHIP_Y);
  for (int i = 0; i < TOTAL_ALIENS; i++) {
    drawAlienSprite((int)aliens[i].x, (int)aliens[i].y, aliens[i].type);
  }
}

static void updateAliens() {
  bool animate = millis() - galaEnemyAnimationAt >= 140;
  if (animate) galaEnemyAnimationAt = millis();

  if (millis() - lastFleetMove > 60) {
    lastFleetMove = millis();
    fleetOffsetX += fleetDir * 1.6f * GALA_ENEMY_MOTION_SCALE;
    if (fleetOffsetX >= 26) {
      fleetOffsetX = 26;
      fleetDir = -1.0f;
    } else if (fleetOffsetX <= -22) {
      fleetOffsetX = -22;
      fleetDir = 1.0f;
    }
  }

  int activeCount = 0;

  for (int i = 0; i < TOTAL_ALIENS; i++) {
    if (!aliens[i].active) continue;
    activeCount++;

    int prevAX = (int)aliens[i].x;
    int prevAY = (int)aliens[i].y;

    if (!aliens[i].diving) {

      float targetX = aliens[i].formX + fleetOffsetX;
      if (abs(aliens[i].x - targetX) > 0.5f || abs(aliens[i].y - aliens[i].formY) > 0.5f) {
        aliens[i].x = targetX;
        aliens[i].y = aliens[i].formY;
        moveAlienSprite(prevAX, prevAY, (int)aliens[i].x, (int)aliens[i].y, aliens[i].type);
      } else if (animate) {
        moveAlienSprite(prevAX, prevAY, prevAX, prevAY, aliens[i].type);
      }


      if (random(1200) < 2) {
        aliens[i].diving = true;
        audio::play(audio::Cue::Dive);
        aliens[i].diveAngle = 0;
        aliens[i].diveSpeed = random(35, 55) / 10.0f * GALA_ENEMY_MOTION_SCALE;
      }
    } else {

      aliens[i].y += aliens[i].diveSpeed * 0.35f;
      aliens[i].x += sin(aliens[i].diveAngle) * 3.2f * GALA_ENEMY_MOTION_SCALE;
      aliens[i].diveAngle += 0.12f * GALA_ENEMY_MOTION_SCALE;


      if (aliens[i].x < 4) {
        aliens[i].x = 4;
        aliens[i].diveAngle = -aliens[i].diveAngle;
      } else if (aliens[i].x > GALA_W - 20) {
        aliens[i].x = GALA_W - 20;
        aliens[i].diveAngle = -aliens[i].diveAngle;
      }


      if (random(75) == 0) {
        alienShoot((int)aliens[i].x, (int)aliens[i].y);
      }


      if (aliens[i].y > GALA_PLAYFIELD_BOTTOM - 12) {
        aliens[i].x = aliens[i].formX + fleetOffsetX;
        aliens[i].y = aliens[i].formY;
        aliens[i].diving = false;
      }

      moveAlienSprite(prevAX, prevAY, (int)aliens[i].x, (int)aliens[i].y, aliens[i].type);


      if (aliens[i].x < shipX + SHIP_W && aliens[i].x + 16 > shipX &&
          aliens[i].y < SHIP_Y + SHIP_H && aliens[i].y + 12 > SHIP_Y) {
        aliens[i].active = false;
        eraseAlienSprite((int)aliens[i].x, (int)aliens[i].y);
        soundPlayerExplode();
        galaLives--;
        updateGalagaScoreHUD();

        if (galaLives <= 0) {
          gameState = 2;
          menu_drawn = false;
          return;
        }
      }
    }
  }


  if (activeCount == 0) {
    gameState = 3;
    menu_drawn = false;
    galaWaveClearSelection = 0;
  }
}




static void handleGalagaInput() {


  int pressedDirection = 0;
  bool firePressed = false;
  bool pausePressed = false;
  int command;
  while ((command = buttonsNextPress()) >= 0) {
    if (command == 1) pressedDirection = 1;
    else if (command == 3) pressedDirection = -1;
    else if (command == 0) firePressed = true;
    else if (command == 4) pausePressed = true;
  }
  if (pausePressed) {
    audio::pauseMusic(true); gameState = 1;
    menu_drawn = false;
    galaPauseSelection = 0;
    galaPauseReady = false;
    return;
  }
  uint8_t held = buttonsHeld();
  bool rightHeld = (held & 2) != 0;
  bool leftHeld = (held & 8) != 0;
  if (pressedDirection != 0) galaLastDirection = pressedDirection;
  int dir = pressedDirection;
  if (dir == 0) {
    if (rightHeld && leftHeld) dir = galaLastDirection;
    else if (rightHeld) dir = 1;
    else if (leftHeld) dir = -1;
  }
  if (dir == -1) {
    shipX -= SHIP_SPEED;
    if (shipX < 4) shipX = 4;
  } else if (dir == 1) {
    shipX += SHIP_SPEED;
    if (shipX > GALA_W - SHIP_W - 4) {
      shipX = GALA_W - SHIP_W - 4;
    }
  }

  if (shipX != prevShipX) {
    eraseShip(prevShipX, SHIP_Y);
    drawShip(shipX, SHIP_Y);
    prevShipX = shipX;
  }
  if (firePressed) {
    firePlayerLaser();
  }
}

static void drawGalagaPauseOption(int item) {
  ui::pauseItem(GALA_PAUSE_ITEMS, 4, item, galaPauseSelection, GALA_PURPLE);
}

static void drawGalagaPauseMenu() {
  buttonsRequireRelease();
  ui::pauseSheet("Galaga", GALA_PAUSE_ITEMS, 4, galaPauseSelection, GALA_PURPLE);
}

static void redrawGalagaPlayfield() {
  if (!restoringCountdownDigit()) tft.fillScreen(GALA_BG);
  drawGalagaBackdrop();
  tft.drawFastHLine(0, GALA_PLAYFIELD_TOP - 1, GALA_W, GALA_SURFACE);
  tft.drawFastHLine(0, GALA_PLAYFIELD_BOTTOM + 1, GALA_W, GALA_SURFACE);
  drawGalagaHeader();
  drawGalagaBottomBar();
  for (int i = 0; i < TOTAL_ALIENS; ++i)
    if (aliens[i].active) drawAlienSprite((int)aliens[i].x, (int)aliens[i].y, aliens[i].type);
  for (int i = 0; i < MAX_ALIEN_BULLETS; ++i)
    drawAlienBullet(i);
  for (int i = 0; i < MAX_PLAYER_BULLETS; ++i) drawGalagaLaser(i);
  drawShip(shipX, SHIP_Y);
  prevShipX = shipX;
}

static void handleGalagaPause() {
  int command = findCommand();
  if (command == 0 || command == 2) {
    int old = galaPauseSelection;
    galaPauseSelection = (galaPauseSelection + (command == 0 ? 3 : 1)) % 4;
    drawGalagaPauseOption(old);
    drawGalagaPauseOption(galaPauseSelection);
    playNavClick();
  } else if (command == 4) {
    playSelectTone();
    if (galaPauseSelection == 0) {
      showResumeCountdown(redrawGalagaPlayfield);
      lastFleetMove = lastAlienShot = millis();
      gameState = 0;
      menu_drawn = false;
    } else if (galaPauseSelection == 1) {
      score = 0;
      galaLives = 3;
      initGalagaWave(1);
      lastPlayerShot = 0;
      gameState = 0;
      menu_drawn = false;
    } else if (galaPauseSelection == 2) {
      showGameHighScores();
      redrawGalagaPlayfield();
      drawGalagaPauseMenu();
    } else {
      switchGame(0);
    }
  }
}

static void showGalagaStartScreen() {
  gameState = -1;
  tft.setRotation(2);
  tft.fillScreen(GALA_BG);
  initGalagaBackdropGrid();
  initStarfield();
  drawGalagaBackdrop();
  drawGalagaHeader();
  static const ui::Control controls[] = {
    {"Left / Right", "Steer"}, {"Up", "Fire"}, {"Shoot", "Pause"}};
  ui::startCard(52, "Galaga", "A little cosmic joy", controls, 3,
                currentGame.highScores[0], GALA_PURPLE, nullptr, "Press Shoot to start");
}

static void drawGalagaGameOver() {
  currentGame.recordScore(score);
  ui::gameOverSheet("Galaga", score, currentGame.highScores[0], GALA_PURPLE);
}

static void handleGalagaGameOver() {
  int cmd = findCommand();
  if (cmd == 4 || cmd == 0) {
    galaSoundEffect = 0;
    setBuzzerTone(0);
  }
  if (cmd == 4) {
    playSelectTone();
    score = 0;
    galaLives = 3;
    initGalagaWave(1);
    gameState = 0;
    menu_drawn = false;
  } else if (cmd == 0) {
    playSelectTone();
    switchGame(0);
  }
}

static const char *const GALA_WAVE_ITEMS[] = {"Next wave", "Main menu"};
static const ui::List GALA_WAVE_LIST = {40, 186, 160, 30, 2};
static void drawGalagaWaveClearOption(int item) {
  ui::listRow(GALA_WAVE_LIST, item, GALA_WAVE_ITEMS[item], item == galaWaveClearSelection,
              GALA_PURPLE);
}

static void drawGalagaWaveClear() {
  audio::play(audio::Cue::Win);
  buttonsRequireRelease();
  ui::card(26, 66, 188, 194);
  ui::heading(40, 80, "Galaga", "Lovely flying!");
  ui::stat(40, 128, "Score", String(score), GALA_PURPLE);
  ui::listAll(GALA_WAVE_LIST, GALA_WAVE_ITEMS, galaWaveClearSelection, GALA_PURPLE);
}

static void handleGalagaWaveClear() {
  int cmd = findCommand();
  if (cmd == 0 || cmd == 2) {
    int old = galaWaveClearSelection;
    galaWaveClearSelection = (galaWaveClearSelection + 1) % 2;
    drawGalagaWaveClearOption(old);
    drawGalagaWaveClearOption(galaWaveClearSelection);
    playNavClick();
  } else if (cmd == 4 && galaWaveClearSelection == 0) {
    galaSoundEffect = 0;
    setBuzzerTone(0);
    playSelectTone();
    score += galaLives * 300;
    initGalagaWave(galaWave + 1);
    gameState = 0;
    menu_drawn = false;
  } else if (cmd == 4) {
    playSelectTone();
    currentGame.recordScore(score);
    switchGame(0);
  }
}




void galagaSetup() {
  galaSoundEffect = 0;
  tft.setRotation(2);
  score = 0;
  galaLives = 3;
  lastPlayerShot = 0;
  galaLastDirection = 0;
  showGalagaStartScreen();
}

void galagaLoop() {
  audio::gameState(gameState);
  updateGalagaSound();
  if (gameState == 0) {
    handleGalagaInput();
    if (gameState != 0) return;
    eraseGalagaPops();
    updateBullets();
    if (gameState != 0) return;
    updateAliens();
    if (gameState != 0) return;
    drawGalagaPops();
    for (int i = 0; i < TOTAL_ALIENS; ++i)
      if (aliens[i].active) drawAlienSprite((int)aliens[i].x, (int)aliens[i].y, aliens[i].type);

    for (int i = 0; i < MAX_PLAYER_BULLETS; ++i) drawGalagaLaser(i);
    if (millis() - galaShipAnimationAt >= 80) drawShip(shipX, SHIP_Y);
    updateGalagaSound();
    delay(12);
    return;
  } else if (gameState == 1) {
    if (!menu_drawn) {
      drawGalagaPauseMenu();
      menu_drawn = true;
    }
    handleGalagaPause();
  } else if (gameState == 2) {
    if (!menu_drawn) {
      drawGalagaGameOver();
      menu_drawn = true;
    }
    handleGalagaGameOver();
  } else if (gameState == 3) {
    if (!menu_drawn) {
      drawGalagaWaveClear();
      menu_drawn = true;
    }
    handleGalagaWaveClear();
  } else if (gameState == -1) {
    int cmd = findCommand();
    if (cmd == 4) {
      playSelectTone();
      score = 0;
      galaLives = 3;
      initGalagaWave(1);
      gameState = 0;
    }
  }
}
# 1 "/home/pahasara/gameConsole/src/game.ino"
#include "rally.h"
#include "sd_game_music.h"
#include <Arduino.h>
#include <Preferences.h>
#include "game.h"
#include "ui_kit.h"
#include "tetris_input.h"
#include "buttons.h"
#include <Adafruit_ADS1X15.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorMP3a.h>
#include <AudioOutputI2S.h>
#include <new>
#include <string.h>
#include "images.h"


const char* gameNames[] = { "Menu", "Tetris", "Snake", "Breakout", "Run", "Galaga", "Puzzle",
                            "TicTacToe", "Pong", "Minesweeper", "Life", "Mandelbrot", "Rally" };
const int GAME_COUNT = sizeof(gameNames) / sizeof(gameNames[0]);
Preferences prefs;

uint16_t uiBg = 0x0000;
uint16_t uiPanel = 0x1082;
uint16_t uiSurface = 0x2104;
uint16_t uiCyan = 0x2EDC;
uint16_t uiGreen = 0xA7E9;
uint16_t uiPurple = 0xB37F;
uint16_t uiYellow = 0xFE4A;
uint16_t uiRed = 0xFA8D;
uint16_t uiText = 0xEF7D;
uint16_t uiMuted = 0x8C51;
uint16_t uiIconBg = 0x18C3;
uint16_t uiHeaderStart = 0xB7FF;
uint16_t uiHeaderEnd = 0xD65F;
bool lightThemeEnabled = false;
uint8_t activeTheme = 0;
uint8_t displayBrightness = 100;
bool soundEnabled = true;
bool batterySaver = false;
static uint8_t savedBrightness = 100;
static bool savedGameMusic = true;

const char* themeName(uint8_t index) {
  static const char* names[] = {"AURORA", "OCEAN", "ORCHID", "ROSE",
                               "CITRUS", "SUNSET", "MINT", "SILVER"};
  return names[index % THEME_COUNT];
}

void applyThemePreset(uint8_t index, bool persist) {
  activeTheme = index % THEME_COUNT;
  lightThemeEnabled = false;


  static const uint16_t palettes[THEME_COUNT][4] = {
    {0x7DFF, 0xB39F, 0xB7FF, 0xD65F},
    {0x4DDF, 0x3CFF, 0xAFFF, 0x9E7F},
    {0xCB5F, 0x93BF, 0xF67F, 0xCE7F},
    {0xFB75, 0xD3BE, 0xFE39, 0xF65C},
    {0xCFE8, 0x6EF3, 0xF7F1, 0xCFF7},
    {0xFD8B, 0xF395, 0xFF32, 0xFE19},
    {0x67D4, 0x4DDF, 0xBFFA, 0xA79F},
    {0xC638, 0x94B5, 0xF7DF, 0xCE79}
  };
  uiBg = 0x0000; uiPanel = 0x1082; uiSurface = 0x2945; uiIconBg = 0x18C3;
  uiText = 0xF7DE; uiMuted = 0xAD55;
  uiCyan = palettes[activeTheme][0];
  uiPurple = palettes[activeTheme][1];
  uiHeaderStart = palettes[activeTheme][2];
  uiHeaderEnd = palettes[activeTheme][3];
  uiGreen = 0xA7E9; uiYellow = 0xFFE8; uiRed = 0xFA8D;
  if (persist) {
    prefs.begin("console", false);
    prefs.putUChar("theme", activeTheme);
    prefs.end();
  }
}

void loadThemePreference() {
  prefs.begin("console", true);
  uint8_t savedTheme = prefs.getUChar("theme", 0);
  displayBrightness = prefs.getUChar("brightness", 100);
  soundEnabled = prefs.getBool("sound", true);
  batterySaver = prefs.getBool("saver", false);
  savedBrightness = prefs.getUChar("saverbright", 100);
  savedGameMusic = prefs.getBool("savermusic", true);
  prefs.end();
  audio::load();
  if (batterySaver) { displayBrightness = 10; audio::gameMusic = false; }
  applyThemePreset(savedTheme, false);
}

void setDisplayBrightness(uint8_t brightness, bool persist) {
  static bool backlightPwmReady = false;
  displayBrightness = constrain(int(brightness), 10, 100);
  if (!backlightPwmReady && !ledcSetup(0, 5000, 8)) {
    Serial.println("[Display] Backlight PWM setup failed");
    return;
  }
  backlightPwmReady = true;

  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, (uint16_t(displayBrightness) * 255) / 100);
  if (persist) {
    prefs.begin("console", false);
    prefs.putUChar("brightness", displayBrightness);
    prefs.end();
  }
}

void setBatterySaver(bool enabled) {
  if (enabled == batterySaver) return;
  if (enabled) {
    savedBrightness = displayBrightness;
    savedGameMusic = audio::gameMusic;
  }
  batterySaver = enabled;
  setDisplayBrightness(enabled ? 10 : savedBrightness, true);
  audio::gameMusic = enabled ? false : savedGameMusic;
  audio::save();
  prefs.begin("console", false);
  prefs.putBool("saver", batterySaver);
  prefs.putUChar("saverbright", savedBrightness);
  prefs.putBool("savermusic", savedGameMusic);
  prefs.end();
}

Game::Game(int i) {
  if (i < 0 || i >= GAME_COUNT) i = 0;
  index = i;
  name = gameNames[index];

  switch (index) {
    case 0:
      gameSetup = menuSetup;
      gameLoop = menuLoop;
      break;
    case 1:
      gameSetup = tetrisSetup;
      gameLoop = tetrisLoop;
      break;
    case 2:
      gameSetup = setupSnake;
      gameLoop = loopSnake;
      break;

    case 3:
      gameSetup = breakoutSetup;
      gameLoop = breakoutLoop;
      break;
    case 4:
      gameSetup = runSetup;
      gameLoop = runLoop;
      break;
    case 5:
      gameSetup = galagaSetup;
      gameLoop = galagaLoop;
      break;
    case 6:
      gameSetup = puzzleSetup;
      gameLoop = puzzleLoop;
      break;
    case 7:
      gameSetup = ticTacToeSetup;
      gameLoop = ticTacToeLoop;
      break;
    case 8:
      gameSetup = pongSetup;
      gameLoop = pongLoop;
      break;
    case 9:
      gameSetup = minesweeperSetup;
      gameLoop = minesweeperLoop;
      break;
    case 10:
      gameSetup = lifeSetup;
      gameLoop = lifeLoop;
      break;
    case 11:
      gameSetup = mandelbrotSetup;
      gameLoop = mandelbrotLoop;
      break;
    case 12:
      gameSetup = rallySetup;
      gameLoop = rallyLoop;
      break;
  }

  prefs.begin(name, false);
  for (int i = 0; i < 5; i++) {
    char key[50];
    snprintf(key, sizeof(key), "score_%d", i);
    highScores[i] = prefs.getInt(key);
  }
  prefs.end();
}


void Game::recordScore(int score) {
  if (score <= 0) return;
  if (score > highScores[0]) audio::play(audio::Cue::Record);
  for (int position = 0; position < 5; ++position) {
    if (score <= highScores[position]) continue;
    for (int move = 4; move > position; --move)
      highScores[move] = highScores[move - 1];
    highScores[position] = score;
    break;
  }

  prefs.begin(name, false);
  for (int i = 0; i < 5; i++) {
    char key[50];
    snprintf(key, sizeof(key), "score_%d", i);
    prefs.putInt(key, highScores[i]);
  }
  prefs.end();
}

static bool countdownDigitRestore = false;
bool restoringCountdownDigit() { return countdownDigitRestore; }



void showResumeCountdown(void (*redrawScene)()) {
  buttonsRequireRelease();
  tft.setViewport(18, 58, 204, 200, false);
  redrawScene();
  tft.resetViewport();
  const int centerX = tft.width() / 2;
  const int centerY = tft.height() / 2;
  for (int number = 3; number >= 1; --number) {
    if (number != 3) {
      countdownDigitRestore = true;
      tft.setViewport(centerX - 32, centerY - 32, 64, 64, false);
      redrawScene();
      tft.resetViewport();
    }
    tft.fillCircle(centerX, centerY, 30, ui::CARD);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4);
    tft.setTextSize(2);
    tft.setTextColor(ui::LABEL, ui::CARD);
    tft.drawString(String(number), centerX, centerY + 2);
    tft.setTextSize(1);
    audio::play(audio::Cue::Countdown);
    unsigned long started = millis();
    while (millis() - started < 1000) delay(5);
  }
  countdownDigitRestore = true;
  tft.setViewport(centerX - 32, centerY - 32, 64, 64, false);
  redrawScene();
  tft.resetViewport();
  countdownDigitRestore = false;
  buttonsRequireRelease();
}


static const char *gameDisplayName(const char *key) {
  if (!strcmp(key, "TicTacToe")) return "Tic-tac-toe";
  if (!strcmp(key, "Minesweeper")) return "Mines";
  return key;
}

void showGameHighScores() {
  const uint16_t medal[4] = {0xFEA0, 0xC618, 0xCB0C, 0x4A69};
  const char *places[] = {"1st", "2nd", "3rd", "4th"};
  tft.fillScreen(UI_BG);
  ui::heading(20, 16, gameDisplayName(currentGame.name), "High scores", UI_BG);

  const int x = 16, y = 74, w = 208, rowH = 46;
  tft.fillRoundRect(x, y, w, rowH * 4, 12, ui::GROUP);
  for (int rank = 0; rank < 4; ++rank) {
    int top = y + rank * rowH, cy = top + rowH / 2;
    if (rank) tft.drawFastHLine(x + 52, top, w - 52, ui::SEPARATOR);
    tft.fillCircle(x + 26, cy, 12, medal[rank]);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(ui::inkOn(medal[rank]), medal[rank]);
    tft.drawString(String(rank + 1), x + 26, cy + 1, 2);

    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(ui::SECONDARY, ui::GROUP);
    tft.drawString(places[rank], x + 52, cy + 1, 2);

    int value = currentGame.highScores[rank];
    String text = value > 0 ? String(value) : String("-");
    if (value > 0 && !strcmp(currentGame.name, "Run")) text += " m";
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(value > 0 ? ui::LABEL : ui::SECONDARY, ui::GROUP);
    tft.drawString(text, x + w - 14, cy + 2, 4);
  }
  ui::pill(60, 274, 120, 30, "Done", UI_CYAN);




  buttonsRequireRelease();
  while (readShoot() != LOW) delay(2);
  delay(22);
  while (true) {
    int command = findCommand(false);
    bool shootPressed = readShoot() != LOW;
    if (shootPressed) {
      delay(22);
      if (readShoot() != LOW) break;
    }
    if (command == 4 || command == 0) break;
    if (Serial.available()) {
      char key = Serial.read();
      if (key == ' ' || key == 'e' || key == 'E' || key == '\n' ||
          key == '\r' || key == 'w' || key == 'W') break;
    }
    delay(4);
  }
  playSelectTone();
  while (readShoot() != LOW) delay(2);
  buttonsRequireRelease();

}

void resetAllGameScores() {


  for (int gameIndex = 1; gameIndex <= 9; ++gameIndex) {
    prefs.begin(gameNames[gameIndex], false);
    for (int rank = 0; rank < 5; ++rank) {
      char key[20];
      snprintf(key, sizeof(key), "score_%d", rank);
      prefs.remove(key);
    }
    prefs.end();
  }
}

extern Game currentGame;

void switchGame(int gameIndex) {
  audio::scene(gameIndex);
  audio::pauseMusic(true);
  audio::play(gameIndex == 0 ? audio::Cue::Back : audio::Cue::Open);
  lifeInputEnd();
  lifeRelease();
  tetrisInputEnd();
  buttonsRequireRelease();
  Serial.printf("[Game] Switching to game %d (%s)\n", gameIndex, (gameIndex >= 0 && gameIndex < GAME_COUNT) ? gameNames[gameIndex] : "Unknown");
  currentGame = Game(gameIndex);
  currentGame.gameSetup();
}

Adafruit_ADS1115 ads;
TFT_eSPI tft = TFT_eSPI();
bool adsConnected = false;

void drawTransparentSpriteFast(int x, int y, int w, int h, const uint16_t *img, uint16_t transparentColor) {
  static uint16_t lineBuffer[128];
  tft.startWrite();
  for (int j = 0; j < h; j++) {
    int pixelsToDraw = 0;
    int startX = -1;
    for (int i = 0; i < w; i++) {
      uint16_t color = pgm_read_word(&img[j * w + i]);
      if (color != transparentColor) {
        if (startX == -1) startX = i;
        lineBuffer[pixelsToDraw++] = color;
      } else {
        if (pixelsToDraw > 0) {
          tft.setAddrWindow(x + startX, y + j, pixelsToDraw, 1);
          tft.pushColors(lineBuffer, pixelsToDraw);
          pixelsToDraw = 0;
          startX = -1;
        }
      }
    }
    if (pixelsToDraw > 0) {
      tft.setAddrWindow(x + startX, y + j, pixelsToDraw, 1);
      tft.pushColors(lineBuffer, pixelsToDraw);
    }
  }
  tft.endWrite();
}

int score = 0;
int gameState = 0;
int menu_drawn = false;
unsigned long lastCommand = 0;
unsigned long lastTimeUpdate = 0;
int selected_menu_item = 1;
const unsigned long debounce_delay = 180;



#include <driver/i2s.h>
static volatile uint32_t i2sToneFreq = 0;
static TaskHandle_t i2sTask = nullptr;
static volatile bool i2sHoldReq = false, i2sHeld = false;
static constexpr int I2S_RATE = 22050;

extern const uint8_t nokiaDataStart[] asm("_binary_assets_nokia_welcome_mp3_start");
extern const uint8_t nokiaDataEnd[] asm("_binary_assets_nokia_welcome_mp3_end");
static AudioFileSourcePROGMEM *nokiaFile = nullptr;
static AudioGeneratorMP3a *nokiaDecoder = nullptr;
static AudioOutputI2S *nokiaOutput = nullptr;
static bool nokiaAudioHeld = false;

static void i2sToneTask(void *) {
  int16_t buf[256], music[256];
  while (true) {
    if (i2sHoldReq) { sd_music::stop(); i2sHeld = true; vTaskDelay(pdMS_TO_TICKS(10)); continue; }
    i2sHeld = false;
    bool embeddedTrack = sd_music::fill(music, 256, soundEnabled && !i2sToneFreq);
    audio::render(buf, 256, i2sToneFreq, soundEnabled, embeddedTrack ? music : nullptr);
    size_t written;
    i2s_write(I2S_NUM_0, buf, sizeof(buf), &written, portMAX_DELAY);
  }
}

static bool installI2S();
static void initI2SAudio() {
  if (installI2S()) xTaskCreate(i2sToneTask, "i2sTone", 8192, nullptr, 3, &i2sTask);
}


void audioReleaseForMusic() {
  audio::external(true);
  i2sToneFreq = 0;
  i2sHoldReq = true;
  while (i2sTask && !i2sHeld) delay(5);
  i2s_driver_uninstall(I2S_NUM_0);
}


void audioReclaimFromMusic() {
  i2s_driver_uninstall(I2S_NUM_0);
  installI2S();

  setDisplayBrightness(displayBrightness, false);
  audio::external(false);
  i2sHoldReq = false;
}

static bool installI2S() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = I2S_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 128;
  cfg.tx_desc_auto_clear = true;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = I2S_BCLK_PIN;
  pins.ws_io_num = I2S_LRCLK_PIN;
  pins.data_out_num = I2S_DOUT_PIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) != ESP_OK ||
      i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) {
    Serial.println("[Audio] I2S init failed");
    return false;
  }
  return true;
}

void setBuzzerTone(unsigned int frequency) {
  i2sToneFreq = (soundEnabled && frequency > 0) ? frequency : 0;
}

void setSoundEnabled(bool enabled, bool persist) {
  soundEnabled = enabled;
  if (!soundEnabled) i2sToneFreq = 0;

  if (persist) {
    prefs.begin("console", false);
    prefs.putBool("sound", soundEnabled);
    prefs.end();
  }
}

void playToneDirect(unsigned int freq, unsigned long durationMs) {
  if (freq > 0) {
    setBuzzerTone(freq);
  } else {
    setBuzzerTone(0);
  }
  delay(durationMs);
  setBuzzerTone(0);
}

void playNavClick() {
  audio::play(audio::Cue::Move);
}

void playSelectTone() {
  audio::play(audio::Cue::Open);
}

static void stopNokiaMusic() {
  if (nokiaDecoder) nokiaDecoder->stop();
  delete nokiaDecoder; nokiaDecoder = nullptr;
  delete nokiaFile; nokiaFile = nullptr;
  delete nokiaOutput; nokiaOutput = nullptr;
  if (nokiaAudioHeld) {
    audioReclaimFromMusic();
    nokiaAudioHeld = false;
  }
}

void playNokiaSweepSound() {
  if (!soundEnabled) return;
  audioReleaseForMusic();
  nokiaAudioHeld = true;
  nokiaFile = new (std::nothrow) AudioFileSourcePROGMEM(
      nokiaDataStart, nokiaDataEnd - nokiaDataStart);
  nokiaDecoder = new (std::nothrow) AudioGeneratorMP3a();
  nokiaOutput = new (std::nothrow) AudioOutputI2S(0, AudioOutputI2S::EXTERNAL_I2S, 16);
  if (!nokiaFile || !nokiaFile->isOpen() || !nokiaDecoder ||
      !nokiaOutput) {
    Serial.println("[Audio] System music open failed");
    stopNokiaMusic();
    return;
  }
  nokiaOutput->SetPinout(I2S_BCLK_PIN, I2S_LRCLK_PIN, I2S_DOUT_PIN);
  nokiaOutput->SetGain(audio::mediaGain(0.35f));
  if (!nokiaDecoder->begin(nokiaFile, nokiaOutput)) {
    Serial.println("[Audio] System music decode failed");
    stopNokiaMusic();
    return;
  }
  Serial.println("[Audio] Playing embedded Nokia welcome tune (no SD required)");
  setDisplayBrightness(displayBrightness, false);
}

static void serviceNokiaMusic() {
  if (nokiaDecoder && nokiaDecoder->isRunning() && !nokiaDecoder->loop())
    stopNokiaMusic();
}

void playTetrisDropSound() {
  audio::play(audio::Cue::Lock);
}

void drawBootHand(int tip, bool fromLeft, uint16_t handColor) {

  auto handX = [tip, fromLeft](int distance, int width) {
    return fromLeft ? tip - distance - width : tip + distance;
  };
  tft.fillRoundRect(handX(57, 56), 138, 56, 23, 5, UI_SURFACE);
  tft.fillRect(handX(91, 8), 138, 8, 23, UI_MUTED);
  tft.fillRoundRect(handX(25, 37), 130, 37, 36, 7, handColor);
  tft.fillRoundRect(handX(0, 31), 126, 31, 6, 2, handColor);
  tft.fillRoundRect(handX(0, 34), 134, 34, 7, 2, handColor);
  tft.fillRoundRect(handX(0, 33), 143, 33, 7, 2, handColor);
  tft.fillRoundRect(handX(4, 27), 152, 27, 6, 2, handColor);
}

void drawBootSpark(int x, int y, uint16_t color) {
  tft.drawFastHLine(x - 5, y, 11, color);
  tft.drawFastVLine(x, y - 5, 11, color);
  tft.drawPixel(x - 3, y - 3, color);
  tft.drawPixel(x + 3, y + 3, color);
}

uint16_t bootRgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void fillBootGradient(int x, int y, int w, int h,
                      uint8_t r1, uint8_t g1, uint8_t b1,
                      uint8_t r2, uint8_t g2, uint8_t b2) {
  for (int row = 0; row < h; row++) {
    float t = h <= 1 ? 0.0f : (float)row / (float)(h - 1);
    uint8_t r = r1 + (int)((r2 - r1) * t);
    uint8_t g = g1 + (int)((g2 - g1) * t);
    uint8_t b = b1 + (int)((b2 - b1) * t);
    tft.drawFastHLine(x, y + row, w, bootRgb565(r, g, b));
  }
}


static uint16_t bootLerp(uint16_t a, uint16_t b, float t) {
  t = fminf(1.0f, fmaxf(0.0f, t));
  const int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  const int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)(((ar + (int)((br - ar) * t)) << 11) |
                    ((ag + (int)((bg - ag) * t)) << 5) | (ab + (int)((bb - ab) * t)));
}


static float bootEase(int frame, int from, int len) {
  float t = (frame - from) / (float)len;
  t = fminf(1.0f, fmaxf(0.0f, t));
  return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}



static void bootDrawIcon(TFT_eSprite *spr, int cx, int cy, int size, float alpha) {
  auto hline = [&](int x, int y, int w, uint16_t c) {
    if (spr) spr->drawFastHLine(x, y, w, c); else tft.drawFastHLine(x, y, w, c);
  };
  const int r = size * 28 / 100;
  for (int row = 0; row < size; ++row) {
    const int edge = min(row, size - 1 - row);
    const int inset = edge < r ? r - (int)sqrtf((float)(r * r - (r - edge) * (r - edge))) : 0;
    const float t = row / (float)max(1, size - 1);
    const uint16_t top = bootRgb565(110, 120, 255), bottom = bootRgb565(70, 60, 200);
    hline(cx - size / 2 + inset, cy - size / 2 + row, size - inset * 2,
          bootLerp(TFT_BLACK, bootLerp(top, bottom, t), alpha));
  }
  const uint16_t ink = bootLerp(TFT_BLACK, TFT_WHITE, alpha);
  const int u = max(2, size / 22);
  const int lx = cx - size / 5, rx = cx + size / 5;
  auto rr = [&](int x, int y, int w, int h) {
    if (spr) spr->fillRoundRect(x, y, w, h, u / 2, ink); else tft.fillRoundRect(x, y, w, h, u / 2, ink);
  };
  auto cc = [&](int x, int y, int rad) {
    if (spr) spr->fillCircle(x, y, rad, ink); else tft.fillCircle(x, y, rad, ink);
  };
  rr(lx - u * 3, cy - u, u * 6, u * 2);
  rr(lx - u, cy - u * 3, u * 2, u * 6);
  cc(rx + u + u / 2, cy - u, u + u / 3);
  cc(rx - u / 2, cy + u + u / 2, u + u / 3);
}

void playBootAnimation() {



  const int cx = tft.width() / 2;
  const int top = (tft.height() - 240) / 2;
  const int iconCY = top + 88;
  const int titleY = top + 160;
  const uint16_t grey = bootRgb565(140, 146, 160);
  const int kFrames = 64, kFrameMs = 25;
  const int S = 100;

  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);

  TFT_eSprite icon(&tft);
  icon.setColorDepth(16);
  const bool buffered = icon.createSprite(S, S) != nullptr;
  playNokiaSweepSound();
  const uint32_t musicStarted = millis();

  for (int frame = 0; frame <= kFrames; ++frame) {
    const unsigned long frameStart = millis();
    serviceNokiaMusic();


    if (frame <= 22) {
      const float t = bootEase(frame, 0, 20);
      const int size = (int)(76 * (0.86f + 0.14f * t)) & ~1;
      if (buffered) {
        icon.fillSprite(TFT_BLACK);
        bootDrawIcon(&icon, S / 2, S / 2, size, t);
        icon.pushSprite(cx - S / 2, iconCY - S / 2);
      } else if (frame == 22) {
        bootDrawIcon(nullptr, cx, iconCY, 76, 1.0f);
      }
    }

    serviceNokiaMusic();

    if (frame >= 12 && frame <= 34) {
      const float t = bootEase(frame, 12, 20);
      const int lift = (int)(8 * (1.0f - t));
      tft.fillRect(cx - 100, titleY - 16, 200, 34, TFT_BLACK);
      tft.setTextColor(bootLerp(TFT_BLACK, TFT_WHITE, t), TFT_BLACK);
      tft.drawString("Game Console", cx, titleY + lift, 4);
    }

    serviceNokiaMusic();

    if (frame >= 24 && frame <= 44) {
      const int half = (int)(28 * bootEase(frame, 24, 20));
      tft.drawFastHLine(cx - half, titleY + 24, half * 2, grey);
    }


    if (frame >= 30 && frame <= 50 && (frame & 1) == 0) {
      tft.setTextColor(bootLerp(TFT_BLACK, grey, bootEase(frame, 30, 20)), TFT_BLACK);
      tft.drawString("Ready to play", cx, titleY + 44, 2);
    }

    while (millis() - frameStart < (unsigned long)kFrameMs) {
      serviceNokiaMusic();
      if (nokiaDecoder) yield();
      else delay(1);
    }
  }
  setBuzzerTone(0);
  if (buffered) icon.deleteSprite();
  while (nokiaDecoder && millis() - musicStarted < 10000) {
    serviceNokiaMusic();
    yield();
  }
  stopNokiaMusic();


  for (int step = 1; step <= 10; ++step) {
    const float a = 1.0f - step / 10.0f;
    bootDrawIcon(nullptr, cx, iconCY, 76, a);
    tft.setTextColor(bootLerp(TFT_BLACK, TFT_WHITE, a), TFT_BLACK);
    tft.drawString("Game Console", cx, titleY, 4);
    tft.drawFastHLine(cx - 28, titleY + 24, 56, bootLerp(TFT_BLACK, grey, a));
    tft.setTextColor(bootLerp(TFT_BLACK, grey, a), TFT_BLACK);
    tft.drawString("Ready to play", cx, titleY + 44, 2);
    delay(22);
  }
  tft.fillScreen(TFT_BLACK);
}
void init_interface() {
  Serial.begin(115200);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(150);
  tft.init();
  tft.setRotation(2);
  loadThemePreference();
  setDisplayBrightness(displayBrightness, false);
  tft.fillScreen(TFT_BLACK);
  Serial.println("[Game] Starting interface initialization...");

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  sd_music::begin();
  initI2SAudio();
  setBuzzerTone(0);
#ifndef BIRTHDAY_DEV
  playBootAnimation();
#endif

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);
  Wire.setTimeOut(20);
  if (!ads.begin(0x48)) {
    Serial.println("[Game] Warning: ADS1115 not detected at 0x48. Check I2C wiring (SDA=10, SCL=20).");
    adsConnected = false;
  } else {
    ads.setGain(GAIN_ONE);
    ads.setDataRate(RATE_ADS1115_860SPS);
    adsConnected = true;
    Serial.println("[Game] ADS1115 ADC initialized at 0x48.");
  }

  pinMode(RIGHT_BTN, INPUT_PULLUP);
  if (!buttonsBegin()) Serial.println("[Input] ERROR: button scanner could not start");
}

static int lastActiveCommand = -1;

bool isDownHeld() { return (buttonsHeld() & 4) != 0; }

int findCommand(bool repeat) {
  unsigned long now = millis();
  int command = buttonsNextPress();
  if (command >= 0) {
    lastActiveCommand = command;
    lastCommand = now;
    return command;
  }
  uint8_t held = buttonsHeld();
  if (lastActiveCommand < 0 || !(held & (1 << lastActiveCommand))) {
    lastActiveCommand = -1;
    return -1;
  }

  if (repeat && lastActiveCommand < 4 && now - lastCommand >= 350) {
    lastCommand = now;
    return lastActiveCommand;
  }
  return -1;
}

void PauseMenuControls() {
  int pressed = findCommand();
  int x = (tft.width() > 250 ? 1 : 0);


  if (pressed == 0 + x) updatePauseMenu(1);
  if (pressed == 2 + x) updatePauseMenu(-1);

  if (pressed == 5) {
    if (gameState == 0) {
      audio::pauseMusic(true); gameState = 1;
      menu_drawn = false;
    } else if (gameState == 1) {
      switch (selected_menu_item) {
        case 1:
          restoreAfterPauseTetris();
        case 0:
          ESP.restart();
        case 2:
          ESP.restart();
      }
    }
  }
}





void updatePauseMenu(int dx) {
  const char* labels[] = {"RETRY", "RESUME", "MENU"};
  int new_item = selected_menu_item + dx;
  if (new_item >= 0 && new_item < 3) selected_menu_item = new_item;

  for (int i = 0; i < 3; i++) {
    int y = 140 + i * 34;
    bool sel = (i == selected_menu_item);
    uint16_t fill = UI_ICON_BG;
    tft.fillRoundRect(36, y, 168, 28, 6, fill);
    if (sel) {
      tft.drawRoundRect(36, y, 168, 28, 6, UI_PURPLE);
      tft.drawRoundRect(37, y + 1, 166, 26, 5, UI_PURPLE);
      tft.drawRoundRect(38, y + 2, 164, 24, 4, UI_CYAN);
    }
    tft.setTextDatum(ML_DATUM);
    tft.setTextFont(2);
    tft.setTextColor(sel ? UI_TEXT : UI_MUTED, fill);
    tft.drawString(labels[i], 48, y + 14);
    if (sel) {
      tft.setTextDatum(MR_DATUM);
      tft.drawString(">", 192, y + 14);
    }
  }
}

void drawPauseMenu(void) {
  tft.fillRoundRect(24, 75, 192, 175, 12, UI_PANEL);
  tft.drawRoundRect(24, 75, 192, 175, 12, UI_SURFACE);
  tft.fillRect(38, 91, 24, 3, 0xFFFF);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString("PAUSED", 38, 99);
  updatePauseMenu(0);
}




void showGameOver() {
  static bool gameOverDrawn = false;
  currentGame.recordScore(score);

  if (!gameOverDrawn) {
    tft.fillRoundRect(24, 65, 192, 190, 12, UI_PANEL);
    tft.drawRoundRect(24, 65, 192, 190, 12, UI_SURFACE);

    tft.fillRect(38, 81, 24, 3, UI_RED);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(1);
    tft.setTextColor(UI_MUTED, UI_PANEL);
    tft.drawString("GAME / FINISHED", 38, 93);

    tft.setTextFont(4);
    tft.setTextColor(UI_TEXT, UI_PANEL);
    tft.drawString("GAME OVER", 38, 107);

    tft.setTextFont(1);
    tft.setTextColor(UI_MUTED, UI_PANEL);
    tft.drawString("SCORE", 38, 145);
    tft.drawString("BEST", 136, 145);

    tft.setTextFont(2);
    tft.setTextColor(UI_TEXT, UI_PANEL);
    tft.drawString(String(score), 38, 158);
    tft.setTextColor(0xFFFF, UI_PANEL);
    tft.drawString(String(currentGame.highScores[0]), 136, 158);

    tft.drawFastHLine(38, 186, 164, UI_SURFACE);
    tft.setTextFont(1);
    tft.setTextColor(0xFFFF, UI_PANEL);
    tft.drawString("SHOOT  RETRY", 38, 198);
    tft.setTextColor(UI_MUTED, UI_PANEL);
    tft.drawString("UP  MAIN MENU", 38, 218);

    gameOverDrawn = true;
  }
}

void drawDefaultBg(){
   int ymax = tft.width() <= 240 ? 32:24;
      int xmax = tft.width() > 240 ? 32:24;


   for (int y = 0; y < ymax; y++) {
      for (int x = 0; x < 32; x++) {
        tft.fillRect(x * 10, y * 10, 10, 10, bg());
      }
    }
}

void drawDefaultBg2(){
   int ymax = tft.width() <= 240 ? 32:24;
      int xmax = tft.width() > 240 ? 32:24;


   for (int y = 0; y < ymax; y++) {
      for (int x = 0; x < 32; x++) {
        tft.fillRect(x * 10, y * 10, 10, 10, (x+y) % 2 == 0? BG1:BG2);
      }
    }
}

int getDirectHorizontalInput() {


  while (buttonsNextPress() >= 0) {}
  uint8_t held = buttonsHeld();
  int direction = ((held & 2) != 0) - ((held & 8) != 0);
  if (direction) return direction;
  if (Serial.available()) {
    char c = Serial.peek();
    if (c == 'd' || c == 'l') { Serial.read(); return 1; }
    if (c == 'a' || c == 'j') { Serial.read(); return -1; }
  }
  return 0;
}
# 1 "/home/pahasara/gameConsole/src/menu.ino"
#include "game.h"
#include "ui_kit.h"
#include "home_icons.h"
#include "home_wallpaper.h"
#include "config.h"
#include "birthday.h"


struct App {
  const char* label;
  int x_coord;
  int y_coord;
  const uint16_t* img;
  int gameIndex;
};

App Apps[] = {
  {"Tetris", 0, 0, tetris_home_icon, 1},
  {"Snake", 0, 0, snake_home_icon, 2},
  {"Breakout", 0, 0, breakout_home_icon, 3},
  {"Run", 0, 0, run_home_icon, 4},
  {"Galaga", 0, 0, galaga_home_icon, 5},
  {"Rally", 0, 0, rally_home_icon, 12},
  {"Puzzle", 0, 0, puzzle_home_icon, 6},
  {"Tic-Tac", 0, 0, tic_tac_home_icon, 7},
  {"Pong", 0, 0, pong_home_icon, 8},
  {"Mines", 0, 0, mines_home_icon, 9},
  {"Conway's", 0, 0, life_home_icon, 10},
  {"Mandel", 0, 0, mandelbrot_home_icon,11},
  {"Music", 0, 0, sound_home_icon, -1},
  {"Videos", 0, 0, videos_home_icon, -1},
  {"Settings", 0, 0, settings_home_icon,-1},
  {"Info", 0, 0, about_home_icon, -1}
};

const int APP_COUNT = sizeof(Apps) / sizeof(App);




constexpr int PORTRAIT_W = 240, PORTRAIT_H = 320;
constexpr int COLS = 3, VISIBLE_ROWS = 3;
constexpr int APPS_PER_PAGE = COLS * VISIBLE_ROWS;
constexpr int PAGE_COUNT = (APP_COUNT + APPS_PER_PAGE - 1) / APPS_PER_PAGE;
constexpr int TILE_W = 72, TILE_H = 76, ICON_SIZE = 52;
constexpr int START_Y = 68, ROW_PITCH = 75, COL_PITCH = 76, MARGIN_LEFT = 8;
#define MENU_ACCENT UI_CYAN
#define MENU_ACCENT_DIM UI_PURPLE
static_assert(MARGIN_LEFT * 2 + TILE_W * COLS + 4 * (COLS - 1) == PORTRAIT_W,
              "Launcher columns must fit the display");
static_assert(START_Y >= 58 + 10, "First row must clear the title panel");
static_assert(START_Y + (VISIBLE_ROWS - 1) * ROW_PITCH + TILE_H <= 294,
              "Third row must clear the page dock");
constexpr int ICON_TOP = 6, ICON_LEFT = 10, ICON_RADIUS = 13;
constexpr int LABEL_TOP = 59, LABEL_H = 16;
static_assert(ICON_LEFT + ICON_SIZE <= TILE_W && ICON_TOP + ICON_SIZE < LABEL_TOP,
              "Icon and label must not overlap");

int selectedApp = 0;
int homePage = 0;
TFT_eSprite launcherTile(&tft);

static uint16_t homeBlend(uint16_t a, uint16_t b, int amount) {
  int r = (((a >> 11) & 31) * (255 - amount) + ((b >> 11) & 31) * amount) / 255;
  int g = (((a >> 5) & 63) * (255 - amount) + ((b >> 5) & 63) * amount) / 255;
  int blue = ((a & 31) * (255 - amount) + (b & 31) * amount) / 255;
  return (r << 11) | (g << 5) | blue;
}

static uint16_t homeSwap(uint16_t color) { return (color >> 8) | (color << 8); }


static int roundRectOutside4(int x, int y, int left, int top, int w, int h, int r) {
  int cx = x < left + r ? left + r - x : x > left + w - 1 - r ? x - (left + w - 1 - r) : 0;
  int cy = y < top + r ? top + r - y : y > top + h - 1 - r ? y - (top + h - 1 - r) : 0;
  if (!cx && !cy) {
    int ox = x < left ? left - x : x >= left + w ? x - left - w + 1 : 0;
    int oy = y < top ? top - y : y >= top + h ? y - top - h + 1 : 0;
    return (ox > oy ? ox : oy) * 4;
  }
  int d = (int)(sqrtf((float)(cx * cx + cy * cy)) * 4.0f) - r * 4;
  return d > 0 ? d : 0;
}





static uint8_t *tileGlass = nullptr, *tileGlow = nullptr, *tileShadow = nullptr;

static bool prepareTileEffects() {
  if (tileGlass) return true;
  auto *mem = static_cast<uint8_t *>(malloc(TILE_W * TILE_H * 3));
  if (!mem) return false;
  tileGlass = mem; tileGlow = mem + TILE_W * TILE_H; tileShadow = mem + 2 * TILE_W * TILE_H;
  for (int ty = 0; ty < TILE_H; ++ty) {
    for (int tx = 0; tx < TILE_W; ++tx) {
      int i = ty * TILE_W + tx;
      int card = roundRectOutside4(tx, ty, 1, 1, TILE_W - 2, TILE_H - 2, 16);
      int inner = roundRectOutside4(tx, ty, 2, 2, TILE_W - 4, TILE_H - 4, 15);
      tileGlass[i] = card ? (card < 4 ? 40 : 0) : inner ? 120 : 52;
      int icon = roundRectOutside4(tx, ty, ICON_LEFT, ICON_TOP, ICON_SIZE, ICON_SIZE, ICON_RADIUS);
      tileGlow[i] = icon > 0 && icon <= 28 ? 210 - icon * 7 : 0;
      int sh = roundRectOutside4(tx, ty, ICON_LEFT + 1, ICON_TOP + 4, ICON_SIZE - 2, ICON_SIZE, ICON_RADIUS);
      tileShadow[i] = icon && sh <= 20 ? 120 - sh * 6 : 0;
    }
  }
  return true;
}

static inline uint16_t tilePixel(int index, int tx, int ty, uint16_t color, bool selected,
                                 uint8_t labelInk) {
  int i = ty * TILE_W + tx;
  if (selected) {
    if (tileGlass[i]) color = homeBlend(color, TFT_WHITE, tileGlass[i]);
  } else if (tileShadow[i]) {
    color = homeBlend(color, 0x0000, tileShadow[i]);
  }
  if (ty >= ICON_TOP && ty < ICON_TOP + ICON_SIZE && tx >= ICON_LEFT && tx < ICON_LEFT + ICON_SIZE) {
    uint16_t ink = Apps[index].img[(ty - ICON_TOP) * ICON_SIZE + tx - ICON_LEFT];
    if (ink != 0xF81F) color = ink;
  }
  if (labelInk) color = labelInk == 2 ? TFT_WHITE : 0x0842;
  return color;
}

void getTileCoordinates(int index, int &x, int &y) {
  int slot = index % APPS_PER_PAGE;
  x = MARGIN_LEFT + (slot % COLS) * COL_PITCH;
  y = START_Y + (slot / COLS) * ROW_PITCH;
}

static bool prepareLauncherTile() {
  if (!prepareTileEffects()) return false;
  if (launcherTile.created()) return true;
  launcherTile.setColorDepth(16);
  return launcherTile.createSprite(TILE_W, TILE_H) != nullptr;
}

static void drawLauncherLabel(const char *label) {
  launcherTile.setTextFont(2);
  launcherTile.setTextDatum(MC_DATUM);
  launcherTile.setTextColor(0x0842);
  launcherTile.drawString(label, TILE_W / 2 + 1, LABEL_TOP + 9);
  launcherTile.setTextColor(TFT_WHITE);
  launcherTile.drawString(label, TILE_W / 2, LABEL_TOP + 8);
}


static void buildLabelMask(int index, uint8_t *mask) {
  auto *tile = static_cast<uint16_t *>(launcherTile.getPointer());
  launcherTile.fillSprite(0xF81F);
  drawLauncherLabel(Apps[index].label);
  for (int y = 0; y < LABEL_H; ++y)
    for (int x = 0; x < TILE_W; ++x) {
      uint16_t ink = tile[(LABEL_TOP + y) * TILE_W + x];
      mask[y * TILE_W + x] = ink == homeSwap(TFT_WHITE) ? 2 : ink == homeSwap(0x0842) ? 1 : 0;
    }
}

void drawAppTile(int index, bool selected) {
  if (index < 0 || index >= APP_COUNT || index / APPS_PER_PAGE != homePage) return;
  int x, y;
  getTileCoordinates(index, x, y);
  Apps[index].x_coord = x;
  Apps[index].y_coord = y;
  if (!prepareLauncherTile()) return;
  uint8_t label[TILE_W * LABEL_H];
  buildLabelMask(index, label);
  auto *pixels = static_cast<uint16_t *>(launcherTile.getPointer());
  for (int ty = 0; ty < TILE_H; ++ty) {
    for (int tx = 0; tx < TILE_W; ++tx) {
      uint8_t ink = (ty >= LABEL_TOP && ty < LABEL_TOP + LABEL_H)
                      ? label[(ty - LABEL_TOP) * TILE_W + tx] : 0;
      uint16_t bg = home_wallpaper[(y + ty) * PORTRAIT_W + x + tx];
      pixels[ty * TILE_W + tx] = homeSwap(tilePixel(index, tx, ty, bg, selected, ink));
    }
  }
  launcherTile.pushSprite(x, y);
}


void drawPageDots() {
  constexpr int WIDTH = 64, HEIGHT = 18, LEFT = (PORTRAIT_W - WIDTH) / 2, TOP = 296;
  uint16_t pixels[WIDTH * HEIGHT];
  int totalW = (PAGE_COUNT - 1) * 12 + 18;
  int cursor = (WIDTH - totalW) / 2;
  for (int y = 0; y < HEIGHT; ++y) {
    for (int x = 0; x < WIDTH; ++x) {
      uint16_t color = home_wallpaper[(TOP + y) * PORTRAIT_W + LEFT + x];
      if (!roundRectOutside4(x, y, 0, 0, WIDTH, HEIGHT, 9))
        color = homeBlend(homeBlend(color, 0x0000, 90), TFT_WHITE, 28);
      int c = cursor;
      for (int p = 0; p < PAGE_COUNT; ++p) {
        int w = p == homePage ? 18 : 6;
        if (!roundRectOutside4(x, y, c, 6, w, 6, 3))
          color = p == homePage ? homeBlend(uiHeaderStart, uiHeaderEnd, (x - c) * 255 / w)
                                : homeBlend(color, TFT_WHITE, 110);
        c += w + 6;
      }
      pixels[y * WIDTH + x] = color;
    }
  }
  tft.setSwapBytes(true);
  tft.pushImage(LEFT, TOP, WIDTH, HEIGHT, pixels);
}


void drawLauncherTitle() {
  constexpr int WIDTH = 224, HEIGHT = 48, LEFT = 8, TOP = 10;
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  auto *pixels = static_cast<uint16_t *>(header.createSprite(WIDTH, HEIGHT));
  if (!pixels) return;
  for (int y = 0; y < HEIGHT; ++y) {
    for (int x = 0; x < WIDTH; ++x) {
      uint16_t color = home_wallpaper[(TOP + y) * PORTRAIT_W + LEFT + x];
      int out = roundRectOutside4(x, y, 0, 0, WIDTH, HEIGHT, 16);
      if (!out) {
        bool rim = roundRectOutside4(x, y, 1, 1, WIDTH - 2, HEIGHT - 2, 15) != 0;
        color = homeBlend(homeBlend(color, 0x0000, 120), TFT_WHITE, rim ? 90 : 34);

        color = homeBlend(color, homeBlend(uiHeaderStart, uiHeaderEnd, x * 255 / WIDTH), 48);
      } else if (out < 4) {
        color = homeBlend(color, TFT_WHITE, 30);
      }
      pixels[y * WIDTH + x] = homeSwap(color);
    }
  }

  int playable = 0;
  for (int i = 0; i < APP_COUNT; ++i) if (Apps[i].gameIndex > 0) ++playable;
  for (int y = -17; y <= 17; ++y)
    for (int x = -17; x <= 17; ++x)
      if (x * x + y * y <= 17 * 17)
        header.drawPixel(196 + x, 24 + y,
                         homeBlend(uiHeaderStart, uiHeaderEnd, (x + y + 34) * 255 / 68));
  header.drawCircle(196, 24, 17, homeBlend(uiHeaderStart, TFT_WHITE, 140));
  char count[8];
  snprintf(count, sizeof(count), "%d", playable);
  header.setTextDatum(MC_DATUM);
  header.setTextFont(2);
  header.setTextColor(0x18C5);
  header.drawString(count, 196, 25);

  header.fillRoundRect(14, 12, 4, 24, 2, uiHeaderStart);
  header.setTextDatum(TL_DATUM);
  header.setTextFont(4);
  header.setTextColor(TFT_WHITE);
  header.drawString("Games", 26, 7);
  header.setTextFont(2);
  header.setTextColor(0xCE79);
  char sub[32];
  snprintf(sub, sizeof(sub), "%d apps, %d playable", APP_COUNT, playable);
  header.drawString(sub, 27, 29);
  header.pushSprite(LEFT, TOP);
}

void redrawVisibleTiles() {
  tft.setSwapBytes(true);
  tft.pushImage(0, 0, PORTRAIT_W, PORTRAIT_H, home_wallpaper);
  drawLauncherTitle();
  int start = homePage * APPS_PER_PAGE;
  int end = min(start + APPS_PER_PAGE, APP_COUNT);
  for (int i = start; i < end; ++i) drawAppTile(i, i == selectedApp);
  drawPageDots();
}




static void composeSlidingPage(uint16_t *strip, int stripY, int lines, int page,
                               int offset, int selected, const uint8_t *labels) {
  int end = min((page + 1) * APPS_PER_PAGE, APP_COUNT);
  for (int index = page * APPS_PER_PAGE; index < end; ++index) {
    int baseX, baseY;
    getTileCoordinates(index, baseX, baseY);
    baseX += offset;
    int x0 = max(0, baseX), x1 = min(PORTRAIT_W, baseX + TILE_W);
    int y0 = max(stripY, baseY), y1 = min(stripY + lines, baseY + TILE_H);
    if (x0 >= x1 || y0 >= y1) continue;
    for (int y = y0; y < y1; ++y) {
      int ty = y - baseY;
      for (int x = x0; x < x1; ++x) {
        int tx = x - baseX, pos = (y - stripY) * PORTRAIT_W + x;
        uint8_t ink = (ty >= LABEL_TOP && ty < LABEL_TOP + LABEL_H)
                        ? labels[index * TILE_W * LABEL_H + (ty - LABEL_TOP) * TILE_W + tx] : 0;
        strip[pos] = tilePixel(index, tx, ty, strip[pos], index == selected, ink);
      }
    }
  }
}

void slideLauncherPage(int oldPage, int newPage, int oldSelected) {
  constexpr int STRIP_H = 8;
  constexpr int VIEW_H = (VISIBLE_ROWS - 1) * ROW_PITCH + TILE_H;
  auto *labels = static_cast<uint8_t *>(malloc(APP_COUNT * TILE_W * LABEL_H));
  auto *strip = static_cast<uint16_t *>(malloc(PORTRAIT_W * STRIP_H * sizeof(uint16_t)));
  if (!labels || !strip || !prepareLauncherTile()) {
    free(labels); free(strip);
    redrawVisibleTiles();
    return;
  }
  for (int i = 0; i < APP_COUNT; ++i) buildLabelMask(i, labels + i * TILE_W * LABEL_H);
  int direction = newPage > oldPage ? 1 : -1;
  unsigned long began = millis();
  while (true) {
    unsigned long elapsed = millis() - began;


    int t = (buttonsPendingPress() || Serial.available()) ? 220 : min((int)elapsed, 220);

    int remaining = 220 - t;
    int shift = PORTRAIT_W - (int)((int64_t)PORTRAIT_W * remaining * remaining * remaining /
                                 (220L * 220L * 220L));
    int oldOffset = -direction * shift;
    int newOffset = direction * (PORTRAIT_W - shift);
    for (int y = START_Y; y < START_Y + VIEW_H; y += STRIP_H) {
      int lines = min(STRIP_H, START_Y + VIEW_H - y);
      memcpy(strip, home_wallpaper + y * PORTRAIT_W, PORTRAIT_W * lines * sizeof(uint16_t));
      composeSlidingPage(strip, y, lines, oldPage, oldOffset, oldSelected, labels);
      composeSlidingPage(strip, y, lines, newPage, newOffset, selectedApp, labels);
      tft.setSwapBytes(true);
      tft.pushImage(0, y, PORTRAIT_W, lines, strip);
    }
    yield();
    if (t == 220) break;
  }
  free(strip); free(labels);
  drawPageDots();
}


static void drawAboutInformation() {
  tft.fillScreen(UI_BG);
  ui::heading(20, 16, "Console", "About", UI_BG);
  static const char *const specs[][2] = {
    {"Processor", "ESP32-C3 RISC-V"},
    {"Clock", "160 MHz"},
    {"Display", "ILI9341 240x320"},
    {"Input", "ADS1115 16-bit"},
    {"Audio", "MAX98357A I2S"}
  };
  ui::List l = {16, 74, 208, 36, 5};
  for (int i = 0; i < 5; i++) ui::infoRow(l, i, specs[i][0], specs[i][1]);
  ui::hint(300, "Shoot to go back", UI_BG);
}

void showAboutScreen() {
  Serial.println("[Info] Open");
  drawAboutInformation();
  buttonsRequireRelease();
  while (true) {
    int c = findCommand(false);
    if (Serial.available()) {
      char key = Serial.read();
      if (key == 'w' || key == 'W') c = 0;
      else if (key == 'd' || key == 'D') c = 1;
      else if (key == 's' || key == 'S') c = 2;
      else if (key == 'a' || key == 'A') c = 3;
      else if (key == ' ' || key == 'e' || key == 'E' || key == '\r' || key == '\n') c = 4;
    }
    if (c == 4) break;
    delay(2);
  }
  menuSetup();
}

struct SongNote {
  uint16_t freq;
  uint16_t duration;
};

struct Track {
  const char* title;
  const char* artist;
  const SongNote* notes;
  int noteCount;
};


const SongNote rickNotes[] = {
  {294, 150}, {330, 150}, {392, 150}, {330, 150},
  {494, 280}, {440, 380}, {0, 140},
  {294, 150}, {330, 150}, {392, 150}, {330, 150},
  {440, 280}, {392, 380}, {0, 140},
  {294, 150}, {330, 150}, {392, 150}, {330, 150},
  {392, 220}, {440, 200}, {370, 280}, {330, 220}, {294, 380}
};


const SongNote astronomiaNotes[] = {
  {587, 110}, {587, 110}, {587, 110}, {587, 110},
  {392, 220}, {494, 220}, {440, 220}, {523, 220},
  {494, 220}, {392, 220}, {440, 220}, {587, 220},
  {494, 220}, {392, 220}, {440, 220}, {587, 220}
};


const SongNote blindingNotes[] = {
  {698, 170}, {698, 170}, {622, 170}, {698, 220},
  {784, 220}, {523, 330}, {0, 90},
  {698, 170}, {698, 170}, {622, 170}, {698, 220},
  {784, 220}, {523, 330}, {0, 90},
  {784, 170}, {784, 170}, {698, 170}, {622, 220}, {523, 380}
};


const SongNote badGuyNotes[] = {
  {392, 130}, {392, 130}, {392, 130}, {294, 130},
  {349, 130}, {392, 130}, {0, 130},
  {392, 130}, {392, 130}, {392, 130}, {294, 130},
  {349, 130}, {392, 130}, {0, 130},
  {466, 130}, {440, 130}, {392, 260}
};


const SongNote despacitoNotes[] = {

  {587, 300}, {554, 180}, {494, 200}, {370, 220}, {0, 100},

  {370, 140}, {370, 140}, {370, 140}, {370, 140},
  {494, 180}, {494, 180}, {494, 180}, {494, 240},
  {440, 160}, {494, 160}, {392, 360}, {0, 120},

  {392, 140}, {392, 140}, {392, 140}, {392, 140},
  {392, 180}, {494, 180}, {494, 180}, {494, 240},
  {440, 160}, {392, 160}, {440, 320}, {0, 120},

  {440, 140}, {440, 140}, {440, 140}, {440, 140},
  {440, 180}, {587, 180}, {587, 180}, {587, 240},
  {554, 160}, {494, 160}, {440, 280}, {370, 360}
};


const SongNote tetrisNotes[] = {
  {659, 220}, {494, 120}, {523, 120}, {587, 220}, {523, 120}, {494, 120},
  {440, 220}, {440, 120}, {523, 120}, {659, 220}, {587, 120}, {523, 120},
  {494, 280}, {523, 120}, {587, 220}, {659, 220},
  {523, 220}, {440, 220}, {440, 320}
};

const Track jukeboxPlaylist[] = {
  {"Never Gonna Give You Up", "Rick Astley", rickNotes, sizeof(rickNotes)/sizeof(rickNotes[0])},
  {"Astronomia (Coffin)", "Vicetone", astronomiaNotes, sizeof(astronomiaNotes)/sizeof(astronomiaNotes[0])},
  {"Blinding Lights", "The Weeknd", blindingNotes, sizeof(blindingNotes)/sizeof(blindingNotes[0])},
  {"Bad Guy", "Billie Eilish", badGuyNotes, sizeof(badGuyNotes)/sizeof(badGuyNotes[0])},
  {"Despacito", "Luis Fonsi", despacitoNotes, sizeof(despacitoNotes)/sizeof(despacitoNotes[0])},
  {"Tetris Korobeiniki", "Retro Classic", tetrisNotes, sizeof(tetrisNotes)/sizeof(tetrisNotes[0])}
};

const int JUKEBOX_COUNT = sizeof(jukeboxPlaylist)/sizeof(jukeboxPlaylist[0]);

void playSoundTestScreen() {
  int currentSong = 4;
  bool isPlaying = false;

  static const ui::List songList = {16, 74, 208, 30, JUKEBOX_COUNT};
  auto drawJukeboxUI = [&]() {
    ui::heading(20, 16, "Sound", "Jukebox", UI_BG);
    for (int i = 0; i < JUKEBOX_COUNT; i++)
      ui::valueRow(songList, i, jukeboxPlaylist[i].title, "", i == currentSong, MENU_ACCENT);
    tft.fillRect(0, 268, 240, 52, UI_BG);
    ui::hint(282, isPlaying ? "Shoot to stop" : "Shoot to play", UI_BG, ui::LABEL);
    ui::hint(300, "Up/Down choose, Left exits", UI_BG);
  };
  tft.fillScreen(UI_BG);

  drawJukeboxUI();
  delay(250);

  while (true) {
    int c = findCommand(false);
    if (Serial.available()) {
      char sc = Serial.read();
      if (sc == ' ' || sc == 'x' || sc == 'e') c = 4;
      else if (sc == 'w') c = 0;
      else if (sc == 's') c = 2;
      else if (sc == 'a' || sc == 'q') c = 3;
    }


    if (c == 3) {
      setBuzzerTone(0);
      playNavClick();
      break;
    }

    if (c == 0) {
      setBuzzerTone(0);
      isPlaying = false;
      currentSong = (currentSong - 1 + JUKEBOX_COUNT) % JUKEBOX_COUNT;
      playNavClick();
      drawJukeboxUI();
      delay(150);
    } else if (c == 2) {
      setBuzzerTone(0);
      isPlaying = false;
      currentSong = (currentSong + 1) % JUKEBOX_COUNT;
      playNavClick();
      drawJukeboxUI();
      delay(150);
    } else if (c == 4) {
      if (isPlaying) {
        setBuzzerTone(0);
        isPlaying = false;
        drawJukeboxUI();
        delay(200);
      } else {
        isPlaying = true;
        drawJukeboxUI();


        const Track& trk = jukeboxPlaylist[currentSong];
        for (int i = 0; i < trk.noteCount; i++) {

          int interruptCmd = findCommand(false);
          if (interruptCmd == 4 || interruptCmd == 0 || interruptCmd == 2 || interruptCmd == 3) {
            setBuzzerTone(0);
            isPlaying = false;
            break;
          }
          if (Serial.available()) {
            Serial.read();
            setBuzzerTone(0);
            isPlaying = false;
            break;
          }

          uint16_t freq = trk.notes[i].freq;
          uint16_t dur = trk.notes[i].duration;

          if (freq > 0) {
            setBuzzerTone(freq);
          } else {
            setBuzzerTone(0);
          }


          int barW = map(freq, 200, 900, 10, 180);
          barW = constrain(barW, 10, 180);
          tft.fillRect(30, 262, 180, 3, UI_BG);
          if (freq > 0) tft.fillRoundRect(30, 262, barW, 3, 1, MENU_ACCENT);

          delay(dur);
          setBuzzerTone(0);
          delay(20);
        }

        setBuzzerTone(0);
        isPlaying = false;
        drawJukeboxUI();
      }
    }
    delay(20);
  }

  setBuzzerTone(0);
  menuSetup();
}

static int readSettingsCommand() {
  int cmd = findCommand(false);
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'w' || c == 'W') cmd = 0;
    else if (c == 'd' || c == 'D') cmd = 1;
    else if (c == 's' || c == 'S') cmd = 2;
    else if (c == 'a' || c == 'A') cmd = 3;
    else if (c == ' ' || c == '\n' || c == '\r' || c == 'e' || c == 'E' || c == 'x' || c == 'X') cmd = 4;
  }
  return cmd;
}


static String titleCase(const char *name) {
  String s = name;
  s.toLowerCase();
  if (s.length()) s.setCharAt(0, toupper(s[0]));
  return s;
}

static const ui::List settingsList = {16, 58, 208, 21, 9};
void drawSettingsRow(int row, bool selected) {
  const char *labels[] = {"Theme", "Brightness", "Sound", "Effects volume", "Music volume", "Game music", "Reset scores", "Battery saver", "Show it"};
  String value = row == 0 ? titleCase(themeName(activeTheme)) :
                 row == 1 ? (batterySaver ? String("10% locked") : String(displayBrightness) + "%") :
                 row == 2 ? String(soundEnabled ? "On" : "Off") :
                 row == 3 ? String(audio::effectsVolume) + "%" :
                 row == 4 ? String(audio::musicVolume) + "%" :
                 row == 5 ? String(batterySaver ? "Off locked" : (audio::gameMusic ? "On" : "Off")) :
                 row == 6 ? String("Reset") : row == 7 ? String(batterySaver ? "On" : "Off") : String("Open");
  ui::valueRow(settingsList, row, labels[row], value.c_str(), selected, MENU_ACCENT,
               row == 6 ? UI_RED : ui::SECONDARY);
}

static void drawSettingsHeader() {
  ui::heading(20, 16, "Console", "Settings", UI_BG);
}

static void drawSettingsPalette() {

  tft.fillRect(16, 250, 208, 24, UI_BG);
  const uint16_t swatches[] = {0x7DFF, 0x4DDF, 0xCB5F, 0xFB75,
                               0xCFE8, 0xFD8B, 0x67D4, 0xC638};
  for (int i = 0; i < THEME_COUNT; ++i) {
    int x = 36 + i * 24;
    tft.fillCircle(x, 261, 6, swatches[i]);
    if (i == activeTheme) tft.drawCircle(x, 261, 9, ui::LABEL);
  }
}

static void drawSettingsFooter() {
  tft.fillRect(0, 277, 240, 43, UI_BG);
  ui::hint(286, "Left / Right changes value", UI_BG, ui::LABEL);
  ui::hint(306, "Shoot saves and exits", UI_BG);
}

static void refreshSettingsTheme(int selectedRow) {



  drawSettingsHeader();
  for (int row=0; row<9; ++row) drawSettingsRow(row, selectedRow == row);
  drawSettingsPalette();
  drawSettingsFooter();
}

void drawSettingsScreen(int selectedRow) {
  tft.fillScreen(UI_BG);
  refreshSettingsTheme(selectedRow);
}


static void drawResetScoresConfirmation(int choice) {
  tft.fillScreen(UI_BG);
  ui::card(26, 92, 188, 136);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString("Reset all scores?", 120, 116, 2);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString("Every game's best scores", 120, 140, 2);
  tft.drawString("will be cleared.", 120, 157, 2);
  const char *labels[] = {"Cancel", "Reset"};
  for (int i = 0; i < 2; ++i) {
    uint16_t fill = i == choice ? (i == 1 ? UI_RED : MENU_ACCENT) : ui::GROUP;
    ui::pill(40 + i * 84, 184, 76, 30, labels[i], fill);
  }
}

static bool confirmResetScores() {
  int choice = 0;
  drawResetScoresConfirmation(choice);
  buttonsRequireRelease();
  while (true) {
    int cmd = readSettingsCommand();
    if (cmd >= 0 && cmd <= 3) {
      choice = 1 - choice;
      playNavClick();
      drawResetScoresConfirmation(choice);
    } else if (cmd == 4) {
      playSelectTone();
      buttonsRequireRelease();
      return choice == 1;
    }
    delay(12);
  }
}

void showSettingsScreen() {
  int selectedRow = 0;
  drawSettingsScreen(selectedRow);
  delay(220);

  while (true) {
    int cmd = readSettingsCommand();

    if (cmd == 0 || cmd == 2) {
      int previousRow = selectedRow;
      selectedRow = (selectedRow + (cmd == 0 ? 8 : 1)) % 9;
      playNavClick();
      drawSettingsRow(previousRow, false);
      drawSettingsRow(selectedRow, true);
    } else if (cmd == 1 || cmd == 3) {
      if (selectedRow == 0) {
        int direction = (cmd == 1) ? 1 : -1;
        applyThemePreset((activeTheme + direction + THEME_COUNT) % THEME_COUNT, false);
        playNavClick();
        refreshSettingsTheme(selectedRow);
      } else if (selectedRow == 7) {
        setBatterySaver(!batterySaver);
        drawSettingsScreen(selectedRow);
      } else if (selectedRow == 1 && !batterySaver) {
        setDisplayBrightness(constrain(int(displayBrightness) + (cmd == 1 ? 10 : -10), 10, 100), false);
        drawSettingsRow(selectedRow, true);
      } else if (selectedRow == 2) {
        setSoundEnabled(!soundEnabled, false);
        playNavClick();
        drawSettingsRow(selectedRow, true);
      } else if (selectedRow == 3 || selectedRow == 4) {
        uint8_t &volume = selectedRow == 3 ? audio::effectsVolume : audio::musicVolume;
        volume = constrain(int(volume) + (cmd == 1 ? 10 : -10), 0, 100);
        audio::play(audio::Cue::Move);
        drawSettingsRow(selectedRow, true);
      } else if (selectedRow == 5 && !batterySaver) {
        audio::gameMusic = !audio::gameMusic;
        playNavClick();
        drawSettingsRow(selectedRow, true);
      }
    } else if (cmd == 4) {
      if (selectedRow == 6) {
        if (confirmResetScores()) {
          resetAllGameScores();
          audio::play(audio::Cue::Saved);
          tft.fillScreen(UI_BG);
          ui::hint(160, "Scores reset", UI_BG, UI_GREEN);
          delay(650);
        }
        drawSettingsScreen(selectedRow);
      } else if (selectedRow == 8) {
        showBirthdayScene();
        menuSetup();
        return;
      } else {
        applyThemePreset(activeTheme, true);
        setDisplayBrightness(displayBrightness, true);
        setSoundEnabled(soundEnabled, true);
        audio::save();
        audio::play(audio::Cue::Saved);
        menuSetup();
        return;
      }
    }
    delay(16);
  }
}

void showNotice(const char* name) {
  ui::card(24, 100, 192, 120);
  ui::heading(40, 114, "Coming soon", name);
  ui::hint(196, "Press any button", ui::CARD);
  delay(300);
  while (true) {
    int c = findCommand(false);
    if (Serial.available()) {
      Serial.read();
      break;
    }
    if (c == 4 || c == 0 || c == 1 || c == 2 || c == 3) break;
    delay(20);
  }
  menuSetup();
}

void menuSetup() {
  audio::scene(0);
  Serial.println("[Menu] Opening phone launcher (9 apps per page)...");
  tft.setRotation(2);
  buttonsRequireRelease();
  selectedApp = 0;
  homePage = 0;
  redrawVisibleTiles();
}

void menuLoop() {

  int cmd = findCommand(false);
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'i' || c == 'I') buttonsPrintDiagnostics();
    if (c == 'r' || c == 'R') buttonsRecordFor(180000);
    if (c == 'w' || c == 'W') cmd = 0;
    else if (c == 'd' || c == 'D') cmd = 1;
    else if (c == 's' || c == 'S') cmd = 2;
    else if (c == 'a' || c == 'A') cmd = 3;
    else if (c == ' ' || c == 'e' || c == 'E' || c == 'x' || c == 'X') cmd = 4;
  }

  if (cmd == -1) return;

  int newSelected = selectedApp;
  const int currentPage = selectedApp / APPS_PER_PAGE;
  const int pageStart = currentPage * APPS_PER_PAGE;
  const int pageEnd = min(pageStart + APPS_PER_PAGE, APP_COUNT);
  const int pageSlot = selectedApp - pageStart;
  const int pageRow = pageSlot / COLS;
  const int pageColumn = pageSlot % COLS;

  switch (cmd) {
    case 0:
      if (pageRow > 0) newSelected = selectedApp - COLS;
      break;

    case 2:
      if (pageRow + 1 < VISIBLE_ROWS && selectedApp + COLS < pageEnd)
        newSelected = selectedApp + COLS;
      break;

    case 1:
      if (pageColumn + 1 < COLS && selectedApp + 1 < pageEnd) {
        newSelected = selectedApp + 1;
      } else if (currentPage + 1 < PAGE_COUNT) {


        int nextPageStart = (currentPage + 1) * APPS_PER_PAGE;
        newSelected = min(nextPageStart + pageRow * COLS, APP_COUNT - 1);
      }
      break;

    case 3:
      if (pageColumn > 0) {
        newSelected = selectedApp - 1;
      } else if (currentPage > 0) {

        int previousPageStart = (currentPage - 1) * APPS_PER_PAGE;
        int previousPageEnd = min(previousPageStart + APPS_PER_PAGE, APP_COUNT);
        newSelected = min(previousPageStart + pageRow * COLS + COLS - 1,
                          previousPageEnd - 1);
      }
      break;

    case 4:
      {
        launcherTile.deleteSprite();
        free(tileGlass);
        tileGlass = tileGlow = tileShadow = nullptr;
        playSelectTone();
        int target = Apps[selectedApp].gameIndex;
        if (target > 0) {
          Serial.printf("[Menu] Launching Game %d: %s\n", target, Apps[selectedApp].label);
          switchGame(target);
          return;
        } else if ((strcasecmp(Apps[selectedApp].label, "info") == 0 || strcasecmp(Apps[selectedApp].label, "about") == 0)) {
          showAboutScreen();
          return;
        } else if (strcasecmp(Apps[selectedApp].label, "music") == 0) {
          audioReleaseForMusic();
          runMusicPlayer();
          audioReclaimFromMusic();
          tft.setSwapBytes(true);
          buttonsRequireRelease();
          menuSetup();
          return;
        } else if (strcasecmp(Apps[selectedApp].label, "videos") == 0) {
          audioReleaseForMusic();
          runVideoPlayer();
          audioReclaimFromMusic();
          tft.setSwapBytes(true);
          buttonsRequireRelease();
          menuSetup();
          return;
        } else if (strcasecmp(Apps[selectedApp].label, "settings") == 0) {
          showSettingsScreen();
          return;
        } else {
          showNotice(Apps[selectedApp].label);
          return;
        }
      }
      break;
  }

  if (newSelected != selectedApp) {
    unsigned long renderStarted = millis();
    playNavClick();
    int oldSelected = selectedApp;
    selectedApp = newSelected;
    int nextPage = selectedApp / APPS_PER_PAGE;
    if (nextPage != homePage) {
      int oldPage = homePage;
      homePage = nextPage;
      slideLauncherPage(oldPage, nextPage, oldSelected);
    } else {
      drawAppTile(oldSelected, false);
      drawAppTile(selectedApp, true);
    }
    if (Serial.availableForWrite() >= 96)
      Serial.printf("[Menu] focus=%d command=%d render=%lums\n", selectedApp,
                    cmd, (unsigned long)(millis() - renderStarted));
  }
}
# 1 "/home/pahasara/gameConsole/src/mini_games.ino"
#include "game.h"
#include <Arduino.h>


#include "ui_kit.h"
#include "arcade_style.h"

namespace mini {
enum Phase { INTRO, PLAY, PAUSE, FINISH };
static Phase phase = INTRO;
static int pauseChoice = 0;
static const char *title = "";
static const char *const PAUSE_ITEMS[] = {"Resume", "New game", "High scores", "Main menu"};
static unsigned long pressedAt = 0;
static bool longSent = false;
static bool chordSent = false;

static void resetInput() {
  pressedAt = 0;
  longSent = false;
  chordSent = false;
}


static int input() {
  int command = findCommand();
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'w' || c == 'W') return 0;
    if (c == 'd' || c == 'D') return 1;
    if (c == 's' || c == 'S') return 2;
    if (c == 'a' || c == 'A') return 3;
    if (c == 'q' || c == 'Q' || c == 'f' || c == 'F') return 5;
    if (c == ' ' || c == 'e' || c == 'E' || c == '\n') return 4;
  }
  bool down = (buttonsHeld() & 16) != 0;


  if (command == 4 && pressedAt == 0) {
    pressedAt = millis() ? millis() : 1;
    longSent = chordSent = false;
  }
  if (down && command == 0 && !chordSent) {
    chordSent = longSent = true;
    if (!pressedAt) pressedAt = millis() ? millis() : 1;
    return 0;
  }
  if (down) {
    if (pressedAt && !longSent && millis() - pressedAt >= 850) {
      longSent = true;
      return 5;
    }
    return -1;
  }
  if (pressedAt) {
    bool shortPress = !longSent;
    resetInput();
    return shortPress ? 4 : -1;
  }
  return command == 4 ? -1 : command;
}

static void header(const char *name, const char *detail) {
  title = name;
  if (!restoringCountdownDigit()) tft.fillScreen(UI_BG);
  tft.fillRoundRect(10, 10, 220, 44, 12, ui::CARD);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString(name, 22, 33, 4);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString(detail, 218, 33, 2);
}

static void footer(const char *top, const char *bottom) {
  tft.fillRect(0, 277, 240, 36, UI_BG);
  ui::hint(286, top, UI_BG, ui::LABEL);
  ui::hint(303, bottom, UI_BG);
}

static void intro(const char *name, const char *line1, const char *line2,
                  const char *line3) {
  header(name, "New game");
  ui::card(18, 66, 204, 206);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString(name, 32, 80, 4);
  tft.drawString(line1, 32, 118, 2);
  tft.drawString(line2, 32, 136, 2);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString(line3, 32, 164, 2);
  ui::pill(32, 226, 176, 30, "Press Shoot to start", UI_CYAN);
  tft.fillRect(0, 277, 240, 36, UI_BG);
  ui::hint(294, "Up for home screen", UI_BG);
}

static void pause(const char *) {
  ui::pauseSheet(title, PAUSE_ITEMS, 4, pauseChoice, UI_CYAN);
}

static void result(const char *outcome, const char *detail) {
  ui::card(24, 66, 192, 186);
  ui::heading(40, 80, title, outcome);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString(detail, 40, 134, 2);
  ui::pill(40, 188, 160, 30, "Play again", UI_CYAN);
  ui::hint(234, "Up for home screen", ui::CARD);
}

static bool commonIntro(int cmd, void (*start)()) {
  if (phase != INTRO) return false;
  if (cmd == 4) { playSelectTone(); start(); }
  else if (cmd == 0) switchGame(0);
  return true;
}

static bool commonPause(int cmd, void (*draw)(), void (*start)()) {
  if (phase != PAUSE) return false;
  if (cmd == 0 || cmd == 2) {
    pauseChoice = (pauseChoice + (cmd == 2 ? 1 : 3)) % 4;
    playNavClick();
    pause("PAUSED");
  } else if (cmd == 4) {
    playSelectTone();
    if (pauseChoice == 0) { showResumeCountdown(draw); resetInput(); phase = PLAY; audio::play(audio::Cue::Resume); }
    else if (pauseChoice == 1) start();
    else if (pauseChoice == 2) {
      showGameHighScores();
      draw();
      pause("PAUSED");
    } else switchGame(0);
  }
  return true;
}

static void enterPause() {
  audio::play(audio::Cue::Pause);
  buttonsRequireRelease();
  pauseChoice = 0;
  audio::pauseMusic(true);
  phase = PAUSE;
  pause("PAUSED");
}
}


namespace tic {
static uint8_t cells[9];
static int cursor = 4;
static int outcome = 0;
static bool recorded = false;
static bool cpuPending = false;
static unsigned long cpuAt = 0;

static int winner() {
  const uint8_t lines[8][3] = {
    {0,1,2},{3,4,5},{6,7,8},{0,3,6},
    {1,4,7},{2,5,8},{0,4,8},{2,4,6}
  };
  for (const auto &line : lines) {
    uint8_t mark = cells[line[0]];
    if (mark && mark == cells[line[1]] && mark == cells[line[2]]) return mark;
  }
  for (uint8_t cell : cells) if (!cell) return 0;
  return 3;
}

static int solve(int turn, int depth) {
  int result = winner();
  if (result == 2) return 10 - depth;
  if (result == 1) return depth - 10;
  if (result == 3) return 0;
  int best = turn == 2 ? -100 : 100;
  for (int i = 0; i < 9; ++i) {
    if (cells[i]) continue;
    cells[i] = turn;
    int value = solve(turn == 2 ? 1 : 2, depth + 1);
    cells[i] = 0;
    if (turn == 2) best = max(best, value);
    else best = min(best, value);
  }
  return best;
}

template<class Surface>
static void paintTile(Surface &canvas, int i, int x, int y) {
  constexpr int size = 60;
  const uint16_t colors[3] = {0xF98B, 0xFD24, 0xFE64};
  uint16_t color = colors[i / 3];
  canvas.fillRoundRect(x, y, size, size, 9, color);
  canvas.drawRoundRect(x, y, size, size, 9, arcadeStyle::fade(color, 135));
  canvas.drawFastHLine(x + 7, y + 2, size - 14, arcadeStyle::mix(color, 0xFFFF, 125));
  canvas.drawFastHLine(x + 9, y + 4, 10, TFT_WHITE);
  canvas.drawFastHLine(x + 7, y + size - 3, size - 14, arcadeStyle::fade(color, 135));
  uint16_t ink = ui::inkOn(color);
  if (cells[i] == 1) {
    for (int offset = -2; offset <= 2; ++offset) {
      canvas.drawLine(x + 17 + offset, y + 18, x + 43 + offset, y + 44, ink);
      canvas.drawLine(x + 43 + offset, y + 18, x + 17 + offset, y + 44, ink);
    }
  } else if (cells[i] == 2) {
    for (int radius = 14; radius <= 18; ++radius)
      canvas.drawCircle(x + 30, y + 31, radius, ink);
  }
  if (i == cursor) {
    canvas.drawRoundRect(x, y, size, size, 9, 0x36FF);
    canvas.drawRoundRect(x + 1, y + 1, size - 2, size - 2, 8, 0x36FF);
    canvas.drawPixel(x + size - 7, y + 5, TFT_WHITE);
    canvas.drawPixel(x + size - 5, y + 7, TFT_WHITE);
  }
}

static void drawTile(int i) {
  const int size = 60, gap = 3;
  int x = 27 + (i % 3) * (size + gap);
  int y = 75 + (i / 3) * (size + gap);
  TFT_eSprite tile(&tft);
  tile.setColorDepth(16);
  if (tile.createSprite(size, size)) {
    tile.setOrigin(-x, -y);
    arcadeStyle::drawBackdrop(tile, y, y + size - 1);
    tic::paintTile(tile, i, x, y);
    tile.pushSprite(x, y);
    tile.deleteSprite();
  } else {
    tic::paintTile(tft, i, x, y);
  }
}

static void draw() {
  mini::header("Tic-tac-toe", "You are X");
  uint16_t row[240];
  bool swap = tft.getSwapBytes();
  tft.setSwapBytes(true);
  for (int y = 58; y <= 276; ++y) {
    for (int x = 0; x < 240; ++x) row[x] = arcadeStyle::backdrop(x, y);
    tft.pushImage(0, y, 240, 1, row);
  }
  tft.setSwapBytes(swap);
  for (int i = 0; i < 9; ++i) drawTile(i);
  mini::footer(cpuPending ? "Console is thinking..." : "Shoot to place",
               "Hold Shoot to pause");
}

static void start() {
  audio::restartMusic();
  audio::stopEnding();
  memset(cells, 0, sizeof(cells));
  cursor = 4;
  recorded = false;
  cpuPending = false;
  mini::phase = mini::PLAY;
  draw();
}

static void finish() {
  audio::pauseMusic(true);
  outcome = winner();
  if (!outcome) return;
  audio::play(outcome == 1 ? audio::Cue::Win : outcome == 2 ? audio::Cue::Lose : audio::Cue::Draw);
  mini::phase = mini::FINISH;
  if (outcome == 1 && !recorded) {
    currentGame.recordScore(100);
    recorded = true;
  }
  mini::result(outcome == 1 ? "You win" : outcome == 2 ? "Console wins" : "Draw",
               outcome == 1 ? "Three in a row." : outcome == 2 ? "Try another round." : "Nobody got three.");
}

static int computerTurn() {
  int best = -100, choice = -1;
  for (int i = 0; i < 9; ++i) {
    if (cells[i]) continue;
    cells[i] = 2;
    int value = solve(1, 1);
    cells[i] = 0;
    if (value > best || choice < 0) { best = value; choice = i; }
  }
  if (choice >= 0) cells[choice] = 2;
  return choice;
}
}

void ticTacToeSetup() {
  tft.setRotation(2);
  mini::phase = mini::INTRO;
  mini::resetInput();
  mini::intro("Tic-tac-toe", "Get three in a row.",
              "The console plays O.", "D-pad moves, Shoot places");
}

void ticTacToeLoop() {
  audio::pauseMusic(mini::phase != mini::PLAY);
  int cmd = mini::input();
  if (mini::commonIntro(cmd, tic::start)) return;
  if (mini::phase == mini::PAUSE) {
    mini::commonPause(cmd, tic::draw, tic::start);
    if (mini::phase == mini::PLAY && tic::cpuPending) tic::cpuAt = millis();
    return;
  }
  if (mini::phase == mini::FINISH) {
    if (cmd == 4) tic::start();
    else if (cmd == 0) switchGame(0);
    return;
  }
  if (cmd == 5) { mini::enterPause(); return; }
  if (tic::cpuPending) {
    if (millis() - tic::cpuAt >= 1000) {
      tic::cpuPending = false;
      int reply = tic::computerTurn();
      if (reply >= 0) { tic::drawTile(reply); audio::play(audio::Cue::CpuMove); }
      mini::footer("Shoot to place", "Hold Shoot to pause");
      tic::finish();
    }
    return;
  }
  if (cmd >= 0 && cmd <= 3) {
    int row = tic::cursor / 3, col = tic::cursor % 3;
    if (cmd == 0 && row > 0) --row;
    if (cmd == 2 && row < 2) ++row;
    if (cmd == 3 && col > 0) --col;
    if (cmd == 1 && col < 2) ++col;
    int next = row * 3 + col;
    if (next != tic::cursor) {
      int previous = tic::cursor;
      tic::cursor = next;
      playNavClick();
      tic::drawTile(previous);
      tic::drawTile(next);
    }
  } else if (cmd == 4 && !tic::cells[tic::cursor]) {
    tic::cells[tic::cursor] = 1;
    audio::play(audio::Cue::PlayerMove);
    tic::drawTile(tic::cursor);
    tic::finish();
    if (mini::phase == mini::PLAY) {
      tic::cpuPending = true;
      tic::cpuAt = millis();
      mini::footer("Console is thinking...", "Hold Shoot to pause");
    }
  }
}


namespace pong {
static float ballX, ballY, vx, vy;
static int paddleX, lives, score;
static int drawnX, drawnY, drawnPaddle, drawnScore, drawnLives;
static unsigned long lastFrame;
static unsigned long lastPaddleStep;
static int lastPaddleDir;
static bool shootWasDown;
static bool recorded;
static constexpr uint16_t ACCENT = 0xFD20;
static constexpr uint16_t ACCENT_LIGHT = 0xFFE0;
static constexpr int FIELD_TOP = 48;
static constexpr int FIELD_BOTTOM = 290;
static float trailX[8], trailY[8];
static int trailCount = 0, impactLife = 0, impactX = 0, impactY = 0;
static uint16_t dirtyBands = 0;

static void dirty(int y, int h) {
  int first = max(y, FIELD_TOP), last = min(y + h - 1, FIELD_BOTTOM);
  if (first > last) return;
  for (int i = (first - FIELD_TOP) / 16; i <= (last - FIELD_TOP) / 16; ++i)
    dirtyBands |= uint16_t(1) << i;
}

static void dirtyObjects() {
  dirty((int)ballY - 6, 13);
  for (int i = 0; i < trailCount; ++i) dirty((int)trailY[i] - 3, 7);
  if (impactLife) dirty(impactY - 20, 41);
}

static void impact(int x, int y) {
  impactX = x; impactY = y; impactLife = 7;
}

template<class Surface>
static void compose(Surface &canvas, int top, int bottom) {
  arcadeStyle::drawBackdrop(canvas, top, bottom);
  canvas.drawFastHLine(16, 107, 208, 0x36FF);
  if (impactLife) {
    int radius = 3 + (7 - impactLife) * 2;
    canvas.drawCircle(impactX, impactY, radius, arcadeStyle::fade(0x36FF, impactLife * 36));
    canvas.fillCircle(impactX - radius, impactY, 1, arcadeStyle::fade(ACCENT, impactLife * 36));
    canvas.fillCircle(impactX + radius, impactY, 1, arcadeStyle::fade(ACCENT, impactLife * 36));
  }
  for (int i = trailCount - 1; i >= 0; --i) {
    uint16_t c = arcadeStyle::mix(ACCENT, 0x36FF, i * 255 / 7);
    canvas.fillCircle((int)trailX[i], (int)trailY[i], i < 3 ? 2 : 1,
                      arcadeStyle::fade(c, 165 - i * 18));
  }
  canvas.fillRoundRect(paddleX, 249, 52, 7, 3, 0xDEFB);
  canvas.fillRoundRect(paddleX + 1, 250, 50, 5, 2, TFT_WHITE);
  canvas.fillRoundRect(paddleX + 1, 250, 6, 5, 2, ACCENT);
  canvas.fillRoundRect(paddleX + 45, 250, 6, 5, 2, ACCENT);
  canvas.drawFastHLine(paddleX + 9, 254, 34, 0x9CF3);
  canvas.fillCircle((int)ballX, (int)ballY, 5, arcadeStyle::fade(ACCENT, 75));
  canvas.fillCircle((int)ballX, (int)ballY, 3, 0xFFE8);
  canvas.drawPixel((int)ballX - 1, (int)ballY - 1, TFT_WHITE);
}

static void render(bool full = false) {
  if (full) dirtyBands = 0xFFFF;
  if (!dirtyBands) return;
  TFT_eSprite band(&tft);
  band.setColorDepth(16);
  if (!band.createSprite(240, 16)) {
    int x = tft.getViewportX(), y = tft.getViewportY();
    int w = tft.getViewportWidth(), h = tft.getViewportHeight();
    int top = max(y, FIELD_TOP), bottom = min(y + h - 1, FIELD_BOTTOM);
    if (top <= bottom) {
      tft.setViewport(x, top, w, bottom - top + 1, false);
      pong::compose(tft, FIELD_TOP, FIELD_BOTTOM);
    }
    tft.setViewport(x, y, w, h, false);
  } else {
    for (int i = 0; i < 16; ++i) {
      if (!(dirtyBands & (uint16_t(1) << i))) continue;
      int y = FIELD_TOP + i * 16;
      int h = min(16, FIELD_BOTTOM - y + 1);
      band.setViewport(0, 0, 240, h);
      band.setOrigin(0, -y);
      pong::compose(band, y, y + h - 1);
      band.pushSprite(0, y, 0, 0, 240, h);
    }
    band.deleteSprite();
  }
  dirtyBands = 0;
}

static unsigned long scoreFlashAt = 0;
static bool drawnScoreFlash = false;
static void drawHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(224, 38)) return;
  header.fillSprite(UI_BG);
  header.fillRoundRect(0, 0, 224, 38, 12, ui::CARD);
  header.setTextDatum(ML_DATUM);
  header.setTextColor(ui::LABEL);
  header.drawString("Pong", 12, 20, 2);
  header.setTextDatum(TR_DATUM);
  header.setTextColor(ACCENT);
  header.drawString("Score", 150, 3, 2);
  header.setTextColor(ui::SECONDARY);
  header.drawString("Lives", 212, 3, 2);
  header.setTextColor(scoreFlashAt && millis() - scoreFlashAt < 320 ? ACCENT_LIGHT : ui::LABEL);
  header.drawString(String(score), 150, 19, 2);
  for (int i = 0; i < 3; ++i)
    header.fillCircle(184 + i * 12, 27, 4, i < lives ? 0xF800 : 0x4208);
  header.pushSprite(8, 5);
  header.deleteSprite();
}

static void drawFooter() {
  tft.fillRect(0, 294, 240, 26, UI_BG);
  ui::hint(306, "Shoot to pause", UI_BG);
}

static void serve() {
  ballX = 120; ballY = 170;
  vx = random(2) ? 1.5f : -1.5f;
  vy = -2.5f;
  paddleX = 94;
  trailCount = 0; impactLife = 0;
  lastFrame = millis();
}

static void draw() {
  if (!restoringCountdownDigit()) tft.fillScreen(UI_BG);
  render(true);
  tft.drawFastHLine(0, FIELD_TOP - 1, 240, UI_SURFACE);
  tft.drawFastHLine(0, FIELD_BOTTOM + 1, 240, UI_SURFACE);
  drawHeader();
  drawFooter();
  drawnX = (int)ballX; drawnY = (int)ballY;
  drawnPaddle = paddleX; drawnScore = score; drawnLives = lives;
  drawnScoreFlash = scoreFlashAt && millis() - scoreFlashAt < 320;
}

static void drawMotion() {
  dirtyObjects();
  for (int i = min(trailCount, 7); i > 0; --i) {
    trailX[i] = trailX[i - 1]; trailY[i] = trailY[i - 1];
  }
  trailX[0] = drawnX; trailY[0] = drawnY;
  trailCount = min(8, trailCount + 1);
  if (impactLife) --impactLife;
  dirtyObjects();
  render();
  bool flash = scoreFlashAt && millis() - scoreFlashAt < 320;
  if (score != drawnScore || lives != drawnLives || flash != drawnScoreFlash) drawHeader();
  drawnScoreFlash = flash;
  drawnX = (int)ballX; drawnY = (int)ballY;
  drawnPaddle = paddleX; drawnScore = score; drawnLives = lives;
}

static void movePaddle(int direction) {
  if (!direction) { lastPaddleDir = 0; return; }
  unsigned long now = millis();
  if (direction == lastPaddleDir && now - lastPaddleStep < 14) return;
  lastPaddleDir = direction;
  lastPaddleStep = now;
  int next = constrain(paddleX + direction * 4, 19, 169);
  if (next == paddleX) return;
  dirty(245, 15);
  paddleX = next;
}

static void showIntro() {
  tft.fillScreen(UI_BG);
  drawHeader();
  static const ui::Control controls[] = {
    {"Left / Right", "Move"}, {"Shoot", "Pause"}, {"Lives", "3 misses"}};
  ui::startCard(52, "Pong", "Keep the ball in play", controls, 3,
                currentGame.highScores[0], ACCENT, nullptr, "Press Shoot to start");
}

static void drawPauseOption(int item) {
  ui::pauseItem(mini::PAUSE_ITEMS, 4, item, mini::pauseChoice, ACCENT);
}

static void drawPongPauseMenu() {
  ui::pauseSheet("Pong", mini::PAUSE_ITEMS, 4, mini::pauseChoice, ACCENT);
}

static void enterPause() {
  audio::play(audio::Cue::Pause);
  buttonsRequireRelease();
  mini::resetInput();
  mini::pauseChoice = 0;
  audio::pauseMusic(true);
  mini::phase = mini::PAUSE;
  drawPongPauseMenu();
}

static void start();

static bool handlePause(int cmd) {
  if (mini::phase != mini::PAUSE) return false;
  if (cmd == 0 || cmd == 2) {
    int old = mini::pauseChoice;
    mini::pauseChoice = (mini::pauseChoice + (cmd == 2 ? 1 : 3)) % 4;
    drawPauseOption(old);
    drawPauseOption(mini::pauseChoice);
    playNavClick();
  } else if (cmd == 4) {
    playSelectTone();
    if (mini::pauseChoice == 0) {
      showResumeCountdown(draw);
      audio::play(audio::Cue::Resume);
      mini::resetInput();
      mini::phase = mini::PLAY;
      shootWasDown = false;
      lastFrame = millis();
    } else if (mini::pauseChoice == 1) start();
    else if (mini::pauseChoice == 2) {
      showGameHighScores();
      draw();
      drawPongPauseMenu();
    } else switchGame(0);
  }
  return true;
}

static void drawGameOver() {
  ui::gameOverSheet("Pong", score, currentGame.highScores[0], ACCENT);
}

static void start() {
  audio::restartMusic();
  audio::stopEnding();
  lives = 3; score = 0; recorded = false;
  scoreFlashAt = 0; drawnScoreFlash = false;
  lastPaddleDir = 0;
  lastPaddleStep = 0;
  shootWasDown = false;
  serve();
  mini::phase = mini::PLAY;
  draw();
}

static void end() {
  audio::pauseMusic(true);
  audio::play(audio::Cue::GameOver);
  mini::phase = mini::FINISH;
  if (!recorded) { currentGame.recordScore(score); recorded = true; }
  drawGameOver();
}
}

void pongSetup() {
  tft.setRotation(2);
  mini::phase = mini::INTRO;
  mini::resetInput();
  pong::score = 0;
  pong::lives = 3;
  pong::showIntro();
}

void pongLoop() {
  audio::pauseMusic(mini::phase != mini::PLAY);

  int cmd = mini::phase == mini::PLAY ? -1 : mini::input();
  if (mini::commonIntro(cmd, pong::start)) return;
  if (pong::handlePause(cmd)) return;
  if (mini::phase == mini::FINISH) {
    if (cmd == 4) pong::start();
    else if (cmd == 0) switchGame(0);
    return;
  }
  bool shootDown = readShoot();
  if (shootDown) pong::shootWasDown = true;
  else if (pong::shootWasDown) {
    pong::shootWasDown = false;
    mini::resetInput();
    pong::enterPause();
    return;
  }
  if (Serial.available()) {
    char c = Serial.peek();
    if (c == 'q' || c == 'Q' || c == ' ' || c == 'e' || c == 'E') {
      Serial.read();
      pong::enterPause();
      return;
    }
  }
  int direction = getDirectHorizontalInput();
  pong::movePaddle(direction);
  unsigned long now = millis();
  if (now - pong::lastFrame < 28) return;
  float steps = min(3.0f, (now - pong::lastFrame) / 28.0f);
  pong::lastFrame = now;
  pong::dirtyObjects();
  pong::ballX += pong::vx * steps;
  pong::ballY += pong::vy * steps;
  if (pong::ballX <= 20 || pong::ballX >= 220) {
    pong::vx = -pong::vx;
    pong::ballX = constrain(pong::ballX, 20.0f, 220.0f);
    pong::impact((int)pong::ballX, (int)pong::ballY);
    audio::play(audio::Cue::Wall);
  }
  if (pong::ballY <= 112) {
    pong::ballY = 112;
    pong::vy = abs(pong::vy);
    ++pong::score;
    pong::scoreFlashAt = millis();
    pong::impact((int)pong::ballX, 107);
    audio::play(audio::Cue::Pickup);
  }
  if (pong::vy > 0 && pong::ballY >= 243 && pong::ballY <= 256 &&
      pong::ballX >= pong::paddleX - 4 && pong::ballX <= pong::paddleX + 56) {
    pong::ballY = 243;
    pong::vy = -min(5.0f, 2.5f + pong::score * 0.08f);
    pong::vx = constrain((pong::ballX - (pong::paddleX + 26)) / 10.0f, -3.5f, 3.5f);
    if (abs(pong::vx) < 0.6f) pong::vx = 0.6f;
    pong::impact((int)pong::ballX, 249);
    audio::play(audio::Cue::Paddle);
  }
  if (pong::ballY > 264) {
    if (--pong::lives <= 0) { pong::end(); return; }
    audio::play(audio::Cue::Lose);
    pong::serve();
  }
  pong::drawMotion();
}


namespace mines {
static constexpr int SIDE = 8;
static constexpr int COUNT = SIDE * SIDE;
static constexpr int BOMBS = 10;
static uint8_t counts[COUNT];
static bool bomb[COUNT], revealed[COUNT], flagged[COUNT];
static bool dirty[COUNT];
static int cursor, exposed, flags, result;
static bool planted, recorded;


static void neighbors(int index, void (*visit)(int)) {
  int row = index / SIDE, col = index % SIDE;
  for (int dr = -1; dr <= 1; ++dr)
    for (int dc = -1; dc <= 1; ++dc) {
      if (!dr && !dc) continue;
      int nr = row + dr, nc = col + dc;
      if (nr >= 0 && nr < SIDE && nc >= 0 && nc < SIDE) visit(nr * SIDE + nc);
    }
}

static void plant(int safe) {
  memset(bomb, 0, sizeof(bomb));
  memset(counts, 0, sizeof(counts));
  int placed = 0;
  while (placed < BOMBS) {
    int index = random(COUNT);
    if (index == safe || bomb[index]) continue;
    bomb[index] = true;
    ++placed;
  }
  for (int i = 0; i < COUNT; ++i)
    if (bomb[i]) neighbors(i, [](int n) { ++counts[n]; });
  planted = true;
}

static void uncover(int index) {
  uint8_t pending[COUNT];
  int head = 0, tail = 0;
  auto reveal = [&](int cell) {
    if (revealed[cell] || flagged[cell] || bomb[cell]) return;

    revealed[cell] = true;
    dirty[cell] = true;
    ++exposed;
    pending[tail++] = cell;
  };
  reveal(index);
  while (head < tail) {
    int cell = pending[head++];
    if (counts[cell]) continue;
    int row = cell / SIDE, col = cell % SIDE;
    for (int dr = -1; dr <= 1; ++dr)
      for (int dc = -1; dc <= 1; ++dc) {
        int nr = row + dr, nc = col + dc;
        if (nr >= 0 && nr < SIDE && nc >= 0 && nc < SIDE)
          reveal(nr * SIDE + nc);
      }
  }
}

static void drawTile(int i) {
  const int tile = 25, gap = 1, left = 16, top = 61;
  int x = left + (i % SIDE) * (tile + gap);
  int y = top + (i / SIDE) * (tile + gap);
  const uint16_t fill = revealed[i] ? 0xEF7D : TFT_WHITE;

  tft.fillRect(x, y, tile + 1, tile + 1, UI_BG);
  tft.fillRoundRect(x + 1, y + 1, tile, tile, 4, 0x6B4D);
  tft.fillRoundRect(x, y, tile, tile, 4, fill);
  tft.drawRoundRect(x, y, tile, tile, 4, revealed[i] ? 0xADB5 : 0xD6BA);
  if (!revealed[i]) {
    tft.drawFastHLine(x + 5, y + tile - 3, tile - 10, 0xDEFB);
    tft.drawFastVLine(x + tile - 3, y + 5, tile - 10, 0xDEFB);
  }
  if (i == cursor)
    for (int inset = 0; inset < 3; ++inset)
      tft.drawRoundRect(x + inset, y + inset, tile - inset * 2,
                        tile - inset * 2, 4 - inset, TFT_MAGENTA);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  if (flagged[i] && !revealed[i]) {
    tft.drawFastVLine(x + 8, y + 6, 14, TFT_BLACK);
    tft.drawFastHLine(x + 5, y + 19, 11, TFT_BLACK);
    tft.fillTriangle(x + 9, y + 6, x + 9, y + 14, x + 18, y + 10, 0xF800);
  } else if (revealed[i] && bomb[i]) {
    tft.fillCircle(x + 12, y + 13, 5, 0x18C3);
    tft.drawLine(x + 15, y + 9, x + 18, y + 5, 0x18C3);
    tft.fillCircle(x + 18, y + 5, 1, 0xF800);
  } else if (revealed[i] && counts[i]) {
    const uint16_t numberColors[] = {0x0000, 0x023F, 0x0440, 0xF800,
                                     0x3012, 0x9800, 0x03EF, 0x18C3, 0x6B4D};
    tft.setTextColor(numberColors[counts[i]], fill);
    tft.drawString(String(counts[i]), x + 12, y + 12);
  } else if (revealed[i]) {

    tft.fillCircle(x + 12, y + 12, 2, 0x94B2);
  }
}

static void drawDirty() {
  for (int i = 0; i < COUNT; ++i)
    if (dirty[i]) { drawTile(i); dirty[i] = false; }
}

static void draw() {
  mini::header("Mines", "10 mines");
  for (int i = 0; i < COUNT; ++i) drawTile(i);
  mini::footer("Shoot reveals, hold to flag", "Shoot + Up to pause");
}

static void start() {
  audio::restartMusic();
  audio::stopEnding();
  memset(revealed, 0, sizeof(revealed));
  memset(flagged, 0, sizeof(flagged));
  memset(bomb, 0, sizeof(bomb));
  memset(counts, 0, sizeof(counts));
  memset(dirty, 0, sizeof(dirty));
  cursor = 27; exposed = 0; flags = 0; result = 0;
  planted = false; recorded = false;
  mini::phase = mini::PLAY;
  draw();
}

static void finish(bool won) {
  audio::pauseMusic(true);
  audio::play(won ? audio::Cue::Win : audio::Cue::Lose);
  result = won ? 1 : -1;
  mini::phase = mini::FINISH;
  if (won && !recorded) { currentGame.recordScore(max(1, 1000 - flags * 10)); recorded = true; }
  mini::result(won ? "Cleared" : "Boom", won ? "Every safe tile found." : "You hit a mine.");
}
}

void minesweeperSetup() {
  tft.setRotation(2);
  mini::phase = mini::INTRO;
  mini::resetInput();
  mini::intro("Mines", "Reveal every safe tile.",
              "Numbers count nearby mines.", "Your first tap is always safe");
}

void minesweeperLoop() {
  audio::pauseMusic(mini::phase != mini::PLAY);
  int cmd = mini::input();
  if (mini::commonIntro(cmd, mines::start)) return;
  if (mini::commonPause(cmd, mines::draw, mines::start)) return;
  if (mini::phase == mini::FINISH) {
    if (cmd == 4) mines::start();
    else if (cmd == 0) switchGame(0);
    return;
  }

  if (readShoot() && cmd == 0) { mini::enterPause(); return; }
  if (cmd >= 0 && cmd <= 3) {
    int row = mines::cursor / mines::SIDE, col = mines::cursor % mines::SIDE;
    if (cmd == 0 && row > 0) --row;
    if (cmd == 2 && row < mines::SIDE - 1) ++row;
    if (cmd == 3 && col > 0) --col;
    if (cmd == 1 && col < mines::SIDE - 1) ++col;
    int next = row * mines::SIDE + col;
    if (next != mines::cursor) {
      int previous = mines::cursor;
      mines::cursor = next;
      playNavClick();
      mines::drawTile(previous);
      mines::drawTile(next);
    }
  } else if (cmd == 5 && !mines::revealed[mines::cursor]) {
    bool &flag = mines::flagged[mines::cursor];
    flag = !flag;
    mines::flags += flag ? 1 : -1;
    audio::play(flag ? audio::Cue::Flag : audio::Cue::Unflag);
    mines::drawTile(mines::cursor);
  } else if (cmd == 4 && !mines::flagged[mines::cursor] && !mines::revealed[mines::cursor]) {
    if (!mines::planted) mines::plant(mines::cursor);
    if (mines::bomb[mines::cursor]) {
      mines::revealed[mines::cursor] = true;
      mines::drawTile(mines::cursor);
      mines::finish(false);
    } else {
      mines::uncover(mines::cursor);
      audio::play(audio::Cue::Reveal);
      mines::drawDirty();
      if (mines::exposed == mines::COUNT - mines::BOMBS) mines::finish(true);
    }
  }
}
# 1 "/home/pahasara/gameConsole/src/run.ino"
#include "game.h"
#include "ui_kit.h"
#include <Arduino.h>
#include "run_background.h"
#include "run_clouds.h"
#include "vampire_runner_sprites.h"
#include "run_rules.h"

static void decodeRunnerFrame(const RunnerSprite &sprite, int frame, uint8_t *pixels);
static void dirtyRunnerRect(int x, int y, int w, int h);
static void renderRunnerDirty();
static void prepareRunMilestone();
# 24 "/home/pahasara/gameConsole/src/run.ino"
constexpr int RUN_W = 240;
constexpr int RUN_H = 320;
constexpr int RUN_PLAYFIELD_TOP = 48;
constexpr int RUN_GROUND_Y = 262;


constexpr int RUNNER_X = 30;

constexpr int RUNNER_W = VAMPIRE_FRAME_W;
constexpr int RUNNER_H = VAMPIRE_FRAME_H;
constexpr int RUNNER_DEATH_W = VAMPIRE_DEAD_FRAME_W;


constexpr int RUNNER_DEATH_X = RUNNER_X - 24;

constexpr float GRAVITY = runrules::gravity;
constexpr float JUMP_VELOCITY = runrules::jumpVelocity;


#define RUN_BG UI_BG
#define RUN_CARD UI_PANEL
#define RUN_SURFACE UI_SURFACE
#define RUN_WHITE UI_TEXT
#define RUN_MUTED UI_MUTED
#define RUN_CYAN UI_CYAN
constexpr uint16_t RUN_GREEN = 0x3666;
constexpr uint16_t RUN_AMBER = 0xFE04;
constexpr uint16_t RUN_RED = 0xFA48;
constexpr uint16_t RUN_ACCENT = 0x07FF;
constexpr uint16_t RUN_ACCENT_HI = 0xBFFF;
constexpr uint16_t RUN_DARK_TEXT = 0x0125;
constexpr uint16_t RUN_FRAME = 0x032C;




static float runnerY = RUN_GROUND_Y - RUNNER_H;
static float prevRunnerY = runnerY;
static float runnerVY = 0;
static bool isJumping = false;
static int legAnimFrame = 0;
static unsigned long runLastAnimTime = 0;
static unsigned long runLastScoreTime = 0;
static float gameSpeed = runrules::startSpeed;


struct RunObstacle {
  float x;
  int w, h;
  uint16_t color;
  uint8_t style;
  bool active;
};
static const int MAX_RUN_OBS = 3;
static RunObstacle obs[MAX_RUN_OBS];
static unsigned long lastObsSpawn = 0;

static int runCharacter = 0;
static runrules::Rewards runRewards;
static uint32_t runTickAt = 0;
static uint32_t runFrameAt = 0;
static uint32_t runMilestoneTime = 0;
static int runMilestone = 0;
static int runPattern = 0, runPatternStep = 0, runSequence = 0;
static float runSpawnDistance = 0;
struct RunPickup { float x; int y; bool shield, active; };
static RunPickup runPickups[16];
struct RunDebris {
  float x, y, vx, vy;
  uint16_t life, color;
  uint8_t size;
};
static RunDebris runDebris[24];
struct RunCollectedCoin { float x, y; uint16_t life; };
static RunCollectedCoin runCollectedCoins[12];
static bool runBonusNext = false;

static void decodeRunnerFrame(const RunnerSprite &sprite, int frame, uint8_t *pixels) {
  uint32_t offset = pgm_read_dword(sprite.offsets + frame % sprite.count);
  const int size = sprite.width * RUNNER_H;
  int out = 0;
  while (out < size) {
    int count = pgm_read_byte(sprite.data + offset++);
    uint8_t color = pgm_read_byte(sprite.data + offset++);
    while (count-- && out < size) pixels[out++] = color;
  }
}

static int runPauseSelection = 0;
static const char *const RUN_PAUSE_ITEMS[] = {"Resume", "Restart", "High scores", "Main menu"};
static bool runPauseReady = false;



static constexpr unsigned long RUN_DAY_SEGMENT_MS = 12000;
static uint32_t runDayCycleStarted = 0;
static uint8_t runDayFrame = 0;
static uint8_t runDayNextFrame = 1;
static uint8_t runDayBlend = 0;
static int runDayRenderY = 43;

struct RunCloud {
  float x;
  int y;
  float pixelsPerSecond;
  uint8_t variant;
  int drawnX;
};
static constexpr uint8_t RUN_CLOUD_LAYER_COUNT = 7;
static RunCloud runClouds[RUN_CLOUD_LAYER_COUNT];
static unsigned long runCloudUpdateAt = 0;
constexpr int RUN_RENDER_BAND = 16;
constexpr int RUN_RENDER_BANDS = (RUN_H + RUN_RENDER_BAND - 1) / RUN_RENDER_BAND;
static int runDirtyLeft[RUN_RENDER_BANDS], runDirtyRight[RUN_RENDER_BANDS];
static uint16_t runRenderPixels[RUN_W * RUN_RENDER_BAND];
static bool runMilestoneDirty = false;
static TFT_eSprite runNotice(&tft);




static const uint8_t RUN_DAY_DITHER_8X8[64] = {
   0, 32, 8, 40, 2, 34, 10, 42,
  48, 16, 56, 24, 50, 18, 58, 26,
  12, 44, 4, 36, 14, 46, 6, 38,
  60, 28, 52, 20, 62, 30, 54, 22,
   3, 35, 11, 43, 1, 33, 9, 41,
  51, 19, 59, 27, 49, 17, 57, 25,
  15, 47, 7, 39, 13, 45, 5, 37,
  63, 31, 55, 23, 61, 29, 53, 21
};

static bool useNextRunnerTimePixel(int x, int y) {
  uint8_t threshold = RUN_DAY_DITHER_8X8[((y & 7) << 3) | (x & 7)] << 2;
  return runDayBlend > threshold;
}

static uint16_t blendRun565(uint16_t first, uint16_t second, uint8_t amount) {
  if (amount == 0) return first;
  uint16_t inverse = 255 - amount;
  uint16_t r = (((first >> 11) * inverse + (second >> 11) * amount + 127) / 255) << 11;
  uint16_t g = ((((first >> 5) & 0x3F) * inverse +
                 ((second >> 5) & 0x3F) * amount + 127) / 255) << 5;
  uint16_t b = ((first & 0x1F) * inverse +
                (second & 0x1F) * amount + 127) / 255;
  return r | g | b;
}

static const uint16_t RUN_OBS_BRICK[RUN_BACKGROUND_FRAME_COUNT] =
  {0x9A65, 0xA2A6, 0xB2E6, 0xA248, 0x7A2B, 0x4A8D, 0x324D, 0x29CC, 0x2189};
static const uint16_t RUN_OBS_STONE[RUN_BACKGROUND_FRAME_COUNT] =
  {0x6BF1, 0x7412, 0x83D1, 0x732F, 0x62CF, 0x4ACE, 0x3A6D, 0x320B, 0x21A9};
static const uint16_t RUN_OBS_MORTAR[RUN_BACKGROUND_FRAME_COUNT] =
  {0x4985, 0x51A5, 0x61C5, 0x5988, 0x4168, 0x2988, 0x2148, 0x2127, 0x18E6};
static const uint16_t RUN_OBS_HIGHLIGHT[RUN_BACKGROUND_FRAME_COUNT] =
  {0xD44A, 0xDCAB, 0xF56D, 0xE44E, 0xBBD2, 0x8496, 0x63F5, 0x5352, 0x42CE};
static const uint16_t RUN_OBS_SHADOW[RUN_BACKGROUND_FRAME_COUNT] =
  {0x5143, 0x5944, 0x6144, 0x5947, 0x4147, 0x2948, 0x2128, 0x1906, 0x10C5};
static const uint16_t RUN_OBS_GRASS_TOP[RUN_BACKGROUND_FRAME_COUNT] =
  {0xB747, 0xAEE7, 0xE689, 0xC588, 0x8CC9, 0x63EA, 0x4B49, 0x3AC8, 0x3267};
static const uint16_t RUN_OBS_GRASS_MID[RUN_BACKGROUND_FRAME_COUNT] =
  {0x5DE6, 0x5DA6, 0x9547, 0x7C67, 0x5BA8, 0x42E8, 0x3288, 0x2A27, 0x21E6};
static const uint16_t RUN_OBS_GRASS_DARK[RUN_BACKGROUND_FRAME_COUNT] =
  {0x23A6, 0x2B86, 0x4B66, 0x42E6, 0x3A67, 0x31E7, 0x21C7, 0x2186, 0x1946};
static const uint16_t RUN_OBS_GRASS_SPECK[RUN_BACKGROUND_FRAME_COUNT] =
  {0xDFAB, 0xD76A, 0xFF0E, 0xEE2B, 0xB4ED, 0x7C4E, 0x63AD, 0x532C, 0x4AAA};

static uint16_t runnerTimeColor(const uint16_t *palette) {
  return blendRun565(palette[runDayFrame], palette[runDayNextFrame],
                     runDayBlend);
}

static void syncRunnerDayCycle() {
  uint32_t elapsed = millis() - runDayCycleStarted;
  uint32_t segment = elapsed / RUN_DAY_SEGMENT_MS;
  runDayFrame = segment % RUN_BACKGROUND_FRAME_COUNT;
  runDayNextFrame = (runDayFrame + 1) % RUN_BACKGROUND_FRAME_COUNT;
  runDayBlend = (uint8_t)(((elapsed % RUN_DAY_SEGMENT_MS) * 255UL) /
                          RUN_DAY_SEGMENT_MS);
}

static uint16_t runnerBackdropAt(int x, int y) {
  x = constrain(x, 0, RUN_W - 1);
  y = constrain(y, 0, RUN_H - 1);

  uint16_t a = runBackgroundPixel(runDayFrame, x, y, 0);
  if (runDayBlend == 0) return a;
  uint16_t b = runBackgroundPixel(runDayNextFrame, x, y, 1);
  return blendRun565(a, b, runDayBlend);
}

static void restoreRunnerBackdropRect(int x, int y, int w, int h) {
  int x0 = max(0, x);
  int x1 = min(RUN_W, x + w);
  int y0 = max(0, y);
  int y1 = min(RUN_H, y + h);
  if (x1 <= x0 || y1 <= y0) return;

  static uint16_t row[RUN_W];
  tft.setSwapBytes(true);
  for (int yy = y0; yy < y1; yy++) {
    for (int xx = x0; xx < x1; xx++) row[xx - x0] = runnerBackdropAt(xx, yy);
    tft.pushImage(x0, yy, x1 - x0, 1, row);
  }
  tft.setSwapBytes(false);
}

static void drawRunnerBackdrop() {
  runMilestoneDirty = true;
  syncRunnerDayCycle();
  static uint16_t row[RUN_W];
  tft.setSwapBytes(true);
  for (int y = 0; y < RUN_H; ++y) {
    for (int x = 0; x < RUN_W; ++x) row[x] = runnerBackdropAt(x, y);
    tft.pushImage(0, y, RUN_W, 1, row);
  }
  tft.setSwapBytes(false);
}

static const uint16_t* runCloudImage(uint8_t variant, uint8_t phase) {
  switch (variant) {
    case 0: return run_cloud_0[phase];
    case 1: return run_cloud_1[phase];
    case 2: return run_cloud_2[phase];
    default: return run_cloud_3[phase];
  }
}

static void drawRunCloudClipped(uint8_t cloudIndex) {
  const RunCloud &cloud = runClouds[cloudIndex];
  int x = (int)cloud.x;
  int w = run_cloud_width[cloud.variant];
  int h = run_cloud_height[cloud.variant];
  int sourceLeft = max(0, -x);
  int sourceRight = min(w, RUN_W - x);
  if (sourceLeft >= sourceRight || cloud.y >= RUN_H || cloud.y + h <= 0) return;

  const uint16_t *firstImage = runCloudImage(cloud.variant, runDayFrame);
  const uint16_t *secondImage = runCloudImage(cloud.variant, runDayNextFrame);
  static uint16_t row[80];
  tft.startWrite();
  for (int y = 0; y < h; ++y) {
    int runStart = -1;
    int count = 0;
    for (int sx = sourceLeft; sx < sourceRight; ++sx) {
      uint16_t color = pgm_read_word(&firstImage[y * w + sx]);
      if (color != RUN_CLOUD_TRANSPARENT && runDayBlend) {
        uint16_t next = pgm_read_word(&secondImage[y * w + sx]);
        if (next != RUN_CLOUD_TRANSPARENT)
          color = blendRun565(color, next, runDayBlend);
      }
      if (color != RUN_CLOUD_TRANSPARENT) {
        if (runStart < 0) runStart = sx;
        row[count++] = color;
      } else if (count > 0) {
        tft.setAddrWindow(x + runStart, cloud.y + y, count, 1);
        tft.pushColors(row, count, true);
        runStart = -1;
        count = 0;
      }
    }
    if (count > 0) {
      tft.setAddrWindow(x + runStart, cloud.y + y, count, 1);
      tft.pushColors(row, count, true);
    }
  }
  tft.endWrite();
}

static void updateRunnerDayCycleBackdrop() {
  if (runDayRenderY == 43) syncRunnerDayCycle();
  int nextY = (runDayRenderY / RUN_RENDER_BAND + 1) * RUN_RENDER_BAND;
  dirtyRunnerRect(0, runDayRenderY, RUN_W, nextY - runDayRenderY);
  runDayRenderY = nextY;
  if (runDayRenderY >= 295) runDayRenderY = 43;
}

static void initRunClouds() {


  const int starts[RUN_CLOUD_LAYER_COUNT] = {10, 160, 80, 210, 25, 135, 225};
  const int heights[RUN_CLOUD_LAYER_COUNT] = {48, 75, 97, 119, 140, 157, 177};
  const float speeds[RUN_CLOUD_LAYER_COUNT] = {2.6f, 3.4f, 4.3f, 5.4f,
                                               6.5f, 8.0f, 10.0f};
  const uint8_t variants[RUN_CLOUD_LAYER_COUNT] = {0, 3, 2, 1, 3, 2, 3};
  for (int i = 0; i < RUN_CLOUD_LAYER_COUNT; ++i) {
    runClouds[i] = {(float)starts[i], heights[i], speeds[i], variants[i],
                    starts[i]};
  }
  runCloudUpdateAt = millis();
}

static void drawRunClouds() {
  for (int i = 0; i < RUN_CLOUD_LAYER_COUNT; ++i) {
    runClouds[i].drawnX = (int)runClouds[i].x;
    drawRunCloudClipped(i);
  }
}

static void updateRunClouds() {
  unsigned long now = millis();
  unsigned long elapsed = min(now - runCloudUpdateAt, 80UL);
  if (elapsed < 20) return;
  runCloudUpdateAt = now;

  bool moved[RUN_CLOUD_LAYER_COUNT] = {};

  for (int i = 0; i < RUN_CLOUD_LAYER_COUNT; ++i) {
    RunCloud &cloud = runClouds[i];
    float nextX = cloud.x - cloud.pixelsPerSecond * elapsed / 1000.0f;
    if (nextX + run_cloud_width[cloud.variant] < 0) {
      nextX = RUN_W + random(12, 70);
    }
    if ((int)nextX != cloud.drawnX) {
      dirtyRunnerRect(cloud.drawnX, cloud.y,
                      run_cloud_width[cloud.variant], run_cloud_height[cloud.variant]);
      moved[i] = true;
    }
    cloud.x = nextX;
  }
  for (int i = 0; i < RUN_CLOUD_LAYER_COUNT; ++i) {
    if (!moved[i]) continue;
    runClouds[i].drawnX = (int)runClouds[i].x;
    dirtyRunnerRect(runClouds[i].drawnX, runClouds[i].y,
                    run_cloud_width[runClouds[i].variant],
                    run_cloud_height[runClouds[i].variant]);
  }
}




static void soundJump() {
  audio::play(audio::Cue::Jump);
}

static void soundCrash() {
  audio::play(audio::Cue::GameOver);
}

static void soundMilestone() {
  audio::play(audio::Cue::Milestone);
}




static void drawRunnerHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(RUN_W, 43)) return;
  header.fillSprite(ui::CARD);

  for (int y = 0; y < 5; ++y) for (int x = 0; x < RUN_W; ++x)
    header.drawPixel(x, y, runnerBackdropAt(x, y));
  for (int y = 5; y < 43; ++y) for (int x = 0; x < 8; ++x) {
    header.drawPixel(x, y, runnerBackdropAt(x, y));
    header.drawPixel(239 - x, y, runnerBackdropAt(239 - x, y));
  }
  for (int y = 0; y < 12; ++y) for (int x = 0; x < 12; ++x) {
    if ((12 - x) * (12 - x) + (12 - y) * (12 - y) <= 144) continue;
    header.drawPixel(8 + x, 5 + y, runnerBackdropAt(8 + x, 5 + y));
    header.drawPixel(231 - x, 5 + y, runnerBackdropAt(231 - x, 5 + y));
    header.drawPixel(8 + x, 42 - y, runnerBackdropAt(8 + x, 42 - y));
    header.drawPixel(231 - x, 42 - y, runnerBackdropAt(231 - x, 42 - y));
  }
  header.setTextDatum(ML_DATUM);
  header.setTextColor(ui::LABEL);
  header.drawString("Coins", 20, 15, 2);
  header.setTextColor(RUN_AMBER);
  header.drawString(String(runRewards.coins), 20, 32, 2);
  header.setTextDatum(TR_DATUM);
  header.setTextColor(ui::SECONDARY);
  header.drawString("Combo", 142, 8, 2);
  header.drawString("Distance", 220, 8, 2);
  header.setTextColor(RUN_ACCENT);
  header.drawString("x" + String(runRewards.multiplier()), 142, 24, 2);
  header.setTextColor(score > 0 && score % 50 < 4 ? RUN_ACCENT : ui::LABEL);
  header.drawString(String(score) + " m", 220, 24, 2);
  header.pushSprite(0, 0);
  header.deleteSprite();
}

static void updateRunnerScoreHUD() {
  static int lastScore = -1, lastCoins = -1, lastCombo = -1;
  if (score == lastScore && runRewards.coins == lastCoins &&
      runRewards.multiplier() == lastCombo) return;
  drawRunnerHeader();
  lastScore = score;
  lastCoins = runRewards.coins;
  lastCombo = runRewards.multiplier();
}

static void drawRunnerBottomBar(bool force = false) {
  static int lastSeconds = -1;
  int seconds = (runRewards.shield + 999) / 1000;
  if (!force && seconds == lastSeconds) return;
  TFT_eSprite bar(&tft);
  bar.setColorDepth(16);
  if (!bar.createSprite(224, 20)) return;
  bar.fillSprite(ui::CARD);
  for (int y = 0; y < 10; ++y) for (int x = 0; x < 10; ++x) {
    if ((10-x)*(10-x)+(10-y)*(10-y)<=100) continue;
    bar.drawPixel(x,y,runnerBackdropAt(8+x,296+y));
    bar.drawPixel(223-x,y,runnerBackdropAt(231-x,296+y));
    bar.drawPixel(x,19-y,runnerBackdropAt(8+x,315-y));
    bar.drawPixel(223-x,19-y,runnerBackdropAt(231-x,315-y));
  }
  String hint = seconds ? "Shield active " + String(seconds) + "s  Right: attack"
                        : "Up: jump   Shoot: pause";
  bar.setTextDatum(MC_DATUM);
  bar.setTextColor(seconds ? RUN_ACCENT : ui::SECONDARY, ui::CARD);
  bar.drawString(hint,112,10,2);
  bar.pushSprite(8,296);
  lastSeconds = seconds;
}




static void drawRunner(int px, int py, int prevPy, int anim, bool inAir) {
  (void)anim;
  const auto &sprite = runner_characters[runCharacter].actions[runRewards.attack ? 3 : inAir ? 1 : 0];
  static int previousLeft = 0, previousRight = RUNNER_W;
  int left = min(previousLeft, (int)sprite.x);
  int right = max(previousRight, sprite.x + sprite.width);
  dirtyRunnerRect(px + left, min(py, prevPy) - 12,
                  right - left, RUNNER_H + abs(prevPy - py) + 12);
  previousLeft = sprite.x;
  previousRight = sprite.x + sprite.width;
  for (const auto &p : runDebris)
    if (p.life) dirtyRunnerRect((int)p.x, (int)p.y, p.size, p.size);
  for (const auto &coin : runCollectedCoins)
    if (coin.life) dirtyRunnerRect((int)coin.x-5,(int)coin.y-5,11,11);
  renderRunnerDirty();
}

static void eraseRunner(int px, int py) {
  restoreRunnerBackdropRect(px - 20, py - 12, 100, RUNNER_H + 12);
}

static void drawRunnerDeathFrame(int py, int frameIndex) {
  const RunnerCharacter &character = runner_characters[runCharacter];
  const RunnerSprite &sprite = character.actions[2];
  static uint8_t frame[RUNNER_DEATH_W * RUNNER_H];
  decodeRunnerFrame(sprite, frameIndex, frame);
  static uint16_t composed[RUNNER_DEATH_W * RUNNER_H];
  for (int y = 0; y < RUNNER_H; ++y)
    for (int x = 0; x < RUNNER_DEATH_W; ++x) {
      uint16_t ink = pgm_read_word(character.palette + frame[y * RUNNER_DEATH_W + x]);
      composed[y * RUNNER_DEATH_W + x] = ink == VAMPIRE_TRANSPARENT
          ? runnerBackdropAt(RUNNER_DEATH_X + x, py + y) : ink;
    }
  tft.setSwapBytes(true);
  tft.pushImage(RUNNER_DEATH_X, py, RUNNER_DEATH_W, RUNNER_H, composed);
  tft.setSwapBytes(false);
}

static void playRunnerDeathAnimation(int py) {

  eraseRunner(RUNNER_X, (int)prevRunnerY);
  restoreRunnerBackdropRect(RUNNER_DEATH_X, py,
                            RUNNER_DEATH_W, RUNNER_H);
  audio::play(audio::Cue::GameOver);
  for (int frame = 0; frame < runner_characters[runCharacter].actions[2].count; ++frame) {
    drawRunnerDeathFrame(py, frame);
    delay(95);
  }
  setBuzzerTone(0);
  restoreRunnerBackdropRect(RUNNER_DEATH_X, py,
                            RUNNER_DEATH_W, RUNNER_H);
}

static void addRunPickup(float x, int y, bool shield) {
  for (auto &pickup : runPickups)
    if (!pickup.active) { pickup = {x, y, shield, true}; return; }
}

static void spawnRunObstacle() {
  if (runBonusNext) {

    const int heights[] = {76, 94, 106, 94, 76};
    for (int i = 0; i < 5; ++i) addRunPickup(250 + i * 28, RUN_GROUND_Y - heights[i], false);
    runSpawnDistance = 112 + 44 * gameSpeed;
    runBonusNext = false;
    return;
  }
  for (auto &obstacle : obs) {
    if (obstacle.active) continue;
    const auto &step = runrules::patterns[runPattern][runPatternStep];
    const int widths[] = {11, 12, 18}, heights[] = {24, 36, 20};
    obstacle = {250, widths[step.style], heights[step.style], 0, step.style, true};

    const int dx[] = {-60, -20, 20, 60};
    const int dy[] = {52, 78, 92, 64};
    for (int i = 0; i < 4; ++i) addRunPickup(250 + dx[i], RUN_GROUND_Y - dy[i], false);
    runSpawnDistance = step.gapFrames * gameSpeed + obstacle.w;
    runBonusNext = runPatternStep == 1;
    if (++runPatternStep == 3) {
      runPatternStep = 0;
      runPattern = (runPattern + 1) % 3;
      if (++runSequence % 2 == 0) addRunPickup(350, RUN_GROUND_Y - 28, true);
    }
    return;
  }
}

static void collectRunCoin(float x, int y) {
  int slot = 0;
  for (int i = 1; i < 12; ++i)
    if (runCollectedCoins[i].life < runCollectedCoins[slot].life) slot = i;
  auto &coin = runCollectedCoins[slot];
  if (coin.life) dirtyRunnerRect((int)coin.x-5,(int)coin.y-5,11,11);
  coin = {x,(float)y,700};
  dirtyRunnerRect((int)x-5,y-5,11,11);
}

static void updateRunCollectedCoins(uint32_t elapsed) {
  for (auto &coin : runCollectedCoins) {
    if (!coin.life) continue;
    dirtyRunnerRect((int)coin.x-5,(int)coin.y-5,11,11);
    coin.life = runrules::tick(coin.life,elapsed);
    coin.y -= min(elapsed,(uint32_t)50) * 0.13f;
    if (coin.life) dirtyRunnerRect((int)coin.x-5,(int)coin.y-5,11,11);
  }
}

static void burstRunObstacle(int index) {
  const auto &o = obs[index];
  uint16_t brick = runnerTimeColor(o.style == 1 ? RUN_OBS_STONE : RUN_OBS_BRICK);
  uint16_t light = runnerTimeColor(RUN_OBS_HIGHLIGHT);
  for (int i = 0; i < 12; ++i) {

    int slot = 0;
    for (int j = 1; j < 24; ++j)
      if (runDebris[j].life < runDebris[slot].life) slot = j;
    auto &p = runDebris[slot];
    if (p.life) dirtyRunnerRect((int)p.x, (int)p.y, p.size, p.size);
    p = {o.x + o.w / 2.0f, RUN_GROUND_Y - o.h / 2.0f,
         (float)(i % 4 * 48 - 72), (float)(-55 - i / 4 * 38),
         (uint16_t)(320 + i * 20), i >= 8 ? RUN_ACCENT_HI : i & 1 ? light : brick,
         (uint8_t)(i >= 8 ? 1 : 2 + i % 2)};
    dirtyRunnerRect((int)p.x, (int)p.y, p.size, p.size);
  }
}

static void updateRunDebris(uint32_t elapsed) {
  float dt = min(elapsed, (uint32_t)50) / 1000.0f;
  for (auto &p : runDebris) {
    if (!p.life) continue;
    dirtyRunnerRect((int)p.x, (int)p.y, p.size, p.size);
    p.life = runrules::tick(p.life, elapsed);
    p.x += p.vx * dt;
    p.y += p.vy * dt;
    p.vy += 240 * dt;
    if (p.x < -3 || p.x >= RUN_W || p.y >= RUN_GROUND_Y) p.life = 0;
    if (p.life) dirtyRunnerRect((int)p.x, (int)p.y, p.size, p.size);
  }
}

static uint16_t runObstacleColors[10];
static void cacheRunnerObstacleColors() {
  const uint16_t *palettes[] = {RUN_OBS_BRICK, RUN_OBS_STONE, RUN_OBS_MORTAR,
    RUN_OBS_HIGHLIGHT, RUN_OBS_SHADOW, RUN_OBS_GRASS_TOP, RUN_OBS_GRASS_MID,
    RUN_OBS_GRASS_DARK, RUN_OBS_GRASS_SPECK};
  for (int i = 0; i < 9; ++i) runObstacleColors[i] = runnerTimeColor(palettes[i]);
  runObstacleColors[9] = blendRun565(runObstacleColors[0], runObstacleColors[3], 44);
}

static uint16_t runnerObstaclePixel(int index, int xx, int yy) {
  const RunObstacle &obstacle = obs[index];
  bool stone = obstacle.style == 1;
  uint16_t base = runObstacleColors[stone ? 1 : 0];
  uint16_t mortar = runObstacleColors[2];
  uint16_t highlight = runObstacleColors[3];
  uint16_t shadow = runObstacleColors[4];
  if (obstacle.style == 2) base = runObstacleColors[9];
  int brickY = max(0, yy - 4);
  int row = brickY / 6;
  int brickX = (xx + (row & 1) * 4) % 8;
  uint16_t color = base;
  if (brickY % 6 == 0 || brickX == 0) color = mortar;
  else if (brickY % 6 == 1) color = highlight;
  else if (brickY % 6 == 5 || brickX == 7) color = shadow;
  if (stone && yy > 7 && (xx + yy * 3) % 23 == 0) color = mortar;
  if (xx == 0 || xx == obstacle.w - 1 || yy == obstacle.h - 1)
    color = shadow;
  if (yy < 3 || (yy < 5 && (xx / 2) % 3 == 0)) {
    color = runObstacleColors[yy == 0 ? 5 : yy == 1 ? 6 : 7];
    if ((xx + yy) % 5 == 0) color = runObstacleColors[8];
  }
  return color;
}


static void dirtyRunnerRect(int x, int y, int w, int h) {
  int x0 = max(0, x), x1 = min(RUN_W, x + w);
  int y0 = max(43, y), y1 = min(295, y + h);
  if (x0 >= x1 || y0 >= y1) return;
  for (int band = y0 / RUN_RENDER_BAND; band <= (y1 - 1) / RUN_RENDER_BAND; ++band) {
    if (runDirtyRight[band] == 0) runDirtyLeft[band] = x0;
    else runDirtyLeft[band] = min(runDirtyLeft[band], x0);
    runDirtyRight[band] = max(runDirtyRight[band], x1);
  }
}

static void renderRunnerDirty() {
  prepareRunMilestone();
  const auto &character = runner_characters[runCharacter];
  int action = runRewards.attack ? 3 : isJumping ? 1 : 0;
  const auto &sprite = character.actions[action];
  int frameIndex = runRewards.attack
      ? runrules::attackFrame(runRewards.attack, sprite.count) : legAnimFrame;
  frameIndex %= sprite.count;
  static uint8_t frame[100 * RUNNER_H];
  static int cachedCharacter = -1, cachedAction = -1, cachedFrame = -1;
  if (cachedCharacter != runCharacter || cachedAction != action || cachedFrame != frameIndex) {
    decodeRunnerFrame(sprite, frameIndex, frame);
    cachedCharacter = runCharacter; cachedAction = action; cachedFrame = frameIndex;
  }
  cacheRunnerObstacleColors();
  tft.setSwapBytes(true);
  tft.startWrite();
  for (int band = 0; band < RUN_RENDER_BANDS; ++band) {
    int x0 = runDirtyLeft[band], x1 = runDirtyRight[band];
    if (x1 == 0) continue;
    int y0 = max(43, band * RUN_RENDER_BAND), y1 = min(295, (band + 1) * RUN_RENDER_BAND);
    int width = x1 - x0;
    for (int y = y0; y < y1; ++y)
      for (int x = x0; x < x1; ++x)
        runRenderPixels[(y - y0) * width + x - x0] = runnerBackdropAt(x, y);

    for (const auto &cloud : runClouds) {
      int cx = (int)cloud.x, cw = run_cloud_width[cloud.variant];
      int left = max(x0, cx), right = min(x1, cx + cw);
      int top = max(y0, cloud.y), bottom = min(y1, cloud.y + run_cloud_height[cloud.variant]);
      if (left >= right || top >= bottom) continue;
      const auto *first = runCloudImage(cloud.variant, runDayFrame);
      const auto *second = runCloudImage(cloud.variant, runDayNextFrame);
      for (int y = top; y < bottom; ++y) for (int x = left; x < right; ++x) {
        int source = (y - cloud.y) * cw + x - cx;
        uint16_t ink = pgm_read_word(first + source);
        if (ink == RUN_CLOUD_TRANSPARENT) continue;
        if (runDayBlend) {
          uint16_t next = pgm_read_word(second + source);
          if (next != RUN_CLOUD_TRANSPARENT) ink = blendRun565(ink, next, runDayBlend);
        }
        runRenderPixels[(y - y0) * width + x - x0] = ink;
      }
    }
    for (int i = 0; i < MAX_RUN_OBS; ++i) {
      const auto &o = obs[i];
      if (!o.active) continue;
      int ox = (int)o.x, oy = RUN_GROUND_Y - o.h;
      for (int y = max(y0, oy); y < min(y1, RUN_GROUND_Y); ++y)
        for (int x = max(x0, ox); x < min(x1, ox + o.w); ++x)
          runRenderPixels[(y - y0) * width + x - x0] = runnerObstaclePixel(i, x - ox, y - oy);
    }
    for (const auto &pickup : runPickups) {
      if (!pickup.active) continue;
      int cx = (int)pickup.x;
      for (int y = max(y0, pickup.y - 6); y < min(y1, pickup.y + 7); ++y)
        for (int x = max(x0, cx - 6); x < min(x1, cx + 7); ++x) {
          int dx = x - cx, dy = y - pickup.y;
          uint16_t ink;
          if (pickup.shield) {
            if (abs(dx) > 6 - (dy + 6) / 3) continue;
            ink = abs(dx) >= 4 - (dy + 6) / 3 || dy == -6 ? RUN_ACCENT_HI : RUN_ACCENT;
          } else {
            if (dx * dx + dy * dy > 20) continue;
            ink = abs(dx) > 2 || abs(dy) > 2 ? RUN_AMBER : 0xFFF0;
          }
          runRenderPixels[(y - y0) * width + x - x0] = ink;
        }
    }
    int sx = RUNNER_X + sprite.x, sy = (int)runnerY;
    for (int y = max(y0, sy); y < min(y1, sy + RUNNER_H); ++y)
      for (int x = max(x0, sx); x < min(x1, sx + sprite.width); ++x) {
        uint16_t ink = pgm_read_word(character.palette + frame[(y - sy) * sprite.width + x - sx]);
        if (ink != VAMPIRE_TRANSPARENT) runRenderPixels[(y - y0) * width + x - x0] = ink;
      }
    if (runRewards.shield) {
      for (int y = max(y0, sy - 10); y < min(y1, sy - 2); ++y)
        for (int x = max(x0, RUNNER_X + 16); x < min(x1, RUNNER_X + 25); ++x) {
          int dx = abs(x - RUNNER_X - 20), dy = y - sy + 10;
          if (dx <= 4 - dy / 2)
            runRenderPixels[(y - y0) * width + x - x0] = dx == 4 - dy / 2 || dy == 0 ? RUN_ACCENT_HI : RUN_ACCENT;
        }
    }
    for (const auto &p : runDebris) {
      if (!p.life) continue;
      int px = (int)p.x, py = (int)p.y;
      for (int y = max(y0, py); y < min(y1, py + p.size); ++y)
        for (int x = max(x0, px); x < min(x1, px + p.size); ++x)
          runRenderPixels[(y - y0) * width + x - x0] = p.color;
    }
    for (const auto &coin : runCollectedCoins) {
      if (!coin.life) continue;
      int cx=(int)coin.x, cy=(int)coin.y;
      uint8_t opacity = coin.life >= 250 ? 255 : coin.life * 255 / 250;
      for (int y=max(y0,cy-4); y<min(y1,cy+5); ++y)
        for (int x=max(x0,cx-4); x<min(x1,cx+5); ++x) {
          int dx=x-cx, dy=y-cy;
          if (dx*dx+dy*dy>20) continue;
          auto &pixel=runRenderPixels[(y-y0)*width+x-x0];
          pixel=blendRun565(pixel,abs(dx)>2 || abs(dy)>2 ? RUN_AMBER : 0xFFF0,opacity);
        }
    }

    if (runMilestoneTime && runNotice.created()) {
      for (int y=max(y0,52); y<min(y1,76); ++y)
        for (int x=max(x0,32); x<min(x1,208); ++x) {
          uint16_t ink=runNotice.readPixel(x-32,y-52);
          if (ink!=VAMPIRE_TRANSPARENT) runRenderPixels[(y-y0)*width+x-x0]=ink;
        }
    }
    tft.pushImage(x0, y0, width, y1 - y0, runRenderPixels);
    runDirtyRight[band] = 0;
  }
  tft.endWrite();
  tft.setSwapBytes(false);
}

static void drawRunnerObstacle(int index, int x, int y) {
  dirtyRunnerRect(x, y, obs[index].w, obs[index].h);
}

static void updateAndDrawObstacles() {
  for (int i = 0; i < MAX_RUN_OBS; i++) {
    if (obs[i].active) {
      int prevX = (int)obs[i].x;
      int oy = RUN_GROUND_Y - obs[i].h;

      obs[i].x -= gameSpeed;
      int curX = (int)obs[i].x;

      dirtyRunnerRect(curX + obs[i].w, oy, prevX - curX, obs[i].h);


      if (obs[i].x + obs[i].w < 0) {
        dirtyRunnerRect(0, oy, obs[i].w + 2, obs[i].h);
        obs[i].active = false;
        continue;
      }

      const RunnerSprite &attack = runner_characters[runCharacter].actions[3];
      int attackFrame = runrules::attackFrame(runRewards.attack, attack.count);
      if (attackFrame == attack.count / 2 &&
          runrules::overlaps(RUNNER_X + 7, (int)runnerY + 12, 79, 48,
                             curX, oy, obs[i].w, obs[i].h)) {
        burstRunObstacle(i);
        obs[i].active = false;
        dirtyRunnerRect(curX, oy, obs[i].w, obs[i].h);
        runRewards.coins += 5;
        audio::play(audio::Cue::Brick);
        continue;
      }
      if (curX < RUN_W) drawRunnerObstacle(i, curX, oy);




      int rx = RUNNER_X + 7;
      int ry = (int)runnerY + 6;
      int rw = RUNNER_W - 14;
      int rh = RUNNER_H - 8;

      if (curX < rx + rw && curX + obs[i].w > rx &&
          oy < ry + rh && oy + obs[i].h > ry) {

        if (runRewards.attack && attackFrame < attack.count / 2) continue;
        if (runRewards.hit()) {
          burstRunObstacle(i);
          obs[i].active = false;
          dirtyRunnerRect(curX, oy, obs[i].w, obs[i].h);
          audio::play(audio::Cue::Hit);
          continue;
        }
        playRunnerDeathAnimation((int)runnerY);
        gameState = 2;
        menu_drawn = false;
        return;
      }
    }
  }
}




static void drawRunPickups() {
  for (const auto &pickup : runPickups)
    if (pickup.active) dirtyRunnerRect((int)pickup.x - 7, pickup.y - 7, 15, 15);
}

static void updateRunPickups() {
  for (auto &pickup : runPickups) {
    if (!pickup.active) continue;
    dirtyRunnerRect((int)pickup.x - 7, pickup.y - 7, 15, 15);
    pickup.x -= gameSpeed;
    if (runrules::overlaps(RUNNER_X + 7, (int)runnerY + 6, RUNNER_W - 14, RUNNER_H - 8,
                           (int)pickup.x - 6, pickup.y - 6, 12, 12)) {
      pickup.active = false;
      if (pickup.shield) {
        runRewards.shield = runrules::shieldDuration;
        audio::play(audio::Cue::Powerup);
      } else {
        runRewards.collect();
        collectRunCoin(pickup.x,pickup.y);
        audio::play(audio::Cue::Pickup);
      }
    } else if (pickup.x < -7) {
      pickup.active = false;
      if (!pickup.shield) runRewards.streak = 0;
    }
  }
}

static void prepareRunMilestone() {
  if (!runMilestoneTime) { runNotice.deleteSprite(); return; }
  if (!runMilestoneDirty && runNotice.created()) return;
  runNotice.setColorDepth(16);
  if (!runNotice.created() && !runNotice.createSprite(176,24)) return;
  runNotice.fillSprite(VAMPIRE_TRANSPARENT);
  runNotice.fillRoundRect(0,0,176,24,12,ui::CARD);
  runNotice.setTextDatum(MC_DATUM);
  runNotice.setTextColor(RUN_ACCENT,ui::CARD);
  runNotice.drawString(String(runMilestone)+" m  -  Keep going",88,12,2);
  runMilestoneDirty=false;
  dirtyRunnerRect(32,52,176,24);
}

static void initRunnerGame() {
  audio::restartMusic();
  score = 0;
  runRewards = {};
  runMilestoneTime = 0;
  runNotice.deleteSprite();
  runBonusNext = false;
  for (auto &coin : runCollectedCoins) coin.life = 0;
  runMilestone = 0;
  runPattern = runPatternStep = runSequence = 0;
  runSpawnDistance = 120;
  runFrameAt = runTickAt = millis();
  for (auto &right : runDirtyRight) right = 0;
  for (auto &pickup : runPickups) pickup.active = false;
  for (auto &p : runDebris) p.life = 0;
  gameSpeed = runrules::startSpeed;
  runnerY = RUN_GROUND_Y - RUNNER_H;
  prevRunnerY = runnerY;
  runnerVY = 0;
  isJumping = false;
  legAnimFrame = 0;
  runLastAnimTime = millis();
  runLastScoreTime = runLastAnimTime;
  lastObsSpawn = millis();
  runDayRenderY = 43;

  for (int i = 0; i < MAX_RUN_OBS; i++) {
    obs[i].active = false;
  }

  tft.fillScreen(RUN_BG);
  drawRunnerBackdrop();
  initRunClouds();
  drawRunClouds();
  drawRunnerHeader();
  drawRunnerBottomBar(true);

  drawRunner(RUNNER_X, (int)runnerY, (int)runnerY, legAnimFrame, isJumping);
}




static void drawRunnerChoice() {

  const auto &character = runner_characters[runCharacter];
  const auto &sprite = character.actions[0];
  uint8_t frame[RUNNER_W * RUNNER_H];
  decodeRunnerFrame(sprite, 0, frame);
  for (int i = 0; i < RUNNER_W * RUNNER_H; ++i) {
    uint16_t ink = pgm_read_word(character.palette + frame[i]);
    runRenderPixels[i] = ink == VAMPIRE_TRANSPARENT ? ui::CARD : ink;
  }
  tft.setSwapBytes(true);
  tft.pushImage(100, 104, RUNNER_W, RUNNER_H, runRenderPixels);
  tft.setSwapBytes(false);
  tft.setTextPadding(176);
  ui::hint(180, character.name, ui::CARD, ui::LABEL);
  tft.setTextPadding(0);
  for (int i = 0; i < 3; ++i)
    tft.fillCircle(108 + i * 12, 198, 3, i == runCharacter ? RUN_ACCENT : ui::SEPARATOR);
}

static void showRunnerStartScreen() {
  gameState = -1;
  tft.setRotation(2);
  tft.fillScreen(RUN_BG);
  drawRunnerBackdrop();
  drawRunClouds();
  drawRunnerHeader();
  ui::card(18, 52, 204, 236);
  ui::heading(34, 62, "Choose your character", "Run");
  drawRunnerChoice();
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString("<", 52, 136, 4);
  tft.drawString(">", 188, 136, 4);
  ui::hint(218, "Up: jump  Right: attack", ui::CARD);
  ui::hint(237, "Collect a shield to attack", ui::CARD);
  ui::pill(34, 254, 172, 26, "Shoot to start", RUN_ACCENT);
  buttonsRequireRelease();
}

static void drawRunnerPauseOption(int item) {
  ui::pauseItem(RUN_PAUSE_ITEMS, 4, item, runPauseSelection, RUN_ACCENT);
}

static void drawRunnerPauseMenu() {
  buttonsRequireRelease();
  ui::pauseSheet("Run", RUN_PAUSE_ITEMS, 4, runPauseSelection, RUN_ACCENT);
}

static void handleRunnerPause() {
  if (!runPauseReady) {
    if (readShoot() == LOW) runPauseReady = true;
    return;
  }

  int cmd = findCommand();
  if (cmd == 0 || cmd == 2) {
    int old = runPauseSelection;
    runPauseSelection = (runPauseSelection + (cmd == 0 ? 3 : 1)) % 4;
    drawRunnerPauseOption(old);
    drawRunnerPauseOption(runPauseSelection);
    playNavClick();
  } else if (cmd == 4) {
    playSelectTone();
    if (runPauseSelection == 0) {
      showResumeCountdown([]() {
      if (!restoringCountdownDigit()) tft.fillScreen(RUN_BG);
      drawRunnerBackdrop();
      drawRunClouds();
      drawRunnerHeader();
      drawRunnerBottomBar(true);
      for (int i = 0; i < MAX_RUN_OBS; ++i)
        if (obs[i].active)
          drawRunnerObstacle(i, (int)obs[i].x, RUN_GROUND_Y - obs[i].h);
      drawRunPickups();
      drawRunner(RUNNER_X, (int)runnerY, (int)runnerY, legAnimFrame, isJumping);
      });
      runLastAnimTime = runLastScoreTime = lastObsSpawn = runCloudUpdateAt = runTickAt = runFrameAt = millis();
      gameState = 0;
      menu_drawn = false;
    } else if (runPauseSelection == 1) {
      initRunnerGame();
      gameState = 0;
      menu_drawn = false;
    } else if (runPauseSelection == 2) {
      showGameHighScores();
      tft.fillScreen(RUN_BG);
      drawRunnerBackdrop();
      drawRunClouds();
      drawRunnerHeader();
      drawRunnerBottomBar(true);
      for (int i = 0; i < MAX_RUN_OBS; ++i)
        if (obs[i].active)
          drawRunnerObstacle(i, (int)obs[i].x, RUN_GROUND_Y - obs[i].h);
      drawRunPickups();
      drawRunner(RUNNER_X, (int)runnerY, (int)runnerY, legAnimFrame, isJumping);
      drawRunnerPauseMenu();
      runPauseReady = false;
    } else {
      runNotice.deleteSprite();
      switchGame(0);
    }
  }
}

static void drawRunnerGameOver() {
  currentGame.recordScore(score);
  String coins = String(runRewards.coins);
  ui::gameOverSheet("Run", score, currentGame.highScores[0], RUN_ACCENT,
                    "Crashed", " m", coins.c_str(), "Coins");
}

static void handleRunnerGameOver() {
  int cmd = findCommand();
  if (cmd == 4) {
    playSelectTone();
    initRunnerGame();
    gameState = 0;
    menu_drawn = false;
  } else if (cmd == 0) {
    playSelectTone();
    runNotice.deleteSprite();
    switchGame(0);
  }
}




void runSetup() {
  tft.setRotation(2);
  score = 0;
  runDayCycleStarted = millis();
  runDayFrame = 0;
  runDayNextFrame = 1;
  runDayBlend = 0;
  runDayRenderY = 43;
  initRunClouds();
  showRunnerStartScreen();
}

void runLoop() {
  audio::gameState(gameState);
  if (gameState == 0) {
    uint32_t frameStarted = millis();
    if (frameStarted - runFrameAt < runrules::frameMs) { delay(1); return; }
    runFrameAt = frameStarted;

    int cmd = findCommand(false);
    if (cmd == 4) {
      audio::pauseMusic(true); gameState = 1;
      menu_drawn = false;
      runPauseSelection = 0;
      runPauseReady = false;
      return;
    }
    uint32_t elapsed = millis() - runTickAt;
    runTickAt = millis();
    runRewards.update(elapsed);
    updateRunDebris(elapsed);
    updateRunCollectedCoins(elapsed);
    uint32_t oldMilestoneTime = runMilestoneTime;
    runMilestoneTime = runrules::tick(runMilestoneTime, elapsed);
    if (oldMilestoneTime && !runMilestoneTime) dirtyRunnerRect(32, 52, 176, 24);
    if (cmd == 1 && runRewards.startAttack(runner_characters[runCharacter].actions[3].count * runrules::attackFrameMs))
      audio::play(audio::Cue::Laser);
    if (cmd == 0 && !isJumping) {
      runnerVY = JUMP_VELOCITY;
      isJumping = true;
      soundJump();
    }



    runnerVY += GRAVITY;
    runnerY += runnerVY;


    if (runnerY >= RUN_GROUND_Y - RUNNER_H) {
      runnerY = RUN_GROUND_Y - RUNNER_H;
      runnerVY = 0;
      isJumping = false;
    }


    unsigned long now = millis();
    const unsigned long animationInterval = isJumping ? 90 : 56;
    if (now - runLastAnimTime >= animationInterval) {
      legAnimFrame = (legAnimFrame + 1) % runner_characters[runCharacter].actions[isJumping ? 1 : 0].count;
      runLastAnimTime = now;
    }

    if (now - runLastScoreTime >= 120) {
      runLastScoreTime = now;
      score++;
      if (score == 100 || score == 250 || score == 500) {
        runMilestone = score;
        runMilestoneTime = 1800;
        runMilestoneDirty = true;
        soundMilestone();
      }

      if (score % 50 == 0 && gameSpeed < runrules::maxSpeed) {
        gameSpeed = min(runrules::maxSpeed, gameSpeed + 0.3125f);
        soundMilestone();
      }
    }



    updateRunnerDayCycleBackdrop();


    updateRunClouds();


    runSpawnDistance -= gameSpeed;
    if (runSpawnDistance <= 0) spawnRunObstacle();
    updateRunPickups();
    updateAndDrawObstacles();
    if (gameState != 0) return;
    drawRunPickups();



    drawRunner(RUNNER_X, (int)runnerY, (int)prevRunnerY, legAnimFrame,
               isJumping);
    prevRunnerY = runnerY;

    updateRunnerScoreHUD();
    drawRunnerBottomBar();
    yield();
    return;
  } else if (gameState == 1) {
    if (!menu_drawn) {
      drawRunnerPauseMenu();
      menu_drawn = true;
    }
    handleRunnerPause();
  } else if (gameState == 2) {
    if (!menu_drawn) {
      drawRunnerGameOver();
      menu_drawn = true;
    }
    handleRunnerGameOver();
  } else if (gameState == -1) {
    int cmd = findCommand();
    if (cmd == 1 || cmd == 3) {
      runCharacter = (runCharacter + (cmd == 1 ? 1 : 2)) % 3;
      playNavClick();
      drawRunnerChoice();
    } else if (cmd == 4) {
      playSelectTone();
      initRunnerGame();
      gameState = 0;
    }
  }
}
# 1 "/home/pahasara/gameConsole/src/sliding_puzzle.ino"
#include "game.h"
#include "ui_kit.h"
#include "arcade_style.h"
#include <Arduino.h>





constexpr int PUZZLE_MAX_SIZE = 6;
constexpr int PUZZLE_Y = 62;

static int puzzleSelectedSize = 4;
static int puzzleSize = 4;
static int puzzleCount = 16;
static int puzzleTile = 46;
static int puzzleGap = 4;
static int puzzleX = 22;
static uint8_t puzzleBoard[PUZZLE_MAX_SIZE * PUZZLE_MAX_SIZE];
static int puzzleBlank = 15;
static int puzzleMoves = 0;
static int puzzleLastMoved = -1;
static int puzzlePauseSelection = 0;
static const char *const PUZZLE_PAUSE_ITEMS[] = {"Resume", "High scores", "Main menu"};
static bool puzzleCompleteRecorded = false;

static int puzzleRow(int index) { return index / puzzleSize; }
static int puzzleCol(int index) { return index % puzzleSize; }

static int puzzleColorBand(int value, int size) {
  return (value - 1) / size;
}

static uint16_t puzzleBandColor(int band) {
  constexpr uint16_t colors[6] = {0xF98B, 0xFD24, 0xFE64, 0x268C, 0x263F, 0xA33F};
  return colors[band];
}

static void setPuzzleGeometry() {
  puzzleSize = puzzleSelectedSize;
  puzzleCount = puzzleSize * puzzleSize;
  puzzleTile = puzzleSize == 4 ? 46 : (puzzleSize == 5 ? 36 : 30);
  puzzleGap = puzzleSize == 4 ? 4 : 3;
  puzzleX = (240 - puzzleSize * puzzleTile - (puzzleSize - 1) * puzzleGap) / 2;
}

static void drawPuzzleGoalPreview() {
  int size = puzzleSelectedSize;
  int tile = size == 4 ? 20 : (size == 5 ? 16 : 13);
  constexpr int gap = 1;
  int gridWidth = size * tile + (size - 1) * gap;
  int startX = (240 - gridWidth) / 2;
  constexpr int startY = 183;

  tft.fillRoundRect(63, 162, 114, 111, 10, ui::GROUP);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::GROUP);
  tft.drawString("Goal", 120, 172, 2);
  tft.setTextFont(1);

  for (int index = 0; index < size * size; ++index) {
    int row = index / size;
    int col = index % size;
    int x = startX + col * (tile + gap);
    int y = startY + row * (tile + gap);
    if (index == size * size - 1) {
      tft.drawRoundRect(x, y, tile, tile, 3, ui::SEPARATOR);
      continue;
    }
    int band = puzzleColorBand(index + 1, size);
    uint16_t fill = puzzleBandColor(band);
    tft.fillRoundRect(x, y, tile, tile, 3, fill);
    tft.setTextColor(ui::inkOn(fill), fill);
    tft.drawString(String(index + 1), x + tile / 2, y + tile / 2 + 1);
  }
}

static void puzzleTilePosition(int index, int &x, int &y) {
  x = puzzleX + puzzleCol(index) * (puzzleTile + puzzleGap);
  y = PUZZLE_Y + puzzleRow(index) * (puzzleTile + puzzleGap);
}

static void restorePuzzleBackdrop(int x, int y, int w, int h) {
  int x0 = max(0, x), x1 = min(240, x + w);
  int y0 = max(48, y), y1 = min(291, y + h);
  if (x0 >= x1 || y0 >= y1) return;
  uint16_t row[240];
  bool swap = tft.getSwapBytes();
  tft.setSwapBytes(true);
  for (int yy = y0; yy < y1; ++yy) {
    for (int xx = x0; xx < x1; ++xx) row[xx - x0] = arcadeStyle::backdrop(xx, yy);
    tft.pushImage(x0, yy, x1 - x0, 1, row);
  }
  tft.setSwapBytes(swap);
}

template<class Surface>
static void drawPuzzleTileAt(Surface &canvas, int x, int y, uint8_t value, bool highlighted) {
  if (value == 0) {
    canvas.fillRoundRect(x, y, puzzleTile, puzzleTile, 7, 0x10C9);
    canvas.drawRoundRect(x, y, puzzleTile, puzzleTile, 7, 0x3A52);
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextFont(1);
    canvas.setTextColor(UI_MUTED, 0x10C9);
    canvas.drawString(puzzleSize == 6 ? "GAP" : "EMPTY", x + puzzleTile / 2, y + puzzleTile / 2);
    return;
  }

  uint16_t tileColor = puzzleBandColor(puzzleColorBand(value, puzzleSize));
  canvas.fillRoundRect(x, y, puzzleTile, puzzleTile, 7, tileColor);
  canvas.drawRoundRect(x, y, puzzleTile, puzzleTile, 7, arcadeStyle::fade(tileColor, 135));
  canvas.drawFastHLine(x + 5, y + 2, puzzleTile - 10, arcadeStyle::mix(tileColor, 0xFFFF, 125));
  canvas.drawFastHLine(x + 7, y + 4, min(10, puzzleTile - 14), TFT_WHITE);
  canvas.drawFastHLine(x + 6, y + puzzleTile - 3, puzzleTile - 12, arcadeStyle::fade(tileColor, 135));

  char number[4];
  snprintf(number, sizeof(number), "%u", value);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextFont(puzzleSize == 6 ? 2 : 4);
  canvas.setTextColor(ui::inkOn(tileColor), tileColor);
  canvas.drawString(number, x + puzzleTile / 2, y + puzzleTile / 2 + 2);

  if (highlighted) {
    canvas.drawRoundRect(x, y, puzzleTile, puzzleTile, 7, 0x36FF);
    canvas.drawPixel(x + puzzleTile - 7, y + 5, TFT_WHITE);
    canvas.drawPixel(x + puzzleTile - 5, y + 7, TFT_WHITE);
  }
}

static void drawPuzzleTile(int index) {
  int x, y;
  puzzleTilePosition(index, x, y);
  TFT_eSprite tile(&tft);
  tile.setColorDepth(16);
  if (tile.createSprite(puzzleTile, puzzleTile)) {
    tile.setOrigin(-x, -y);
    for (int yy = y; yy < y + puzzleTile; ++yy)
      for (int xx = x; xx < x + puzzleTile; ++xx)
        tile.drawPixel(xx, yy, arcadeStyle::backdrop(xx, yy));
    drawPuzzleTileAt(tile, x, y, puzzleBoard[index], index == puzzleLastMoved);
    tile.pushSprite(x, y);
    tile.deleteSprite();
  } else {
    drawPuzzleTileAt(tft, x, y, puzzleBoard[index], index == puzzleLastMoved);
  }
}

static void animatePuzzleSlide(int fromIndex, int toIndex, uint8_t value) {
  int fromX, fromY, toX, toY;
  puzzleTilePosition(fromIndex, fromX, fromY);
  puzzleTilePosition(toIndex, toX, toY);
  int dirtyX = min(fromX, toX), dirtyY = min(fromY, toY);
  int dirtyW = abs(toX - fromX) + puzzleTile;
  int dirtyH = abs(toY - fromY) + puzzleTile;
  TFT_eSprite frame(&tft);
  frame.setColorDepth(16);
  bool buffered = frame.createSprite(dirtyW, dirtyH);
  for (int step = 1; step <= 5; ++step) {
    int x = fromX + (toX - fromX) * step / 5;
    int y = fromY + (toY - fromY) * step / 5;
    if (buffered) {
      frame.setOrigin(-dirtyX, -dirtyY);
      for (int yy = dirtyY; yy < dirtyY + dirtyH; ++yy)
        for (int xx = dirtyX; xx < dirtyX + dirtyW; ++xx)
          frame.drawPixel(xx, yy, arcadeStyle::backdrop(xx, yy));
      drawPuzzleTileAt(frame, fromX, fromY, 0, false);
      drawPuzzleTileAt(frame, toX, toY, 0, false);
      drawPuzzleTileAt(frame, x, y, value, step >= 3);
      frame.pushSprite(dirtyX, dirtyY);
    } else {
      restorePuzzleBackdrop(dirtyX, dirtyY, dirtyW, dirtyH);
      drawPuzzleTileAt(tft, fromX, fromY, 0, false);
      drawPuzzleTileAt(tft, toX, toY, 0, false);
      drawPuzzleTileAt(tft, x, y, value, step >= 3);
    }
    delay(6);
  }
  if (buffered) frame.deleteSprite();
}

static void drawPuzzleMoves() {
  TFT_eSprite count(&tft);
  count.setColorDepth(16);
  if (count.createSprite(62, 16)) {
    count.fillSprite(ui::CARD);
    count.setTextDatum(TR_DATUM);
    count.setTextColor(ui::LABEL, ui::CARD);
    count.drawString(String(puzzleMoves), 62, 0, 2);
    count.pushSprite(158, 24);
    count.deleteSprite();
    return;
  }
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.setTextPadding(40);
  tft.drawString(String(puzzleMoves), 220, 24, 2);
  tft.setTextPadding(0);
}

static void drawPuzzleHeader() {
  tft.fillRoundRect(8, 5, 224, 38, 12, ui::CARD);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString("Puzzle", 20, 25, 4);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString("Grid", 158, 8, 2);
  tft.drawString("Moves", 220, 8, 2);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.setTextPadding(40);
  tft.drawString(String(puzzleSize) + "x" + String(puzzleSize), 158, 24, 2);
  tft.setTextPadding(0);
  drawPuzzleMoves();
}

static void drawPuzzleBottomBar() {
  tft.fillRect(0, 288, 240, 32, UI_BG);
  ui::hint(304, "Shoot to pause", UI_BG);
}

static void drawPuzzleBoard() {
  if (!restoringCountdownDigit()) tft.fillScreen(UI_BG);
  restorePuzzleBackdrop(0, 48, 240, 243);
  drawPuzzleHeader();
  for (int i = 0; i < puzzleCount; i++) {
    int x, y;
    puzzleTilePosition(i, x, y);
    drawPuzzleTileAt(tft, x, y, puzzleBoard[i], i == puzzleLastMoved);
  }
  drawPuzzleBottomBar();
}

static bool slidePuzzleGap(int command, bool countMove) {
  int row = puzzleRow(puzzleBlank);
  int col = puzzleCol(puzzleBlank);
  int target = puzzleBlank;

  if (command == 0 && row > 0) target = puzzleBlank - puzzleSize;
  else if (command == 1 && col < puzzleSize - 1) target = puzzleBlank + 1;
  else if (command == 2 && row < puzzleSize - 1) target = puzzleBlank + puzzleSize;
  else if (command == 3 && col > 0) target = puzzleBlank - 1;
  else return false;

  int oldBlank = puzzleBlank;
  uint8_t moved = puzzleBoard[target];
  int oldHighlight = puzzleLastMoved;
  if (countMove) animatePuzzleSlide(target, oldBlank, moved);
  puzzleBoard[puzzleBlank] = moved;
  puzzleBoard[target] = 0;
  puzzleBlank = target;
  puzzleLastMoved = oldBlank;
  if (countMove) puzzleMoves++;

  if (countMove) {
    if (oldHighlight >= 0 && oldHighlight != oldBlank && oldHighlight != target)
      drawPuzzleTile(oldHighlight);
    drawPuzzleMoves();
  }
  return true;
}

static bool puzzleIsSolved() {
  for (int i = 0; i < puzzleCount - 1; i++)
    if (puzzleBoard[i] != i + 1) return false;
  return puzzleBoard[puzzleCount - 1] == 0;
}

static void shufflePuzzle() {
  for (int i = 0; i < puzzleCount - 1; i++) puzzleBoard[i] = i + 1;
  puzzleBoard[puzzleCount - 1] = 0;
  puzzleBlank = puzzleCount - 1;

  int previous = -1;
  for (int i = 0; i < 220 + (puzzleSize - 4) * 90; i++) {
    int choices[4];
    int choiceCount = 0;
    for (int command = 0; command < 4; command++) {
      if (command == previous) continue;
      int row = puzzleRow(puzzleBlank);
      int col = puzzleCol(puzzleBlank);
      bool valid = (command == 0 && row > 0) ||
                   (command == 1 && col < puzzleSize - 1) ||
                   (command == 2 && row < puzzleSize - 1) ||
                   (command == 3 && col > 0);
      if (valid) choices[choiceCount++] = command;
    }
    int command = choices[random(choiceCount)];
    int opposite = (command == 0) ? 2 : (command == 2 ? 0 : (command == 1 ? 3 : 1));
    slidePuzzleGap(command, false);
    previous = opposite;
  }
  if (puzzleIsSolved()) slidePuzzleGap(0, false);
  puzzleMoves = 0;
  puzzleLastMoved = -1;
}

static int puzzleSerialCommand() {
  if (!Serial.available()) return -1;
  char c = Serial.read();
  if (c == 'w' || c == 'W') return 0;
  if (c == 'd' || c == 'D') return 1;
  if (c == 's' || c == 'S') return 2;
  if (c == 'a' || c == 'A') return 3;
  if (c == ' ' || c == '\n' || c == '\r' || c == 'e' || c == 'E' || c == 'x' || c == 'X') return 4;
  return -1;
}


static void drawPuzzleStartChoices() {
  tft.fillRect(28, 92, 184, 64, ui::CARD);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString("Grid size", 30, 96, 2);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString(String(puzzleSelectedSize * puzzleSelectedSize - 1) + " tiles", 210, 96, 2);
  tft.fillRoundRect(30, 118, 180, 32, 10, ui::GROUP);
  for (int i = 0; i < 3; ++i) {
    int size = i + 4, x = 32 + i * 59;
    bool selected = size == puzzleSelectedSize;
    if (selected) tft.fillRoundRect(x, 120, 58, 28, 8, UI_CYAN);
    else if (i > 0 && size - 1 != puzzleSelectedSize) tft.drawFastVLine(x - 1, 126, 16, ui::SEPARATOR);
    tft.setTextDatum(MC_DATUM);
    uint16_t fill = selected ? UI_CYAN : ui::GROUP;
    tft.setTextColor(selected ? ui::inkOn(UI_CYAN) : ui::LABEL, fill);
    tft.drawString(String(size) + " x " + String(size), x + 29, 135, 2);
  }
  drawPuzzleGoalPreview();
}

static void drawPuzzleStartScreen() {
  tft.fillScreen(UI_BG);
  restorePuzzleBackdrop(0, 48, 240, 243);
  ui::card(14, 24, 212, 284);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString("Puzzle", 30, 38, 4);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString("Slide the tiles into order", 30, 66, 2);
  drawPuzzleStartChoices();
  ui::hint(286, "Shoot to start", ui::CARD);
}

static void drawPuzzlePause() {
  ui::pauseSheet("Puzzle", PUZZLE_PAUSE_ITEMS, 3, puzzlePauseSelection, UI_CYAN);
}

static void drawPuzzleComplete() {
  ui::card(24, 66, 192, 186);
  ui::heading(40, 80, "Puzzle", "Solved");
  const ui::List stats = {32, 130, 176, 24, 2};
  ui::infoRow(stats, 0, "Moves", String(puzzleMoves).c_str());
  String grid = String(puzzleSize) + "x" + String(puzzleSize);
  ui::infoRow(stats, 1, "Grid", grid.c_str());
  ui::pill(40, 188, 160, 30, "Play again", UI_CYAN);
  ui::hint(234, "Up for home screen", ui::CARD);
}

static void startPuzzle() {
  audio::restartMusic();
  gameState = 0;
  menu_drawn = false;
  puzzleCompleteRecorded = false;
  setPuzzleGeometry();
  shufflePuzzle();
  drawPuzzleBoard();
  delay(180);
}

void puzzleSetup() {
  tft.setRotation(2);
  gameState = -1;
  puzzleSelectedSize = 4;
  setPuzzleGeometry();
  puzzlePauseSelection = 0;
  drawPuzzleStartScreen();
}

void puzzleLoop() {
  audio::gameState(gameState);
  int command = findCommand();
  int serialCommand = puzzleSerialCommand();
  if (serialCommand >= 0) command = serialCommand;

  if (gameState == -1) {
    if (command == 4) { playSelectTone(); startPuzzle(); }
    else if (command == 0) switchGame(0);
    else if (command == 1 && puzzleSelectedSize < 6) {
      puzzleSelectedSize++;
      playNavClick();
      drawPuzzleStartChoices();
    } else if (command == 3 && puzzleSelectedSize > 4) {
      puzzleSelectedSize--;
      playNavClick();
      drawPuzzleStartChoices();
    }
    return;
  }

  if (gameState == 0) {
    if (command >= 0 && command <= 3) {
      if (slidePuzzleGap(command, true)) {
        audio::play(audio::Cue::Slide);
        if (puzzleIsSolved()) {
          gameState = 2;
          menu_drawn = false;
          audio::play(audio::Cue::Win);
        }
      } else audio::play(audio::Cue::Error);
    } else if (command == 4) {
      puzzlePauseSelection = 0;
      audio::pauseMusic(true); gameState = 1;
      menu_drawn = false;
    }
  } else if (gameState == 1) {
    if (!menu_drawn) { drawPuzzlePause(); menu_drawn = true; }
    if (command == 0 || command == 2) {
      puzzlePauseSelection = (puzzlePauseSelection + (command == 0 ? 2 : 1)) % 3;
      drawPuzzlePause();
      playNavClick();
    } else if (command == 4) {
      playSelectTone();
      if (puzzlePauseSelection == 0) {
        showResumeCountdown(drawPuzzleBoard);
        gameState = 0;
        menu_drawn = false;
      } else if (puzzlePauseSelection == 1) {
        showGameHighScores();
        drawPuzzleBoard();
        drawPuzzlePause();
      } else switchGame(0);
    }
  } else if (gameState == 2) {
    if (!puzzleCompleteRecorded) {
      currentGame.recordScore(max(1, 10000 - puzzleMoves));
      puzzleCompleteRecorded = true;
      drawPuzzleComplete();
    }
    if (command == 4) { playSelectTone(); startPuzzle(); }
    else if (command == 0) switchGame(0);
  }
}
# 1 "/home/pahasara/gameConsole/src/snake.ino"
#include "game.h"
#include "ui_kit.h"
#include <Arduino.h>
#include "snake_background_240x320.h"





constexpr int PLAYFIELD_X = 0;
constexpr int PLAYFIELD_Y = 48;
constexpr int PLAYFIELD_W = 240;
constexpr int PLAYFIELD_H = 240;
constexpr int BLOCK_SIZE = 10;
constexpr int GRID_COLS = PLAYFIELD_W / BLOCK_SIZE;
constexpr int GRID_ROWS = PLAYFIELD_H / BLOCK_SIZE;
constexpr int MAX_SNAKE_LEN = GRID_COLS * GRID_ROWS;

static int snakeWallLayout = 0;
static const char *const SNAKE_WALL_NAMES[] = {
  "No walls", "Twin bars", "Four corners", "Cross"
};


static bool snakeWallAt(int x, int y) {
  switch (snakeWallLayout) {
    case 1: return (y == 7 || y == 17) && x >= 6 && x <= 17;
    case 2: {
      int dx = min(x, GRID_COLS - 1 - x);
      int dy = min(y, GRID_ROWS - 1 - y);
      return (dx == 4 && dy >= 4 && dy <= 8) ||
             (dy == 4 && dx >= 4 && dx <= 8);
    }
    case 3: return (x == 12 && y >= 8 && y <= 16) ||
                   (y == 12 && x >= 10 && x <= 14);
    default: return false;
  }
}

static void drawSnakeBrick(int x, int y, int size) {
  tft.fillRect(x, y, size, size, 0x6103);
  tft.drawFastHLine(x, y, size, 0x9226);
  tft.drawFastVLine(x, y, size, 0x9226);
  if (size >= 4) {
    tft.drawFastHLine(x, y + size / 2, size, 0x2801);
    tft.drawFastVLine(x + size / 2, y, size / 2, 0x2801);
    tft.drawFastVLine(x + size / 3, y + size / 2, size / 2, 0x2801);
  }
}

#define IOS_BG UI_BG
constexpr uint16_t IOS_GREEN = 0x2EE9;
constexpr uint16_t IOS_GREEN_END = 0xAFE0;
constexpr uint16_t IOS_RED = 0xFA48;
constexpr uint16_t IOS_AMBER = 0xFE64;


constexpr uint16_t SNAKE_HEAD = 0x001F;
constexpr uint16_t SNAKE_BODY_1 = 0x10B0;
constexpr uint16_t SNAKE_BODY_2 = 0x03BF;

static uint16_t snakeMix(uint16_t a, uint16_t b, int amount) {
  int r = (((a >> 11) & 31) * (255 - amount) + ((b >> 11) & 31) * amount) / 255;
  int g = (((a >> 5) & 63) * (255 - amount) + ((b >> 5) & 63) * amount) / 255;
  int blue = ((a & 31) * (255 - amount) + (b & 31) * amount) / 255;
  return (uint16_t)((r << 11) | (g << 5) | blue);
}


static inline uint16_t grassNativeAt(int sx, int sy) {
  return pgm_read_word(&snake_background_240x320[sy * 240 + sx]);
}




struct SnakeParticle {
  float x, y;
  float speedX, speedY;
  uint16_t color;
  int life;
};

struct SnakeScorePopup {
  int x;
  int y;
  int prevY;
  int w, h;
  int value;
  int life;
  int maxLife;
  bool drawn;
};




static Vec2 snakeBody[MAX_SNAKE_LEN];
static int snakeLength = 4;
static int currentDirection = 1;
static int lastDirection = 1;
static int queuedDirection = -1;
static Vec2 foodPos;
static unsigned long lastMoveTime = 0;
static unsigned long lastFxTime = 0;
static int gameSpeed_ms = 210;
static int snakeCombo = 0;
static unsigned long lastEatMs = 0;

static int snakePauseSelection = 0;
static const char *const SNAKE_PAUSE_ITEMS[] = {"Resume", "Restart", "High scores", "Main menu"};
static bool snakePauseReady = false;
static bool snakeShootReady = false;


static SnakeParticle particles[16];
static int particleCount = 0;
static SnakeScorePopup scorePopup = {0, 0, 0, 28, 14, 1, 0, 18, false};




static void playBiteSound() {
  audio::play(audio::Cue::Pickup);
}

static void playDeathSound() {
  audio::play(audio::Cue::GameOver);
}




static void drawGrassyCell(int gx, int gy) {
  if (gx < 0 || gx >= GRID_COLS || gy < 0 || gy >= GRID_ROWS) return;
  int px = PLAYFIELD_X + gx * BLOCK_SIZE;
  int py = PLAYFIELD_Y + gy * BLOCK_SIZE;
  if (snakeWallAt(gx, gy)) {
    drawSnakeBrick(px, py, BLOCK_SIZE);
    return;
  }
  static uint16_t tile[BLOCK_SIZE * BLOCK_SIZE];
  for (int yy = 0; yy < BLOCK_SIZE; yy++) {
    for (int xx = 0; xx < BLOCK_SIZE; xx++) {
      tile[yy * BLOCK_SIZE + xx] =
          pgm_read_word(&snake_background_240x320[(py + yy) * 240 + px + xx]);
    }
  }
  tft.setSwapBytes(true);
  tft.pushImage(px, py, BLOCK_SIZE, BLOCK_SIZE, tile);
  tft.setSwapBytes(false);
}

static void renderFullGrassBoard() {
  tft.setSwapBytes(true);
  tft.pushImage(0, PLAYFIELD_Y, PLAYFIELD_W, PLAYFIELD_H,
                &snake_background_240x320[PLAYFIELD_Y * 240]);
  tft.setSwapBytes(false);
  for (int y = 0; y < GRID_ROWS; ++y)
    for (int x = 0; x < GRID_COLS; ++x)
      if (snakeWallAt(x, y))
        drawSnakeBrick(x * BLOCK_SIZE, PLAYFIELD_Y + y * BLOCK_SIZE, BLOCK_SIZE);
  tft.drawFastHLine(0, PLAYFIELD_Y - 1, PLAYFIELD_W, 0x0540);
  tft.drawFastHLine(0, PLAYFIELD_Y + PLAYFIELD_H, PLAYFIELD_W, 0x0540);
}




static void drawModernHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(224, 40)) return;
  header.fillSprite(IOS_BG);
  header.fillRoundRect(0, 0, 224, 40, 12, ui::CARD);


  header.fillRoundRect(8, 8, 24, 24, 7, 0x1145);
  header.fillRoundRect(11, 16, 8, 8, 2, SNAKE_BODY_2);
  header.fillRoundRect(18, 16, 9, 8, 2, SNAKE_HEAD);
  header.fillCircle(24, 18, 1, 0xFFFF);
  header.fillCircle(24, 22, 1, 0xFFFF);

  header.setTextDatum(TL_DATUM);
  header.setTextColor(ui::LABEL, ui::CARD);
  header.drawString("Snake", 38, 4, 2);

  if (snakeCombo >= 2 && gameState == 0) {
    String cStr = "x" + String(snakeCombo) + " Nice!";
    int cw = header.textWidth(cStr, 2) + 10;
    header.fillRoundRect(38, 21, cw, 15, 7, IOS_AMBER);
    header.setTextDatum(MC_DATUM);
    header.setTextColor(ui::INK_DARK, IOS_AMBER);
    header.drawString(cStr, 38 + cw / 2, 29, 2);
  } else {
    header.setTextDatum(TL_DATUM);
    header.setTextColor(IOS_GREEN, ui::CARD);
    header.drawString(SNAKE_WALL_NAMES[snakeWallLayout], 38, 21, 2);
  }

  header.setTextDatum(TR_DATUM);
  header.setTextColor(ui::SECONDARY, ui::CARD);
  header.drawString("Best", 160, 3, 2);
  header.setTextColor(IOS_GREEN_END, ui::CARD);
  header.drawString("Score", 214, 3, 2);

  int bestShown = max(score, currentGame.highScores[0]);
  header.setTextColor(IOS_AMBER, ui::CARD);
  header.drawString(String(bestShown), 160, 20, 2);

  String scStr(score);
  int scFont = header.textWidth(scStr, 4) <= 48 ? 4 : 2;
  header.setTextColor(ui::LABEL, ui::CARD);
  header.drawString(scStr, 214, scFont == 4 ? 16 : 20, scFont);

  header.pushSprite(8, 4);
  header.deleteSprite();
}

static void updateScoreHUD() {
  drawModernHeader();
}

static void drawBottomControlBar() {
  tft.fillRect(0, 290, 240, 30, IOS_BG);
  ui::hint(305, "D-pad steer  /  Shoot pause", IOS_BG);
}




static int snakeNeighborDirection(Vec2 from, Vec2 to) {
  if (to.x == from.x && to.y == (from.y + GRID_ROWS - 1) % GRID_ROWS) return 0;
  if (to.y == from.y && to.x == (from.x + 1) % GRID_COLS) return 1;
  if (to.x == from.x && to.y == (from.y + 1) % GRID_ROWS) return 2;
  if (to.y == from.y && to.x == (from.x + GRID_COLS - 1) % GRID_COLS) return 3;
  return -1;
}

void drawSnakeHead() {
  Vec2 head = snakeBody[0];
  int px = PLAYFIELD_X + head.x * BLOCK_SIZE;
  int py = PLAYFIELD_Y + head.y * BLOCK_SIZE;

  drawGrassyCell(head.x, head.y);


  int neckDir = (snakeLength > 1) ? snakeNeighborDirection(head, snakeBody[1]) : -1;
  tft.fillRoundRect(px, py, BLOCK_SIZE, BLOCK_SIZE, 3, SNAKE_HEAD);
  if (neckDir == 0) tft.fillRect(px, py, BLOCK_SIZE, 4, SNAKE_HEAD);
  else if (neckDir == 2) tft.fillRect(px, py + BLOCK_SIZE - 4, BLOCK_SIZE, 4, SNAKE_HEAD);
  else if (neckDir == 3) tft.fillRect(px, py, 4, BLOCK_SIZE, SNAKE_HEAD);
  else if (neckDir == 1) tft.fillRect(px + BLOCK_SIZE - 4, py, 4, BLOCK_SIZE, SNAKE_HEAD);


  int e1x = px + 2, e1y = py + 3;
  int e2x = px + 7, e2y = py + 3;
  int pdx = 0, pdy = 0;

  switch (currentDirection) {
    case 0:
      e1x = px + 2; e1y = py + 3;
      e2x = px + 7; e2y = py + 3;
      pdy = -1;
      break;
    case 2:
      e1x = px + 2; e1y = py + 6;
      e2x = px + 7; e2y = py + 6;
      pdy = 1;
      break;
    case 3:
      e1x = px + 3; e1y = py + 2;
      e2x = px + 3; e2y = py + 7;
      pdx = -1;
      break;
    case 1:
      e1x = px + 6; e1y = py + 2;
      e2x = px + 6; e2y = py + 7;
      pdx = 1;
      break;
  }

  tft.fillCircle(e1x, e1y, 2, 0xFFFF);
  tft.fillCircle(e2x, e2y, 2, 0xFFFF);
  tft.drawPixel(e1x + pdx, e1y + pdy, 0x0000);
  tft.drawPixel(e2x + pdx, e2y + pdy, 0x0000);
}


static void drawSnakeSegment(int index) {
  if (index <= 0 || index >= snakeLength) return;
  Vec2 seg = snakeBody[index];
  int px = PLAYFIELD_X + seg.x * BLOCK_SIZE;
  int py = PLAYFIELD_Y + seg.y * BLOCK_SIZE;
  uint16_t col = ((seg.x + seg.y) & 1) ? SNAKE_BODY_1 : SNAKE_BODY_2;
  int towardHead = snakeNeighborDirection(seg, snakeBody[index - 1]);
  int towardTail = index + 1 < snakeLength
                       ? snakeNeighborDirection(seg, snakeBody[index + 1]) : -1;
  if (towardHead < 0) return;

  if ((towardHead + 2) % 4 == towardTail) {
    tft.fillRect(px, py, BLOCK_SIZE, BLOCK_SIZE, col);
    return;
  }


  static uint16_t tile[BLOCK_SIZE * BLOCK_SIZE];
  const int cornerX = (towardHead == 1 || towardTail == 1) ? 20 : 0;
  const int cornerY = (towardHead == 2 || towardTail == 2) ? 20 : 0;
  for (int y = 0; y < BLOCK_SIZE; ++y) {
    for (int x = 0; x < BLOCK_SIZE; ++x) {
      bool body;
      if (towardTail < 0) {
        int distance = towardHead == 0 ? y : towardHead == 1 ? 9 - x
                     : towardHead == 2 ? 9 - y : x;
        int across = (towardHead == 0 || towardHead == 2) ? x : y;
        int width = 9 - distance;
        body = width > 0 && abs(across * 2 - 9) <= width;
      } else {
        int dx = x * 2 + 1 - cornerX;
        int dy = y * 2 + 1 - cornerY;
        body = dx * dx + dy * dy <= 400;
      }
      tile[y * BLOCK_SIZE + x] = body
          ? col
          : pgm_read_word(&snake_background_240x320[(py + y) * 240 + px + x]);
    }
  }
  tft.setSwapBytes(true);
  tft.pushImage(px, py, BLOCK_SIZE, BLOCK_SIZE, tile);
  tft.setSwapBytes(false);
}

static void drawApple() {
  int px = PLAYFIELD_X + foodPos.x * BLOCK_SIZE;
  int py = PLAYFIELD_Y + foodPos.y * BLOCK_SIZE;

  drawGrassyCell(foodPos.x, foodPos.y);


  tft.fillCircle(px + 4, py + 5, 4, 0xF800);
  tft.fillCircle(px + 6, py + 5, 3, 0xD800);
  tft.drawLine(px + 5, py + 1, px + 5, py + 2, 0x4A00);
  tft.fillTriangle(px + 5, py + 2, px + 8, py + 1, px + 7, py + 3, IOS_GREEN);
  tft.drawPixel(px + 3, py + 3, 0xFFFF);
  tft.drawPixel(px + 3, py + 4, 0xFFE0);
}


static void restorePlayfieldRect(int rx, int ry, int rw, int rh) {
  int gx0 = max(0, (rx - PLAYFIELD_X) / BLOCK_SIZE);
  int gx1 = min(GRID_COLS - 1, (rx + rw - 1 - PLAYFIELD_X) / BLOCK_SIZE);
  int gy0 = max(0, (ry - PLAYFIELD_Y) / BLOCK_SIZE);
  int gy1 = min(GRID_ROWS - 1, (ry + rh - 1 - PLAYFIELD_Y) / BLOCK_SIZE);
  if (gx0 > gx1 || gy0 > gy1) return;

  for (int gy = gy0; gy <= gy1; ++gy) {
    for (int gx = gx0; gx <= gx1; ++gx) {
      drawGrassyCell(gx, gy);
    }
  }

  if (foodPos.x >= gx0 && foodPos.x <= gx1 && foodPos.y >= gy0 && foodPos.y <= gy1) {
    drawApple();
  }

  for (int i = 1; i < snakeLength; ++i) {
    if (snakeBody[i].x >= gx0 && snakeBody[i].x <= gx1 &&
        snakeBody[i].y >= gy0 && snakeBody[i].y <= gy1) {
      drawSnakeSegment(i);
    }
  }
  if (snakeBody[0].x >= gx0 && snakeBody[0].x <= gx1 &&
      snakeBody[0].y >= gy0 && snakeBody[0].y <= gy1) {
    drawSnakeHead();
  }
}




static void drawTinyDigit(TFT_eSprite &sp, int x, int y, int d, uint16_t col) {
  static const uint8_t glyphs[10][5] = {
    {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1},
    {7,4,7,1,7}, {7,4,7,5,7}, {7,1,2,2,2}, {7,5,7,5,7}, {7,5,7,1,7}
  };
  if (d < 0 || d > 9) return;
  for (int r = 0; r < 5; ++r)
    for (int c = 0; c < 3; ++c)
      if (glyphs[d][r] & (4 >> c)) sp.fillRect(x + c * 2, y + r * 2, 2, 2, col);
}

static void eraseScorePopup() {
  if (!scorePopup.drawn) return;
  restorePlayfieldRect(scorePopup.x, scorePopup.prevY, scorePopup.w, scorePopup.h);
  scorePopup.drawn = false;
}

static void spawnScorePopup(int gx, int gy, int val) {
  eraseScorePopup();

  int digits = (val >= 100) ? 3 : (val >= 10 ? 2 : 1);
  int w = 14 + digits * 8;
  int h = 14;
  int cx = PLAYFIELD_X + gx * BLOCK_SIZE + BLOCK_SIZE / 2;
  int cy = PLAYFIELD_Y + gy * BLOCK_SIZE - 6;

  int px = constrain(cx - w / 2, 4, PLAYFIELD_W - w - 4);
  int py = constrain(cy, PLAYFIELD_Y + 18, PLAYFIELD_Y + PLAYFIELD_H - h - 4);

  scorePopup = {px, py, py, w, h, val, 18, 18, false};
}

static void renderScorePopupStep() {
  if (scorePopup.life <= 0) {
    eraseScorePopup();
    return;
  }


  if (scorePopup.drawn) {
    restorePlayfieldRect(scorePopup.x, scorePopup.prevY, scorePopup.w, scorePopup.h);
  }

  TFT_eSprite sp(&tft);
  sp.setColorDepth(16);
  if (sp.createSprite(scorePopup.w, scorePopup.h)) {

    for (int yy = 0; yy < scorePopup.h; ++yy) {
      for (int xx = 0; xx < scorePopup.w; ++xx) {
        int sx = scorePopup.x + xx;
        int sy = scorePopup.y + yy;
        sp.drawPixel(xx, yy, grassNativeAt(sx, sy));
      }
    }

    int fade = min(255, scorePopup.life * 20);
    uint16_t ink = snakeMix(grassNativeAt(scorePopup.x, scorePopup.y),
                            scorePopup.value > 10 ? IOS_AMBER : IOS_GREEN_END, fade);
    int gx = 3, gy = 3;
    sp.fillRect(gx, gy + 4, 6, 2, ink);
    sp.fillRect(gx + 2, gy + 2, 2, 6, ink);
    gx += 9;
    int v = scorePopup.value;
    if (v >= 100) { drawTinyDigit(sp, gx, gy, (v / 100) % 10, ink); gx += 8; }
    if (v >= 10) { drawTinyDigit(sp, gx, gy, (v / 10) % 10, ink); gx += 8; }
    drawTinyDigit(sp, gx, gy, v % 10, ink);

    sp.pushSprite(scorePopup.x, scorePopup.y);
    sp.deleteSprite();

    scorePopup.prevY = scorePopup.y;
    scorePopup.drawn = true;
  }

  scorePopup.y = max(PLAYFIELD_Y + 2, scorePopup.y - 1);
  scorePopup.life--;
  if (scorePopup.life == 0) {
    eraseScorePopup();
  }
}

static bool cellHasSnakeOrApple(int gx, int gy) {
  if (gx == foodPos.x && gy == foodPos.y) return true;
  for (int i = 0; i < snakeLength; ++i)
    if (snakeBody[i].x == gx && snakeBody[i].y == gy) return true;
  return false;
}

static void createAppleParticles(int gx, int gy) {
  int cx = PLAYFIELD_X + gx * BLOCK_SIZE + 5;
  int cy = PLAYFIELD_Y + gy * BLOCK_SIZE + 5;
  particleCount = 0;
  for (int i = 0; i < 12; i++) {
    particles[i].x = cx;
    particles[i].y = cy;
    float angle = (i / 12.0f) * 6.283f;
    float speed = random(12, 26) / 10.0f;
    particles[i].speedX = cosf(angle) * speed;
    particles[i].speedY = sinf(angle) * speed;
    particles[i].color = (i % 3 == 0) ? IOS_AMBER : ((i & 1) ? IOS_RED : IOS_GREEN_END);
    particles[i].life = random(6, 12);
    particleCount++;
  }
}

void updateParticles() {
  int active = 0;
  for (int i = 0; i < particleCount; i++) {
    if (particles[i].life > 0) {
      int prevX = (int)particles[i].x;
      int prevY = (int)particles[i].y;
      int pgx = prevX / BLOCK_SIZE;
      int pgy = (prevY - PLAYFIELD_Y) / BLOCK_SIZE;

      if (prevX >= PLAYFIELD_X && prevX < PLAYFIELD_X + PLAYFIELD_W &&
          prevY >= PLAYFIELD_Y && prevY < PLAYFIELD_Y + PLAYFIELD_H &&
          !snakeWallAt(pgx, pgy) && !cellHasSnakeOrApple(pgx, pgy)) {
        tft.drawPixel(prevX, prevY, grassNativeAt(prevX, prevY));
      }

      particles[i].x += particles[i].speedX;
      particles[i].y += particles[i].speedY;
      particles[i].life--;

      int nx = (int)particles[i].x;
      int ny = (int)particles[i].y;
      int ngx = nx / BLOCK_SIZE;
      int ngy = (ny - PLAYFIELD_Y) / BLOCK_SIZE;
      if (particles[i].life > 0 &&
          nx >= PLAYFIELD_X && nx < PLAYFIELD_X + PLAYFIELD_W &&
          ny >= PLAYFIELD_Y && ny < PLAYFIELD_Y + PLAYFIELD_H &&
          !snakeWallAt(ngx, ngy) && !cellHasSnakeOrApple(ngx, ngy)) {
        tft.drawPixel(nx, ny, particles[i].color);
        active++;
      }
    }
  }
  if (active == 0) particleCount = 0;
}




void generateFood() {
  int available = 0;
  for (int y = 0; y < GRID_ROWS; ++y) {
    for (int x = 0; x < GRID_COLS; ++x) {
      if (snakeWallAt(x, y)) continue;
      bool occupied = false;
      for (int i = 0; i < snakeLength; ++i)
        if (snakeBody[i].x == x && snakeBody[i].y == y) occupied = true;
      if (!occupied && random(++available) == 0) foodPos = {x, y};
    }
  }
  if (!available) { audio::play(audio::Cue::Win); gameState = 2; menu_drawn = false; }
}

bool checkSelfCollision(Vec2 head) {
  for (int i = 1; i < snakeLength; i++) {
    if (head.x == snakeBody[i].x && head.y == snakeBody[i].y) {
      return true;
    }
  }
  return false;
}




void handleInput() {
  if (readShoot() == LOW) snakeShootReady = true;

  int cmd;
  while ((cmd = buttonsNextPress()) >= 0) {
    if (cmd == 4 && snakeShootReady) {
      audio::pauseMusic(true); gameState = 1;
      menu_drawn = false;
      snakePauseSelection = 0;
      snakePauseReady = false;
      return;
    }
    if (cmd >= 0 && cmd < 4) {
      int baseDir = (queuedDirection >= 0) ? queuedDirection : lastDirection;
      if (abs(cmd - baseDir) != 2 && cmd != baseDir) {
        if (queuedDirection < 0 && abs(cmd - lastDirection) != 2) {
          currentDirection = cmd;
          queuedDirection = cmd;
        } else {
          queuedDirection = cmd;
        }
      }
    }
  }
}

void moveSnake() {
  if (queuedDirection >= 0 && abs(queuedDirection - lastDirection) != 2) {
    currentDirection = queuedDirection;
  }
  queuedDirection = -1;

  Vec2 oldTail = snakeBody[snakeLength - 1];

  for (int i = snakeLength - 1; i > 0; i--) {
    snakeBody[i] = snakeBody[i - 1];
  }

  Vec2 newHead = snakeBody[0];
  switch (currentDirection) {
    case 0: newHead.y--; break;
    case 1: newHead.x++; break;
    case 2: newHead.y++; break;
    case 3: newHead.x--; break;
  }


  if (newHead.x < 0) newHead.x = GRID_COLS - 1;
  if (newHead.x >= GRID_COLS) newHead.x = 0;
  if (newHead.y < 0) newHead.y = GRID_ROWS - 1;
  if (newHead.y >= GRID_ROWS) newHead.y = 0;

  snakeBody[0] = newHead;
  lastDirection = currentDirection;

  if (snakeWallAt(newHead.x, newHead.y) || checkSelfCollision(newHead)) {
    eraseScorePopup();
    playDeathSound();
    gameState = 2;
    menu_drawn = false;
    return;
  }

  if (newHead.x == foodPos.x && newHead.y == foodPos.y) {
    int eatenX = foodPos.x;
    int eatenY = foodPos.y;

    if (snakeLength < MAX_SNAKE_LEN) {
      snakeBody[snakeLength] = oldTail;
      snakeLength++;
    }

    unsigned long now = millis();
    if (now - lastEatMs <= 3800) {
      snakeCombo = min(5, snakeCombo + 1);
    } else {
      snakeCombo = 1;
    }
    lastEatMs = now;

    int pts = 10 * snakeCombo;
    score += pts;

    createAppleParticles(eatenX, eatenY);
    playBiteSound();
    if ((snakeLength - 4) % 10 == 0) audio::play(audio::Cue::Milestone);
    drawGrassyCell(newHead.x, newHead.y);

    if (gameSpeed_ms > 90) {
      gameSpeed_ms -= 5;
    }

    updateScoreHUD();
    generateFood();
    if (gameState == 0) drawApple();


    spawnScorePopup(eatenX, eatenY, pts);
  } else {
    drawGrassyCell(oldTail.x, oldTail.y);
  }

  drawSnakeHead();
  if (snakeLength > 1) {
    drawSnakeSegment(1);
    if (snakeLength > 2) drawSnakeSegment(snakeLength - 1);
  }
}




static void renderIntroGrassPreview() {

  constexpr int PX = 44, PY = 116, PW = 152, PH = 76;
  tft.drawRoundRect(PX - 2, PY - 2, PW + 4, PH + 4, 8, ui::SEPARATOR);

  uint16_t previewRow[PW];
  tft.setSwapBytes(true);
  for (int y = 0; y < PH; ++y) {
    int srcY = PLAYFIELD_Y + y * PLAYFIELD_H / PH;
    for (int x = 0; x < PW; ++x) {
      int srcX = x * PLAYFIELD_W / PW;
      previewRow[x] = pgm_read_word(&snake_background_240x320[srcY * 240 + srcX]);
    }
    tft.pushImage(PX, PY + y, PW, 1, previewRow);
  }
  tft.setSwapBytes(false);


  for (int y = 0; y < GRID_ROWS; ++y) {
    for (int x = 0; x < GRID_COLS; ++x) {
      if (snakeWallAt(x, y)) {
        int bx = PX + x * PW / GRID_COLS;
        int by = PY + y * PH / GRID_ROWS;
        drawSnakeBrick(bx, by, 5);
      }
    }
  }


  int sy = PY + 42;
  int sx = PX + 26;

  tft.fillTriangle(sx, sy + 3, sx + 6, sy, sx + 6, sy + 6, SNAKE_BODY_2);

  for (int i = 0; i < 4; ++i) {
    tft.fillRect(sx + 6 + i * 7, sy, 7, 7, (i & 1) ? SNAKE_BODY_1 : SNAKE_BODY_2);
  }

  int hx = sx + 6 + 4 * 7;
  tft.fillRoundRect(hx, sy, 8, 7, 2, SNAKE_HEAD);
  tft.fillRect(hx, sy, 3, 7, SNAKE_HEAD);
  tft.fillCircle(hx + 5, sy + 1, 1, 0xFFFF);
  tft.fillCircle(hx + 5, sy + 5, 1, 0xFFFF);


  int ax = PX + 112, ay = PY + 44;
  tft.fillCircle(ax, ay, 4, 0xF800);
  tft.fillTriangle(ax, ay - 4, ax + 3, ay - 5, ax + 2, ay - 2, IOS_GREEN);
  tft.drawPixel(ax - 1, ay - 1, 0xFFFF);

  tft.fillRoundRect(ax - 13, ay - 21, 28, 13, 5, 0x0943);
  tft.drawRoundRect(ax - 13, ay - 21, 28, 13, 5, IOS_GREEN_END);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(0xFFFF, 0x0943);
  tft.drawString("+10", ax + 1, ay - 14, 2);
}

static void renderIntroSelectorRow() {
  tft.fillRoundRect(32, 200, 176, 28, 10, ui::GROUP);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(IOS_GREEN, ui::GROUP);
  tft.drawString("<", 42, 214, 2);
  tft.setTextDatum(MR_DATUM);
  tft.drawString(">", 198, 214, 2);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(ui::LABEL, ui::GROUP);
  tft.drawString(SNAKE_WALL_NAMES[snakeWallLayout], 120, 214, 2);


  for (int i = 0; i < 4; ++i) {
    int dx = 102 + i * 12;
    tft.fillCircle(dx, 236, 3, ui::CARD);
    tft.fillCircle(dx, 236, (i == snakeWallLayout) ? 3 : 2,
                   (i == snakeWallLayout) ? IOS_GREEN : ui::SEPARATOR);
  }
}

void showSnakeMenu() {
  gameState = -1;
  tft.setRotation(2);
  tft.fillScreen(IOS_BG);
  drawModernHeader();

  ui::card(16, 50, 208, 244);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString("Snake", 32, 62, 4);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString("Eat apples and avoid walls", 32, 90, 2);

  if (currentGame.highScores[0] > 0) {
    String bestChip = "Best " + String(currentGame.highScores[0]);
    ui::chip(208 - tft.textWidth(bestChip, 2) - 8, 64, bestChip.c_str(), IOS_AMBER);
  }

  renderIntroGrassPreview();
  renderIntroSelectorRow();

  ui::gradientPill(32, 248, 176, 32, "Press Shoot to play", IOS_GREEN, IOS_GREEN_END);
  ui::hint(306, "D-pad: choose stage", IOS_BG);
}

void initGame() {
  audio::restartMusic();
  gameState = 0;
  snakeLength = 4;
  currentDirection = 1;
  lastDirection = 1;
  queuedDirection = -1;
  score = 0;
  snakeCombo = 0;
  lastEatMs = 0;
  gameSpeed_ms = 210;
  particleCount = 0;
  scorePopup.life = 0;
  scorePopup.drawn = false;
  snakeShootReady = false;

  tft.fillScreen(IOS_BG);
  drawModernHeader();
  renderFullGrassBoard();
  drawBottomControlBar();

  int startX = 6;
  int startY = 12;
  for (int i = 0; i < snakeLength; i++) {
    snakeBody[i] = { startX - i, startY };
  }

  for (int i = 1; i < snakeLength; i++) {
    drawSnakeSegment(i);
  }
  drawSnakeHead();

  generateFood();
  drawApple();
  lastMoveTime = millis();
  lastFxTime = millis();
}

static void redrawFullSnakeScene() {
  if (!restoringCountdownDigit()) {
    tft.fillScreen(IOS_BG);
    drawModernHeader();
    drawBottomControlBar();
  }
  renderFullGrassBoard();
  drawApple();
  for (int i = 1; i < snakeLength; ++i) drawSnakeSegment(i);
  drawSnakeHead();
}

static void drawSnakePauseOption(int item) {
  ui::pauseItem(SNAKE_PAUSE_ITEMS, 4, item, snakePauseSelection, IOS_GREEN);
}

static void drawSnakePauseMenu() {
  buttonsRequireRelease();
  ui::pauseSheet("Snake", SNAKE_PAUSE_ITEMS, 4, snakePauseSelection, IOS_GREEN);
}

static void handleSnakePause() {
  if (!snakePauseReady) {
    if (readShoot() == LOW) snakePauseReady = true;
    return;
  }

  int cmd = findCommand();
  if (cmd == 0 || cmd == 2) {
    int old = snakePauseSelection;
    snakePauseSelection = (snakePauseSelection + (cmd == 0 ? 3 : 1)) % 4;
    drawSnakePauseOption(old);
    drawSnakePauseOption(snakePauseSelection);
    playNavClick();
  } else if (cmd == 4) {
    playSelectTone();
    if (snakePauseSelection == 0) {
      scorePopup.life = 0;
      scorePopup.drawn = false;
      showResumeCountdown(redrawFullSnakeScene);
      lastMoveTime = millis();
      lastFxTime = millis();
      gameState = 0;
      menu_drawn = false;
      snakeShootReady = false;
    } else if (snakePauseSelection == 1) {
      initGame();
    } else if (snakePauseSelection == 2) {
      showGameHighScores();
      redrawFullSnakeScene();
      drawSnakePauseMenu();
      snakePauseReady = false;
    } else {
      switchGame(0);
    }
  }
}

static void drawSnakeGameOverScreen() {
  bool isNewBest = (score > 0 && score > currentGame.highScores[0]);
  currentGame.recordScore(score);

  ui::card(22, 62, 196, 194);
  ui::heading(38, 74, "Snake", isNewBest ? "New record!" : "Nice run!");
  if (isNewBest) {
    ui::chip(202 - tft.textWidth("Top score", 2) - 16, 74, "Top score", IOS_AMBER);
  } else {
    String lenTag = "Len " + String(snakeLength);
    ui::chip(202 - tft.textWidth(lenTag, 2) - 16, 74, lenTag.c_str(), IOS_GREEN);
  }

  ui::stat(38, 128, "Score", String(score), IOS_GREEN_END);
  ui::stat(128, 128, "Best", String(currentGame.highScores[0]),
           currentGame.highScores[0] > 0 ? IOS_AMBER : ui::SECONDARY);

  ui::gradientPill(38, 188, 164, 32, "Play again", IOS_GREEN, IOS_GREEN_END);
  ui::hint(238, "Up for home screen", ui::CARD);
}

static void handleSnakeGameOver() {
  int cmd = findCommand();
  if (cmd == 4) {
    playSelectTone();
    initGame();
  } else if (cmd == 0) {
    playSelectTone();
    switchGame(0);
  }
}




void setupSnake() {
  tft.setRotation(2);
  snakeWallLayout = 0;
  showSnakeMenu();
}

void loopSnake() {
  audio::gameState(gameState);
  if (gameState == 0) {
    handleInput();
    if (gameState != 0) return;

    unsigned long now = millis();
    if (snakeCombo >= 2 && now - lastEatMs > 3800) {
      snakeCombo = 0;
      updateScoreHUD();
    }

    if (now - lastMoveTime >= (unsigned long)gameSpeed_ms) {
      moveSnake();
      lastMoveTime = now;
      if (gameState != 0) return;
    }


    if (now - lastFxTime >= 30) {
      lastFxTime = now;
      if (particleCount > 0) updateParticles();
      if (scorePopup.life > 0 || scorePopup.drawn) renderScorePopupStep();
    }
    return;
  } else if (gameState == 1) {
    if (!menu_drawn) {
      eraseScorePopup();
      drawSnakePauseMenu();
      menu_drawn = true;
    }
    handleSnakePause();
  } else if (gameState == 2) {
    if (!menu_drawn) {
      drawSnakeGameOverScreen();
      menu_drawn = true;
    }
    handleSnakeGameOver();
  } else if (gameState == -1) {
    int cmd = findCommand();
    if (cmd >= 0 && cmd < 4) {
      snakeWallLayout = (snakeWallLayout + ((cmd == 0 || cmd == 3) ? 3 : 1)) % 4;
      playNavClick();
      drawModernHeader();
      renderIntroGrassPreview();
      renderIntroSelectorRow();
    } else if (cmd == 4) {
      playSelectTone();
      initGame();
    }
  }
}
# 1 "/home/pahasara/gameConsole/src/tetris.ino"

#include <string.h>
#include <Preferences.h>
#include "config.h"
#include "game.h"
#include "ui_kit.h"
#include "tetris_input.h"

#define SIZE 10
#define SPACE_WIDTH 20
#define SPACE_HEIGHT 20

constexpr int TETRIS_BOARD_X = 20, TETRIS_BOARD_Y = 52;
constexpr int TETRIS_HUD_Y = 266;
constexpr int TETRIS_HUD_H = 46;
constexpr unsigned long TETRIS_SOFT_DROP_MS = 35;
uint16_t TETRIS_GRID_DARK = 0x0802;
uint16_t TETRIS_GRID_LIGHT = 0x1044;
uint16_t TETRIS_CARD = 0x18E3;
constexpr uint16_t TETRIS_HIGHLIGHT = 0xFFFF;
static_assert(2 * TETRIS_BOARD_X + SPACE_WIDTH * SIZE == 240, "Board must be horizontally centered");
static_assert(TETRIS_BOARD_Y + SPACE_HEIGHT * SIZE + 14 == TETRIS_HUD_Y, "Board and HUD need a clear gap");
static_assert(TETRIS_HUD_Y + TETRIS_HUD_H <= 312, "HUD must leave a bottom margin");

static uint16_t tetrisMix(uint16_t a, uint16_t b, int amount) {
  int r = (((a >> 11) & 31) * (255 - amount) + ((b >> 11) & 31) * amount) / 255;
  int g = (((a >> 5) & 63) * (255 - amount) + ((b >> 5) & 63) * amount) / 255;
  int blue = ((a & 31) * (255 - amount) + (b & 31) * amount) / 255;
  return (r << 11) | (g << 5) | blue;
}

int tetrisLines = 0;
int tetrisLevel = 1;



static uint16_t brickTone(int row, int k) {
  uint16_t base = tetrisMix(UI_BG, 0xC3A8, 105);
  int v = (row * 7 + k * 13 + row * k) % 3;
  return tetrisMix(base, 0x0000, v * 28);
}

static uint16_t brickColorAt(int x, int y) {
  int row = y / 10, ly = y % 10;
  int xx = x + ((row & 1) ? 12 : 0);
  int lx = xx % 24, k = xx / 24;
  if (ly == 9 || lx == 23) return tetrisMix(UI_BG, 0x0000, 90);
  uint16_t c = brickTone(row, k);
  return ly == 0 ? tetrisMix(c, 0xFFFF, 40) : c;
}

static void drawTetrisBricks(int cx, int cy, int cw, int ch) {
  tft.fillRect(cx, cy, cw, ch, tetrisMix(UI_BG, 0x0000, 90));
  for (int row = cy / 10; row <= (cy + ch - 1) / 10; ++row) {
    int off = (row & 1) ? 12 : 0;
    int y0 = row * 10;
    int ry0 = max(y0, cy), ry1 = min(y0 + 9, cy + ch);
    if (ry0 >= ry1) continue;
    for (int k = (cx + off) / 24; k * 24 - off < cx + cw; ++k) {
      int x0 = k * 24 - off;
      int rx0 = max(x0, cx), rx1 = min(x0 + 23, cx + cw);
      if (rx0 >= rx1) continue;
      uint16_t c = brickTone(row, k);
      tft.fillRect(rx0, ry0, rx1 - rx0, ry1 - ry0, c);
      if (y0 >= cy) tft.drawFastHLine(rx0, y0, rx1 - rx0, tetrisMix(c, 0xFFFF, 40));
    }
  }
}


void drawTetrisHeader() {
  constexpr int W = 224, H = 28, R = H / 2;
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(W, H)) return;
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) header.drawPixel(x, y, brickColorAt(8 + x, 8 + y));
  for (int x = 0; x < W; ++x) {
    int dx = x < R ? R - x : x > W - 1 - R ? x - (W - 1 - R) : 0;
    int half = dx ? (int)sqrtf((float)(R * R - dx * dx)) : R;
    header.drawFastVLine(x, R - half, 2 * half,
                         tetrisMix(uiHeaderStart, uiHeaderEnd, x * 255 / (W - 1)));
  }
  uint16_t ink = ui::inkOn(tetrisMix(uiHeaderStart, uiHeaderEnd, 60));
  header.setTextColor(ink);
  header.setTextDatum(ML_DATUM);
  header.drawString("Tetris", 16, 15, 2);

  char level[12];
  snprintf(level, sizeof(level), "Level %d", tetrisLevel);
  int chipW = header.textWidth(level, 2) + 18;
  uint16_t chip = tetrisMix(uiHeaderEnd, 0x0000, 120);
  header.fillRoundRect(W - chipW - 5, 4, chipW, H - 8, (H - 8) / 2, chip);
  header.setTextColor(TFT_WHITE);
  header.setTextDatum(MC_DATUM);
  header.drawString(level, W - 5 - chipW / 2, 15, 2);
  header.pushSprite(8, 8);
}

void drawTetrisPlayfieldFrame() {

  drawTetrisBricks(8, 40, 224, 224);
  const int x = TETRIS_BOARD_X - 5, y = TETRIS_BOARD_Y - 5;
  const int w = SPACE_WIDTH * SIZE + 10, h = SPACE_HEIGHT * SIZE + 10;

  tft.drawRoundRect(x - 1, y - 1, w + 2, h + 2, 10, tetrisMix(UI_BG, UI_CYAN, 50));
  tft.fillRoundRect(x, y, w, h, 9, tetrisMix(UI_CYAN, UI_PURPLE, 128));
  tft.fillRoundRect(x + 2, y + 2, w - 4, h - 4, 7, TETRIS_GRID_DARK);
}

unsigned long drop_interval = 600;
unsigned long lastDrop = 0;
unsigned long lastTick = 0;
unsigned long elapsedPlayMs = 0;
unsigned long lastShownSecond = 0;
unsigned long lastSoftDrop = 0;
bool tetrisDownWasHeld = false;
int tetrisPauseSelection = 0;
static const char *const TETRIS_PAUSE_ITEMS[] = {"Resume", "Restart", "High scores", "Main menu"};
uint8_t tetrisPressed = 0;
int tetrisSideHeld = 0;
unsigned long tetrisSideAt = 0;
bool tetrisSideRepeating = false;


uint32_t current_color;
int current_shape;

int current_pos[2] = { 0, 0 };
int prev_pos[2] = { 0, 0 };
bool current_body[4][4];
bool prev_body[4][4];


uint32_t colors[] = { 0x06FF, 0xFEA0, 0xAA9F, 0xFC60, 0x2B7F, 0x3EEB, 0xFA2B };
uint32_t space[SPACE_WIDTH * SPACE_HEIGHT];
int next_shape = 0;
uint32_t next_color = colors[0];

void updatePauseMenu(int dx);
void tetrisSetup();

bool bodies[7][4][4] = {
  { { 0, 0, 0, 0 }, { 1, 1, 1, 1 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 0, 1, 1, 0 }, { 0, 1, 1, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 1, 1, 1, 0 }, { 0, 1, 0, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 1, 1, 1, 0 }, { 1, 0, 0, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 1, 1, 1, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 0, 1, 1, 0 }, { 1, 1, 0, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 1, 1, 0, 0 }, { 0, 1, 1, 0 }, { 0, 0, 0, 0 } }
};

int bodySize() {
  return current_shape == 0 ? 4 : 3;
}

int getEncode(int x, int y) {
  return x + y * SPACE_WIDTH;
}


void drawRect(int x, int y, uint32_t color) {
  int px = TETRIS_BOARD_X + x * SIZE, py = TETRIS_BOARD_Y + y * SIZE;
  uint16_t c = color;
  uint16_t light = tetrisMix(c, 0xFFFF, 120), dark = tetrisMix(c, 0x0000, 100);
  tft.drawFastHLine(px, py, SIZE, TETRIS_GRID_DARK);
  tft.drawFastVLine(px, py + 1, SIZE - 1, TETRIS_GRID_DARK);
  tft.fillRect(px + 1, py + 1, SIZE - 1, SIZE - 1, c);
  tft.drawFastHLine(px + 1, py + 1, SIZE - 2, light);
  tft.drawFastVLine(px + 1, py + 2, SIZE - 3, light);
  tft.drawFastHLine(px + 2, py + SIZE - 1, SIZE - 2, dark);
  tft.drawFastVLine(px + SIZE - 1, py + 2, SIZE - 3, dark);
  tft.fillRect(px + 3, py + 3, 2, 2, tetrisMix(c, 0xFFFF, 190));
}


void drawRect(int x, int y) {
  int px = TETRIS_BOARD_X + x * SIZE;
  int py = TETRIS_BOARD_Y + y * SIZE;
  tft.fillRect(px, py, SIZE, SIZE, TETRIS_GRID_DARK);
  tft.drawPixel(px + SIZE / 2, py + SIZE / 2, TETRIS_GRID_LIGHT);
}

void redrawChangedCells() {
  tft.startWrite();
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      int px_old = prev_pos[0] + i - 1;
      int py_old = prev_pos[1] + j - 1;
      int px_new = current_pos[0] + i - 1;
      int py_new = current_pos[1] + j - 1;

      if (prev_body[i][j]) {
        bool still_covered = false;
        int di = px_old - (current_pos[0] - 1);
        int dj = py_old - (current_pos[1] - 1);
        if (di >= 0 && di < 4 && dj >= 0 && dj < 4 && current_body[di][dj])
          still_covered = true;

        if (!still_covered && px_old >= 0 && px_old < SPACE_WIDTH
            && py_old >= 0 && py_old < SPACE_HEIGHT) {
          uint32_t dead = space[getEncode(px_old, py_old)];
          if (dead != TFT_BLACK) drawRect(px_old, py_old, dead);
          else drawRect(px_old, py_old);
        }
      }

      if (current_body[i][j] && px_new >= 0 && px_new < SPACE_WIDTH
          && py_new >= 0 && py_new < SPACE_HEIGHT) {
        drawRect(px_new, py_new, current_color);
      }
    }
  }
  tft.endWrite();
}

void drawBody() {
  tft.startWrite();
  int x = current_pos[0];
  int y = current_pos[1];
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (current_body[i][j]) {
        int px = i + x - 1;
        int py = j + y - 1;
        if (px >= 0 && px < SPACE_WIDTH && py >= 0 && py < SPACE_HEIGHT)
          drawRect(px, py, current_color);
      }
  tft.endWrite();
}

void drawNext() {
  TFT_eSprite preview(&tft);
  preview.setColorDepth(16);
  if (!preview.createSprite(56, 26)) return;
  preview.fillSprite(TETRIS_CARD);
  int minX = 4, minY = 4, maxX = 0, maxY = 0;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (bodies[next_shape][i][j]) {
        minX = min(minX, i); maxX = max(maxX, i);
        minY = min(minY, j); maxY = max(maxY, j);
      }
  int left = (56 - (maxX - minX + 1) * 6) / 2;
  int top = (26 - (maxY - minY + 1) * 6) / 2;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
      if (!bodies[next_shape][i][j]) continue;
      int x = left + (i - minX) * 6, y = top + (j - minY) * 6;
      preview.fillRect(x, y, 5, 5, next_color);
      preview.drawFastHLine(x, y, 5, tetrisMix(next_color, 0xFFFF, 120));
      preview.drawFastHLine(x, y + 4, 5, tetrisMix(next_color, 0x0000, 100));
    }
  preview.pushSprite(92, TETRIS_HUD_Y + 19);
}

void redrawRowsAbove(int cleared_row) {
  tft.startWrite();
  for (int y = 0; y <= cleared_row; y++) {
    for (int x = 0; x < SPACE_WIDTH; x++) {
      uint32_t c = space[getEncode(x, y)];
      if (c != TFT_BLACK) drawRect(x, y, c);
      else drawRect(x, y);
    }
  }
  tft.endWrite();
}

void drawMenu(void) {
  const int cardX[] = {8, 84, 160};
  const char *labels[] = {"Score", "Next", "Time"};
  const uint16_t dots[] = {UI_YELLOW, UI_PURPLE, UI_CYAN};
  for (int i = 0; i < 3; i++) {
    tft.fillRoundRect(cardX[i], TETRIS_HUD_Y, 72, TETRIS_HUD_H, 12, TETRIS_CARD);
    tft.fillCircle(cardX[i] + 12, TETRIS_HUD_Y + 12, 3, dots[i]);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(0x6226, TETRIS_CARD);
    tft.drawString(labels[i], cardX[i] + 20, TETRIS_HUD_Y + 4, 2);
  }
}

static unsigned long tetrisScoreFlashAt = 0;

void drawTetrisScore() {
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", score);
  TFT_eSprite value(&tft);
  value.setColorDepth(16);
  if (!value.createSprite(56, 24)) return;
  value.fillSprite(TETRIS_CARD);
  value.setTextDatum(TL_DATUM);
  value.setTextFont(2);
  if (value.textWidth(buf) > 56) value.setTextFont(1);
  value.setTextColor(millis() - tetrisScoreFlashAt < 350 ? UI_CYAN : 0x2104);
  value.drawString(buf, 0, 2);
  value.pushSprite(17, TETRIS_HUD_Y + 21);
}

void updateScore() {
  score += 10;
  tetrisScoreFlashAt = millis();
  drawTetrisScore();
}

void updateTime() {
  unsigned long totalSeconds = elapsedPlayMs / 1000;
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu", totalSeconds / 60, totalSeconds % 60);
  TFT_eSprite value(&tft);
  value.setColorDepth(16);
  if (!value.createSprite(56, 24)) return;
  value.fillSprite(TETRIS_CARD);
  value.setTextDatum(TL_DATUM);
  value.setTextFont(2);
  if (value.textWidth(buffer) > 56) value.setTextFont(1);
  value.setTextColor(0x2104);
  value.drawString(buffer, 0, 2);
  value.pushSprite(169, TETRIS_HUD_Y + 21);
}

void addToDead() {
  int x = current_pos[0];
  int y = current_pos[1];
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
      int px = x + i - 1;
      int py = y + j - 1;
      if (px >= 0 && px < SPACE_WIDTH && py >= 0 && py < SPACE_HEIGHT)
        if (current_body[i][j])
          space[getEncode(px, py)] = current_color;
    }
}

bool checkCollision(int dx, int dy) {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (current_body[i][j]) {
        int px = current_pos[0] + i - 1 + dx;
        int py = current_pos[1] + j - 1 + dy;
        if (px < 0 || px >= SPACE_WIDTH || py >= SPACE_HEIGHT) return true;
        if (py >= 0 && space[getEncode(px, py)] != TFT_BLACK) return true;
      }
  return false;
}

bool newTetro() {
  int cleared = 0;
  bool levelUp = false;
  current_color = next_color;
  current_shape = next_shape;
  next_shape = random(7);
  next_color = colors[next_shape];
  current_pos[0] = random(SPACE_WIDTH - bodySize() + 1) + 1;
  current_pos[1] = 1;
  drawNext();

  for (int j = 0; j < SPACE_HEIGHT; j++) {
    int fill = 0;
    for (int i = 0; i < SPACE_WIDTH; i++)
      if (space[getEncode(i, j)] != TFT_BLACK) fill++;

    if (fill == SPACE_WIDTH) {
      ++cleared;

      tft.startWrite();
      for (int x = 0; x < SPACE_WIDTH; x++) drawRect(x, j, 0xFFFF);
      tft.endWrite();
      delay(70);
      for (int y = j; y > 0; y--)
        for (int x = 0; x < SPACE_WIDTH; x++)
          space[getEncode(x, y)] = space[getEncode(x, y - 1)];

      for (int x = 0; x < SPACE_WIDTH; x++)
        space[getEncode(x, 0)] = TFT_BLACK;

      redrawRowsAbove(j);
      updateScore();

      int level = ++tetrisLines / 5 + 1;
      if (level != tetrisLevel) {
        levelUp = true;
        tetrisLevel = level;
        drop_interval = max(150, 600 - (tetrisLevel - 1) * 50);
        drawTetrisHeader();
      }
      j--;
    }
  }

  if (cleared) audio::play(cleared >= 4 ? audio::Cue::FourLines :
                           levelUp ? audio::Cue::Level : audio::Cue::Clear);
  memcpy(current_body, bodies[current_shape], sizeof(current_body));
  memcpy(prev_pos, current_pos, sizeof(current_pos));
  memcpy(prev_body, current_body, sizeof(current_body));
  return !checkCollision(0, 0);
}

void tryRotate() {
  if (current_shape == 1) return;
  bool temp[4][4] = {};
  int size = bodySize();
  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++)
      temp[i][j] = current_body[size - 1 - j][i];

  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++)
      if (temp[i][j]) {
        int px = current_pos[0] + i - 1;
        int py = current_pos[1] + j - 1;
        if (px < 0 || px >= SPACE_WIDTH || py >= SPACE_HEIGHT) return;
        if (py >= 0 && space[getEncode(px, py)] != TFT_BLACK) return;
      }

  memcpy(prev_pos, current_pos, sizeof(current_pos));
  memcpy(prev_body, current_body, sizeof(current_body));
  memcpy(current_body, temp, sizeof(temp));
  audio::play(audio::Cue::Rotate);
  redrawChangedCells();
}

void updatePosition(int dx = 0, int dy = 1) {
  if (!checkCollision(dx, dy)) {
    memcpy(prev_pos, current_pos, sizeof(current_pos));
    memcpy(prev_body, current_body, sizeof(current_body));
    current_pos[0] += dx;
    current_pos[1] += dy;
    redrawChangedCells();
  } else if (dy == 1) {
    addToDead();
    playTetrisDropSound();
    if (!newTetro()) {
      audio::play(audio::Cue::GameOver);
      gameState = 2;
      menu_drawn = false;
      tetrisInputRequireRelease();
    } else drawBody();
  }
}




void getCommand() {
  if (tetrisPressed & 16) {
    audio::pauseMusic(true); gameState = 1;
    menu_drawn = false;
    tetrisPauseSelection = 0;
    tetrisInputRequireRelease();
    tetrisSideHeld = 0;
    return;
  }
  if (tetrisPressed & 1) tryRotate();
  uint8_t held = tetrisInputHeld();
  int side = ((held & 2) != 0) - ((held & 8) != 0);
  unsigned long now = millis();

  int tappedSide = (tetrisPressed & 2) ? 1 : (tetrisPressed & 8) ? -1 : 0;
  if (tappedSide) {
    updatePosition(tappedSide, 0);
    tetrisSideHeld = side == tappedSide ? side : 0;
    tetrisSideAt = now;
    tetrisSideRepeating = false;
  } else if (side != tetrisSideHeld) {


    tetrisSideHeld = 0;
  } else if (side && now - tetrisSideAt >= (tetrisSideRepeating ? 65UL : 180UL)) {
    updatePosition(side, 0);
    tetrisSideAt = now;
    tetrisSideRepeating = true;
  }
}


void restoreAfterPauseTetris() {
  showResumeCountdown([]() {
  if (!restoringCountdownDigit()) drawTetrisBricks(0, 0, 240, 320);
  drawTetrisHeader();
  if (!restoringCountdownDigit()) drawTetrisPlayfieldFrame();
  tft.startWrite();
  for (int y = 0; y < SPACE_HEIGHT; y++)
    for (int x = 0; x < SPACE_WIDTH; x++) {
      uint32_t c = space[getEncode(x, y)];
      if (c != TFT_BLACK) drawRect(x, y, c);
      else drawRect(x, y);
    }
  tft.endWrite();
  drawBody();
  drawMenu();
  drawTetrisScore();
  drawNext();
  updateTime();
  });
  lastDrop = millis();
  lastTick = lastDrop;
  lastSoftDrop = lastDrop;
  tetrisDownWasHeld = false;
  gameState = 0;
  menu_drawn = false;
  tetrisSideHeld = 0;
  tetrisInputRequireRelease();
}

void drawTetrisPauseOption(int item) {
  ui::pauseItem(TETRIS_PAUSE_ITEMS, 4, item, tetrisPauseSelection, UI_CYAN);
}

void drawTetrisPauseMenu() {
  ui::pauseSheet("Tetris", TETRIS_PAUSE_ITEMS, 4, tetrisPauseSelection, UI_CYAN);
}

void handleTetrisPause() {
  int pressed = (tetrisPressed & 16) ? 4 : (tetrisPressed & 1) ? 0 : (tetrisPressed & 4) ? 2 : -1;
  if (pressed == 0 || pressed == 2) {
    int oldSelection = tetrisPauseSelection;
    tetrisPauseSelection = (tetrisPauseSelection + (pressed == 0 ? 3 : 1)) % 4;
    drawTetrisPauseOption(oldSelection);
    drawTetrisPauseOption(tetrisPauseSelection);
    playNavClick();
  } else if (pressed == 4) {
    playSelectTone();
    if (tetrisPauseSelection == 0) restoreAfterPauseTetris();
    else if (tetrisPauseSelection == 1) tetrisSetup();
    else if (tetrisPauseSelection == 2) {
      showGameHighScores();
      tetrisPressed = 0;
      tetrisInputRequireRelease();
      drawTetrisBricks(0, 0, 240, 320);
      drawTetrisHeader();
      drawTetrisPlayfieldFrame();
      tft.startWrite();
      for (int y = 0; y < SPACE_HEIGHT; ++y)
        for (int x = 0; x < SPACE_WIDTH; ++x) {
          uint32_t color = space[getEncode(x, y)];
          if (color != TFT_BLACK) drawRect(x, y, color);
          else drawRect(x, y);
        }
      tft.endWrite();
      drawMenu();
      drawNext();
      drawTetrisScore();
      updateTime();
      drawBody();
      drawTetrisPauseMenu();
    } else switchGame(0);
  }
}

void drawTetrisGameOver() {
  currentGame.recordScore(score);
  ui::gameOverSheet("Tetris", score, currentGame.highScores[0], UI_CYAN);
}

void handleTetrisGameOver() {
  int pressed = (tetrisPressed & 16) ? 4 : (tetrisPressed & 1) ? 0 : -1;
  if (pressed == 4) tetrisSetup();
  else if (pressed == 0) switchGame(0);
}



void tetrisSetup() {
  audio::restartMusic();
  tetrisInputBegin();
  tetrisSideHeld = 0;
  TETRIS_GRID_DARK = tetrisMix(UI_BG, UI_PURPLE, 22);
  TETRIS_GRID_LIGHT = tetrisMix(UI_BG, UI_CYAN, 90);
  TETRIS_CARD = 0xDE32;
  score = 0;
  tetrisLines = 0;
  tetrisLevel = 1;
  gameState = 0;
  menu_drawn = false;
  drop_interval = 600;
  elapsedPlayMs = 0;
  lastShownSecond = 0;
  tetrisPauseSelection = 0;
  lastDrop = millis();
  lastTick = lastDrop;
  lastSoftDrop = lastDrop;
  tetrisDownWasHeld = false;
  next_shape = random(7);
  next_color = colors[next_shape];
  for (int i = 0; i < SPACE_WIDTH * SPACE_HEIGHT; i++)
    space[i] = TFT_BLACK;

  tft.setRotation(2);
  drawTetrisBricks(0, 0, 240, 320);
  drawTetrisHeader();
  drawTetrisPlayfieldFrame();

  tft.startWrite();
  for (int i = 0; i < SPACE_WIDTH; i++)
    for (int j = 0; j < SPACE_HEIGHT; j++)
      drawRect(i, j);
  tft.endWrite();

  drawMenu();
  drawTetrisScore();
  updateTime();
  newTetro();
  drawBody();
  lastDrop = millis();
  lastTick = lastDrop;
}

void tetrisLoop() {
  audio::gameState(gameState);
  tetrisPressed = tetrisInputPoll();
  if (gameState == 0) {
    unsigned long currentTime = millis();
    elapsedPlayMs += currentTime - lastTick;
    lastTick = currentTime;
    getCommand();
    if (gameState != 0) return;
    currentTime = millis();

    bool downHeld = (tetrisInputHeld() & 4) != 0;
    if ((tetrisPressed & 4) || (downHeld && tetrisDownWasHeld &&
                     currentTime - lastSoftDrop >= TETRIS_SOFT_DROP_MS)) {
      updatePosition(0, 1);
      if (gameState != 0) return;
      lastSoftDrop = millis();
      lastDrop = lastSoftDrop;
      currentTime = lastSoftDrop;
    }
    tetrisDownWasHeld = downHeld && (tetrisDownWasHeld || (tetrisPressed & 4));

    unsigned long shownSecond = elapsedPlayMs / 1000;
    if (shownSecond != lastShownSecond) {
      lastShownSecond = shownSecond;
      updateTime();
    }

    if (currentTime - lastDrop >= drop_interval) {
      lastDrop = currentTime;
      updatePosition();
    }
  } else if (gameState == 1) {
    if (!menu_drawn) {
      drawTetrisPauseMenu();
      menu_drawn = true;
    }
    handleTetrisPause();
  } else if (gameState == 2) {
    if (!menu_drawn) {
      drawTetrisGameOver();
      menu_drawn = true;
    }
    handleTetrisGameOver();
  }
}