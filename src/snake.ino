#include "game.h"
#include "ui_kit.h"
#include <Arduino.h>
#include "snake_background_240x320.h"

// =================================================================
// 1. MODERN IPHONE-INSPIRED SNAKE (PORTRAIT ORIENTATION: 240x320)
// =================================================================

constexpr int PLAYFIELD_X = 0;
constexpr int PLAYFIELD_Y = 48;
constexpr int PLAYFIELD_W = 240;
constexpr int PLAYFIELD_H = 240;
constexpr int BLOCK_SIZE  = 10;
constexpr int GRID_COLS   = PLAYFIELD_W / BLOCK_SIZE; // 24
constexpr int GRID_ROWS   = PLAYFIELD_H / BLOCK_SIZE; // 24
constexpr int MAX_SNAKE_LEN = GRID_COLS * GRID_ROWS;  // 576

constexpr uint16_t GRASS1 = 0x0143;
constexpr uint16_t GRASS2 = 0x01C4;

static int snakeWallLayout = 0;
static const char *const SNAKE_WALL_NAMES[] = {
  "No walls", "Twin bars", "Four corners", "Cross"};

// Shared by preview, rendering, collision and food placement.
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

static uint8_t lawnMap[GRID_COLS][GRID_ROWS];

static inline uint16_t getGrassColor(int gx, int gy) {
  if (gx < 0 || gx >= GRID_COLS || gy < 0 || gy >= GRID_ROWS) return GRASS1;
  return ((gx + gy) & 1) ? GRASS2 : GRASS1;
}

#define IOS_BG UI_BG
#define IOS_CARD UI_PANEL
#define IOS_SURFACE UI_SURFACE
#define IOS_WHITE UI_TEXT
#define IOS_MUTED UI_MUTED
constexpr uint16_t IOS_GREEN     = 0xAFE0;
constexpr uint16_t IOS_RED       = 0xFA48;
constexpr uint16_t IOS_AMBER     = 0xFFE0;
constexpr uint16_t SNAKE_HEADER_START = 0xAFE0;
constexpr uint16_t SNAKE_HEADER_END   = 0xFFE0;
constexpr uint16_t SNAKE_DARK_TEXT    = 0x0841;
constexpr uint16_t SNAKE_FRAME        = 0x3D80;

constexpr uint16_t SNAKE_HEAD    = 0x001F;
constexpr uint16_t SNAKE_BODY_1  = 0x10B0;  // dark navy blue
constexpr uint16_t SNAKE_BODY_2  = 0x03BF;  // clear blue

static uint16_t snakeMix(uint16_t a, uint16_t b, int amount) {
  int r = (((a >> 11) & 31) * (255 - amount) + ((b >> 11) & 31) * amount) / 255;
  int g = (((a >> 5) & 63) * (255 - amount) + ((b >> 5) & 63) * amount) / 255;
  int blue = ((a & 31) * (255 - amount) + (b & 31) * amount) / 255;
  return (r << 11) | (g << 5) | blue;
}

// -----------------------------------------------------------------
// 2. DATA STRUCTURES & PARTICLES
// -----------------------------------------------------------------
struct SnakeParticle {
  float x, y;
  float speedX, speedY;
  uint16_t color;
  int life;
};

// -----------------------------------------------------------------
// 3. GAME STATE VARIABLES
// -----------------------------------------------------------------
static Vec2 snakeBody[MAX_SNAKE_LEN];
static int snakeLength = 4;
static int currentDirection = 1; // 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT
static int lastDirection = 1;
static Vec2 foodPos;
static unsigned long lastMoveTime = 0;
static int gameSpeed_ms = 220;
static int snakePauseSelection = 0;
static const char *const SNAKE_PAUSE_ITEMS[] = {"Resume", "Restart", "High scores", "Main menu"};
static bool snakePauseReady = false;
static bool snakeShootReady = false;

// Visual FX
static SnakeParticle particles[20];
static int particleCount = 0;
static bool foodPulse = false;

// -----------------------------------------------------------------
// 4. SOUND EFFECTS
// -----------------------------------------------------------------
static void playBiteSound() {
  audio::play(audio::Cue::Pickup);
}

static void playDeathSound() {
  audio::play(audio::Cue::Death);
}

// -----------------------------------------------------------------
// 5. RANDOM GRASS TILE & PLAYFIELD DRAWING
// -----------------------------------------------------------------
static void initRandomLawn() {
  for (int x = 0; x < GRID_COLS; x++) {
    for (int y = 0; y < GRID_ROWS; y++) {
      lawnMap[x][y] = (x + y) & 1;
    }
  }
}

