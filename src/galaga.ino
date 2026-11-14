#include "game.h"
#include "ui_kit.h"
#include <Arduino.h>

// =================================================================
// MODERN VERTICAL SPACE SHOOTER / GALAGA (240x320 PORTRAIT)
// =================================================================

// Portrait dimensions: 240 width x 320 height (Rotation 2)
// Header Card: y = 5..43 (h = 38)
// Playfield Area: y = 48..290 (w = 240, h = 242)
// Player ship base: y = 268
// Bottom Dynamic Pill Bar: y = 295..316 (h = 21)

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
constexpr uint16_t GALA_LASER_COLOR = 0xF800;

constexpr int MAX_PLAYER_BULLETS = 4;
constexpr int MAX_ALIEN_BULLETS = 6;
constexpr int ALIEN_ROWS = 3;
constexpr int ALIEN_COLS = 5;
constexpr int TOTAL_ALIENS = ALIEN_ROWS * ALIEN_COLS; // 15 aliens per fleet

// Modern Colors (OLED Dark UI)
#define GALA_BG UI_BG
#define GALA_CARD UI_PANEL
#define GALA_SURFACE UI_SURFACE
#define GALA_WHITE UI_TEXT
#define GALA_MUTED UI_MUTED
#define GALA_CYAN UI_CYAN
constexpr uint16_t GALA_GREEN   = 0x3666;
constexpr uint16_t GALA_AMBER   = 0xFE04;
constexpr uint16_t GALA_RED     = 0xFA48;
constexpr uint16_t GALA_PURPLE  = 0xB37F;
constexpr uint16_t GALA_DARK_TEXT = 0x2087;
constexpr uint16_t GALA_FRAME     = 0x416D;
constexpr float GALA_ENEMY_MOTION_SCALE = 0.20f;

// -----------------------------------------------------------------
// GAME STRUCTURES & STATE
// -----------------------------------------------------------------
struct GalaBullet {
  float x, y;
  float vy;
  bool active;
};

