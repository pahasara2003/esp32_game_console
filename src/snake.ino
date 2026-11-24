#include "game.h"
#include "ui_kit.h"
#include <Arduino.h>
#include "snake_background_240x320.h"

// =================================================================
// MODERN POLISHED SNAKE (PORTRAIT ORIENTATION: 240x320)
// =================================================================

constexpr int PLAYFIELD_X = 0;
constexpr int PLAYFIELD_Y = 48;
constexpr int PLAYFIELD_W = 240;
constexpr int PLAYFIELD_H = 240;
constexpr int BLOCK_SIZE  = 10;
constexpr int GRID_COLS   = PLAYFIELD_W / BLOCK_SIZE; // 24
constexpr int GRID_ROWS   = PLAYFIELD_H / BLOCK_SIZE; // 24
constexpr int MAX_SNAKE_LEN = GRID_COLS * GRID_ROWS;  // 576

static int snakeWallLayout = 0;
static const char *const SNAKE_WALL_NAMES[] = {
  "No walls", "Twin bars", "Four corners", "Cross"
};

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

#define IOS_BG UI_BG
constexpr uint16_t IOS_GREEN       = 0x2EE9; // Bright spring green
constexpr uint16_t IOS_GREEN_END   = 0xAFE0; // Sunlit lime
constexpr uint16_t IOS_RED         = 0xFA48;
constexpr uint16_t IOS_AMBER       = 0xFE64;

// Classic crisp snake palette from original snake.ino
constexpr uint16_t SNAKE_HEAD      = 0x001F;
constexpr uint16_t SNAKE_BODY_1    = 0x10B0; // dark navy blue
constexpr uint16_t SNAKE_BODY_2    = 0x03BF; // clear blue

static uint16_t snakeMix(uint16_t a, uint16_t b, int amount) {
  int r = (((a >> 11) & 31) * (255 - amount) + ((b >> 11) & 31) * amount) / 255;
  int g = (((a >> 5) & 63) * (255 - amount) + ((b >> 5) & 63) * amount) / 255;
  int blue = ((a & 31) * (255 - amount) + (b & 31) * amount) / 255;
  return (uint16_t)((r << 11) | (g << 5) | blue);
}

// Read grass pixel in standard RGB565 directly from snake_background_240x320.h
static inline uint16_t grassNativeAt(int sx, int sy) {
  return pgm_read_word(&snake_background_240x320[sy * 240 + sx]);
}

// -----------------------------------------------------------------
// DATA STRUCTURES: PARTICLES & FLOATING SCORE POPUP
// -----------------------------------------------------------------
struct SnakeParticle {
  float x, y;
  float speedX, speedY;
  uint16_t color;
  int life;
};

struct SnakeScorePopup {
  int x;          // top-left X on screen
  int y;          // current top-left Y on screen
  int prevY;      // last drawn Y for clean grass restore
  int w, h;
  int value;
  int life;
  int maxLife;
  bool drawn;
};

// -----------------------------------------------------------------
// GAME STATE VARIABLES
// -----------------------------------------------------------------
static Vec2 snakeBody[MAX_SNAKE_LEN];
static int snakeLength = 4;
static int currentDirection = 1; // 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT
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

// Visual FX
static SnakeParticle particles[16];
static int particleCount = 0;
static SnakeScorePopup scorePopup = {0, 0, 0, 28, 14, 1, 0, 18, false};

// -----------------------------------------------------------------
// SOUND EFFECTS
// -----------------------------------------------------------------
static void playBiteSound() {
  audio::play(audio::Cue::Pickup);
}

static void playDeathSound() {
<<<<<<< HEAD
  audio::play(audio::Cue::Death);
=======
  audio::play(audio::Cue::GameOver);
>>>>>>> codex/rally-3d
}

// -----------------------------------------------------------------
// ORIGINAL GRASS IMAGE & PLAYFIELD RESTORATION
// -----------------------------------------------------------------
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