static void drawGrassyCell(int gx, int gy) {
  if (gx < 0 || gx >= GRID_COLS || gy < 0 || gy >= GRID_ROWS) return;
  int px = PLAYFIELD_X + gx * BLOCK_SIZE;
  int py = PLAYFIELD_Y + gy * BLOCK_SIZE;
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

// -----------------------------------------------------------------
// 6. MODERN IPHONE HEADER & HUD
// -----------------------------------------------------------------
static void drawModernHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(224, 38)) return;
  header.fillSprite(IOS_BG);
  header.fillRoundRect(0, 0, 224, 38, 12, ui::CARD);
  header.setTextDatum(ML_DATUM);
  header.setTextColor(ui::LABEL);
  header.drawString("Snake", 12, 20, 4);
  header.setTextDatum(TR_DATUM);
  header.setTextColor(ui::SECONDARY);
  header.drawString("Best", 150, 3, 2);
  header.drawString("Score", 212, 3, 2);
  header.setTextColor(IOS_AMBER);
  header.drawString(String(currentGame.highScores[0]), 150, 19, 2);
  header.setTextColor(ui::LABEL);
  header.drawString(String(score), 212, 19, 2);
  header.pushSprite(8, 5);
  header.deleteSprite();
}

static void updateScoreHUD() {
  drawModernHeader();
}

static void drawBottomControlBar() {
  tft.fillRect(0, 294, 240, 26, IOS_BG);
  ui::hint(306, "Shoot to pause", IOS_BG);
}

// -----------------------------------------------------------------
// 7. ENTITY RENDERING (SNAKE & APPLE)
// -----------------------------------------------------------------
void drawSnakeHead() {
  Vec2 head = snakeBody[0];
  int px = PLAYFIELD_X + head.x * BLOCK_SIZE;
  int py = PLAYFIELD_Y + head.y * BLOCK_SIZE;

  tft.fillRoundRect(px, py, BLOCK_SIZE, BLOCK_SIZE, 3, SNAKE_HEAD);

  int eye1X = px + 2, eye1Y = py + 2;
  int eye2X = px + 6, eye2Y = py + 2;

  switch (currentDirection) {
    case 0: // UP
      eye1X = px + 2; eye1Y = py + 2;
      eye2X = px + 6; eye2Y = py + 2;
      break;
    case 2: // DOWN
      eye1X = px + 2; eye1Y = py + 6;
      eye2X = px + 6; eye2Y = py + 6;
      break;
    case 3: // LEFT
      eye1X = px + 2; eye1Y = py + 2;
      eye2X = px + 2; eye2Y = py + 6;
      break;
    case 1: // RIGHT
      eye1X = px + 6; eye1Y = py + 2;
      eye2X = px + 6; eye2Y = py + 6;
      break;
  }

  tft.fillCircle(eye1X, eye1Y, 1, IOS_WHITE);
  tft.fillCircle(eye2X, eye2Y, 1, IOS_WHITE);
}

static int snakeNeighborDirection(Vec2 from, Vec2 to) {
  if (to.x == from.x && to.y == (from.y + GRID_ROWS - 1) % GRID_ROWS) return 0;
  if (to.y == from.y && to.x == (from.x + 1) % GRID_COLS) return 1;
  if (to.x == from.x && to.y == (from.y + 1) % GRID_ROWS) return 2;
  if (to.y == from.y && to.x == (from.x + GRID_COLS - 1) % GRID_COLS) return 3;
  return -1;
}

static void drawSnakeSegment(int index) {
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

  // Compose the tapered tail or rounded turn with its grass in one transfer.
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
  tft.fillCircle(px + 6, py + 5, 3, 0xF800);
  tft.drawPixel(px + 5, py + 2, 0xFFE0);
  tft.drawPixel(px + 3, py + 4, 0xFFE0);
}

// -----------------------------------------------------------------
// 8. PARTICLE FX
// -----------------------------------------------------------------
static void createAppleParticles(int gx, int gy) {
  int cx = PLAYFIELD_X + gx * BLOCK_SIZE + 5;
  int cy = PLAYFIELD_Y + gy * BLOCK_SIZE + 5;
  particleCount = 0;
  for (int i = 0; i < 12; i++) {
    particles[i].x = cx;
    particles[i].y = cy;
    float angle = (i / 12.0f) * 6.283f;
    float speed = random(12, 28) / 10.0f;
    particles[i].speedX = cos(angle) * speed;
    particles[i].speedY = sin(angle) * speed;
    particles[i].color = (i % 2 == 0) ? IOS_RED : TFT_GREEN;
    particles[i].life = random(6, 12);
    particleCount++;
  }
}

