extern TFT_eSPI tft;
#include "game.h"
#include "ui_kit.h"
#include <Arduino.h>

// =================================================================
// MODERN VERTICAL BREAKOUT / ARKANOID (240x320 PORTRAIT ORIENTATION)
// =================================================================

// Portrait dimensions: 240 width x 320 height (Rotation 2)
// Header Card: y = 5..43 (h = 38)
// Top Wall / Ceiling: y = 47
// Playfield Area: y = 48..290 (w = 240, h = 242)
// Paddle zone: y = 278, thickness = 8
// Bottom Dynamic Pill Bar: y = 295..316 (h = 21)

constexpr int BO_WIDTH  = 240;
constexpr int BO_HEIGHT = 320;
constexpr int BO_PLAYFIELD_TOP = 48;
constexpr int BO_PLAYFIELD_BOTTOM = 290;

// Grid of Bricks
// 6 columns across 240px:
// Left margin = 6px. Each brick = 36px wide, gap = 2px.
// 6 * 36 + 5 * 2 = 216 + 10 = 226px + 7px left/right margins = 240px!
constexpr int BRICK_ROWS = 6;
constexpr int BRICK_COLS = 6;
constexpr int BRICK_WIDTH = 36;
constexpr int BRICK_HEIGHT = 11;
constexpr int BRICK_GAP_X = 2;
constexpr int BRICK_GAP_Y = 3;
constexpr int BRICK_START_X = 7;
constexpr int BRICK_START_Y = 56;

// Paddle & Ball
constexpr int BASE_PADDLE_WIDTH = 46;
constexpr int PADDLE_HEIGHT = 8;
constexpr int PADDLE_Y = 276;
constexpr int PADDLE_SPEED = 4;
constexpr int BALL_SIZE = 6;
constexpr float BALL_SPEED_INIT = 2.25f;

// Colors (Consistent with Game Console OLED Modern Theme)
#define BO_BG UI_BG
#define BO_CARD UI_PANEL
#define BO_SURFACE UI_SURFACE
#define BO_WHITE UI_TEXT
#define BO_MUTED UI_MUTED
#define BO_CYAN UI_CYAN
constexpr uint16_t BO_GREEN   = 0x3666;
constexpr uint16_t BO_AMBER   = 0xFE04;
constexpr uint16_t BO_RED     = 0xF800; // Pure bright red for lives and danger states
constexpr uint16_t BO_HEADER_START = 0xFD20; // Orange
constexpr uint16_t BO_HEADER_END   = 0xFFE0; // Gold
constexpr uint16_t BO_DARK_TEXT    = 0x20A0;
constexpr uint16_t BO_FRAME        = 0x6A20;
constexpr uint16_t BO_PADDLE  = 0xFFFF;
constexpr uint16_t BO_BALL    = 0xFFE0; // Neon Yellow Ball

// Brick Colors by Row (Lush vibrant spectrum)
static const uint16_t ROW_BRICK_COLORS[BRICK_ROWS] = {
  0xF800, // Crimson Red (Row 0 - Top)
  0xFD20, // Vibrant Orange (Row 1)
  0xFFE0, // Bright Gold (Row 2)
  0x07E0, // Vibrant Emerald Green (Row 3)
  0x07FF, // Electric Cyan (Row 4)
  0x91BF  // Neon Violet (Row 5 - Bottom)
};

static const int ROW_POINTS[BRICK_ROWS] = { 70, 60, 50, 40, 30, 20 };
static int breakoutLevel = 1;

constexpr int BO_GRID_SIZE = 10;
constexpr int BO_GRID_COLS = BO_WIDTH / BO_GRID_SIZE;
constexpr int BO_GRID_ROWS = (BO_PLAYFIELD_BOTTOM - BO_PLAYFIELD_TOP + 1 + BO_GRID_SIZE - 1) / BO_GRID_SIZE;
static uint8_t boBackdropGrid[BO_GRID_COLS][BO_GRID_ROWS];
static const uint16_t BO_GRID_COLORS[] = {0x0003, 0x0005, 0x0007, 0x0808};
constexpr uint16_t BO_GRID_LINE = 0x0001;

