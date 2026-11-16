#include "game.h"
#include "ui_kit.h"
#include <Arduino.h>
#include "run_background_240x320.h"
#include "run_clouds.h"
#include "vampire_runner_sprites.h"

// =================================================================
// MODERN VERTICAL ENDLESS RUNNER (240x320 PORTRAIT ORIENTATION)
// =================================================================

// Portrait dimensions: 240 width x 320 height (Rotation 2)
// Header Card: y = 5..43 (h = 38)
// Playfield Area: y = 48..290 (w = 240, h = 242)
// Ground Line: y = 265, Ground Block: y = 266..290
// Bottom Dynamic Pill Bar: y = 295..316 (h = 21)

constexpr int RUN_W = 240;
constexpr int RUN_H = 320;
constexpr int RUN_PLAYFIELD_TOP = 48;
constexpr int RUN_GROUND_Y = 262;

// Runner Character Physics (Vertical Portrait space)
constexpr int RUNNER_X = 30;
// Sprite art is 32x48; draw it scaled 5/4 (40x60) with nearest-neighbour.
constexpr int RUNNER_SCALE_NUM = 5;
constexpr int RUNNER_SCALE_DEN = 4;
constexpr int RUNNER_W = VAMPIRE_FRAME_W * RUNNER_SCALE_NUM / RUNNER_SCALE_DEN;
constexpr int RUNNER_H = VAMPIRE_FRAME_H * RUNNER_SCALE_NUM / RUNNER_SCALE_DEN;
constexpr int RUNNER_DEATH_W =
    VAMPIRE_DEAD_FRAME_W * RUNNER_SCALE_NUM / RUNNER_SCALE_DEN;
// The death effect spreads farther left than the normal 32px character. Its
// wider canvas is offset so the body remains aligned with the running sprite.
constexpr int RUNNER_DEATH_X = RUNNER_X - 22;
// Gentler ascent and descent, retaining the ~50px peak with ~11% more airtime.
constexpr float GRAVITY = 0.70f;
constexpr float JUMP_VELOCITY = -8.75f;

// Colors matching OLED Modern Palette
#define RUN_BG UI_BG
#define RUN_CARD UI_PANEL
#define RUN_SURFACE UI_SURFACE
#define RUN_WHITE UI_TEXT
#define RUN_MUTED UI_MUTED
#define RUN_CYAN UI_CYAN
constexpr uint16_t RUN_GREEN     = 0x3666;
constexpr uint16_t RUN_AMBER     = 0xFE04;
constexpr uint16_t RUN_RED       = 0xFA48;
constexpr uint16_t RUN_ACCENT    = 0x07FF;
constexpr uint16_t RUN_ACCENT_HI = 0xBFFF;
constexpr uint16_t RUN_DARK_TEXT = 0x0125;
constexpr uint16_t RUN_FRAME     = 0x032C;

// -----------------------------------------------------------------
// GAME STATE
// -----------------------------------------------------------------
static float runnerY = RUN_GROUND_Y - RUNNER_H;
static float prevRunnerY = runnerY;
static float runnerVY = 0;
static bool isJumping = false;
static int legAnimFrame = 0;
static unsigned long runLastAnimTime = 0;
static unsigned long runLastScoreTime = 0;
static float gameSpeed = 4.2f;

// Obstacles
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

static int runPauseSelection = 0;
static const char *const RUN_PAUSE_ITEMS[] = {"Resume", "Restart", "High scores", "Main menu"};
static bool runPauseReady = false;

// One complete day/night orbit passes through all nine supplied frames. Each
// segment continuously crossfades into the next, including night back to dawn.
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

