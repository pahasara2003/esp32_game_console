extern TFT_eSPI tft;
#include "game.h"
#include "ui_kit.h"
#include <Arduino.h>

// =================================================================
// MODERN VERTICAL BREAKOUT / ARKANOID (240x320 PORTRAIT ORIENTATION)
// =================================================================

// Portrait dimensions: 240 width x 320 height (Rotation 2)
// Header card: y = 3..44 (h = 42)
// Top Wall / Ceiling: y = 47
// Playfield Area: y = 48..290 (w = 240, h = 242)
// Paddle zone: y = 276, thickness = 8
// Bottom Dynamic Pill Bar: y = 294..319

constexpr int BO_WIDTH  = 240;
constexpr int BO_HEIGHT = 320;
constexpr int BO_PLAYFIELD_TOP = 48;
constexpr int BO_PLAYFIELD_BOTTOM = 290;

// Grid of Bricks
// 6 columns across 240px:
// Left margin = 7px. Each brick = 36px wide, gap = 2px.
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
constexpr uint16_t BO_AMBER        = 0xFE04;
constexpr uint16_t BO_CYAN_ACCENT  = 0x36FF;
constexpr uint16_t BO_EMERALD      = 0x26E9;
constexpr uint16_t BO_CORAL        = 0xFA8B;
constexpr uint16_t BO_HEADER_START = 0xFD20; // Vibrant Orange
constexpr uint16_t BO_HEADER_END   = 0xFFE0; // Gold
constexpr uint16_t BO_PADDLE       = 0xFFFF;
constexpr uint16_t BO_BALL         = 0xFFE8; // Sunlit Warm White-Gold Ball

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

// Vibrant row-based jewel spectrum from top (highest value) to bottom:
// Coral Rose -> Sunset Tangerine -> Sunlit Gold -> Mint Emerald -> Sky Cyan -> Electric Violet
static const uint16_t BO_ROW_PALETTE[BRICK_ROWS] = {
  0xF98B, // Row 0 (70 pts): Coral Rose
  0xFD24, // Row 1 (60 pts): Sunset Tangerine
  0xFE64, // Row 2 (50 pts): Sunlit Gold
  0x268C, // Row 3 (40 pts): Mint Emerald
  0x263F, // Row 4 (30 pts): Sky Cyan
  0xA33F  // Row 5 (20 pts): Electric Violet
};

static uint16_t breakoutBrickColor(uint8_t row, uint8_t shade = 0) {
  uint16_t base = BO_ROW_PALETTE[row % BRICK_ROWS];
  if (shade == 0) return base;
  return mixBreakoutColor(base, 0xFFFF, min(90, int(shade) * 22));
}

static const int ROW_POINTS[BRICK_ROWS] = { 70, 60, 50, 40, 30, 20 };
static int breakoutLevel = 1;

// Stylish Aurora-Glass Playfield Backdrop
constexpr int BO_GRID_SIZE = 12;
constexpr int BO_GRID_COLS = BO_WIDTH / BO_GRID_SIZE;
constexpr int BO_GRID_ROWS = (BO_PLAYFIELD_BOTTOM - BO_PLAYFIELD_TOP + 1 + BO_GRID_SIZE - 1) / BO_GRID_SIZE;
static uint8_t boBackdropGrid[BO_GRID_COLS][BO_GRID_ROWS];
static uint16_t boRowBg[BO_PLAYFIELD_BOTTOM - BO_PLAYFIELD_TOP + 1];
static uint16_t boRowGrid[BO_PLAYFIELD_BOTTOM - BO_PLAYFIELD_TOP + 1];

struct BreakoutStar {
  uint8_t x;
  uint16_t y;
  uint8_t bright; // 0..2
};
constexpr int BO_STAR_COUNT = 22;
static BreakoutStar boStars[BO_STAR_COUNT];