static void initBreakoutBackdropGrid() {
  for (int x = 0; x < BO_GRID_COLS; x++)
    for (int y = 0; y < BO_GRID_ROWS; y++)
      boBackdropGrid[x][y] = random(0, 4);
}

static uint16_t breakoutBackdropColor(int x, int y) {
  if (y < BO_PLAYFIELD_TOP || y > BO_PLAYFIELD_BOTTOM) return BO_BG;
  if ((x % BO_GRID_SIZE) == 0 || ((y - BO_PLAYFIELD_TOP) % BO_GRID_SIZE) == 0)
    return BO_GRID_LINE;
  int gx = constrain(x / BO_GRID_SIZE, 0, BO_GRID_COLS - 1);
  int gy = constrain((y - BO_PLAYFIELD_TOP) / BO_GRID_SIZE, 0, BO_GRID_ROWS - 1);
  return BO_GRID_COLORS[boBackdropGrid[gx][gy]];
}

static void restoreBreakoutBackdropRect(int x, int y, int w, int h) {
  int x0 = max(0, x);
  int x1 = min(BO_WIDTH, x + w);
  int y0 = max(BO_PLAYFIELD_TOP, y);
  int y1 = min(BO_PLAYFIELD_BOTTOM + 1, y + h);
  if (x1 <= x0 || y1 <= y0) return;

  tft.startWrite();
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++)
      tft.drawPixel(xx, yy, breakoutBackdropColor(xx, yy));
  tft.endWrite();
}

static void drawBreakoutBackdrop() {
  for (int gy = 0; gy < BO_GRID_ROWS; gy++) {
    int y = BO_PLAYFIELD_TOP + gy * BO_GRID_SIZE;
    int h = min(BO_GRID_SIZE, BO_PLAYFIELD_BOTTOM - y + 1);
    for (int gx = 0; gx < BO_GRID_COLS; gx++) {
      int x = gx * BO_GRID_SIZE;
      tft.fillRect(x, y, BO_GRID_SIZE, h, BO_GRID_COLORS[boBackdropGrid[gx][gy]]);
    }
  }
  for (int x = 0; x < BO_WIDTH; x += BO_GRID_SIZE)
    tft.drawFastVLine(x, BO_PLAYFIELD_TOP, BO_PLAYFIELD_BOTTOM - BO_PLAYFIELD_TOP + 1, BO_GRID_LINE);
  for (int y = BO_PLAYFIELD_TOP; y <= BO_PLAYFIELD_BOTTOM; y += BO_GRID_SIZE)
    tft.drawFastHLine(0, y, BO_WIDTH, BO_GRID_LINE);
}

// -----------------------------------------------------------------
// GAME STATE
// -----------------------------------------------------------------
static uint8_t bricks[BRICK_ROWS][BRICK_COLS]; // 0 = empty, 1 = normal, 2 = silver (2 hits)
static uint8_t brickHits[BRICK_ROWS][BRICK_COLS];

static int paddleX = (BO_WIDTH - BASE_PADDLE_WIDTH) / 2;
static int prevPaddleX = paddleX;
static int paddleWidth = BASE_PADDLE_WIDTH;
static int breakoutLastDirection = 0;

static float ballX = BO_WIDTH / 2.0f;
static float ballY = PADDLE_Y - BALL_SIZE - 2;
static float prevBallX = ballX;
static float prevBallY = ballY;
static float ballVelX = BALL_SPEED_INIT * 0.65f;
static float ballVelY = -BALL_SPEED_INIT;

static bool ballLaunched = false;
static int lives = 3;
static int pauseSelection = 0;
static const char *const BO_PAUSE_ITEMS[] = {"Resume", "Restart", "High scores", "Main menu"};
static bool pauseReady = false;

// Power-ups
struct BreakoutPowerup {
  float x, y;
  int type; // 1 = Expand Paddle, 2 = Slow Ball, 3 = Extra Life
  bool active;
};
static BreakoutPowerup powerup;