struct GalaAlien {
  float x, y;
  float formX, formY;
  int type; // 0 = drone, 1 = fighter, 2 = commander
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

// Starfield background
struct GalaStar {
  float x, y;
  float speed;
  uint16_t color;
};
static GalaStar stars[25];

constexpr int GALA_GRID_SIZE = 10;
constexpr int GALA_GRID_COLS = GALA_W / GALA_GRID_SIZE;
constexpr int GALA_GRID_ROWS = (GALA_PLAYFIELD_BOTTOM - GALA_PLAYFIELD_TOP + 1 + GALA_GRID_SIZE - 1) / GALA_GRID_SIZE;
static uint8_t galaBackdropGrid[GALA_GRID_COLS][GALA_GRID_ROWS];
// Near-black navy tiles keep the playfield textured without competing with sprites.
static const uint16_t GALA_GRID_COLORS[] = {0x0000, 0x0001, 0x0002, 0x0003};
constexpr uint16_t GALA_GRID_LINE = 0x0001;

static void initGalagaBackdropGrid() {
  for (int x = 0; x < GALA_GRID_COLS; x++)
    for (int y = 0; y < GALA_GRID_ROWS; y++)
      galaBackdropGrid[x][y] = random(0, 4);
}

static uint16_t galagaBackdropColor(int x, int y) {
  if (y < GALA_PLAYFIELD_TOP || y > GALA_PLAYFIELD_BOTTOM) return GALA_BG;
  if ((x % GALA_GRID_SIZE) == 0 || ((y - GALA_PLAYFIELD_TOP) % GALA_GRID_SIZE) == 0)
    return GALA_GRID_LINE;
  int gx = constrain(x / GALA_GRID_SIZE, 0, GALA_GRID_COLS - 1);
  int gy = constrain((y - GALA_PLAYFIELD_TOP) / GALA_GRID_SIZE, 0, GALA_GRID_ROWS - 1);
  return GALA_GRID_COLORS[galaBackdropGrid[gx][gy]];
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
  for (int gy = 0; gy < GALA_GRID_ROWS; gy++) {
    int y = GALA_PLAYFIELD_TOP + gy * GALA_GRID_SIZE;
    int h = min(GALA_GRID_SIZE, GALA_PLAYFIELD_BOTTOM - y + 1);
    for (int gx = 0; gx < GALA_GRID_COLS; gx++)
      tft.fillRect(gx * GALA_GRID_SIZE, y, GALA_GRID_SIZE, h,
                   GALA_GRID_COLORS[galaBackdropGrid[gx][gy]]);
  }
  for (int x = 0; x < GALA_W; x += GALA_GRID_SIZE)
    tft.drawFastVLine(x, GALA_PLAYFIELD_TOP, GALA_PLAYFIELD_BOTTOM - GALA_PLAYFIELD_TOP + 1, GALA_GRID_LINE);
  for (int y = GALA_PLAYFIELD_TOP; y <= GALA_PLAYFIELD_BOTTOM; y += GALA_GRID_SIZE)
    tft.drawFastHLine(0, y, GALA_W, GALA_GRID_LINE);
}

// -----------------------------------------------------------------
// SOUND EFFECTS
// -----------------------------------------------------------------
static int galaSoundEffect = 0;
static unsigned long galaSoundStarted = 0;

static void updateGalagaSound() {
  // Sequencing now runs in the shared I2S task.
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
  startGalagaSound(3);
}

// -----------------------------------------------------------------
// STARFIELD RENDERING
// -----------------------------------------------------------------
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

// -----------------------------------------------------------------
// MODERN IPHONE HEADER & HUD
// -----------------------------------------------------------------
static void drawGalagaHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(224, 38)) return;
  header.fillSprite(GALA_BG);
  header.fillRoundRect(0, 0, 224, 38, 12, ui::CARD);
  header.setTextDatum(ML_DATUM);
  header.setTextColor(ui::LABEL);
  header.drawString("Galaga", 12, 20, 4);
  header.setTextDatum(TR_DATUM);
  header.setTextColor(ui::SECONDARY);
  header.drawString("Score", 150, 3, 2);
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
  tft.fillRoundRect(40, 296, 160, 20, 10, ui::CARD);
  ui::hint(306, "Up to fire", ui::CARD);
}

// -----------------------------------------------------------------
// SHIP & ALIEN RENDERING
// -----------------------------------------------------------------
static TFT_eSprite galaShipArt(&tft);
static TFT_eSprite galaEnemyArt(&tft);
static unsigned long galaShipAnimationAt = 0;
static unsigned long galaEnemyAnimationAt = 0;

