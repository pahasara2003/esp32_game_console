#include <Arduino.h>
#include "game.h"
#include "ui_kit.h"
#include "rally.h"
#include "rally_grass.h"
#include <math.h>
#include <string.h>
#include <Preferences.h>
#include <utility>

namespace {
// An 8-bit framebuffer keeps heap usage modest on the C3.
TFT_eSprite frame(&tft);
constexpr float length = 24000.f;
float position = 0, speed = 0, lateral = 0, heading = 0;
uint32_t lastFrame = 0;
int lap = 1;
bool ready = false;
// 0 start card, 1 driving, 2 paused, 3 game over
int state = 0, pauseSelection = 0, best = 0;
float timeLeft = 0, distance = 0, nextCheckpoint = 0;
uint32_t flashUntil = 0;
char flashText[24] = "";
int checkpoints = 0, coins = 0;
constexpr float CHECKPOINT_GAP = 3000.f, START_TIME = 25.f;
// Rivals and coins live in the same "distance" space as the player.
constexpr int RIVALS = 5, COINS = 6;
// Car footprint in road units (sprite is ~22 x 46 px at mapScale .24).
constexpr float CAR_W = 90.f, CAR_L = 188.f;
struct Rival { float z, lat, speed, laneTarget; uint32_t hitCooldown; uint16_t paint, dark; };
Rival rivals[RIVALS];
struct Coin { float z, lat; };
Coin coinList[COINS];
void flash(const char *text) {
  strncpy(flashText, text, sizeof(flashText) - 1);
  flashUntil = millis() + 1200;
}
constexpr uint16_t ACCENT = 0xEAC8;
const char *const PAUSE_ITEMS[] = {"Resume", "Restart", "Main menu"};
float center(float z) {
  // Straight start/finish stretch: curves fade in over the first 1500 units
  // and back out before the lap wraps, so every lap starts straight.
  z = fmodf(z, length); if (z < 0) z += length;
  float edge = min(z, length - z);
  float t = constrain((edge - 400.f) / 1500.f, 0.f, 1.f);
  t = t * t * (3 - 2 * t);
  return t * 650 * sinf(z * TWO_PI / length * 2) +
         t * 280 * sinf(z * TWO_PI / length * 5);
}
float roadHeading(float z) {
  return atanf((center(z+20.f)-center(z-20.f))/40.f);
}
// Fixed north-up drone camera follows the car. Positive heading turns right.
constexpr float mapScale = .24f;
constexpr int carY = 218;
void quad(float x1, float y1, float x2, float y2, float halfWidth,
          uint16_t color) {
  frame.fillTriangle(int(x1-halfWidth),int(y1),int(x1+halfWidth),int(y1),
                     int(x2-halfWidth),int(y2),color);
  frame.fillTriangle(int(x1+halfWidth),int(y1),int(x2+halfWidth),int(y2),
                     int(x2-halfWidth),int(y2),color);
}
struct P { float x, y; };
float carSin = 0, carCos = 1, carOX = 120, carOY = carY;
inline void carPt(float x, float y, int &sx, int &sy) {
  sx = int(carOX + x * carCos - y * carSin);
  sy = int(carOY + x * carSin + y * carCos);
}
// Convex polygon in car space (nose points to -y), rotated with the car.
void carPoly(const P *p, int n, uint16_t color, float ox = 0, float oy = 0) {
  int x0, y0, x1, y1, x2, y2;
  carPt(p[0].x + ox, p[0].y + oy, x0, y0);
  carPt(p[1].x + ox, p[1].y + oy, x1, y1);
  for (int i = 2; i < n; ++i) {
    carPt(p[i].x + ox, p[i].y + oy, x2, y2);
    frame.fillTriangle(x0, y0, x1, y1, x2, y2, color);
    x1 = x2; y1 = y2;
  }
}
void carRect(float l, float t, float r, float b, uint16_t color) {
  P p[] = {{l, t}, {r, t}, {r, b}, {l, b}};
  carPoly(p, 4, color);
}
constexpr uint16_t CAR_RED = 0xE8E4, CAR_DARK_RED = 0x9800, CAR_GLASS = 0x2190,
                   CAR_GLASS_HI = 0x5ADF, CAR_TYRE = 0x10A2;
void drawCar(float ox, float oy, float angle, uint16_t CAR_RED, uint16_t CAR_DARK_RED) {
  carOX = ox; carOY = oy;
  carSin = sinf(angle); carCos = cosf(angle);
  // Body outline shared by the shadow and the paint.
  static const P body[] = {{-6, -23}, {6, -23}, {10, -17}, {10, 17},
                           {8, 21}, {-8, 21}, {-10, 17}, {-10, -17}};
  carPoly(body, 8, 0x0841, 3, 4);                    // drop shadow
  carRect(-12, -16, -8, -7, CAR_TYRE);               // tyres
  carRect(8, -16, 12, -7, CAR_TYRE);
  carRect(-12, 8, -8, 18, CAR_TYRE);
  carRect(8, 8, 12, 18, CAR_TYRE);
  carPoly(body, 8, CAR_DARK_RED);
  static const P paint[] = {{-5, -22}, {5, -22}, {9, -16}, {9, 16},
                            {7, 20}, {-7, 20}, {-9, 16}, {-9, -16}};
  carPoly(paint, 8, CAR_RED);
  carRect(-2.5f, -22, 2.5f, 20, TFT_WHITE);          // racing stripe
  static const P screen[] = {{-7, -9}, {7, -9}, {6, -2}, {-6, -2}};
  carPoly(screen, 4, CAR_GLASS);
  static const P shine[] = {{-5, -8}, {-2, -8}, {-4, -3}, {-5, -3}};
  carPoly(shine, 4, CAR_GLASS_HI);
  carRect(-6, -2, 6, 8, CAR_RED);                    // roof
  carRect(-2.5f, -2, 2.5f, 8, TFT_WHITE);
  static const P rear[] = {{-6, 8}, {6, 8}, {7, 13}, {-7, 13}};
  carPoly(rear, 4, CAR_GLASS);
  carRect(-11, -5, -9, -3, CAR_DARK_RED);            // mirrors
  carRect(9, -5, 11, -3, CAR_DARK_RED);
  carRect(-6, -23, -2, -21, 0xFFF0);                 // headlights
  carRect(2, -23, 6, -21, 0xFFF0);
  carRect(-11, 18, 11, 22, 0x2104);                  // rear wing
  carRect(-8, 19, -4, 21, TFT_RED);                  // tail lights
  carRect(4, 19, 8, 21, TFT_RED);
}
int place() {
  int p = 1;
  for (const Rival &r : rivals) if (r.z > distance) ++p;
  return p;
}
void placeCoin(Coin &c, float from) {
  c.z = from + 600 + random(0, 1400);
  c.lat = random(-140, 141);
}
// ---- Centre-line cache ---------------------------------------------------
// The C3 has no FPU, so sinf is costly. Sample the road centre every 80 units
// once per frame and interpolate for every per-row/per-object lookup.
constexpr int CACHE_N = 24;
float cacheC[CACHE_N];
float cacheZ0 = 0;
void buildCache() {
  cacheZ0 = floorf((distance - 400.f) / 80.f) * 80.f;
  for (int i = 0; i < CACHE_N; ++i) cacheC[i] = center(cacheZ0 + i * 80.f);
}
float fastCenter(float z) {
  float f = (z - cacheZ0) * (1.f / 80.f);
  int i = int(f);
  if (f < 0 || i >= CACHE_N - 1) return center(z);
  float t = f - i;
  return cacheC[i] + (cacheC[i + 1] - cacheC[i]) * t;
}
// ---- Scenery -------------------------------------------------------------
// Everything is derived from a hash of the world cell, so nothing is stored.
uint32_t hash2(int a, int b) {
  uint32_t h = uint32_t(a) * 73856093u ^ uint32_t(b) * 19349663u ^ 0x9E3779B9u;
  h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
  return h;
}
constexpr float RIVER_GAP = 2600.f, RIVER_W = 150.f, BANK = 18.f;
// River k spans [riverStart(k), riverStart(k)+RIVER_W) in distance units.
float riverStart(int k) { return k * RIVER_GAP + 1500.f; }
// Rivers meander: their centre line wobbles in z as world x changes.
constexpr float RIVER_SWING = 170.f;
float riverMid(int k, float x) {
  return riverStart(k) + RIVER_W / 2 + 120.f * sinf(x * .0035f + k * 1.7f) +
         50.f * sinf(x * .011f + k * 4.1f);
}
float riverHalf(int k, float x) { return RIVER_W / 2 + 18.f * sinf(x * .007f + k); }
bool inRiver(float z, float x, float pad = 0) {
  int k = int(floorf((z - 1500.f + RIVER_GAP / 2) / RIVER_GAP));
  if (k < 0) return false;
  return fabsf(z - riverMid(k, x)) < riverHalf(k, x) + pad;
}
bool nearRiver(float z, float margin) {
  margin += RIVER_SWING;
  int k = int(floorf((z - 1500.f + RIVER_GAP / 2) / RIVER_GAP));
  float d = z - riverStart(k);
  return k >= 0 && d > -margin && d < RIVER_W + margin;
}
constexpr float CELL = 320.f;
void drawTree(int x, int y, int r) {
  frame.fillCircle(x + 3, y + 3, r, 0x0841);
  frame.fillCircle(x, y, r, 0x1AE4);
  frame.fillCircle(x - r / 3, y - r / 3, r / 2, 0x3D47);
}
void drawScenery(float cameraX) {
  // Rivers: drawn per column so the banks can wind across the map.
  int k0 = max(0, int(floorf((distance - 1500.f - 1500.f) / RIVER_GAP)));
  for (int k = k0; k <= k0 + 2; ++k) {
    float zMid = riverStart(k) + RIVER_W / 2;
    if (fabsf(zMid - distance) > 1600.f) continue;
    // Sample the winding bank every 8 columns and interpolate between.
    float mids[31], halves[31];
    for (int n = 0; n <= 30; ++n) {
      float wx = cameraX + (n * 8 - 120) / mapScale;
      mids[n] = riverMid(k, wx); halves[n] = riverHalf(k, wx);
    }
    uint32_t tick = millis() / 300;
    for (int sx = 0; sx < 240; ++sx) {
      int n = sx >> 3; float t = (sx & 7) * .125f;
      float m = mids[n] + (mids[n + 1] - mids[n]) * t;
      float hw = halves[n] + (halves[n + 1] - halves[n]) * t;
      int yTop = carY - int((m + hw + BANK - distance) * mapScale);
      int yBot = carY - int((m - hw - BANK - distance) * mapScale);
      if (yBot < 26 || yTop > 288) continue;
      int wTop = carY - int((m + hw - distance) * mapScale);
      int wBot = carY - int((m - hw - distance) * mapScale);
      frame.drawFastVLine(sx, yTop, yBot - yTop, 0xD5B0);   // sandy banks
      frame.drawFastVLine(sx, wTop, wBot - wTop, 0x2B3B);   // water
      // Drifting ripples, fixed to the world so they scroll with the map.
      int wx = int(cameraX * mapScale) + sx;
      uint32_t h = hash2((wx >> 2) + tick, k);
      if ((h & 31u) == 0) frame.drawPixel(sx, wTop + int((h >> 8) % max(1, wBot - wTop)), 0x5CDF);
    }
  }
  int firstCell = int(floorf((distance - 150.f) / CELL)) - 1;
  for (int cell = firstCell; cell < firstCell + 6; ++cell) {
    // Trees scattered along both sides of the road.
    for (int t = 0; t < 4; ++t) {
      uint32_t h = hash2(cell * 7 + 3, t);
      float z = cell * CELL + float(h & 255u) * CELL / 256.f;
      if (cell < 1 || nearRiver(z, 40.f)) continue;
      int side = (h & 256u) ? 1 : -1;
      float lat = 240.f + float((h >> 10) & 511u);
      int x = 120 + int((fastCenter(z) + side * lat - cameraX) * mapScale);
      int y = carY - int((z - distance) * mapScale);
      if (y < 10 || y > 300) continue;
      drawTree(x, y, 5 + int((h >> 20) & 3u));
    }
  }
}
void drawBridges(float cameraX) {
  // The road keeps its normal asphalt over the water; the bridge adds a
  // shadow on the water, guard rails and posts just outside the kerbs.
  for (int y = 26; y < 288; ++y) {
    float z = distance + (carY - y) / mapScale;
    if (!nearRiver(z, 0.f)) { continue; }
    float cx = fastCenter(z);
    if (!inRiver(z, cx, 4.f)) continue;
    int x = 120 + int((cx - cameraX) * mapScale);
    frame.drawFastHLine(x + 54, y + 4, 6, 0x10A3);  // shadow on water
    bool post = (int(z) & 31) < 5;
    frame.drawFastHLine(x - 55, y, 6, post ? 0xE71C : 0x8C51);
    frame.drawFastHLine(x + 49, y, 6, post ? 0xE71C : 0x8C51);
  }
}
void render() {
  buildCache();
  float cameraX=fastCenter(distance)+lateral;
  // Copy the CC0 grass tile into the 8-bit frame at world-aligned coordinates.
  uint8_t *pixels=static_cast<uint8_t *>(frame.getPointer());
  int textureX=int(floorf(cameraX*mapScale-120)) & (RALLY_GRASS_SIZE-1);
  int textureY=int(floorf(position*mapScale+carY));
  int firstSpan=min(240,RALLY_GRASS_SIZE-textureX);
  for(int y=0;y<288;++y) {
    const uint8_t *source=rallyGrass+(((textureY-y)&(RALLY_GRASS_SIZE-1))*RALLY_GRASS_SIZE);
    uint8_t *row=pixels+y*240;
    memcpy(row,source+textureX,firstSpan);
    if(firstSpan<240) memcpy(row+firstSpan,source,240-firstSpan);
  }
  drawScenery(cameraX);
  // About 20 road strips instead of 180 perspective segments and scanlines.
  int first=int(floorf((position-320)/80));
  for(int i=first;i<first+20;++i) {
    float z1=i*80.f,z2=z1+80;
    float x1=120+(fastCenter(z1+distance-position)-cameraX)*mapScale;
    float x2=120+(fastCenter(z2+distance-position)-cameraX)*mapScale;
    float y1=carY-(z1-position)*mapScale;
    float y2=carY-(z2-position)*mapScale;
    quad(x1,y1,x2,y2,49,(i&1)?TFT_WHITE:0xB986);
    quad(x1,y1,x2,y2,45,0x4A49);
    if(i&1) quad(x1,y1,x2,y2,1,0xD69A);
  }
  // Checkered checkpoint line across the road.
  float cpZ = ceilf(position / CHECKPOINT_GAP) * CHECKPOINT_GAP;
  for (float z = cpZ; z < position + 1000; z += CHECKPOINT_GAP) {
    float sy = carY - (z - position) * mapScale;
    float cx = 120 + (center(z) - cameraX) * mapScale;
    // Lay the checks perpendicular to the road direction at this point.
    float slope = (center(z + 20.f) - center(z - 20.f)) / 40.f;
    float ang = atanf(slope);
    float px = cosf(ang), py = sinf(ang);       // across the road
    float dx = sinf(ang), dy = -cosf(ang);      // along the road (screen up)
    float cell = 7.5f / px;                     // 12 cells reach the kerbs
    for (int k = 0; k < 12; ++k)
      for (int r = 0; r < 2; ++r) {
        float u0 = (k - 6) * cell, u1 = u0 + cell, v0 = r * 4.f - 4.f, v1 = v0 + 4.f;
        int x0 = int(cx + px * u0 + dx * v0), y0 = int(sy + py * u0 + dy * v0);
        int x1 = int(cx + px * u1 + dx * v0), y1 = int(sy + py * u1 + dy * v0);
        int x2 = int(cx + px * u1 + dx * v1), y2 = int(sy + py * u1 + dy * v1);
        int x3 = int(cx + px * u0 + dx * v1), y3 = int(sy + py * u0 + dy * v1);
        uint16_t c = ((k + r) & 1) ? TFT_WHITE : TFT_BLACK;
        frame.fillTriangle(x0, y0, x1, y1, x2, y2, c);
        frame.fillTriangle(x0, y0, x2, y2, x3, y3, c);
      }
  }
  // Sparse asphalt flecks are fixed to the road, not to screen pixels.
  int firstGrain=int(floorf((position-300.f)/24.f));
  for(int nz=firstGrain;nz<firstGrain+52;++nz) {
    float z=nz*24.f;
    int sy=carY-int((z-position)*mapScale);
    float roadX=120+(fastCenter(z+distance-position)-cameraX)*mapScale;
    for(int nx=-9;nx<=9;++nx) {
      uint32_t grain=uint32_t(nz)*73856093u ^ uint32_t(nx)*19349663u;
      grain^=grain>>16;
      grain*=2246822519u;
      if((grain&7u)>1u) continue;
      int sx=int(roadX+nx*18*mapScale)+int((grain>>8)&3u)-1;
      int y=sy+int((grain>>12)&3u)-1;
      uint16_t color=(grain&0x18u)==0u?TFT_BLACK:0x6B4D;
      frame.fillRect(sx,y,(grain&0x20u)?2:1,1,color);
    }
  }
  drawBridges(cameraX);
  // Coins: gold discs with a darker rim and a highlight.
  for (const Coin &c : coinList) {
    int sy = carY - int((c.z - distance) * mapScale);
    if (sy < 20 || sy > 300) continue;
    int sx = 120 + int((fastCenter(c.z) + c.lat - cameraX) * mapScale);
    frame.fillCircle(sx, sy, 6, 0xC440);
    frame.fillCircle(sx, sy, 4, 0xFEA0);
    frame.drawFastVLine(sx - 1, sy - 2, 4, TFT_WHITE);
  }
  for (const Rival &r : rivals) {
    float sy = carY - (r.z - distance) * mapScale;
    if (sy < -30 || sy > 320) continue;
    float sx = 120 + (fastCenter(r.z) + r.lat - cameraX) * mapScale;
    drawCar(sx, sy, atanf((fastCenter(r.z + 20.f) - fastCenter(r.z - 20.f)) / 40.f), r.paint, r.dark);
  }
  drawCar(120, carY, heading, 0xE8E4, 0x9800);
  // HUD in the shared card colour.
  frame.fillRect(0, 0, 240, 26, ui::CARD);
  frame.setTextFont(2);
  frame.setTextDatum(TL_DATUM);
  frame.setTextColor(ui::LABEL, ui::CARD);
  frame.drawString(String(int(speed * .3f)) + " km/h", 8, 5);
  frame.setTextDatum(MC_DATUM);
  frame.setTextColor(timeLeft < 8 ? TFT_RED : ui::LABEL, ui::CARD);
  frame.drawString(String(int(ceilf(timeLeft))) + " s", 120, 13);
  frame.setTextDatum(TR_DATUM);
  frame.setTextColor(ui::SECONDARY, ui::CARD);
  frame.drawString(String(distance / 10000.f, 2) + " km", 232, 5);
  frame.fillCircle(142, 13, 5, 0xFEA0);
  frame.setTextDatum(TL_DATUM);
  frame.setTextColor(ui::LABEL, ui::CARD);
  frame.drawString(String(coins), 151, 5);
  // Standings panel: one colour swatch per car, leader on top, player boxed.
  int order[RIVALS + 1];
  for (int i = 0; i <= RIVALS; ++i) order[i] = i;  // RIVALS = player
  auto zOf = [](int i) { return i == RIVALS ? distance : rivals[i].z; };
  for (int i = 1; i <= RIVALS; ++i)
    for (int j = i; j > 0 && zOf(order[j]) > zOf(order[j - 1]); --j)
      std::swap(order[j], order[j - 1]);
  frame.fillRoundRect(202, 32, 34, (RIVALS + 1) * 18 + 6, 6, ui::CARD);
  for (int i = 0; i <= RIVALS; ++i) {
    int y = 35 + i * 18;
    bool me = order[i] == RIVALS;
    uint16_t c = me ? 0xE8E4 : rivals[order[i]].paint;
    if (me) frame.fillRoundRect(204, y, 30, 17, 4, ui::LABEL);
    frame.setTextDatum(TL_DATUM);
    frame.setTextColor(me ? ui::INK_DARK : ui::SECONDARY, me ? ui::LABEL : ui::CARD);
    frame.drawString(String(i + 1), 207, y + 1);
    frame.fillRoundRect(218, y + 3, 13, 11, 3, c);
    if (me) frame.drawRoundRect(218, y + 3, 13, 11, 3, ui::INK_DARK);
  }
  if (millis() < flashUntil) {
    frame.fillRoundRect(60, 36, 120, 22, 11, ACCENT);
    frame.setTextDatum(MC_DATUM);
    frame.setTextColor(ui::inkOn(ACCENT), ACCENT);
    frame.drawString(flashText, 120, 47);
  }
  frame.setTextDatum(TL_DATUM);
  frame.pushSprite(0, 0);
}
int score() { return int(distance / 10) + coins * 20; }
void drawFooter() {
  tft.fillRect(0, 288, 240, 32, TFT_BLACK);
  ui::hint(303, "Down brake  Shoot pause", TFT_BLACK);
}
void resetRace() {
  position = 0; speed = 0; lateral = 0; heading = roadHeading(position);
  lap = 1; distance = 0; timeLeft = START_TIME;
  nextCheckpoint = CHECKPOINT_GAP; flashUntil = 0;
  checkpoints = 0; coins = 0;
  // Rivals start on a staggered grid ahead of the player.
  static const uint16_t paints[RIVALS][2] = {
    {0x1C9F, 0x0010}, {0xFE60, 0xA400}, {0x2E8A, 0x0320}, {0xF81F, 0x8010}, {0xFFFF, 0x8410}};
  for (int i = 0; i < RIVALS; ++i) {
    Rival &r = rivals[i];
    // Two-lane staggered grid with clear gaps between every car.
    r.z = 240.f + i * 230.f;
    r.lat = (i & 1) ? 75.f : -75.f;
    r.laneTarget = r.lat;
    r.speed = 0;
    r.hitCooldown = 0;
    r.paint = paints[i][0]; r.dark = paints[i][1];
  }
  for (int i = 0; i < COINS; ++i) placeCoin(coinList[i], i * 400.f);
}
void startRace() {
  resetRace();
  tft.fillScreen(TFT_BLACK);
  render();
  drawFooter();
  showResumeCountdown([]() { render(); });
  state = 1;
  buttonsAllowHeldDirections();
  lastFrame = millis();
}
constexpr int START_PAGES = 3;
int startPage = 0;
void drawStartPage() {
  // Live track scene behind the card instead of a plain black screen.
  render();
  tft.fillRect(0, 288, 240, 32, TFT_BLACK);
  if (startPage == 0) {
    static const ui::Control controls[] = {
      {"Left / Right", "Steer"}, {"Down", "Brake"}, {"Shoot", "Pause"}};
    ui::startCard(40, "Rally", "Reach checkpoints in time", controls, 3, best, ACCENT, nullptr,
                  "Press Shoot to start");
  } else {
    static const ui::Control howTo[] = {
      {"Checkpoint", "Adds time"}, {"Timer", "Race ends at 0"},
      {"Grass", "Slows you down"}, {"Bump rival", "+3 coins"}};
    static const ui::Control scoring[] = {
      {"Distance", "1 pt / 10 m"}, {"Coin", "20 pts"},
      {"Place", "Pass rivals"}, {"Best", "Saved score"}};
    const ui::Control *rows = startPage == 1 ? howTo : scoring;
    ui::card(18, 60, 204, 200);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(ui::LABEL, ui::CARD);
    tft.drawString(startPage == 1 ? "How to play" : "Scoring", 32, 76, 4);
    ui::List l = {32, 116, 176, 30, 4};
    for (int i = 0; i < 4; ++i) ui::infoRow(l, i, rows[i].key, rows[i].action);
  }
  for (int i = 0; i < START_PAGES; ++i)
    tft.fillCircle(108 + i * 12, 298, 3, i == startPage ? ACCENT : 0x4208);
  ui::hint(312, "< > pages   Shoot start", TFT_BLACK);
}
void showStart() {
  state = 0;
  startPage = 0;
  resetRace();
  drawStartPage();
  buttonsRequireRelease();
}
void gameOver() {
  state = 3;
  int s = score();
  if (s > best) {
    best = s;
    Preferences p; p.begin("Rally", false); p.putInt("best", best); p.end();
  }
  render();
  static char title[20];
  snprintf(title, sizeof(title), "Place %d of %d", place(), RIVALS + 1);
  ui::gameOverSheet(title, s, best, ACCENT, "Time up");
  buttonsRequireRelease();
}
}
void rallySetup() {
  frame.setColorDepth(8);
  if (!frame.created()) ready = frame.createSprite(240, 288) != nullptr;
  tft.fillScreen(TFT_BLACK);
  if (!ready) { ui::heading(20, 80, "Rally", "Low memory"); return; }
  Preferences p; p.begin("Rally", true); best = p.getInt("best", 0); p.end();
  showStart();
}
void rallyLoop() {
  if (!ready) { delay(20); return; }
  int key;
  while ((key = buttonsNextPress()) >= 0) {
    if (state == 0) {
      if (key == 4) { playSelectTone(); startRace(); }
      else if (key == 1 || key == 3) {
        startPage = (startPage + (key == 1 ? 1 : START_PAGES - 1)) % START_PAGES;
        playNavClick();
        drawStartPage();
      }
    } else if (state == 1) {
      if (key == 4) {
        state = 2; pauseSelection = 0;
        ui::pauseSheet("Rally", PAUSE_ITEMS, 3, pauseSelection, ACCENT);
      }
    } else if (state == 2) {
      if (key == 0 || key == 2) {
        int old = pauseSelection;
        pauseSelection = (pauseSelection + (key == 0 ? 2 : 1)) % 3;
        ui::pauseItem(PAUSE_ITEMS, 3, old, pauseSelection, ACCENT);
        ui::pauseItem(PAUSE_ITEMS, 3, pauseSelection, pauseSelection, ACCENT);
        playNavClick();
      } else if (key == 4) {
        playSelectTone();
        if (pauseSelection == 0) {
          showResumeCountdown([]() { render(); });
          buttonsAllowHeldDirections();
          state = 1; lastFrame = millis();
        } else if (pauseSelection == 1) {
          startRace();
        } else {
          frame.deleteSprite(); ready = false;
          switchGame(0);
          return;
        }
      }
    } else if (state == 3) {
      if (key == 4) { playSelectTone(); startRace(); }
      else if (key == 0) {
        playSelectTone();
        frame.deleteSprite(); ready = false;
        switchGame(0);
        return;
      }
    }
  }
  if (state != 1) { delay(5); return; }
  uint32_t now = millis();
  if (now - lastFrame < 20) { delay(1); return; }
  float dt = min(.1f, (now - lastFrame) * .001f); lastFrame = now;
  uint8_t held = buttonsHeld();
  int steer = ((held & 2) ? 1 : 0) - ((held & 8) ? 1 : 0);
  float edge = fabsf(lateral);
  bool onRoad = edge <= 180.f;
  // Auto throttle: always accelerating toward top speed; Down brakes.
  // Off road: speed is capped low so the car can still crawl back.
  float topSpeed = onRoad ? 750.f : 190.f;
  float accel;
  // Always on the throttle; Down brakes.
  if (held & 4) accel = -650.f;
  else accel = onRoad ? 600.f - speed * .35f : 240.f;
  speed += accel * dt;
  if (speed > topSpeed) speed = max(topSpeed, speed - 520.f * dt);
  speed = constrain(speed, 0.f, 750.f);
  // Steering is quicker at low speed and slightly damped at top speed.
  float steerRate = 2.6f - 0.9f * speed / 750.f;
  heading += steer * steerRate * dt;
  // Without input the car straightens toward the road direction; off road it
  // also steers gently back toward the tarmac so nobody gets stranded.
  float target = roadHeading(position);
  if (!onRoad) target += constrain(-lateral / 400.f, -.6f, .6f);
  if (!steer) heading += (target - heading) * min(1.f, (onRoad ? 3.f : 1.5f) * dt);
  heading = constrain(heading, -1.25f, 1.25f);
  float oldCenter = center(position);
  float forward = cosf(heading) * speed * dt;
  float newPosition = position + forward;
  lateral += sinf(heading) * speed * dt - (center(newPosition) - oldCenter);
  // Soft barrier: hitting the outer limit scrubs speed instead of sticking.
  if (fabsf(lateral) > 420.f) {
    lateral = constrain(lateral, -420.f, 420.f);
    speed *= .9f;
  }
  position = newPosition;
  distance += forward;
  // Off-road water: wading through a river is slow.
  if (!onRoad && inRiver(distance, center(position) + lateral) && speed > 70.f) speed = max(70.f, speed - 900.f * dt);
  if (position >= length) { position -= length; ++lap; }
  if (distance >= nextCheckpoint) {
    nextCheckpoint += CHECKPOINT_GAP;
    // Bonus shrinks each checkpoint, so the clock eventually wins.
    int bonus = max(2, 8 - checkpoints);
    ++checkpoints;
    timeLeft += bonus;
    char msg[24]; snprintf(msg, sizeof(msg), "Checkpoint +%d s", bonus);
    flash(msg);
    playSelectTone();
  }
  // Rivals: each has its own pace with mild rubber-banding toward the player.
  for (int i = 0; i < RIVALS; ++i) {
    Rival &r = rivals[i];
    float gap = r.z - distance;
    float target = 570.f + i * 35.f + constrain(-gap * .1f, -120.f, 140.f);
    r.speed += constrain(target - r.speed, -300.f * dt, 300.f * dt);
    r.z += r.speed * dt;
    if (random(0, 400) == 0) r.laneTarget = random(-110, 111);
    r.lat += constrain(r.laneTarget - r.lat, -120.f * dt, 120.f * dt);
  }
  // Rivals keep their own distance: brake behind a slower car, and steer
  // around it when there is room.
  for (int i = 0; i < RIVALS; ++i)
    for (int j = 0; j < RIVALS; ++j) {
      if (i == j) continue;
      Rival &a = rivals[i], &b = rivals[j];
      float dz = b.z - a.z;
      if (dz > 0 && dz < 320.f && fabsf(b.lat - a.lat) < CAR_W + 20.f) {
        if (a.speed > b.speed) a.speed = max(b.speed, a.speed - 400.f * dt);
        a.laneTarget = b.lat > 0 ? b.lat - CAR_W - 40.f : b.lat + CAR_W + 40.f;
        a.laneTarget = constrain(a.laneTarget, -150.f, 150.f);
      }
    }
  // Rigid-body separation in road space. Cars are boxes CAR_W x CAR_L;
  // overlaps are pushed apart along the shallower axis.
  for (int i = 0; i < RIVALS; ++i) {
    for (int j = i + 1; j < RIVALS; ++j) {
      Rival &a = rivals[i], &b = rivals[j];
      float dz = b.z - a.z, dl = b.lat - a.lat;
      float pz = CAR_L - fabsf(dz), pl = CAR_W - fabsf(dl);
      if (pz <= 0 || pl <= 0) continue;
      if (pl * 2.f < pz) {
        float push = (dl >= 0 ? pl : -pl) * .5f;
        a.lat -= push; b.lat += push;
      } else {
        float push = (dz >= 0 ? pz : -pz) * .5f;
        a.z -= push; b.z += push;
        Rival &back = dz >= 0 ? a : b, &front = dz >= 0 ? b : a;
        float v = (back.speed + front.speed) * .5f;
        back.speed = v * .95f; front.speed = max(front.speed, v);
      }
    }
    // Player against this rival.
    Rival &r = rivals[i];
    float dz = r.z - distance, dl = r.lat - lateral;
    float pz = CAR_L - fabsf(dz), pl = CAR_W - fabsf(dl);
    if (pz <= 0 || pl <= 0) continue;
    bool side = pl * 2.f < pz;
    if (side) {
      // Side swipe: both cars bounce sideways, the rival takes more of it.
      float push = dl >= 0 ? pl : -pl;
      lateral -= push * .4f; r.lat += push * .6f;
      r.laneTarget = constrain(r.lat + (dl >= 0 ? 60.f : -60.f), -170.f, 170.f);
      heading -= (dl >= 0 ? .08f : -.08f);
      speed *= .97f;
    } else {
      float push = dz >= 0 ? pz : -pz;
      // Shift the rival rather than the player so the camera never jumps.
      r.z += push;
      if (dz >= 0) {               // rear-ending the rival: share momentum
        float v = (speed + r.speed) * .5f;
        r.speed = max(r.speed, v * 1.05f);
        speed = min(speed, v * .9f);
      } else {                     // rival rear-ends the player
        float v = (speed + r.speed) * .5f;
        speed = max(speed, v);
        r.speed = v * .85f;
      }
    }
    if (now > r.hitCooldown && (side || dz >= 0)) {
      r.hitCooldown = now + 900;
      speed *= .85f;
      coins += 3;
      flash("Bump +3 coins");
      playNavClick();
    }
  }
  for (Coin &c : coinList) {
    if (fabsf(c.z - distance) < 110.f && fabsf(c.lat - lateral) < 65.f) {
      ++coins;
      playNavClick();
      placeCoin(c, distance);
    } else if (c.z < distance - 200.f) {
      placeCoin(c, distance);
    }
  }
  timeLeft -= dt;
  if (timeLeft <= 0) { timeLeft = 0; gameOver(); return; }
  render();
}
