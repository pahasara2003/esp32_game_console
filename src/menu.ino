#include "game.h"
#include "ui_kit.h"
#include "home_icons.h"
#include "home_wallpaper.h"
#include "config.h"
#include "birthday.h"
#include "birthday_code.h"

// Define apps
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
        // Colourful frosted tint sweeping across the widget.
        color = homeBlend(color, homeBlend(uiHeaderStart, uiHeaderEnd, x * 255 / WIDTH), 48);
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
  BirthdayCode secret;
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
    if (secret.press(c, millis())) {
      showBirthdayScene();
      drawAboutInformation();
      buttonsRequireRelease();
    }
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

// 1. Never Gonna Give You Up (Rick Astley)
const SongNote rickNotes[] = {
  {294, 150}, {330, 150}, {392, 150}, {330, 150},
  {494, 280}, {440, 380}, {0, 140},
  {294, 150}, {330, 150}, {392, 150}, {330, 150},
  {440, 280}, {392, 380}, {0, 140},
  {294, 150}, {330, 150}, {392, 150}, {330, 150},
  {392, 220}, {440, 200}, {370, 280}, {330, 220}, {294, 380}
};

// 2. Astronomia / Coffin Dance (Vicetone)
const SongNote astronomiaNotes[] = {
  {587, 110}, {587, 110}, {587, 110}, {587, 110},
  {392, 220}, {494, 220}, {440, 220}, {523, 220},
  {494, 220}, {392, 220}, {440, 220}, {587, 220},
  {494, 220}, {392, 220}, {440, 220}, {587, 220}
};

// 3. Blinding Lights (The Weeknd)
const SongNote blindingNotes[] = {
  {698, 170}, {698, 170}, {622, 170}, {698, 220},
  {784, 220}, {523, 330}, {0, 90},
  {698, 170}, {698, 170}, {622, 170}, {698, 220},
  {784, 220}, {523, 330}, {0, 90},
  {784, 170}, {784, 170}, {698, 170}, {622, 220}, {523, 380}
};

// 4. Bad Guy (Billie Eilish)
const SongNote badGuyNotes[] = {
  {392, 130}, {392, 130}, {392, 130}, {294, 130},
  {349, 130}, {392, 130}, {0, 130},
  {392, 130}, {392, 130}, {392, 130}, {294, 130},
  {349, 130}, {392, 130}, {0, 130},
  {466, 130}, {440, 130}, {392, 260}
};

// 5. Despacito (Luis Fonsi) - Extended Chorus & Main Hook
const SongNote despacitoNotes[] = {
  // Main iconic intro hook: D5 - C#5 - B4 - F#4
  {587, 300}, {554, 180}, {494, 200}, {370, 220}, {0, 100},
  // "Des - pa - ci - to..."
  {370, 140}, {370, 140}, {370, 140}, {370, 140},
  {494, 180}, {494, 180}, {494, 180}, {494, 240},
  {440, 160}, {494, 160}, {392, 360}, {0, 120},
  // Second phrase: "Quiero respirar tu cuello despacito..."
  {392, 140}, {392, 140}, {392, 140}, {392, 140},
  {392, 180}, {494, 180}, {494, 180}, {494, 240},
  {440, 160}, {392, 160}, {440, 320}, {0, 120},
  // Third phrase: "Deja que te diga cosas al oído..."
  {440, 140}, {440, 140}, {440, 140}, {440, 140},
  {440, 180}, {587, 180}, {587, 180}, {587, 240},
  {554, 160}, {494, 160}, {440, 280}, {370, 360}
};

// 6. Tetris Theme A (Korobeiniki)
const SongNote tetrisNotes[] = {
  {659, 220}, {494, 120}, {523, 120}, {587, 220}, {523, 120}, {494, 120},
  {440, 220}, {440, 120}, {523, 120}, {659, 220}, {587, 120}, {523, 120},
  {494, 280}, {523, 120}, {587, 220}, {659, 220},
  {523, 220}, {440, 220}, {440, 320}
};