// Particles
struct BreakoutParticle {
  float x, y;
  float vx, vy;
  uint16_t color;
  int life;
};
static BreakoutParticle boParticles[16];
static int boParticleCount = 0;

// -----------------------------------------------------------------
// SOUND EFFECTS
// -----------------------------------------------------------------
static uint8_t audioBrickCombo = 0;
static void soundPaddleHit() {
  audioBrickCombo = 0;
  audio::play(audio::Cue::Paddle);
}

static void soundBrickHit(uint16_t freq = 1319) {
  audioBrickCombo = min(12, int(audioBrickCombo) + 1);
  audio::play(audio::Cue::Brick, freq + audioBrickCombo * 35);
  if (audioBrickCombo % 5 == 0) audio::play(audio::Cue::Milestone);
}

static void soundLostBall() {
  audioBrickCombo = 0;
  audio::play(audio::Cue::Hit);
}

static void soundPowerup() {
  audio::play(audio::Cue::Powerup);
}

// -----------------------------------------------------------------
// PARTICLES
// -----------------------------------------------------------------
static void spawnParticles(int cx, int cy, uint16_t color) {
  boParticleCount = 0;
  for (int i = 0; i < 12; i++) {
    boParticles[i].x = cx;
    boParticles[i].y = cy;
    float angle = (i / 12.0f) * 6.283f;
    float speed = random(10, 26) / 10.0f;
    boParticles[i].vx = cos(angle) * speed;
    boParticles[i].vy = sin(angle) * speed;
    boParticles[i].color = color;
    boParticles[i].life = random(6, 12);
    boParticleCount++;
  }
}

static void updateAndDrawParticles() {
  for (int i = 0; i < boParticleCount; i++) {
    if (boParticles[i].life > 0) {
      tft.drawPixel((int)boParticles[i].x, (int)boParticles[i].y, BO_BG);
      boParticles[i].x += boParticles[i].vx;
      boParticles[i].y += boParticles[i].vy;
      boParticles[i].vy += 0.2f; // subtle gravity
      boParticles[i].life--;

      int nx = (int)boParticles[i].x;
      int ny = (int)boParticles[i].y;
      if (boParticles[i].life > 0 && nx >= 0 && nx < BO_WIDTH && ny >= BO_PLAYFIELD_TOP && ny <= BO_PLAYFIELD_BOTTOM) {
        tft.drawPixel(nx, ny, boParticles[i].color);
      }
    }
  }
}

// -----------------------------------------------------------------
// BRICKS RENDERING
// -----------------------------------------------------------------
static void getBrickRect(int r, int c, int &bx, int &by) {
  bx = BRICK_START_X + c * (BRICK_WIDTH + BRICK_GAP_X);
  by = BRICK_START_Y + r * (BRICK_HEIGHT + BRICK_GAP_Y);
}

static void drawBrick(int r, int c) {
  if (bricks[r][c] == 0) return;
  int bx, by;
  getBrickRect(r, c, bx, by);

  uint16_t col;
  if (bricks[r][c] == 2) { // Silver brick (2-hit)
    col = (brickHits[r][c] == 0) ? 0xCE79 : 0x8410;
  } else {
    col = ROW_BRICK_COLORS[r];
  }

  // Rounded glossy brick with top highlight
  tft.fillRoundRect(bx, by, BRICK_WIDTH, BRICK_HEIGHT, 2, col);
  tft.drawFastHLine(bx + 1, by, BRICK_WIDTH - 2, 0xFFFF);
}

static void eraseBrick(int r, int c) {
  int bx, by;
  getBrickRect(r, c, bx, by);
  restoreBreakoutBackdropRect(bx, by, BRICK_WIDTH, BRICK_HEIGHT);
}

static void renderAllBricks() {
  tft.startWrite();
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      drawBrick(r, c);
    }
  }
  tft.endWrite();
}

