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
// Frames are generated at display size and drawn without scaling.
constexpr int RUNNER_W = VAMPIRE_FRAME_W;
constexpr int RUNNER_H = VAMPIRE_FRAME_H;
constexpr int RUNNER_DEATH_W = VAMPIRE_DEAD_FRAME_W;
// The death effect spreads farther left than the normal 40px character. Its
// wider canvas is offset so the body remains aligned with the running sprite.
constexpr int RUNNER_DEATH_X = RUNNER_X - 24;
// Faster ascent and landing retain the roughly 50px jump height.
constexpr float GRAVITY = runrules::gravity;
constexpr float JUMP_VELOCITY = runrules::jumpVelocity;

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
static float gameSpeed = runrules::startSpeed;

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
constexpr int RUN_RENDER_BAND = 16;
constexpr int RUN_RENDER_BANDS = (RUN_H + RUN_RENDER_BAND - 1) / RUN_RENDER_BAND;
static int runDirtyLeft[RUN_RENDER_BANDS], runDirtyRight[RUN_RENDER_BANDS];
static uint16_t runRenderPixels[RUN_W * RUN_RENDER_BAND];
static bool runMilestoneDirty = false;
static TFT_eSprite runNotice(&tft);

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
  // True colour blend (no dither noise) between the two time-of-day frames.
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
  // Mark both positions; the compositor restores clouds and foreground together.
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

// -----------------------------------------------------------------
// SOUND EFFECTS
// -----------------------------------------------------------------
static void soundJump() {
  audio::play(audio::Cue::Jump);
}

static void soundCrash() {
<<<<<<< HEAD
  audio::play(audio::Cue::Death);
=======
  audio::play(audio::Cue::GameOver);
>>>>>>> codex/rally-3d
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
  if (!header.createSprite(RUN_W, 43)) return;
  header.fillSprite(ui::CARD);
  // Refresh the top and both side margins with the same sky as the playfield.
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

// -----------------------------------------------------------------
// RUNNER SPRITE & OBSTACLE RENDERING
// -----------------------------------------------------------------
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
  // Remove the last live pose once, then replace each death frame atomically.
  eraseRunner(RUNNER_X, (int)prevRunnerY);
  restoreRunnerBackdropRect(RUNNER_DEATH_X, py,
<<<<<<< HEAD
                            VAMPIRE_DEAD_FRAME_W, RUNNER_H);
  audio::play(audio::Cue::Death);
  for (int frame = 0; frame < VAMPIRE_DEAD_FRAME_COUNT; ++frame) {
=======
                            RUNNER_DEATH_W, RUNNER_H);
  audio::play(audio::Cue::GameOver);
  for (int frame = 0; frame < runner_characters[runCharacter].actions[2].count; ++frame) {
>>>>>>> codex/rally-3d
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
    // A separate jump in open space, with landing room before the next obstacle.
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
    // Coins describe the rising, peak and falling portions of a jump.
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
    // Reuse the shortest-lived piece if two bursts already fill the pool.
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

// Merge old/new bounds, then compose each affected band once. No erase pass.
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
    // Clip each layer to the band before visiting its pixels.
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
    // The notice is part of the final band, never erased and redrawn on the panel.
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
      // Include the exact trail when a fractional step moves an extra pixel.
      dirtyRunnerRect(curX + obs[i].w, oy, prevX - curX, obs[i].h);

      // If obstacle completely passed screen
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

      // Collision Check with Runner (AABB with 2px forgiveness)
      // The transparent canvas is wider than her body. Keep collision bounds
      // centered on the visible sprite so the larger art does not feel unfair.
      int rx = RUNNER_X + 7;
      int ry = (int)runnerY + 6;
      int rw = RUNNER_W - 14;
      int rh = RUNNER_H - 8;

      if (curX < rx + rw && curX + obs[i].w > rx &&
          oy < ry + rh && oy + obs[i].h > ry) {
        // Contact during wind-up must not consume the shield and erase the block early.
        if (runRewards.attack && attackFrame < attack.count / 2) continue;
        if (runRewards.hit()) {
          burstRunObstacle(i);
          obs[i].active = false;
          dirtyRunnerRect(curX, oy, obs[i].w, obs[i].h);
          audio::play(audio::Cue::Hit);
          continue;
        }
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

// -----------------------------------------------------------------
// MODAL SCREENS (START / PAUSE / GAME OVER)
// -----------------------------------------------------------------
static void drawRunnerChoice() {
  // Native-size portrait: no second scaling pass in the selector either.
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
      drawRunnerBottomBar(true);
      for (int i = 0; i < MAX_RUN_OBS; ++i)
        if (obs[i].active)
          drawRunnerObstacle(i, (int)obs[i].x, RUN_GROUND_Y - obs[i].h);
      drawRunPickups();
      drawRunner(RUNNER_X, (int)runnerY, (int)runnerY, legAnimFrame, isJumping);
      drawRunnerPauseMenu();
      runPauseReady = false;
    } else { // Exit
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
    uint32_t frameStarted = millis();
    if (frameStarted - runFrameAt < runrules::frameMs) { delay(1); return; }
    runFrameAt = frameStarted;
    // Each confirmed Up press jumps; Shoot opens the pause menu immediately.
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

    // Match animation to the faster physics; scoring uses its own 120ms clock.
    unsigned long now = millis();
    const unsigned long animationInterval = isJumping ? 90 : 56;
    if (now - runLastAnimTime >= animationInterval) {
      legAnimFrame = (legAnimFrame + 1) % runner_characters[runCharacter].actions[isJumping ? 1 : 0].count;
      runLastAnimTime = now;
    }

    if (now - runLastScoreTime >= 120) {
      runLastScoreTime = now;
      score++; // Increment distance
      if (score == 100 || score == 250 || score == 500) {
        runMilestone = score;
        runMilestoneTime = 1800;
        runMilestoneDirty = true;
        soundMilestone();
      }
      // Progressive speed increase
      if (score % 50 == 0 && gameSpeed < runrules::maxSpeed) {
        gameSpeed = min(runrules::maxSpeed, gameSpeed + 0.3125f);
        soundMilestone();
      }
    }

    // Morph a small background band per loop to keep the day/night transition
    // smooth without blocking controls or redrawing the whole display.
    updateRunnerDayCycleBackdrop();

    // Update cloud parallax and obstacles.
    updateRunClouds();

    // Advance a fixed pattern in world distance; speed changes retain landing room.
    runSpawnDistance -= gameSpeed;
    if (runSpawnDistance <= 0) spawnRunObstacle();
    updateRunPickups();
    updateAndDrawObstacles();
    if (gameState != 0) return;
    drawRunPickups();

    // Redraw the runner without an exposed clear interval. At ground level the
    // opaque composite directly replaces the previous animation frame.
    drawRunner(RUNNER_X, (int)runnerY, (int)prevRunnerY, legAnimFrame,
               isJumping);
    prevRunnerY = runnerY;

    updateRunnerScoreHUD();
    drawRunnerBottomBar();
    yield();
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