static void drawShip(int px, int py) {
  if (!galaShipArt.created() && !galaShipArt.createSprite(18, 22)) return;
  for (int y = 0; y < 22; ++y)
    for (int x = 0; x < 18; ++x)
      galaShipArt.drawPixel(x, y, galagaBackdropColor(px + x, py + y));
  int phase = (millis() / 80) % 3;
  int flameTip = 19 + phase;
  galaShipArt.fillTriangle(6, 15, 11, 15, 9, flameTip, 0xF800);
  galaShipArt.fillTriangle(7, 16, 10, 16, 9, flameTip - 1, 0xFD20);
  galaShipArt.drawFastVLine(9, 16, 2 + phase, 0xFFE0);
  // Swept white wings, red tips, and a shaded silver fuselage.
  galaShipArt.fillTriangle(8, 0, 3, 15, 14, 15, 0x2948);
  galaShipArt.fillTriangle(4, 6, 0, 14, 6, 13, 0xFFFF);
  galaShipArt.fillTriangle(13, 6, 17, 14, 11, 13, 0xFFFF);
  galaShipArt.drawFastHLine(0, 14, 5, 0xF800);
  galaShipArt.drawFastHLine(13, 14, 5, 0xF800);
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

static void drawAlienSprite(int x, int y, int type) {
  if (!galaEnemyArt.created() && !galaEnemyArt.createSprite(16, 12)) return;
  for (int yy = 0; yy < 12; ++yy)
    for (int xx = 0; xx < 16; ++xx)
      galaEnemyArt.drawPixel(xx, yy, galagaBackdropColor(x + xx, y + yy));
  int phase = ((millis() / 140) + type) & 1;
  if (type == 2) {
    galaEnemyArt.fillTriangle(0, 2, 5, 4, 1, 10, 0xFE04);
    galaEnemyArt.fillTriangle(15, 2, 10, 4, 14, 10, 0xFE04);
    galaEnemyArt.fillRoundRect(3, 2, 10, 8, 2, 0x6953);
    galaEnemyArt.fillRoundRect(4, 2, 8, 6, 2, GALA_PURPLE);
    galaEnemyArt.drawFastHLine(5, 2, 6, 0xE6FF);
    galaEnemyArt.fillRect(5, 5, 6, 2, 0x2087);
    galaEnemyArt.drawFastHLine(6, 5, 4, phase ? 0xFFE0 : 0xFD20);
    galaEnemyArt.fillRect(6, 9, 4, 1 + phase, 0x07FF);
  } else if (type == 1) {
    galaEnemyArt.fillTriangle(0, 1 + phase, 7, 4, 3, 10, 0x3D45);
    galaEnemyArt.fillTriangle(15, 1 + phase, 8, 4, 12, 10, 0x3D45);
    galaEnemyArt.drawLine(1, 2 + phase, 4, 5, 0xBFE8);
    galaEnemyArt.drawLine(14, 2 + phase, 11, 5, 0xBFE8);
    galaEnemyArt.fillTriangle(5, 2, 10, 2, 8, 11, 0x87E8);
    galaEnemyArt.fillRect(6, 4, 4, 3, 0x0125);
    galaEnemyArt.drawFastHLine(7, 4, 2, 0x07FF);
    galaEnemyArt.drawPixel(8, 9, phase ? 0xFFFF : 0x07FF);
  } else {
    galaEnemyArt.fillRoundRect(3, 2, 10, 7, 2, 0xB805);
    galaEnemyArt.fillRect(4, 2, 8, 4, 0xFA48);
    galaEnemyArt.drawFastHLine(5, 2, 6, 0xFCB4);
    galaEnemyArt.fillRect(5, 5, 2, 2, 0xFFE0);
    galaEnemyArt.fillRect(9, 5, 2, 2, 0xFFE0);
    galaEnemyArt.drawLine(3, 4, 0, 6 + phase, 0xFA48);
    galaEnemyArt.drawLine(12, 4, 15, 6 + phase, 0xFA48);
    galaEnemyArt.drawLine(5, 8, 2 + phase, 11, 0xFA48);
    galaEnemyArt.drawLine(10, 8, 13 - phase, 11, 0xFA48);
  }
  int visibleHeight = min(12, GALA_PLAYFIELD_BOTTOM + 1 - y);
  if (visibleHeight > 0) galaEnemyArt.pushSprite(x, y, 0, 0, 16, visibleHeight);
}

static void eraseAlienSprite(int x, int y) {
  restoreGalagaBackdropRect(x, y, 16, min(12, GALA_PLAYFIELD_BOTTOM + 1 - y));
}

static void drawGalagaLaser(int index) {
  if (!pBullets[index].active) return;
  int x = (int)pBullets[index].x - GALA_LASER_WIDTH / 2;
  int y = (int)pBullets[index].y;
  tft.fillRect(x, y, GALA_LASER_WIDTH, GALA_LASER_HEIGHT, GALA_LASER_COLOR);
  tft.drawFastVLine(x + 1, y + 1, 8, (millis() / 60) & 1 ? 0xFCB4 : 0xFFFF);
  tft.drawFastHLine(x + 1, y, 2, 0xFFFF);
  tft.drawFastHLine(x, y + GALA_LASER_HEIGHT - 1, GALA_LASER_WIDTH, 0xA800);
}

static void drawAlienBullet(int index) {
  if (!aBullets[index].active) return;
  // Stagger each missile's red/yellow pulse so the volley alternates visually.
  bool yellow = (((millis() / 180) + index) & 1) != 0;
  tft.fillCircle((int)aBullets[index].x, (int)aBullets[index].y, 2,
                 yellow ? 0xFFE0 : 0xF800);
}

// -----------------------------------------------------------------
// BULLETS & COMBAT PHYSICS
// -----------------------------------------------------------------
static void firePlayerLaser() {
  // One shot per Up press, with a short limit to prevent bullet-pool flooding.
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
  // Update Player Lasers
  for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
    if (pBullets[i].active) {
      restoreGalagaBackdropRect((int)pBullets[i].x - GALA_LASER_WIDTH / 2,
                               (int)pBullets[i].y, GALA_LASER_WIDTH, GALA_LASER_HEIGHT);
      pBullets[i].y += pBullets[i].vy;

      if (pBullets[i].y < GALA_PLAYFIELD_TOP + 2) {
        pBullets[i].active = false;
        continue;
      }

      // Check hit with Aliens
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

            score += (aliens[a].type == 2) ? 150 : (aliens[a].type == 1 ? 80 : 40);
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

  // Update Alien Plasma Bullets
  for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
    if (aBullets[i].active) {
      restoreGalagaBackdropRect((int)aBullets[i].x - 2, (int)aBullets[i].y - 2, 5, 5);
      aBullets[i].y += aBullets[i].vy;

      if (aBullets[i].y > GALA_PLAYFIELD_BOTTOM - 2) {
        aBullets[i].active = false;
        continue;
      }

      // Check hit with Player Ship
      if (aBullets[i].x >= shipX + 2 && aBullets[i].x <= shipX + SHIP_W - 2 &&
          aBullets[i].y >= SHIP_Y && aBullets[i].y <= SHIP_Y + SHIP_H) {
        aBullets[i].active = false;
        soundPlayerExplode();
        galaLives--;
        updateGalagaScoreHUD();

        if (galaLives <= 0) {
          gameState = 2; // Game Over
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

// -----------------------------------------------------------------
// FLEET MOVEMENT & DIVING AI
// -----------------------------------------------------------------
static void initGalagaWave(int wave) {
  audio::play(audio::Cue::Wave);
  galaWave = wave;
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
  drawGalagaBackdrop();
  tft.drawFastHLine(0, GALA_PLAYFIELD_TOP - 1, GALA_W, GALA_SURFACE);
  tft.drawFastHLine(0, GALA_PLAYFIELD_BOTTOM + 1, GALA_W, GALA_SURFACE);

  drawGalagaHeader();
  drawGalagaBottomBar();
  initStarfield();

  drawShip(shipX, SHIP_Y);
  for (int i = 0; i < TOTAL_ALIENS; i++) {
    drawAlienSprite((int)aliens[i].x, (int)aliens[i].y, aliens[i].type);
  }
}

static void updateAliens() {
  bool animate = millis() - galaEnemyAnimationAt >= 140;
  if (animate) galaEnemyAnimationAt = millis();
  // Sway formation left and right
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
      // Formation swaying
      float targetX = aliens[i].formX + fleetOffsetX;
      if (abs(aliens[i].x - targetX) > 0.5f || abs(aliens[i].y - aliens[i].formY) > 0.5f) {
        eraseAlienSprite(prevAX, prevAY);
        aliens[i].x = targetX;
        aliens[i].y = aliens[i].formY;
        drawAlienSprite((int)aliens[i].x, (int)aliens[i].y, aliens[i].type);
      } else if (animate) {
        drawAlienSprite(prevAX, prevAY, aliens[i].type);
      }

      // Random dive launch
      if (random(1200) < 2) {
        aliens[i].diving = true;
        audio::play(audio::Cue::Dive);
        aliens[i].diveAngle = 0;
        aliens[i].diveSpeed = random(35, 55) / 10.0f * GALA_ENEMY_MOTION_SCALE;
      }
    } else {
      // Diving trajectory (swooping curve towards player)
      eraseAlienSprite(prevAX, prevAY);
      aliens[i].y += aliens[i].diveSpeed * 0.35f;
      aliens[i].x += sin(aliens[i].diveAngle) * 3.2f * GALA_ENEMY_MOTION_SCALE;
      aliens[i].diveAngle += 0.12f * GALA_ENEMY_MOTION_SCALE;

      // Reflect at playfield walls instead of letting divers leave the screen.
      if (aliens[i].x < 4) {
        aliens[i].x = 4;
        aliens[i].diveAngle = -aliens[i].diveAngle;
      } else if (aliens[i].x > GALA_W - 20) {
        aliens[i].x = GALA_W - 20;
        aliens[i].diveAngle = -aliens[i].diveAngle;
      }

      // Alien fires bullet during dive
      if (random(75) == 0) {
        alienShoot((int)aliens[i].x, (int)aliens[i].y);
      }

      // Loop around back to top formation
      if (aliens[i].y > GALA_PLAYFIELD_BOTTOM - 12) {
        aliens[i].x = aliens[i].formX + fleetOffsetX;
        aliens[i].y = aliens[i].formY;
        aliens[i].diving = false;
      }

      drawAlienSprite((int)aliens[i].x, (int)aliens[i].y, aliens[i].type);

      // Collision with player ship
      if (aliens[i].x < shipX + SHIP_W && aliens[i].x + 16 > shipX &&
          aliens[i].y < SHIP_Y + SHIP_H && aliens[i].y + 12 > SHIP_Y) {
        aliens[i].active = false;
        eraseAlienSprite((int)aliens[i].x, (int)aliens[i].y);
        soundPlayerExplode();
        galaLives--;
        updateGalagaScoreHUD();

        if (galaLives <= 0) {
          gameState = 2; // Game Over
          menu_drawn = false;
          return;
        }
      }
    }
  }

  // Wave Clear
  if (activeCount == 0) {
    gameState = 3; // Stage Complete
    menu_drawn = false;
    galaWaveClearSelection = 0;
  }
}

// -----------------------------------------------------------------
// INPUT & MODAL SCREENS
// -----------------------------------------------------------------
static void handleGalagaInput() {
  // Consume confirmed taps before looking at held state; rendering may have
  // finished after a quick physical press was already released.
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
    gameState = 1;
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
  if (dir == -1) { // LEFT
    shipX -= SHIP_SPEED;
    if (shipX < 4) shipX = 4;
  } else if (dir == 1) { // RIGHT
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
  for (int i = 0; i < 25; ++i)
    tft.drawPixel((int)stars[i].x, (int)stars[i].y, stars[i].color);
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
  drawGalagaBackdrop();
  drawGalagaHeader();
  static const ui::Control controls[] = {
    {"Left / Right", "Steer"}, {"Up", "Fire"}, {"Shoot", "Pause"}};
  ui::startCard(52, "Galaga", "Defend against the swarm", controls, 3,
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
  ui::heading(40, 80, "Galaga", "Wave cleared");
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

// -----------------------------------------------------------------
// LIFECYCLE (galagaSetup & galagaLoop)
// -----------------------------------------------------------------
void galagaSetup() {
  galaSoundEffect = 0;
  tft.setRotation(2); // Strict Vertical / Portrait Orientation
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
    updateBullets();
    if (gameState != 0) return;
    updateAliens();
    if (gameState != 0) return;
    // Repair foreground art after nearby moving objects restore their backgrounds.
    for (int i = 0; i < MAX_PLAYER_BULLETS; ++i) drawGalagaLaser(i);
    if (millis() - galaShipAnimationAt >= 80) drawShip(shipX, SHIP_Y);
    updateGalagaSound();
    delay(12); // ~60 FPS smooth motion
    return;
  } else if (gameState == 1) { // Pause Menu
    if (!menu_drawn) {
      drawGalagaPauseMenu();
      menu_drawn = true;
    }
    handleGalagaPause();
  } else if (gameState == 2) { // Game Over
    if (!menu_drawn) {
      drawGalagaGameOver();
      menu_drawn = true;
    }
    handleGalagaGameOver();
  } else if (gameState == 3) { // Wave Clear
    if (!menu_drawn) {
      drawGalagaWaveClear();
      menu_drawn = true;
    }
    handleGalagaWaveClear();
  } else if (gameState == -1) { // Title Screen
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