// -----------------------------------------------------------------
// MODERN IPHONE HEADER & HUD
// -----------------------------------------------------------------
static void drawBreakoutHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(224, 38)) return;
  header.fillSprite(BO_BG);
  header.fillRoundRect(0, 0, 224, 38, 12, ui::CARD);
  header.setTextDatum(ML_DATUM);
  header.setTextColor(ui::LABEL);
  header.drawString("Breakout", 12, 20, 4);
  header.setTextDatum(TR_DATUM);
  header.setTextColor(ui::SECONDARY);
  header.drawString("Score", 150, 3, 2);
  header.drawString("Lives", 212, 3, 2);
  header.setTextColor(ui::LABEL);
  header.drawString(String(score), 150, 19, 2);
  for (int i = 0; i < 3; i++)
    header.fillCircle(184 + i * 12, 27, 4, i < lives ? BO_RED : 0x4208);
  header.pushSprite(8, 5);
  header.deleteSprite();
}

static void updateScoreAndLivesHUD() {
  drawBreakoutHeader();
}

static void drawBreakoutBottomBar() {
  tft.fillRect(0, 294, 240, 26, BO_BG);
  ui::hint(306, ballLaunched ? "Shoot to pause" : "Up to launch", BO_BG);
}

// -----------------------------------------------------------------
// PADDLE, BALL & POWERUPS
// -----------------------------------------------------------------
static void drawPaddle(int x, uint16_t color) {
  tft.fillRoundRect(x, PADDLE_Y, paddleWidth, PADDLE_HEIGHT, 3, color);
  // Glossy highlight on paddle
  if (color == BO_PADDLE) {
    tft.drawFastHLine(x + 2, PADDLE_Y, paddleWidth - 4, 0xCE79);
  }
}

static void erasePaddle(int x) {
  restoreBreakoutBackdropRect(x, PADDLE_Y, paddleWidth, PADDLE_HEIGHT);
}

static void drawBall(float x, float y) {
  tft.fillCircle((int)x, (int)y, BALL_SIZE / 2, BO_BALL);
}

static void eraseBall(float x, float y) {
  int ix = (int)x;
  int iy = (int)y;
  int rad = BALL_SIZE / 2 + 1;
  restoreBreakoutBackdropRect(ix - rad, iy - rad, rad * 2 + 1, rad * 2 + 1);

  // If the erase circle overlapped the top or bottom boundary line, repair the line immediately!
  if (iy - rad <= BO_PLAYFIELD_TOP) {
    tft.drawFastHLine(0, BO_PLAYFIELD_TOP - 1, BO_WIDTH, BO_SURFACE);
  }
  if (iy + rad >= BO_PLAYFIELD_BOTTOM) {
    tft.drawFastHLine(0, BO_PLAYFIELD_BOTTOM + 1, BO_WIDTH, BO_SURFACE);
  }
  // If the erase circle overlapped the paddle area, restore the paddle
  if (iy + rad >= PADDLE_Y && iy - rad <= PADDLE_Y + PADDLE_HEIGHT &&
      ix + rad >= paddleX && ix - rad <= paddleX + paddleWidth) {
    drawPaddle(paddleX, BO_PADDLE);
  }

  // Also repair any neighboring bricks that might have been nicked by the ball erase
  int startCol = (ix - rad - BRICK_START_X) / (BRICK_WIDTH + BRICK_GAP_X);
  int endCol   = (ix + rad - BRICK_START_X) / (BRICK_WIDTH + BRICK_GAP_X);
  int startRow = (iy - rad - BRICK_START_Y) / (BRICK_HEIGHT + BRICK_GAP_Y);
  int endRow   = (iy + rad - BRICK_START_Y) / (BRICK_HEIGHT + BRICK_GAP_Y);

  startCol = constrain(startCol, 0, BRICK_COLS - 1);
  endCol   = constrain(endCol, 0, BRICK_COLS - 1);
  startRow = constrain(startRow, 0, BRICK_ROWS - 1);
  endRow   = constrain(endRow, 0, BRICK_ROWS - 1);

  for (int r = startRow; r <= endRow; r++) {
    for (int c = startCol; c <= endCol; c++) {
      if (bricks[r][c] > 0) {
        int bx, by;
        getBrickRect(r, c, bx, by);
        // Check if erase circle actually touched this brick
        if (ix + rad >= bx && ix - rad <= bx + BRICK_WIDTH &&
            iy + rad >= by && iy - rad <= by + BRICK_HEIGHT) {
          drawBrick(r, c);
        }
      }
    }
  }
}