// A 64-step ordered pattern produces a visually smooth crossfade while each
// screen pixel is copied from only one source frame. This removes the costly
// per-channel divisions that previously ran thousands of times per game loop.
static const uint8_t RUN_DAY_DITHER_8X8[64] = {
   0, 32,  8, 40,  2, 34, 10, 42,
  48, 16, 56, 24, 50, 18, 58, 26,
  12, 44,  4, 36, 14, 46,  6, 38,
  60, 28, 52, 20, 62, 30, 54, 22,
   3, 35, 11, 43,  1, 33,  9, 41,
  51, 19, 59, 27, 49, 17, 57, 25,
  15, 47,  7, 39, 13, 45,  5, 37,
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
  int index = y * RUN_W + x;
  // True colour blend (no dither noise) between the two time-of-day frames.
  uint16_t a = pgm_read_word(&run_background_frames[runDayFrame][index]);
  if (runDayBlend == 0) return a;
  uint16_t b = pgm_read_word(&run_background_frames[runDayNextFrame][index]);
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
  // Latch the blend level once per full sweep so the whole screen sits at one
  // uniform tint; each sweep advances it by a barely visible step.
  if (runDayRenderY == 43) syncRunnerDayCycle();
  constexpr int rowsPerUpdate = 8;
  int y0 = runDayRenderY;
  int y1 = min(295, y0 + rowsPerUpdate);
  static uint16_t pixels[RUN_W * rowsPerUpdate];
  for (int y = y0; y < y1; ++y) {
    for (int x = 0; x < RUN_W; ++x) {
      pixels[(y - y0) * RUN_W + x] = runnerBackdropAt(x, y);
    }
  }
  tft.setSwapBytes(true);
  tft.pushImage(0, y0, RUN_W, y1 - y0, pixels);
  tft.setSwapBytes(false);

  // Repair only clouds touched by this background band. Each cloud occupies a
  // separate vertical band, so this is both clean and inexpensive.
  for (int i = 0; i < RUN_CLOUD_LAYER_COUNT; ++i) {
    int cloudBottom = runClouds[i].y + run_cloud_height[runClouds[i].variant];
    if (runClouds[i].y < y1 && cloudBottom > y0) drawRunCloudClipped(i);
  }
  runDayRenderY = y1 >= 295 ? 43 : y1;
}

static void initRunClouds() {
  // Seven independent cloud bands create parallax without allowing one cloud's
  // background restore to cut through another cloud.
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
  // Remove every moved cloud first, then draw their new positions. The bands
  // do not overlap, so one layer cannot erase another.
  for (int i = 0; i < RUN_CLOUD_LAYER_COUNT; ++i) {
    RunCloud &cloud = runClouds[i];
    float nextX = cloud.x - cloud.pixelsPerSecond * elapsed / 1000.0f;
    if (nextX + run_cloud_width[cloud.variant] < 0) {
      nextX = RUN_W + random(12, 70);
    }
    if ((int)nextX != cloud.drawnX) {
      restoreRunnerBackdropRect(cloud.drawnX, cloud.y,
                                run_cloud_width[cloud.variant],
                                run_cloud_height[cloud.variant]);
      moved[i] = true;
    }
    cloud.x = nextX;
  }
  for (int i = 0; i < RUN_CLOUD_LAYER_COUNT; ++i) {
    if (!moved[i]) continue;
    runClouds[i].drawnX = (int)runClouds[i].x;
    drawRunCloudClipped(i);
  }
}

// -----------------------------------------------------------------
// SOUND EFFECTS
// -----------------------------------------------------------------
static void soundJump() {
  audio::play(audio::Cue::Jump);
}

static void soundCrash() {
  audio::play(audio::Cue::Death);
}

static void soundMilestone() {
  audio::play(audio::Cue::Milestone);
}