// -----------------------------------------------------------------
// POLISHED HEADER & FOOTER HUD
// -----------------------------------------------------------------
static void drawModernHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(224, 40)) return;
  header.fillSprite(IOS_BG);
  header.fillRoundRect(0, 0, 224, 40, 12, ui::CARD);

  // Cute mini classic-blue snake badge on left
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

// -----------------------------------------------------------------
// CLASSIC SNAKE RENDERING (PREV STYLE + EXPRESSIVE TWO EYES)
// -----------------------------------------------------------------
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

  // Connect back seamlessly to neck segment
  int neckDir = (snakeLength > 1) ? snakeNeighborDirection(head, snakeBody[1]) : -1;
  tft.fillRoundRect(px, py, BLOCK_SIZE, BLOCK_SIZE, 3, SNAKE_HEAD);
  if (neckDir == 0) tft.fillRect(px, py, BLOCK_SIZE, 4, SNAKE_HEAD);
  else if (neckDir == 2) tft.fillRect(px, py + BLOCK_SIZE - 4, BLOCK_SIZE, 4, SNAKE_HEAD);
  else if (neckDir == 3) tft.fillRect(px, py, 4, BLOCK_SIZE, SNAKE_HEAD);
  else if (neckDir == 1) tft.fillRect(px + BLOCK_SIZE - 4, py, 4, BLOCK_SIZE, SNAKE_HEAD);

  // Crisp cute two eyes with pupils facing travel direction
  int e1x = px + 2, e1y = py + 3;
  int e2x = px + 7, e2y = py + 3;
  int pdx = 0, pdy = 0;

  switch (currentDirection) {
    case 0: // UP
      e1x = px + 2; e1y = py + 3;
      e2x = px + 7; e2y = py + 3;
      pdy = -1;
      break;
    case 2: // DOWN
      e1x = px + 2; e1y = py + 6;
      e2x = px + 7; e2y = py + 6;
      pdy = 1;
      break;
    case 3: // LEFT
      e1x = px + 3; e1y = py + 2;
      e2x = px + 3; e2y = py + 7;
      pdx = -1;
      break;
    case 1: // RIGHT
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

  // Compose the tapered tail or rounded turn with the original grass image in one transfer.
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

  // Plump glossy red apple with leaf & highlight over original grass
  tft.fillCircle(px + 4, py + 5, 4, 0xF800);
  tft.fillCircle(px + 6, py + 5, 3, 0xD800);
  tft.drawLine(px + 5, py + 1, px + 5, py + 2, 0x4A00);
  tft.fillTriangle(px + 5, py + 2, px + 8, py + 1, px + 7, py + 3, IOS_GREEN);
  tft.drawPixel(px + 3, py + 3, 0xFFFF);
  tft.drawPixel(px + 3, py + 4, 0xFFE0);
}

// Redraw any playfield cells overlapped by a pixel rectangle (grass + walls + apple + snake)
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