static void spawnPowerup(int x, int y) {
  if (powerup.active) return;
  powerup.x = x + BRICK_WIDTH / 2.0f;
  powerup.y = y;
  powerup.type = random(1, 4); // 1 = expand, 2 = slow, 3 = extra life
  powerup.active = true;
}

static void updatePowerup() {
  if (!powerup.active) return;
  // Erase old
  restoreBreakoutBackdropRect((int)powerup.x - 6, (int)powerup.y - 6, 13, 13);

  powerup.y += 1.15f; // Gentle floating descent

  // Catch with paddle
  if (powerup.y >= PADDLE_Y - 4 && powerup.y <= PADDLE_Y + PADDLE_HEIGHT &&
      powerup.x >= paddleX - 4 && powerup.x <= paddleX + paddleWidth + 4) {
    powerup.active = false;
    soundPowerup();
    if (powerup.type == 1) { // Expand paddle
      erasePaddle(paddleX);
      paddleWidth = 62;
      drawPaddle(paddleX, BO_PADDLE);
      score += 50;
    } else if (powerup.type == 2) { // Slow ball
      ballVelX *= 0.75f;
      ballVelY *= 0.75f;
      score += 40;
    } else if (powerup.type == 3) { // Extra life
      if (lives < 3) lives++;
      score += 100;
    }
    updateScoreAndLivesHUD();
    return;
  }

  // Dropped out of bounds
  if (powerup.y > BO_PLAYFIELD_BOTTOM) {
    powerup.active = false;
    return;
  }

  // Draw power-up capsule
  uint16_t pCol = (powerup.type == 1) ? BO_CYAN : (powerup.type == 2 ? BO_AMBER : BO_GREEN);
  tft.fillCircle((int)powerup.x, (int)powerup.y, 5, pCol);
  tft.drawCircle((int)powerup.x, (int)powerup.y, 5, 0xFFFF);
}

static void resetBallOnPaddle() {
  ballX = paddleX + paddleWidth / 2.0f;
  ballY = PADDLE_Y - BALL_SIZE / 2.0f - 2;
  prevBallX = ballX;
  prevBallY = ballY;
  float dirX = (random(2) == 0 ? 1.0f : -1.0f);
  ballVelX = BALL_SPEED_INIT * 0.65f * dirX;
  ballVelY = -BALL_SPEED_INIT;
  ballLaunched = false;
  drawBreakoutBottomBar();
}

// -----------------------------------------------------------------
// LEVEL INITIALIZATION
// -----------------------------------------------------------------
static void initBreakoutLevel(int lvl) {
  breakoutLevel = lvl;
  initBreakoutBackdropGrid();
  paddleWidth = BASE_PADDLE_WIDTH;
  paddleX = (BO_WIDTH - paddleWidth) / 2;
  prevPaddleX = paddleX;
  breakoutLastDirection = 0;
  powerup.active = false;

  // Initialize Bricks
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      if (lvl == 1) {
        bricks[r][c] = 1; // Standard colorful rows
      } else if (lvl == 2) {
        // Silver 2-hit bricks on top row
        bricks[r][c] = (r == 0) ? 2 : 1;
      } else {
        // Alternating strong bricks
        bricks[r][c] = ((r + c) % 3 == 0) ? 2 : 1;
      }
      brickHits[r][c] = 0;
    }
  }

  tft.fillScreen(BO_BG);
  drawBreakoutBackdrop();

  // Playfield separator lines
  tft.drawFastHLine(0, BO_PLAYFIELD_TOP - 1, BO_WIDTH, BO_SURFACE);
  tft.drawFastHLine(0, BO_PLAYFIELD_BOTTOM + 1, BO_WIDTH, BO_SURFACE);

  drawBreakoutHeader();
  renderAllBricks();
  drawBreakoutBottomBar();
  drawPaddle(paddleX, BO_PADDLE);
  resetBallOnPaddle();
  drawBall(ballX, ballY);
}