// -----------------------------------------------------------------
// MODERN IPHONE HEADER & HUD
// -----------------------------------------------------------------
static void drawRunnerHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(224, 38)) return;
  // Transparent outside the rounded card so the sky shows at the corners.
  // Paint the real backdrop behind the card so the corners and any area
  // around it are refreshed with the current sky, not left stale.
  for (int y = 0; y < 38; ++y)
    for (int x = 0; x < 224; ++x)
      header.drawPixel(x, y, runnerBackdropAt(8 + x, 5 + y));
  header.fillRoundRect(0, 0, 224, 38, 12, ui::CARD);
  header.setTextDatum(ML_DATUM);
  header.setTextColor(ui::LABEL);
  header.drawString("Run", 12, 20, 4);
  header.setTextDatum(TR_DATUM);
  header.setTextColor(ui::SECONDARY);
  header.drawString("Best", 150, 3, 2);
  header.drawString("Distance", 212, 3, 2);
  header.setTextColor(RUN_ACCENT);
  header.drawString(String(currentGame.highScores[0]) + " m", 150, 19, 2);
  header.setTextColor(score > 0 && score % 50 < 4 ? RUN_ACCENT : ui::LABEL);
  header.drawString(String(score) + " m", 212, 19, 2);
  header.pushSprite(8, 5);
  header.deleteSprite();
}

static void updateRunnerScoreHUD() {
  drawRunnerHeader();
}

static void drawRunnerBottomBar() {
  // Small floating pill so the scenery stays visible around it.
  tft.fillRoundRect(52, 296, 136, 20, 10, ui::CARD);
  ui::hint(306, "Up to jump", ui::CARD);
}

// -----------------------------------------------------------------
// RUNNER SPRITE & OBSTACLE RENDERING
// -----------------------------------------------------------------
static void drawRunner(int px, int py, int prevPy, int anim, bool inAir) {
  const uint16_t *frame = inAir
      ? vampire_jump_frames[anim % VAMPIRE_FRAME_COUNT]
      : vampire_run_frames[anim % VAMPIRE_FRAME_COUNT];
  // Compose the character and the exact image behind it in RAM, then transfer
  // one opaque rectangle. This prevents the display from briefly showing the
  // erased background between animation frames.
  // The pushed area covers both the old and new position, so moving up or
  // down never shows an erased frame (no flicker while jumping).
  constexpr int MAX_DY = 24;
  static uint16_t composed[RUNNER_W * (RUNNER_H + MAX_DY)];
  int dy = constrain(prevPy - py, -MAX_DY, MAX_DY);
  int top = dy < 0 ? py + dy : py;
  int h = RUNNER_H + abs(dy);
  for (int y = 0; y < h; ++y) {
    int ly = top + y - py;
    for (int x = 0; x < RUNNER_W; ++x) {
      uint16_t ink = VAMPIRE_TRANSPARENT;
      if (ly >= 0 && ly < RUNNER_H) {
        int sx = x * RUNNER_SCALE_DEN / RUNNER_SCALE_NUM;
        int sy = ly * RUNNER_SCALE_DEN / RUNNER_SCALE_NUM;
        ink = pgm_read_word(&frame[sy * VAMPIRE_FRAME_W + sx]);
      }
      composed[y * RUNNER_W + x] = ink == VAMPIRE_TRANSPARENT
          ? runnerSceneAt(px + x, top + y) : ink;
    }
  }
  tft.setSwapBytes(true);
  tft.pushImage(px, top, RUNNER_W, h, composed);
  tft.setSwapBytes(false);
}

static void eraseRunner(int px, int py) {
  restoreRunnerBackdropRect(px - 2, py - 2, RUNNER_W + 4, RUNNER_H + 4);
}

static void drawRunnerDeathFrame(int py, int frameIndex) {
  const uint16_t *frame = vampire_dead_frames[
      frameIndex % VAMPIRE_DEAD_FRAME_COUNT];
  static uint16_t composed[RUNNER_DEATH_W * RUNNER_H];
  for (int y = 0; y < RUNNER_H; ++y) {
    for (int x = 0; x < RUNNER_DEATH_W; ++x) {
      int sx = x * RUNNER_SCALE_DEN / RUNNER_SCALE_NUM;
      int sy = y * RUNNER_SCALE_DEN / RUNNER_SCALE_NUM;
      uint16_t ink = pgm_read_word(&frame[sy * VAMPIRE_DEAD_FRAME_W + sx]);
      composed[y * RUNNER_DEATH_W + x] =
          ink == VAMPIRE_TRANSPARENT
              ? runnerBackdropAt(RUNNER_DEATH_X + x, py + y) : ink;
    }
  }
  tft.setSwapBytes(true);
  tft.pushImage(RUNNER_DEATH_X, py, RUNNER_DEATH_W,
                RUNNER_H, composed);
  tft.setSwapBytes(false);
}