// -----------------------------------------------------------------
// PARTICLES & FLOATING SCORE INCREMENT ANIMATION ("+10", "+20"...)
// -----------------------------------------------------------------
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

  // Restore exact cells from previous popup position before drawing new lifted frame
  if (scorePopup.drawn) {
    restorePlayfieldRect(scorePopup.x, scorePopup.prevY, scorePopup.w, scorePopup.h);
  }

  TFT_eSprite sp(&tft);
  sp.setColorDepth(16);
  if (sp.createSprite(scorePopup.w, scorePopup.h)) {
    // Seed sprite with exact original grass pixels so rounded corners & fade blend seamlessly!
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

// -----------------------------------------------------------------
// FOOD GENERATION & COLLISION
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

// -----------------------------------------------------------------
// INPUT & MOVEMENT
// -----------------------------------------------------------------
void handleInput() {
  if (readShoot() == LOW) snakeShootReady = true;

  int cmd;
  while ((cmd = buttonsNextPress()) >= 0) {
    if (cmd == 4 && snakeShootReady) {
      audio::pauseMusic(true); gameState = 1; // Pause
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

  // Toroidal wrapping
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
<<<<<<< HEAD
    if (score % 10 == 0) audio::play(audio::Cue::Milestone);
=======
    if ((snakeLength - 4) % 10 == 0) audio::play(audio::Cue::Milestone);
>>>>>>> codex/rally-3d
    drawGrassyCell(newHead.x, newHead.y);

    if (gameSpeed_ms > 90) {
      gameSpeed_ms -= 5;
    }

    updateScoreHUD();
    generateFood();
    if (gameState == 0) drawApple();

    // Trigger floating score increment animation right near the eaten apple!
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

// -----------------------------------------------------------------
// POLISHED INTRO WINDOW, PAUSE & GAME OVER PANELS
// -----------------------------------------------------------------
static void renderIntroGrassPreview() {
  // Downsample the actual original grass image into a crisp framed preview window!
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

  // Draw selected brick wall layout proportionally inside the grass preview
  for (int y = 0; y < GRID_ROWS; ++y) {
    for (int x = 0; x < GRID_COLS; ++x) {
      if (snakeWallAt(x, y)) {
        int bx = PX + x * PW / GRID_COLS;
        int by = PY + y * PH / GRID_ROWS;
        drawSnakeBrick(bx, by, 5);
      }
    }
  }

  // Draw classic blue snake with cute two eyes & tapered tail on the grass preview
  int sy = PY + 42;
  int sx = PX + 26;
  // Tapered tail
  tft.fillTriangle(sx, sy + 3, sx + 6, sy, sx + 6, sy + 6, SNAKE_BODY_2);
  // Body segments
  for (int i = 0; i < 4; ++i) {
    tft.fillRect(sx + 6 + i * 7, sy, 7, 7, (i & 1) ? SNAKE_BODY_1 : SNAKE_BODY_2);
  }
  // Head with two eyes
  int hx = sx + 6 + 4 * 7;
  tft.fillRoundRect(hx, sy, 8, 7, 2, SNAKE_HEAD);
  tft.fillRect(hx, sy, 3, 7, SNAKE_HEAD);
  tft.fillCircle(hx + 5, sy + 1, 1, 0xFFFF);
  tft.fillCircle(hx + 5, sy + 5, 1, 0xFFFF);

  // Juicy apple + "+10" floating badge preview
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

  // Stage indicator dots
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
  if (cmd == 0 || cmd == 2) { // UP / DOWN
    int old = snakePauseSelection;
    snakePauseSelection = (snakePauseSelection + (cmd == 0 ? 3 : 1)) % 4;
    drawSnakePauseOption(old);
    drawSnakePauseOption(snakePauseSelection);
    playNavClick();
  } else if (cmd == 4) { // SHOOT: Select
    playSelectTone();
    if (snakePauseSelection == 0) { // Resume
      scorePopup.life = 0;
      scorePopup.drawn = false;
      showResumeCountdown(redrawFullSnakeScene);
      lastMoveTime = millis();
      lastFxTime = millis();
      gameState = 0;
      menu_drawn = false;
      snakeShootReady = false;
    } else if (snakePauseSelection == 1) { // Restart
      initGame();
    } else if (snakePauseSelection == 2) {
      showGameHighScores();
      redrawFullSnakeScene();
      drawSnakePauseMenu();
      snakePauseReady = false;
    } else { // Exit to main menu
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

// -----------------------------------------------------------------
// LIFECYCLE (setupSnake & loopSnake)
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

    // Smooth animation tick for particles and floating score popup.
    if (now - lastFxTime >= 30) {
      lastFxTime = now;
      if (particleCount > 0) updateParticles();
      if (scorePopup.life > 0 || scorePopup.drawn) renderScorePopupStep();
    }
    return;
  } else if (gameState == 1) { // Pause Menu
    if (!menu_drawn) {
      eraseScorePopup();
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
      drawModernHeader();
      renderIntroGrassPreview();
      renderIntroSelectorRow();
    } else if (cmd == 4) {
      playSelectTone();
      initGame();
    }
  }
}