void updateParticles() {
  for (int i = 0; i < particleCount; i++) {
    if (particles[i].life > 0) {
      int prevX = (int)particles[i].x;
      int prevY = (int)particles[i].y;

      if (prevX >= PLAYFIELD_X && prevX < PLAYFIELD_X + PLAYFIELD_W &&
          prevY >= PLAYFIELD_Y && prevY < PLAYFIELD_Y + PLAYFIELD_H &&
          !snakeWallAt(prevX / BLOCK_SIZE, (prevY - PLAYFIELD_Y) / BLOCK_SIZE)) {
        uint16_t grassPixel = pgm_read_word(
            &snake_background_240x320[prevY * 240 + prevX]);
        tft.drawPixel(prevX, prevY, grassPixel);
      }

      particles[i].x += particles[i].speedX;
      particles[i].y += particles[i].speedY;
      particles[i].life--;

      int nx = (int)particles[i].x;
      int ny = (int)particles[i].y;
      if (particles[i].life > 0 && nx >= PLAYFIELD_X && nx < PLAYFIELD_X + PLAYFIELD_W &&
          ny >= PLAYFIELD_Y && ny < PLAYFIELD_Y + PLAYFIELD_H &&
          !snakeWallAt(nx / BLOCK_SIZE, (ny - PLAYFIELD_Y) / BLOCK_SIZE)) {
        tft.drawPixel(nx, ny, particles[i].color);
      }
    }
  }
}

// -----------------------------------------------------------------
// 9. FOOD GENERATION & COLLISION
// -----------------------------------------------------------------
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
  if (!available) { gameState = 2; menu_drawn = false; }

}

bool checkSelfCollision(Vec2 head) {
  for (int i = 1; i < snakeLength; i++) {
    if (head.x == snakeBody[i].x && head.y == snakeBody[i].y) {
      return true;
    }
  }
  return false;
}

// -----------------------------------------------------------------
// 10. INPUT & MOVEMENT
// -----------------------------------------------------------------
void handleInput() {
  if (readShoot() == LOW) snakeShootReady = true;

  int cmd = findCommand();
  if (cmd == 4 && snakeShootReady) {
    gameState = 1; // Pause
    menu_drawn = false;
    snakePauseSelection = 0;
    snakePauseReady = false;
    return;
  }

  // Prevent 180-degree self-reversals
  bool isReverse = (cmd >= 0) && (abs(cmd - lastDirection) == 2);
  if (!isReverse && cmd >= 0 && cmd < 4) {
    currentDirection = cmd;
  }
}

void moveSnake() {
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

  // Toroidal wrapping
  if (newHead.x < 0) newHead.x = GRID_COLS - 1;
  if (newHead.x >= GRID_COLS) newHead.x = 0;
  if (newHead.y < 0) newHead.y = GRID_ROWS - 1;
  if (newHead.y >= GRID_ROWS) newHead.y = 0;

  snakeBody[0] = newHead;
  lastDirection = currentDirection;

  if (snakeWallAt(newHead.x, newHead.y) || checkSelfCollision(newHead)) {
    playDeathSound();
    gameState = 2;
    menu_drawn = false;
    return;
  }

  if (newHead.x == foodPos.x && newHead.y == foodPos.y) {
    if (snakeLength < MAX_SNAKE_LEN) {
      snakeBody[snakeLength] = oldTail;
      snakeLength++;
    }
    score++;
    createAppleParticles(foodPos.x, foodPos.y);
    playBiteSound();
    if (score % 10 == 0) audio::play(audio::Cue::Milestone);
    drawGrassyCell(newHead.x, newHead.y);

    if (gameSpeed_ms > 90) {
      gameSpeed_ms -= 6;
    }

    updateScoreHUD();
    generateFood();
    if (gameState == 0) drawApple();
  } else {
    drawGrassyCell(oldTail.x, oldTail.y);
  }

  drawSnakeHead();
  if (snakeLength > 1) {
    drawSnakeSegment(1);
    if (snakeLength > 2) drawSnakeSegment(snakeLength - 1);
  }
}