static void playRunnerDeathAnimation(int py) {
  // Remove the last live pose once, then replace each death frame atomically.
  eraseRunner(RUNNER_X, (int)prevRunnerY);
  restoreRunnerBackdropRect(RUNNER_DEATH_X, py,
                            RUNNER_DEATH_W, RUNNER_H);
  audio::play(audio::Cue::Death);
  for (int frame = 0; frame < VAMPIRE_DEAD_FRAME_COUNT; ++frame) {
    drawRunnerDeathFrame(py, frame);
    delay(95);
  }
  setBuzzerTone(0);
  restoreRunnerBackdropRect(RUNNER_DEATH_X, py,
                            RUNNER_DEATH_W, RUNNER_H);
}

static void spawnRunObstacle() {
  for (int i = 0; i < MAX_RUN_OBS; i++) {
    if (!obs[i].active) {
      obs[i].x = RUN_W + 10;
      int obsType = random(0, 3);
      obs[i].style = obsType;
      if (obsType == 0) {
        // Short grass-topped brick block.
        obs[i].w = 11;
        obs[i].h = 24;
        obs[i].color = 0xA347;
      } else if (obsType == 1) {
        // Taller moss-covered stone pillar.
        obs[i].w = 12;
        obs[i].h = 36;
        obs[i].color = 0x738E;
      } else {
        // Wide weathered brick block.
        obs[i].w = 18;
        obs[i].h = 20;
        obs[i].color = 0xB3A7;
      }
      obs[i].active = true;
      lastObsSpawn = millis();
      break;
    }
  }
}

static uint16_t runnerObstaclePixel(int index, int xx, int yy) {
  const RunObstacle &obstacle = obs[index];
  bool stone = obstacle.style == 1;
  uint16_t base = runnerTimeColor(stone ? RUN_OBS_STONE : RUN_OBS_BRICK);
  uint16_t mortar = runnerTimeColor(RUN_OBS_MORTAR);
  uint16_t highlight = runnerTimeColor(RUN_OBS_HIGHLIGHT);
  uint16_t shadow = runnerTimeColor(RUN_OBS_SHADOW);
  if (obstacle.style == 2) base = blendRun565(base, highlight, 44);
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
    color = runnerTimeColor(yy == 0 ? RUN_OBS_GRASS_TOP
                            : yy == 1 ? RUN_OBS_GRASS_MID
                                      : RUN_OBS_GRASS_DARK);
    if ((xx + yy) % 5 == 0) color = runnerTimeColor(RUN_OBS_GRASS_SPECK);
  }
  return color;
}

// Backdrop plus any obstacle at this pixel, so the runner composite never
// paints sky over a block it overlaps.
static uint16_t runnerSceneAt(int x, int y) {
  for (int i = 0; i < MAX_RUN_OBS; ++i) {
    if (!obs[i].active) continue;
    int ox = (int)obs[i].x, oy = RUN_GROUND_Y - obs[i].h;
    if (x >= ox && x < ox + obs[i].w && y >= oy && y < RUN_GROUND_Y)
      return runnerObstaclePixel(i, x - ox, y - oy);
  }
  return runnerBackdropAt(x, y);
}

static void drawRunnerObstacle(int index, int x, int y) {
  const RunObstacle &obstacle = obs[index];
  static uint16_t pixels[28 * 36];
  for (int yy = 0; yy < obstacle.h; ++yy) {
    for (int xx = 0; xx < obstacle.w; ++xx) {
      uint16_t color = runnerObstaclePixel(index, xx, yy);
      pixels[yy * obstacle.w + xx] = color;
    }
  }
  tft.setSwapBytes(true);
  tft.pushImage(x, y, obstacle.w, obstacle.h, pixels);
  tft.setSwapBytes(false);
}