const Track jukeboxPlaylist[] = {
  {"Never Gonna Give You Up", "Rick Astley", rickNotes, sizeof(rickNotes)/sizeof(rickNotes[0])},
  {"Astronomia (Coffin)",     "Vicetone",    astronomiaNotes, sizeof(astronomiaNotes)/sizeof(astronomiaNotes[0])},
  {"Blinding Lights",         "The Weeknd",  blindingNotes, sizeof(blindingNotes)/sizeof(blindingNotes[0])},
  {"Bad Guy",                 "Billie Eilish", badGuyNotes, sizeof(badGuyNotes)/sizeof(badGuyNotes[0])},
  {"Despacito",               "Luis Fonsi",  despacitoNotes, sizeof(despacitoNotes)/sizeof(despacitoNotes[0])},
  {"Tetris Korobeiniki",      "Retro Classic", tetrisNotes, sizeof(tetrisNotes)/sizeof(tetrisNotes[0])}
};

const int JUKEBOX_COUNT = sizeof(jukeboxPlaylist)/sizeof(jukeboxPlaylist[0]);

void playSoundTestScreen() {
  int currentSong = 4; // Default directly to Despacito (Track #5)
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
    
    // Exit back to menu on LEFT (command 3)
    if (c == 3) {
      setBuzzerTone(0);
      playNavClick();
      break;
    }

    if (c == 0) { // UP
      setBuzzerTone(0);
      isPlaying = false;
      currentSong = (currentSong - 1 + JUKEBOX_COUNT) % JUKEBOX_COUNT;
      playNavClick();
      drawJukeboxUI();
      delay(150);
    } else if (c == 2) { // DOWN
      setBuzzerTone(0);
      isPlaying = false;
      currentSong = (currentSong + 1) % JUKEBOX_COUNT;
      playNavClick();
      drawJukeboxUI();
      delay(150);
    } else if (c == 4) { // SHOOT: Play / Stop
      if (isPlaying) {
        setBuzzerTone(0);
        isPlaying = false;
        drawJukeboxUI();
        delay(200);
      } else {
        isPlaying = true;
        drawJukeboxUI();
        
        // Play selected track with interactive abort on any button press
        const Track& trk = jukeboxPlaylist[currentSong];
        for (int i = 0; i < trk.noteCount; i++) {
          // Check for button press to stop or change song
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
          
          // Mini visualizer bar in card
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

static const ui::List settingsList = {16, 68, 208, 30, 6};
void drawSettingsRow(int row, bool selected) {
  const char *labels[] = {"Theme", "Sound", "Effects volume", "Music volume", "Game music", "Reset scores"};
  String value = row == 0 ? titleCase(themeName(activeTheme)) :
                 row == 1 ? String(soundEnabled ? "On" : "Off") :
                 row == 2 ? String(audio::effectsVolume) + "%" :
                 row == 3 ? String(audio::musicVolume) + "%" :
                 row == 4 ? String(audio::gameMusic ? "On" : "Off") : String("Reset");
  ui::valueRow(settingsList, row, labels[row], value.c_str(), selected, MENU_ACCENT,
               row == 5 ? UI_RED : ui::SECONDARY);
}

static void drawSettingsHeader() {
  ui::heading(20, 16, "Console", "Settings", UI_BG);
}

static void drawSettingsPalette() {
  // Theme swatches; the ring marks the active one.
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
  // Theme changes affect the header and accent colors, but the neutral panel
  // stays identical. Updating these regions in place avoids a visible black
  // frame between the old and new themes.
  drawSettingsHeader();
  for (int row=0; row<6; ++row) drawSettingsRow(row, selectedRow == row);
  drawSettingsPalette();
  drawSettingsFooter();
}

void drawSettingsScreen(int selectedRow) {
  tft.fillScreen(UI_BG);
  refreshSettingsTheme(selectedRow);
}

// iOS-style alert with two side-by-side buttons.
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
      selectedRow = (selectedRow + (cmd == 0 ? 5 : 1)) % 6;
      playNavClick();
      drawSettingsRow(previousRow, false);
      drawSettingsRow(selectedRow, true);
    } else if (cmd == 1 || cmd == 3) {
      if (selectedRow == 0) {
        int direction = (cmd == 1) ? 1 : -1;
        applyThemePreset((activeTheme + direction + THEME_COUNT) % THEME_COUNT, false);
        playNavClick();
        refreshSettingsTheme(selectedRow);
      } else if (selectedRow == 1) {
        setSoundEnabled(!soundEnabled, false);
        playNavClick();
        drawSettingsRow(selectedRow, true);
      } else if (selectedRow == 2 || selectedRow == 3) {
        uint8_t &volume = selectedRow == 2 ? audio::effectsVolume : audio::musicVolume;
        volume = constrain(int(volume) + (cmd == 1 ? 10 : -10), 0, 100);
        audio::play(audio::Cue::Move);
        drawSettingsRow(selectedRow, true);
      } else if (selectedRow == 4) {
        audio::gameMusic = !audio::gameMusic;
        playNavClick();
        drawSettingsRow(selectedRow, true);
      }
    } else if (cmd == 4) {
      if (selectedRow == 5) {
        if (confirmResetScores()) {
          resetAllGameScores();
          audio::play(audio::Cue::Saved);
          tft.fillScreen(UI_BG);
          ui::hint(160, "Scores reset", UI_BG, UI_GREEN);
          delay(650);
        }
        drawSettingsScreen(selectedRow);
      } else {
        applyThemePreset(activeTheme, true);
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
  tft.setRotation(2); // Vertical (Portrait) mode: 240x320
  buttonsRequireRelease();
  selectedApp = 0;
  homePage = 0;
  redrawVisibleTiles();
}

void menuLoop() {
  // Check inputs
  int cmd = findCommand(false);
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'i' || c == 'I') buttonsPrintDiagnostics();
    if (c == 'r' || c == 'R') buttonsRecordFor(180000);
    if (c == 'w' || c == 'W') cmd = 0;       // UP
    else if (c == 'd' || c == 'D') cmd = 1;  // RIGHT
    else if (c == 's' || c == 'S') cmd = 2;  // DOWN
    else if (c == 'a' || c == 'A') cmd = 3;  // LEFT
    else if (c == ' ' || c == 'e' || c == 'E' || c == 'x' || c == 'X') cmd = 4; // SELECT
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
    case 0: // UP
      if (pageRow > 0) newSelected = selectedApp - COLS;
      break;

    case 2: // DOWN
      if (pageRow + 1 < VISIBLE_ROWS && selectedApp + COLS < pageEnd)
        newSelected = selectedApp + COLS;
      break;

    case 1: // RIGHT
      if (pageColumn + 1 < COLS && selectedApp + 1 < pageEnd) {
        newSelected = selectedApp + 1;
      } else if (currentPage + 1 < PAGE_COUNT) {
        // At the right edge, enter the next page on the same row. The final
        // page may be incomplete, so select its last available app if needed.
        int nextPageStart = (currentPage + 1) * APPS_PER_PAGE;
        newSelected = min(nextPageStart + pageRow * COLS, APP_COUNT - 1);
      }
      break;

    case 3: // LEFT
      if (pageColumn > 0) {
        newSelected = selectedApp - 1;
      } else if (currentPage > 0) {
        // At the left edge, return to the rightmost app on the matching row.
        int previousPageStart = (currentPage - 1) * APPS_PER_PAGE;
        int previousPageEnd = min(previousPageStart + APPS_PER_PAGE, APP_COUNT);
        newSelected = min(previousPageStart + pageRow * COLS + COLS - 1,
                          previousPageEnd - 1);
      }
      break;

    case 4: // SELECT
      {
        launcherTile.deleteSprite();
        free(tileGlass);  // give the ~16 KB of launcher masks back to the app
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