// -----------------------------------------------------------------
// GAME LOGIC & COLLISIONS
// -----------------------------------------------------------------
static void checkBrickCollisions() {
  int remaining = 0;

  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      if (bricks[r][c] == 0) continue;
      remaining++;

      int bx, by;
      getBrickRect(r, c, bx, by);

      // AABB overlap check between ball and brick
      if (ballX + BALL_SIZE / 2.0f >= bx &&
          ballX - BALL_SIZE / 2.0f <= bx + BRICK_WIDTH &&
          ballY + BALL_SIZE / 2.0f >= by &&
          ballY - BALL_SIZE / 2.0f <= by + BRICK_HEIGHT) {

        // Determine horizontal or vertical bounce
        float ballCenterX = ballX;
        float ballCenterY = ballY;
        float brickCenterX = bx + BRICK_WIDTH / 2.0f;
        float brickCenterY = by + BRICK_HEIGHT / 2.0f;

        float diffX = ballCenterX - brickCenterX;
        float diffY = ballCenterY - brickCenterY;

        if (abs(diffX) / BRICK_WIDTH > abs(diffY) / BRICK_HEIGHT) {
          ballVelX = (diffX > 0) ? abs(ballVelX) : -abs(ballVelX);
        } else {
          ballVelY = (diffY > 0) ? abs(ballVelY) : -abs(ballVelY);
        }

        // Brick damage / break
        if (bricks[r][c] == 2) { // Silver brick
          brickHits[r][c]++;
          if (brickHits[r][c] >= 2) {
            bricks[r][c] = 0;
            score += 50;
            eraseBrick(r, c);
            // explosion effect removed
            soundBrickHit(1568);
            if (random(10) > 6) spawnPowerup(bx, by);
          } else {
            drawBrick(r, c);
            soundBrickHit(880);
          }
        } else {
          bricks[r][c] = 0;
          score += ROW_POINTS[r];
          eraseBrick(r, c);
          // explosion effect removed
          soundBrickHit(1175 + r * 60);
          if (random(10) > 7) spawnPowerup(bx, by);
        }

        updateScoreAndLivesHUD();
        return; // Handle one collision per frame for stability
      }
    }
  }

  // Level Clear check
  if (remaining == 0) {
    audio::play(audio::Cue::Win);
    gameState = 3; // Level clear state
    menu_drawn = false;
  }
}

static void updateBallAndPhysics() {
  eraseBall(prevBallX, prevBallY);

  ballX += ballVelX;
  ballY += ballVelY;

  // Left & Right Wall bounce
  if (ballX - BALL_SIZE / 2.0f <= 1) {
    ballX = 1 + BALL_SIZE / 2.0f;
    ballVelX = abs(ballVelX);
    soundPaddleHit();
  } else if (ballX + BALL_SIZE / 2.0f >= BO_WIDTH - 2) {
    ballX = BO_WIDTH - 2 - BALL_SIZE / 2.0f;
    ballVelX = -abs(ballVelX);
    soundPaddleHit();
  }

  // Top Wall bounce
  if (ballY - BALL_SIZE / 2.0f <= BO_PLAYFIELD_TOP) {
    ballY = BO_PLAYFIELD_TOP + BALL_SIZE / 2.0f;
    ballVelY = abs(ballVelY);
    soundPaddleHit();
  }

  // Paddle Collision
  if (ballVelY > 0 &&
      ballY + BALL_SIZE / 2.0f >= PADDLE_Y &&
      ballY - BALL_SIZE / 2.0f <= PADDLE_Y + PADDLE_HEIGHT &&
      ballX + BALL_SIZE / 2.0f >= paddleX - 1 &&
      ballX - BALL_SIZE / 2.0f <= paddleX + paddleWidth + 1) {

    ballY = PADDLE_Y - BALL_SIZE / 2.0f - 1;
    ballVelY = -abs(ballVelY);

    // Angle of deflection based on impact offset from paddle center
    float offset = (ballX - (paddleX + paddleWidth / 2.0f)) / (paddleWidth / 2.0f);
    ballVelX = offset * (BALL_SPEED_INIT * 1.3f);
    if (abs(ballVelX) < 0.6f) ballVelX = (ballVelX >= 0) ? 0.8f : -0.8f;

    // Crisp immediate redraw of the bat upon ball strike
    drawPaddle(paddleX, BO_PADDLE);
    soundPaddleHit();
  }

  // Bottom Void (Lost Life)
  if (ballY > BO_PLAYFIELD_BOTTOM) {
    soundLostBall();
    lives--;
    updateScoreAndLivesHUD();

    if (lives <= 0) {
      audio::play(audio::Cue::Death);
      gameState = 2; // Game Over
      menu_drawn = false;
      return;
    } else {
      erasePaddle(paddleX);
      paddleWidth = BASE_PADDLE_WIDTH;
      drawPaddle(paddleX, BO_PADDLE);
      resetBallOnPaddle();
    }
  }

  // Check Bricks
  checkBrickCollisions();

  drawBall(ballX, ballY);
  prevBallX = ballX;
  prevBallY = ballY;
}