// -----------------------------------------------------------------
// 11. MODAL SCREENS (START / PAUSE / GAME OVER)
// -----------------------------------------------------------------
void showSnakeMenu() {
  gameState = -1;
  tft.setRotation(2);
  tft.fillScreen(IOS_BG);
  initRandomLawn();
  drawModernHeader();
  ui::card(8, 52, 224, 236);
  ui::heading(20, 64, "BRICK WALLS", SNAKE_WALL_NAMES[snakeWallLayout]);
  // Downsample the actual playfield art so the preview matches gameplay.
  static uint16_t preview[96 * 96];
  for (int y = 0; y < 96; ++y)
    for (int x = 0; x < 96; ++x)
      preview[y * 96 + x] = pgm_read_word(&snake_background_240x320[
          (PLAYFIELD_Y + y * PLAYFIELD_H / 96) * 240 + x * PLAYFIELD_W / 96]);
  tft.setSwapBytes(true);
  tft.pushImage(72, 114, 96, 96, preview);
  tft.setSwapBytes(false);
  for (int y = 0; y < GRID_ROWS; ++y)
    for (int x = 0; x < GRID_COLS; ++x)
      if (snakeWallAt(x, y)) drawSnakeBrick(72 + x * 4, 114 + y * 4, 4);
  for (int i = 0; i < 3; ++i)
    tft.fillRect(84 + i * 4, 162, 4, 4, i & 1 ? SNAKE_BODY_1 : SNAKE_BODY_2);
  tft.fillRoundRect(96, 162, 4, 4, 1, SNAKE_HEAD);
  tft.drawPixel(99, 162, TFT_WHITE);
  tft.drawPixel(99, 165, TFT_WHITE);
  tft.fillCircle(146, 152, 2, TFT_RED);
  tft.drawPixel(146, 149, 0x0540);
  tft.drawPixel(145, 151, 0xFFE0);
  ui::hint(230, "D-pad: choose layout", ui::CARD);
  ui::hint(254, "Shoot: start", ui::CARD);
  ui::hint(306, "Eat apples. Avoid bricks.", IOS_BG);
}

void initGame() {
  gameState = 0;
  snakeLength = 4;
  currentDirection = 1;
  lastDirection = 1;
  score = 0;
  gameSpeed_ms = 220;
  particleCount = 0;
  snakeShootReady = false;

  initRandomLawn();

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
}

static void drawSnakePauseOption(int item) {
  ui::pauseItem(SNAKE_PAUSE_ITEMS, 4, item, snakePauseSelection, IOS_GREEN);
}

static void drawSnakePauseMenu() {
  ui::pauseSheet("Snake", SNAKE_PAUSE_ITEMS, 4, snakePauseSelection, IOS_GREEN);
}

static void handleSnakePause() {
  if (!snakePauseReady) {
    if (readShoot() == LOW) snakePauseReady = true;
    return;
  }

  int cmd = findCommand();
  if (cmd == 0 || cmd == 2) { // UP / DOWN
    int old = snakePauseSelection;
    snakePauseSelection = (snakePauseSelection + (cmd == 0 ? 3 : 1)) % 4;
    drawSnakePauseOption(old);
    drawSnakePauseOption(snakePauseSelection);
    playNavClick();
  } else if (cmd == 4) { // SHOOT: Select
    playSelectTone();
    if (snakePauseSelection == 0) { // Resume
      showResumeCountdown([]() {
      if (!restoringCountdownDigit()) tft.fillScreen(IOS_BG);
      drawModernHeader();
      renderFullGrassBoard();
      drawBottomControlBar();
      drawApple();
      for (int i = 1; i < snakeLength; ++i) drawSnakeSegment(i);
      drawSnakeHead();
      });
      lastMoveTime = millis();
      gameState = 0;
      menu_drawn = false;
      snakeShootReady = false;
    } else if (snakePauseSelection == 1) { // Restart
      initGame();
    } else if (snakePauseSelection == 2) {
      showGameHighScores();
      tft.fillScreen(IOS_BG);
      drawModernHeader();
      renderFullGrassBoard();
      drawBottomControlBar();
      drawApple();
      for (int i = 1; i < snakeLength; ++i) drawSnakeSegment(i);
      drawSnakeHead();
      drawSnakePauseMenu();
      snakePauseReady = false;
    } else { // Exit to main menu
      switchGame(0);
    }
  }
}

static void drawSnakeGameOverScreen() {
  currentGame.recordScore(score);
  ui::gameOverSheet("Snake", score, currentGame.highScores[0], IOS_GREEN);
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

// -----------------------------------------------------------------
// 12. LIFECYCLE (setupSnake & loopSnake)
// -----------------------------------------------------------------
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

    if (millis() - lastMoveTime >= (unsigned long)gameSpeed_ms) {
      moveSnake();
      lastMoveTime = millis();
      // (removed redundant drawApple() here; it was already drawn on eat)
    }

    if (particleCount > 0) {
      updateParticles();
    }
    return;
  } else if (gameState == 1) { // Pause Menu
    if (!menu_drawn) {
      drawSnakePauseMenu();
      menu_drawn = true;
    }
    handleSnakePause();
  } else if (gameState == 2) { // Game Over
    if (!menu_drawn) {
      drawSnakeGameOverScreen();
      menu_drawn = true;
    }
    handleSnakeGameOver();
  } else if (gameState == -1) { // Title Screen
    int cmd = findCommand();
    if (cmd >= 0 && cmd < 4) {
      snakeWallLayout = (snakeWallLayout + ((cmd == 0 || cmd == 3) ? 3 : 1)) % 4;
      playNavClick();
      showSnakeMenu();
    } else if (cmd == 4) {
      playSelectTone();
      initGame();
    }
  }
}
