// Standalone preview of the launcher UI (no games, buttons or audio).
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "home_icons.h"
#include "home_wallpaper.h"
TFT_eSPI tft;
uint16_t uiCyan = 0x5E7F, uiPurple = 0x9A3F, uiHeaderStart = 0x7DFF, uiHeaderEnd = 0xCB5F;
#define UI_CYAN uiCyan
#define UI_PURPLE uiPurple
static bool buttonsPendingPress() { return false; }
void runMusicPlayer();
struct App {
  const char* label;
  int x_coord;
  int y_coord;
  const uint16_t* img;
  int gameIndex; // Positive indices launch a game/app; -1 opens a utility.
};

App Apps[] = {
  {"Tetris",   0, 0, tetris_home_icon,   1},
  {"Snake",    0, 0, snake_home_icon,    2},
  {"Breakout", 0, 0, breakout_home_icon, 3},
  {"Run",      0, 0, run_home_icon,      4},
  {"Galaga",   0, 0, galaga_home_icon,   5},
  {"Puzzle",   0, 0, puzzle_home_icon,   6},
  {"Tic-Tac",  0, 0, tic_tac_home_icon,  7},
  {"Pong",     0, 0, pong_home_icon,     8},
  {"Mines",    0, 0, mines_home_icon,    9},
  {"Conway's", 0, 0, life_home_icon,    10},
  {"Mandel",   0, 0, mandelbrot_home_icon,11},
  {"Music",    0, 0, sound_home_icon,   -1},
  {"Settings", 0, 0, settings_home_icon,-1},
  {"Info",     0, 0, about_home_icon,   -1}
};

const int APP_COUNT = sizeof(Apps) / sizeof(App);

// Modern phone-style launcher: frosted-glass header widget, floating icons
// with soft drop shadows, a glass focus card with an accent glow, and an
// iOS-style capsule page indicator. All composition happens in small buffers.
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

// Distance (in 1/4 px) outside a rounded rectangle; 0 when inside.
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

// Precomputed per-pixel tile effects, built once (about 16 KB):
//   glass  – white frosting for the focused card (with a brighter rim)
//   glow   – accent halo hugging the focused icon
//   shadow – soft drop shadow beneath every icon
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
  launcherTile.setTextFont(2);  // proportional sans, easier to read than the 5x7 font
  launcherTile.setTextDatum(MC_DATUM);
  launcherTile.setTextColor(0x0842);
  launcherTile.drawString(label, TILE_W / 2 + 1, LABEL_TOP + 9);
  launcherTile.setTextColor(TFT_WHITE);
  launcherTile.drawString(label, TILE_W / 2, LABEL_TOP + 8);
}

// Renders a label into a 0/1/2 mask (none / shadow / white).
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

// Frosted dock with an elongated capsule for the active page.
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
          color = p == homePage ? TFT_WHITE : homeBlend(color, TFT_WHITE, 110);
        c += w + 6;
      }
      pixels[y * WIDTH + x] = color;
    }
  }
  tft.setSwapBytes(true);
  tft.pushImage(LEFT, TOP, WIDTH, HEIGHT, pixels);
}

// Frosted-glass header widget.
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
      } else if (out < 4) {
        color = homeBlend(color, TFT_WHITE, 30);
      }
      pixels[y * WIDTH + x] = homeSwap(color);
    }
  }
  // Accent orb with the playable-game count.
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

// Draw moving foreground over a stationary wallpaper, using native RGB565
// throughout. Only small label masks and one eight-row strip are cached;
// two full-page frame buffers would consume too much memory on this console.
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
    // Finish the transition on new input instead of making the next command
    // wait through every remaining frame of this relatively expensive draw.
    int t = (buttonsPendingPress() || Serial.available()) ? 220 : min((int)elapsed, 220);
    // Cubic ease-out, in fixed point. Always finish at exactly one page width.
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


void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(2);
  redrawVisibleTiles();
  Serial.println("Menu preview: w/a/s/d to move, e to open Music");
}

void loop() {
  if (!Serial.available()) { delay(5); return; }
  char c = Serial.read();
  if (c == 'e' || c == ' ') {
    if (strcmp(Apps[selectedApp].label, "Music") == 0) {
      launcherTile.deleteSprite();
      free(tileGlass);  // release launcher effect masks (~16 KB) while the player runs
      tileGlass = tileGlow = tileShadow = nullptr;
      runMusicPlayer();
      redrawVisibleTiles();
    }
    return;
  }
  int page = selectedApp / APPS_PER_PAGE, slot = selectedApp % APPS_PER_PAGE;
  int row = slot / COLS, col = slot % COLS, n = selectedApp;
  int pageEnd = min((page + 1) * APPS_PER_PAGE, APP_COUNT);
  if (c == 'w' && row > 0) n -= COLS;
  else if (c == 's' && row + 1 < VISIBLE_ROWS && n + COLS < pageEnd) n += COLS;
  else if (c == 'd') {
    if (col + 1 < COLS && n + 1 < pageEnd) n++;
    else if (page + 1 < PAGE_COUNT) n = min((page + 1) * APPS_PER_PAGE + row * COLS, APP_COUNT - 1);
  } else if (c == 'a') {
    if (col > 0) n--;
    else if (page > 0) n = (page - 1) * APPS_PER_PAGE + row * COLS + COLS - 1;
  }
  if (n == selectedApp) return;
  int old = selectedApp; selectedApp = n;
  int np = n / APPS_PER_PAGE;
  if (np != homePage) { int op = homePage; homePage = np; slideLauncherPage(op, np, old); }
  else { drawAppTile(old, false); drawAppTile(n, true); }
}