static void handleBreakoutInput() {
  // The scanner queues confirmed presses, including taps shorter than a frame.
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
    gameState = 1;
    menu_drawn = false;
    pauseSelection = 0;
    pauseReady = false;
    return;
  }

  // Read held state without draining any presses arriving during this frame.
  uint8_t held = buttonsHeld();
  bool rightHeld = (held & 2) != 0;
  bool leftHeld = (held & 8) != 0;
  if (pressedDirection != 0) breakoutLastDirection = pressedDirection;
  // Preserve a short confirmed tap even if it ended during drawing or sound.
  // During overlapping holds, the most recent press wins instead of cancelling.
  int direction = pressedDirection;
  if (direction == 0) {
    if (rightHeld && leftHeld) direction = breakoutLastDirection;
    else if (rightHeld) direction = 1;
    else if (leftHeld) direction = -1;
  }
  paddleX = constrain(paddleX + direction * PADDLE_SPEED,
                      4, BO_WIDTH - paddleWidth - 4);

  // If paddle moved, redraw immediately
  if (paddleX != prevPaddleX) {
    erasePaddle(prevPaddleX);
    drawPaddle(paddleX, BO_PADDLE);
    prevPaddleX = paddleX;

    // Ball stays glued to paddle before launch
    if (!ballLaunched) {
      eraseBall(prevBallX, prevBallY);
      ballX = paddleX + paddleWidth / 2.0f;
      ballY = PADDLE_Y - BALL_SIZE / 2.0f - 2;
      drawBall(ballX, ballY);
      prevBallX = ballX;
      prevBallY = ballY;
    }
  }
  if (launchPressed && !ballLaunched) {
    ballLaunched = true;
    playSelectTone();
    drawBreakoutBottomBar();
  }
}