static void updateAndDrawObstacles() {
  for (int i = 0; i < MAX_RUN_OBS; i++) {
    if (obs[i].active) {
      int prevX = (int)obs[i].x;
      int oy = RUN_GROUND_Y - obs[i].h;

      obs[i].x -= gameSpeed;
      int curX = (int)obs[i].x;
      // Fractional speeds sometimes move an extra pixel; erase the exact trail.
      restoreRunnerBackdropRect(curX + obs[i].w, oy,
                                prevX - curX, obs[i].h);

      // If obstacle completely passed screen
      if (obs[i].x + obs[i].w < 0) {
        restoreRunnerBackdropRect(0, oy, obs[i].w + 2, obs[i].h);
        obs[i].active = false;
        continue;
      }

      if (curX < RUN_W) {
        drawRunnerObstacle(i, curX, oy);
      }

      // Collision Check with Runner (AABB with 2px forgiveness)
      // The transparent canvas is wider than her body. Keep collision bounds
      // centered on the visible sprite so the larger art does not feel unfair.
      int rx = RUNNER_X + 7;
      int ry = (int)runnerY + 6;
      int rw = RUNNER_W - 14;
      int rh = RUNNER_H - 8;

      if (curX < rx + rw && curX + obs[i].w > rx &&
          oy < ry + rh && oy + obs[i].h > ry) {
        playRunnerDeathAnimation((int)runnerY);
        gameState = 2; // Game Over
        menu_drawn = false;
        return;
      }
    }
  }
}

// -----------------------------------------------------------------
// GAME INITIALIZATION
// -----------------------------------------------------------------
static void initRunnerGame() {
  score = 0;
  gameSpeed = 4.2f;
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
  drawRunnerBottomBar();

  drawRunner(RUNNER_X, (int)runnerY, (int)runnerY, legAnimFrame, isJumping);
}

// -----------------------------------------------------------------
// MODAL SCREENS (START / PAUSE / GAME OVER)
// -----------------------------------------------------------------
static void showRunnerStartScreen() {
  gameState = -1;
  tft.setRotation(2);
  tft.fillScreen(RUN_BG);
  drawRunnerBackdrop();
  drawRunClouds();
  drawRunnerHeader();
  static const ui::Control controls[] = {
    {"Up", "Jump"}, {"Shoot", "Pause"}, {"Score", "Distance"}};
  ui::startCard(52, "Run", "Jump and keep running", controls, 3,
                currentGame.highScores[0], RUN_ACCENT, " m", "Press Shoot to start");
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
  if (cmd == 0 || cmd == 2) { // UP / DOWN
    int old = runPauseSelection;
    runPauseSelection = (runPauseSelection + (cmd == 0 ? 3 : 1)) % 4;
    drawRunnerPauseOption(old);
    drawRunnerPauseOption(runPauseSelection);
    playNavClick();
  } else if (cmd == 4) { // SHOOT: Select
    playSelectTone();
    if (runPauseSelection == 0) { // Resume
      showResumeCountdown([]() {
      if (!restoringCountdownDigit()) tft.fillScreen(RUN_BG);
      drawRunnerBackdrop();
      drawRunClouds();
      drawRunnerHeader();
      drawRunnerBottomBar();
      for (int i = 0; i < MAX_RUN_OBS; ++i)
        if (obs[i].active)
          drawRunnerObstacle(i, (int)obs[i].x, RUN_GROUND_Y - obs[i].h);
      drawRunner(RUNNER_X, (int)runnerY, (int)runnerY, legAnimFrame, isJumping);
      });
      runLastAnimTime = runLastScoreTime = lastObsSpawn = runCloudUpdateAt = millis();
      gameState = 0;
      menu_drawn = false;
    } else if (runPauseSelection == 1) { // Restart
      initRunnerGame();
      gameState = 0;
      menu_drawn = false;
    } else if (runPauseSelection == 2) {
      showGameHighScores();
      tft.fillScreen(RUN_BG);
      drawRunnerBackdrop();
      drawRunClouds();
      drawRunnerHeader();
      drawRunnerBottomBar();
      for (int i = 0; i < MAX_RUN_OBS; ++i)
        if (obs[i].active)
          drawRunnerObstacle(i, (int)obs[i].x, RUN_GROUND_Y - obs[i].h);
      drawRunner(RUNNER_X, (int)runnerY, (int)runnerY, legAnimFrame, isJumping);
      drawRunnerPauseMenu();
      runPauseReady = false;
    } else { // Exit
      switchGame(0);
    }
  }
}