static void initBreakoutBackdropGrid() {
  for (int x = 0; x < BO_GRID_COLS; x++)
    for (int y = 0; y < BO_GRID_ROWS; y++)
      boBackdropGrid[x][y] = (x * 7 + y * 13 + x * y) % 4;

  // Precompute smooth modern twilight-aurora vertical gradient & subtle glass grid lines
  constexpr uint16_t TOP_COL = 0x1045; // Deep twilight indigo
  constexpr uint16_t MID_COL = 0x0887; // Rich ocean midnight
  constexpr uint16_t BOT_COL = 0x0928; // Subtle horizon teal-blue
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

// -----------------------------------------------------------------
// GAME STATE
// -----------------------------------------------------------------
static uint8_t bricks[BRICK_ROWS][BRICK_COLS]; // 0 = empty, 1 = normal, 2 = reinforced (2 hits)
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

// Power-ups
struct BreakoutPowerup {
  float x, y;
  int type; // 1 = Expand Paddle, 2 = Slow Ball, 3 = Extra Life
  bool active;
};
static BreakoutPowerup powerup;

// Visual Effects: Particles, Shockwave Rings, Floating Popups, Wall Glow
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
static int boSideFlashY = 0; // side flash is a short glow at the hit point, not full height
constexpr int BO_SIDE_FLASH_HALF = 20;

// -----------------------------------------------------------------
// SOUND EFFECTS
// -----------------------------------------------------------------
static uint8_t audioBrickCombo = 0;
<<<<<<< HEAD
static void soundPaddleHit() {
  audioBrickCombo = 0;
  audio::play(audio::Cue::Paddle);
=======

static void soundPaddleHit() {
  audioBrickCombo = 0;
  audio::play(audio::Cue::Paddle);
}

static void soundWallHit() {
  audio::play(audio::Cue::Wall);
>>>>>>> codex/rally-3d
}

static void soundBrickHit(uint16_t freq = 1319) {
  audioBrickCombo = min(12, int(audioBrickCombo) + 1);
<<<<<<< HEAD
  audio::play(audio::Cue::Brick, freq + audioBrickCombo * 35);
  if (audioBrickCombo % 5 == 0) audio::play(audio::Cue::Milestone);
=======
  audio::play(audio::Cue::Brick, freq + audioBrickCombo * 45);
  if (audioBrickCombo % 4 == 0) audio::play(audio::Cue::Sparkle);
}

static void soundBrickCrack() {
  audio::play(audio::Cue::Lock);
>>>>>>> codex/rally-3d
}

static void soundLostBall() {
  audioBrickCombo = 0;
  audio::play(audio::Cue::Hit);
}

static void soundPowerup() {
  audio::play(audio::Cue::Powerup);
}

// -----------------------------------------------------------------
// EFFECTS & DIRTY-BAND RENDERER
// -----------------------------------------------------------------
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

// 3x5 glyph drawn at 2x for floating "+pts" popups inside playfield bands
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

// Compose every layer before a band reaches the panel, including overlapping objects.
template<class Surface>
static void composeBreakout(Surface &canvas, int top, int bottom) {
  // 1. Aurora twilight gradient & subtle modern geometric glass grid
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

  // Soft horizon glow near paddle zone
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

  // Wall bounce neon edge flashes
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

  // 2. Vibrant glossy 3D jewel bricks
  for (int r = 0; r < BRICK_ROWS; ++r) {
    int y = BRICK_START_Y + r * (BRICK_HEIGHT + BRICK_GAP_Y);
    if (y > bottom || y + BRICK_HEIGHT <= top) continue;
    for (int c = 0; c < BRICK_COLS; ++c) {
      uint8_t state = bricks[r][c];
      if (!state) continue;
      int x = BRICK_START_X + c * (BRICK_WIDTH + BRICK_GAP_X);
      uint16_t base, hi, lo;
      if (state == 2) {
        // Platinum / titanium reinforced 2-hit brick
        bool cracked = brickHits[r][c] > 0;
        base = cracked ? 0x7BEF : 0xCE79;
        hi   = cracked ? 0xBDF7 : 0xFFFF;
        lo   = cracked ? 0x4228 : 0x6B6D;
      } else {
        base = breakoutBrickColor(r, boBrickShade[r][c]);
        hi   = mixBreakoutColor(base, 0xFFFF, 125);
        lo   = fadeBreakout(base, 135);
      }
      canvas.fillRoundRect(x, y, BRICK_WIDTH, BRICK_HEIGHT, 3, base);
      canvas.drawFastHLine(x + 2, y + 1, BRICK_WIDTH - 4, hi);
      canvas.drawFastHLine(x + 2, y + BRICK_HEIGHT - 1, BRICK_WIDTH - 4, lo);
      // Specular glass glint dot
      canvas.drawFastHLine(x + 4, y + 2, 6, 0xFFFF);
      if (state == 2 && brickHits[r][c] > 0) {
        // Crisp crack lines on damaged armor brick
        int mx = x + BRICK_WIDTH / 2;
        canvas.drawLine(mx - 4, y + 2, mx, y + 6, 0x2124);
        canvas.drawLine(mx, y + 6, mx + 4, y + BRICK_HEIGHT - 2, 0x2124);
      }
    }
  }

  // 3. Expanding shockwave rings
  for (int i = 0; i < BO_MAX_RINGS; ++i) {
    if (boRings[i].life > 0) {
      uint16_t rc = fadeBreakout(boRings[i].color, min(255, boRings[i].life * 38));
      canvas.drawCircle(boRings[i].x, boRings[i].y, boRings[i].radius, rc);
    }
  }

  // 4. Sparkle particles
  for (int i = 0; i < boParticleCount; ++i) {
    auto &p = boParticles[i];
    if (p.life > 0) {
      uint16_t pc = fadeBreakout(p.color, min(255, p.life * 26));
      canvas.fillCircle((int)p.x, (int)p.y, p.life > 7 ? 1 : 0, pc);
    }
  }

  // 5. Floating score popup ("+70")
  if (boPopup.life > 0) {
    int py = (int)boPopup.y;
    if (py + 10 >= top && py - 2 <= bottom) {
      uint16_t pc = fadeBreakout(boPopup.color, min(255, boPopup.life * 28));
      int digits = boPopup.value >= 100 ? 3 : 2;
      int px = boPopup.x - (digits * 8 + 6) / 2;
      // '+' sign
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

  // 6. Comet ball trail (cyan -> gold -> white gradient)
  for (int i = boTrailCount - 1; i >= 0; --i) {
    uint16_t tc = mixBreakoutColor(BO_HEADER_START, BO_CYAN_ACCENT, i * 255 / 7);
    canvas.fillCircle((int)boTrail[i].x, (int)boTrail[i].y, i < 3 ? 2 : 1,
                      fadeBreakout(tc, 165 - i * 18));
  }

  // 7. Glowing distinct power-up capsules
  if (powerup.active) {
    int x = (int)powerup.x, y = (int)powerup.y;
    uint16_t color = powerup.type == 1 ? BO_CYAN_ACCENT
                   : (powerup.type == 2 ? BO_AMBER : BO_CORAL);
    canvas.drawCircle(x, y, 7, fadeBreakout(color, 110));
    canvas.fillCircle(x, y, 5, color);
    canvas.drawPixel(x - 2, y - 2, 0xFFFF);
    if (powerup.type == 1) { // Expand: <-> horizontal arrows
      canvas.drawFastHLine(x - 3, y, 7, ui::INK_DARK);
      canvas.drawPixel(x - 2, y - 1, ui::INK_DARK);
      canvas.drawPixel(x + 2, y - 1, ui::INK_DARK);
    } else if (powerup.type == 2) { // Slow: '-' bar
      canvas.drawFastHLine(x - 2, y, 5, ui::INK_DARK);
    } else { // Extra life: '+' cross
      canvas.drawFastHLine(x - 2, y, 5, ui::INK_DARK);
      canvas.drawFastVLine(x, y - 2, 5, ui::INK_DARK);
    }
  }

  // 8. Sleek modern metallic-neon paddle with glowing edge bumper caps
  if (boPaddlePulse) {
    canvas.drawRoundRect(paddleX - 2, PADDLE_Y - 2,
                         paddleWidth + 4, PADDLE_HEIGHT + 4, 5,
                         fadeBreakout(BO_CYAN_ACCENT, boPaddlePulse * 25));
  }
  canvas.fillRoundRect(paddleX, PADDLE_Y, paddleWidth, PADDLE_HEIGHT, 4, 0xDEFB);
  canvas.fillRoundRect(paddleX + 1, PADDLE_Y + 1, paddleWidth - 2, PADDLE_HEIGHT - 2, 3, BO_PADDLE);
  // Cyan / orange energy core & end-caps
  uint16_t capCol = paddleWidth > BASE_PADDLE_WIDTH ? BO_CYAN_ACCENT : BO_HEADER_START;
  canvas.fillRoundRect(paddleX + 1, PADDLE_Y + 1, 6, PADDLE_HEIGHT - 2, 2, capCol);
  canvas.fillRoundRect(paddleX + paddleWidth - 7, PADDLE_Y + 1, 6, PADDLE_HEIGHT - 2, 2, capCol);
  canvas.drawFastHLine(paddleX + 9, PADDLE_Y + PADDLE_HEIGHT - 2, paddleWidth - 18, 0x9CF3);

  // 9. Luminous energy ball with halo & specular highlight
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

// -----------------------------------------------------------------
// MODERN HEADER & HUD (ui::CARD conventions, fonts 2 & 4 only)
// -----------------------------------------------------------------
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

// -----------------------------------------------------------------
// PADDLE, BALL & POWERUPS
// -----------------------------------------------------------------
static void spawnPowerup(int x, int y) {
  if (powerup.active) return;
  powerup.x = x + BRICK_WIDTH / 2.0f;
  powerup.y = y;
  powerup.type = random(1, 4); // 1 = expand, 2 = slow, 3 = extra life
  powerup.active = true;
}

static void updatePowerup() {
  if (!powerup.active) return;

  powerup.y += 1.15f; // Gentle floating descent

  // Catch with paddle
  if (powerup.y >= PADDLE_Y - 4 && powerup.y <= PADDLE_Y + PADDLE_HEIGHT &&
      powerup.x >= paddleX - 4 && powerup.x <= paddleX + paddleWidth + 4) {
    int px = (int)powerup.x;
    int ptype = powerup.type;
    powerup.active = false;
    soundPowerup();
    if (ptype == 1) { // Expand paddle
      paddleWidth = 64;
      paddleX = constrain(paddleX, 4, BO_WIDTH - paddleWidth - 4);
      boPaddlePulse = 10;
      score += 50;
      spawnParticles(px, PADDLE_Y, BO_CYAN_ACCENT);
      spawnPopup(px, PADDLE_Y - 8, 50, BO_CYAN_ACCENT);
    } else if (ptype == 2) { // Slow ball
      ballVelX *= 0.78f;
      ballVelY *= 0.78f;
      if (abs(ballVelY) < 1.65f) ballVelY = (ballVelY < 0 ? -1.65f : 1.65f);
      score += 40;
      spawnParticles(px, PADDLE_Y, BO_AMBER);
      spawnPopup(px, PADDLE_Y - 8, 40, BO_AMBER);
    } else if (ptype == 3) { // Extra life
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

  // Dropped out of bounds
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

// -----------------------------------------------------------------
// LEVEL INITIALIZATION
// -----------------------------------------------------------------
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

  // Initialize Bricks with stylish stage patterns
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

  // Clean subtle playfield boundary lines
  tft.drawFastHLine(0, BO_PLAYFIELD_TOP - 1, BO_WIDTH, ui::SEPARATOR);
  tft.drawFastHLine(0, BO_PLAYFIELD_BOTTOM + 1, BO_WIDTH, ui::SEPARATOR);

  drawBreakoutHeader();
  drawBreakoutBottomBar();
  resetBallOnPaddle();
  renderBreakoutFrame(true);
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

      int bx = BRICK_START_X + c * (BRICK_WIDTH + BRICK_GAP_X);
      int by = BRICK_START_Y + r * (BRICK_HEIGHT + BRICK_GAP_Y);

      // AABB overlap check between ball and brick
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

        // Brick damage / break
        if (bricks[r][c] == 2) { // Reinforced platinum brick
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
          // Musical major-pentatonic pitch rise from bottom row to top row
          static const uint16_t rowPitch[BRICK_ROWS] = {1568, 1319, 1175, 1047, 880, 784};
          soundBrickHit(rowPitch[r]);
          if (random(10) > 7) spawnPowerup(bx, by);
          remaining--;
        }

        updateScoreAndLivesHUD();
        if (remaining == 0) {
          audio::play(audio::Cue::Win);
          gameState = 3; // Level clear state
          menu_drawn = false;
        }
        return; // Handle one collision per frame for stability
      }
    }
  }

  if (remaining == 0) {
    audio::play(audio::Cue::Win);
<<<<<<< HEAD
    gameState = 3; // Level clear state
=======
    gameState = 3;
>>>>>>> codex/rally-3d
    menu_drawn = false;
  }
}

static void updateBallAndPhysics() {
  ballX += ballVelX;
  ballY += ballVelY;

  // Left & Right Wall bounce
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

  // Top Wall bounce
  if (ballY - BALL_SIZE / 2.0f <= BO_PLAYFIELD_TOP) {
    ballY = BO_PLAYFIELD_TOP + BALL_SIZE / 2.0f;
    ballVelY = abs(ballVelY);
    boCeilingFlash = 5;
    soundWallHit();
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

    boPaddlePulse = 10;
    spawnRing((int)ballX, PADDLE_Y, BO_CYAN_ACCENT);
    bool hadCombo = audioBrickCombo >= 2;
    soundPaddleHit();
    if (hadCombo) updateScoreAndLivesHUD();
  }

  // Bottom Void (Lost Life)
  if (ballY > BO_PLAYFIELD_BOTTOM) {
    soundLostBall();
    lives--;
    if (lives > 0) audio::play(audio::Cue::Lose);
    updateScoreAndLivesHUD();

    if (lives <= 0) {
<<<<<<< HEAD
      audio::play(audio::Cue::Death);
=======
      audio::play(audio::Cue::GameOver);
>>>>>>> codex/rally-3d
      gameState = 2; // Game Over
      menu_drawn = false;
      return;
    } else {
      paddleWidth = BASE_PADDLE_WIDTH;
      boTrailCount = 0;
      resetBallOnPaddle();
    }
  }

  // Check Bricks
  checkBrickCollisions();
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
    audio::pauseMusic(true); gameState = 1;
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
        tft.drawFastHLine(0, BO_PLAYFIELD_TOP - 1, BO_WIDTH, ui::SEPARATOR);
        tft.drawFastHLine(0, BO_PLAYFIELD_BOTTOM + 1, BO_WIDTH, ui::SEPARATOR);
        drawBreakoutHeader();
        drawBreakoutBottomBar();
        renderBreakoutFrame(true);
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
      tft.drawFastHLine(0, BO_PLAYFIELD_TOP - 1, BO_WIDTH, ui::SEPARATOR);
      tft.drawFastHLine(0, BO_PLAYFIELD_BOTTOM + 1, BO_WIDTH, ui::SEPARATOR);
      drawBreakoutHeader();
      drawBreakoutBottomBar();
      renderBreakoutFrame(true);
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

    // Fixed 16 ms frame so heavy effect frames don't slow the ball.
    uint32_t spent = millis() - frameStart;
    if (spent < 16) delay(16 - spent);
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