// -----------------------------------------------------------------
// MODAL SCREENS (START / PAUSE / GAME OVER / LEVEL CLEAR)
// -----------------------------------------------------------------
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
  if (cmd == 0 || cmd == 2) { // UP / DOWN
    int old = pauseSelection;
    pauseSelection = (pauseSelection + (cmd == 0 ? 3 : 1)) % 4;
    drawBreakoutPauseOption(old);
    drawBreakoutPauseOption(pauseSelection);
    playNavClick();
  } else if (cmd == 4) { // SHOOT: Select
    playSelectTone();
    if (pauseSelection == 0) { // Resume
      showResumeCountdown([]() {
      if (!restoringCountdownDigit()) tft.fillScreen(BO_BG);
      drawBreakoutBackdrop();
      tft.drawFastHLine(0, BO_PLAYFIELD_TOP - 1, BO_WIDTH, BO_SURFACE);
      tft.drawFastHLine(0, BO_PLAYFIELD_BOTTOM + 1, BO_WIDTH, BO_SURFACE);
      drawBreakoutHeader();
      renderAllBricks();
      drawBreakoutBottomBar();
      drawPaddle(paddleX, BO_PADDLE);
      drawBall(ballX, ballY);
      if (powerup.active) {
        uint16_t color = powerup.type == 1 ? BO_CYAN
                         : powerup.type == 2 ? BO_AMBER : BO_GREEN;
        tft.fillCircle((int)powerup.x, (int)powerup.y, 5, color);
        tft.drawCircle((int)powerup.x, (int)powerup.y, 5, 0xFFFF);
      }
      });
      gameState = 0;
      menu_drawn = false;
    } else if (pauseSelection == 1) { // Restart
      score = 0;
      lives = 3;
      initBreakoutLevel(1);
      gameState = 0;
      menu_drawn = false;
    } else if (pauseSelection == 2) {
      showGameHighScores();
      tft.fillScreen(BO_BG);
      drawBreakoutBackdrop();
      tft.drawFastHLine(0, BO_PLAYFIELD_TOP - 1, BO_WIDTH, BO_SURFACE);
      tft.drawFastHLine(0, BO_PLAYFIELD_BOTTOM + 1, BO_WIDTH, BO_SURFACE);
      drawBreakoutHeader();
      renderAllBricks();
      drawBreakoutBottomBar();
      drawPaddle(paddleX, BO_PADDLE);
      drawBall(ballX, ballY);
      if (powerup.active) {
        uint16_t color = powerup.type == 1 ? BO_CYAN
                         : powerup.type == 2 ? BO_AMBER : BO_GREEN;
        tft.fillCircle((int)powerup.x, (int)powerup.y, 5, color);
        tft.drawCircle((int)powerup.x, (int)powerup.y, 5, 0xFFFF);
      }
      drawBreakoutPauseMenu();
      pauseReady = false;
    } else { // Exit
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
  tft.fillRoundRect(24, 75, 192, 175, 12, BO_CARD);
  tft.drawRoundRect(24, 75, 192, 175, 12, BO_FRAME);

  tft.fillRoundRect(38, 89, 164, 28, 6, BO_HEADER_START);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(BO_DARK_TEXT, BO_HEADER_START);
  tft.drawString("LEVEL CLEARED!", 50, 97);

  tft.setTextFont(1);
  tft.setTextColor(BO_MUTED, BO_CARD);
  tft.drawString("LEVEL BONUS: +" + String(lives * 200), 38, 138);

  tft.setTextFont(2);
  tft.setTextColor(BO_HEADER_END, BO_CARD);
  tft.drawString("SCORE: " + String(score), 38, 158);

  tft.drawFastHLine(38, 186, 164, BO_SURFACE);
  tft.setTextFont(1);
  tft.setTextColor(BO_HEADER_END, BO_CARD);
  tft.drawString("SHOOT  NEXT STAGE", 38, 202);
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

// -----------------------------------------------------------------
// LIFECYCLE (breakoutSetup & breakoutLoop)
// -----------------------------------------------------------------
void breakoutSetup() {
  tft.setRotation(2); // Strict Vertical / Portrait Orientation
  score = 0;
  lives = 3;
  showBreakoutStartScreen();
}

void breakoutLoop() {
  audio::gameState(gameState);
  if (gameState == 0) {
    handleBreakoutInput();
    if (gameState != 0) return;

    if (ballLaunched) {
      updateBallAndPhysics();
    }
    updatePowerup();
    // particles removed

    delay(12); // ~60 FPS smooth motion
    return;
  } else if (gameState == 1) { // Pause Menu
    if (!menu_drawn) {
      drawBreakoutPauseMenu();
      menu_drawn = true;
    }
    handleBreakoutPause();
  } else if (gameState == 2) { // Game Over
    if (!menu_drawn) {
      drawBreakoutGameOver();
      menu_drawn = true;
    }
    handleBreakoutGameOver();
  } else if (gameState == 3) { // Level Complete
    if (!menu_drawn) {
      drawBreakoutLevelComplete();
      menu_drawn = true;
    }
    handleBreakoutLevelComplete();
  } else if (gameState == -1) { // Title Screen
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