static void drawRunnerGameOver() {
  currentGame.recordScore(score);
  ui::gameOverSheet("Run", score, currentGame.highScores[0], RUN_ACCENT, "Crashed", " m");
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
    switchGame(0);
  }
}

// -----------------------------------------------------------------
// LIFECYCLE (runSetup & runLoop)
// -----------------------------------------------------------------
void runSetup() {
  tft.setRotation(2); // Native vertical / portrait orientation
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
    // Each confirmed Up press jumps; Shoot opens the pause menu immediately.
    int cmd = findCommand(false);
    if (cmd == 4) {
      gameState = 1;
      menu_drawn = false;
      runPauseSelection = 0;
      runPauseReady = false;
      return;
    }
    if (cmd == 0 && !isJumping) {
      runnerVY = JUMP_VELOCITY;
      isJumping = true;
      soundJump();
    }

    // Physics. Keep the old sprite visible while the rest of the frame is
    // calculated; it is restored immediately before the replacement frame.
    runnerVY += GRAVITY;
    runnerY += runnerVY;

    // Floor landing
    if (runnerY >= RUN_GROUND_Y - RUNNER_H) {
      runnerY = RUN_GROUND_Y - RUNNER_H;
      runnerVY = 0;
      isJumping = false;
    }

    // Animate ground running more quickly while retaining the existing jump
    // animation rate. Scoring remains on its original independent 120ms clock.
    unsigned long now = millis();
    const unsigned long animationInterval = isJumping ? 120 : 75;
    if (now - runLastAnimTime >= animationInterval) {
      legAnimFrame = (legAnimFrame + 1) % VAMPIRE_FRAME_COUNT;
      runLastAnimTime = now;
    }

    if (now - runLastScoreTime >= 120) {
      runLastScoreTime = now;
      score++; // Increment distance
      updateRunnerScoreHUD();

      // Progressive speed increase
      if (score % 50 == 0 && gameSpeed < 7.5f) {
        gameSpeed += 0.25f;
        soundMilestone();
      }
    }

    // Morph a small background band per loop to keep the day/night transition
    // smooth without blocking controls or redrawing the whole display.
    updateRunnerDayCycleBackdrop();

    // Update cloud parallax and obstacles.
    updateRunClouds();

    // Spawn obstacles at dynamic distance
    if (millis() - lastObsSpawn > (unsigned long)(random(1100, 2200) * (4.2f / gameSpeed))) {
      spawnRunObstacle();
    }
    updateAndDrawObstacles();

    // Redraw the runner without an exposed clear interval. At ground level the
    // opaque composite directly replaces the previous animation frame.
    drawRunner(RUNNER_X, (int)runnerY, (int)prevRunnerY, legAnimFrame,
               isJumping);
    prevRunnerY = runnerY;

    delay(14); // ~60 FPS smooth motion
    return;
  } else if (gameState == 1) { // Pause
    if (!menu_drawn) {
      drawRunnerPauseMenu();
      menu_drawn = true;
    }
    handleRunnerPause();
  } else if (gameState == 2) { // Game Over
    if (!menu_drawn) {
      drawRunnerGameOver();
      menu_drawn = true;
    }
    handleRunnerGameOver();
  } else if (gameState == -1) { // Title Screen
    int cmd = findCommand();
    if (cmd == 4) {
      playSelectTone();
      initRunnerGame();
      gameState = 0;
    }
  }
}
