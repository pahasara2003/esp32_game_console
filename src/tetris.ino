
#include <string.h>
#include <Preferences.h>
#include "config.h"
#include "game.h"
#include "ui_kit.h"
#include "tetris_input.h"

#define SIZE 10
#define SPACE_WIDTH 20
#define SPACE_HEIGHT 20

constexpr int TETRIS_BOARD_X = 20, TETRIS_BOARD_Y = 52;
constexpr int TETRIS_HUD_Y = 266;
constexpr int TETRIS_HUD_H = 46;
constexpr unsigned long TETRIS_SOFT_DROP_MS = 35;
uint16_t TETRIS_GRID_DARK = 0x0802;
uint16_t TETRIS_GRID_LIGHT = 0x1044;
uint16_t TETRIS_CARD = 0x18E3;  // HUD card colour, set per theme in tetrisSetup()
constexpr uint16_t TETRIS_HIGHLIGHT = 0xFFFF;
static_assert(2 * TETRIS_BOARD_X + SPACE_WIDTH * SIZE == 240, "Board must be horizontally centered");
static_assert(TETRIS_BOARD_Y + SPACE_HEIGHT * SIZE + 14 == TETRIS_HUD_Y, "Board and HUD need a clear gap");
static_assert(TETRIS_HUD_Y + TETRIS_HUD_H <= 312, "HUD must leave a bottom margin");

static uint16_t tetrisMix(uint16_t a, uint16_t b, int amount) {
  int r = (((a >> 11) & 31) * (255 - amount) + ((b >> 11) & 31) * amount) / 255;
  int g = (((a >> 5) & 63) * (255 - amount) + ((b >> 5) & 63) * amount) / 255;
  int blue = ((a & 31) * (255 - amount) + (b & 31) * amount) / 255;
  return (r << 11) | (g << 5) | blue;
}

int tetrisLines = 0;
int tetrisLevel = 1;

// ---- Brick-wall backdrop ---------------------------------------------------
// 24x10 px bricks (23x9 plus mortar), every other row shifted half a brick.
static uint16_t brickTone(int row, int k) {
  uint16_t base = tetrisMix(UI_BG, 0xC3A8, 105);
  int v = (row * 7 + k * 13 + row * k) % 3;
  return tetrisMix(base, 0x0000, v * 28);
}

static uint16_t brickColorAt(int x, int y) {
  int row = y / 10, ly = y % 10;
  int xx = x + ((row & 1) ? 12 : 0);
  int lx = xx % 24, k = xx / 24;
  if (ly == 9 || lx == 23) return tetrisMix(UI_BG, 0x0000, 90);
  uint16_t c = brickTone(row, k);
  return ly == 0 ? tetrisMix(c, 0xFFFF, 40) : c;
}

static void drawTetrisBricks(int cx, int cy, int cw, int ch) {
  tft.fillRect(cx, cy, cw, ch, tetrisMix(UI_BG, 0x0000, 90));
  for (int row = cy / 10; row <= (cy + ch - 1) / 10; ++row) {
    int off = (row & 1) ? 12 : 0;
    int y0 = row * 10;
    int ry0 = max(y0, cy), ry1 = min(y0 + 9, cy + ch);
    if (ry0 >= ry1) continue;
    for (int k = (cx + off) / 24; k * 24 - off < cx + cw; ++k) {
      int x0 = k * 24 - off;
      int rx0 = max(x0, cx), rx1 = min(x0 + 23, cx + cw);
      if (rx0 >= rx1) continue;
      uint16_t c = brickTone(row, k);
      tft.fillRect(rx0, ry0, rx1 - rx0, ry1 - ry0, c);
      if (y0 >= cy) tft.drawFastHLine(rx0, y0, rx1 - rx0, tetrisMix(c, 0xFFFF, 40));
    }
  }
}

// Gradient capsule header: game name left, level chip right.
void drawTetrisHeader() {
  constexpr int W = 224, H = 28, R = H / 2;
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(W, H)) return;
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) header.drawPixel(x, y, brickColorAt(8 + x, 8 + y));
  for (int x = 0; x < W; ++x) {
    int dx = x < R ? R - x : x > W - 1 - R ? x - (W - 1 - R) : 0;
    int half = dx ? (int)sqrtf((float)(R * R - dx * dx)) : R;
    header.drawFastVLine(x, R - half, 2 * half,
                         tetrisMix(uiHeaderStart, uiHeaderEnd, x * 255 / (W - 1)));
  }
  uint16_t ink = ui::inkOn(tetrisMix(uiHeaderStart, uiHeaderEnd, 60));
  header.setTextColor(ink);
  header.setTextDatum(ML_DATUM);
  header.drawString("Tetris", 16, 15, 2);

  char level[12];
  snprintf(level, sizeof(level), "Level %d", tetrisLevel);
  int chipW = header.textWidth(level, 2) + 18;
  uint16_t chip = tetrisMix(uiHeaderEnd, 0x0000, 120);
  header.fillRoundRect(W - chipW - 5, 4, chipW, H - 8, (H - 8) / 2, chip);
  header.setTextColor(TFT_WHITE);
  header.setTextDatum(MC_DATUM);
  header.drawString(level, W - 5 - chipW / 2, 15, 2);
  header.pushSprite(8, 8);
}

void drawTetrisPlayfieldFrame() {
  // Also clear the area occupied by pause/game-over dialogs outside the board.
  drawTetrisBricks(8, 40, 224, 224);
  const int x = TETRIS_BOARD_X - 5, y = TETRIS_BOARD_Y - 5;
  const int w = SPACE_WIDTH * SIZE + 10, h = SPACE_HEIGHT * SIZE + 10;
  // Soft two-ring accent glow around a rounded well.
  tft.drawRoundRect(x - 1, y - 1, w + 2, h + 2, 10, tetrisMix(UI_BG, UI_CYAN, 50));
  tft.fillRoundRect(x, y, w, h, 9, tetrisMix(UI_CYAN, UI_PURPLE, 128));
  tft.fillRoundRect(x + 2, y + 2, w - 4, h - 4, 7, TETRIS_GRID_DARK);
}

unsigned long drop_interval = 600;
unsigned long lastDrop = 0;
unsigned long lastTick = 0;
unsigned long elapsedPlayMs = 0;
unsigned long lastShownSecond = 0;
unsigned long lastSoftDrop = 0;
bool tetrisDownWasHeld = false;
int tetrisPauseSelection = 0;
static const char *const TETRIS_PAUSE_ITEMS[] = {"Resume", "Restart", "High scores", "Main menu"};
uint8_t tetrisPressed = 0;
int tetrisSideHeld = 0;
unsigned long tetrisSideAt = 0;
bool tetrisSideRepeating = false;


uint32_t current_color;
int current_shape;

int current_pos[2] = { 0, 0 };
int prev_pos[2] = { 0, 0 };
bool current_body[4][4];
bool prev_body[4][4];
// One vivid colour per piece, matching the shape order in bodies[]:
// I cyan, O yellow, T purple, L orange, J blue, S green, Z red.
uint32_t colors[] = { 0x06FF, 0xFEA0, 0xAA9F, 0xFC60, 0x2B7F, 0x3EEB, 0xFA2B };
uint32_t space[SPACE_WIDTH * SPACE_HEIGHT];
int next_shape = 0;
uint32_t next_color = colors[0];

void updatePauseMenu(int dx);
void tetrisSetup();

bool bodies[7][4][4] = {
  { { 0, 0, 0, 0 }, { 1, 1, 1, 1 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 0, 1, 1, 0 }, { 0, 1, 1, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 1, 1, 1, 0 }, { 0, 1, 0, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 1, 1, 1, 0 }, { 1, 0, 0, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 1, 1, 1, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 0, 1, 1, 0 }, { 1, 1, 0, 0 }, { 0, 0, 0, 0 } },
  { { 0, 0, 0, 0 }, { 1, 1, 0, 0 }, { 0, 1, 1, 0 }, { 0, 0, 0, 0 } }
};

int bodySize() {
  return current_shape == 0 ? 4 : 3;
}

int getEncode(int x, int y) {
  return x + y * SPACE_WIDTH;
}

// Glossy bevelled block: light top/left edge, dark bottom/right, small shine.
void drawRect(int x, int y, uint32_t color) {
  int px = TETRIS_BOARD_X + x * SIZE, py = TETRIS_BOARD_Y + y * SIZE;
  uint16_t c = color;
  uint16_t light = tetrisMix(c, 0xFFFF, 120), dark = tetrisMix(c, 0x0000, 100);
  tft.drawFastHLine(px, py, SIZE, TETRIS_GRID_DARK);
  tft.drawFastVLine(px, py + 1, SIZE - 1, TETRIS_GRID_DARK);
  tft.fillRect(px + 1, py + 1, SIZE - 1, SIZE - 1, c);
  tft.drawFastHLine(px + 1, py + 1, SIZE - 2, light);
  tft.drawFastVLine(px + 1, py + 2, SIZE - 3, light);
  tft.drawFastHLine(px + 2, py + SIZE - 1, SIZE - 2, dark);
  tft.drawFastVLine(px + SIZE - 1, py + 2, SIZE - 3, dark);
  tft.fillRect(px + 3, py + 3, 2, 2, tetrisMix(c, 0xFFFF, 190));
}

// Empty cell: dark well with a faint dot grid.
void drawRect(int x, int y) {
  int px = TETRIS_BOARD_X + x * SIZE;
  int py = TETRIS_BOARD_Y + y * SIZE;
  tft.fillRect(px, py, SIZE, SIZE, TETRIS_GRID_DARK);
  tft.drawPixel(px + SIZE / 2, py + SIZE / 2, TETRIS_GRID_LIGHT);
}

void redrawChangedCells() {
  tft.startWrite();
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      int px_old = prev_pos[0] + i - 1;
      int py_old = prev_pos[1] + j - 1;
      int px_new = current_pos[0] + i - 1;
      int py_new = current_pos[1] + j - 1;

      if (prev_body[i][j]) {
        bool still_covered = false;
        int di = px_old - (current_pos[0] - 1);
        int dj = py_old - (current_pos[1] - 1);
        if (di >= 0 && di < 4 && dj >= 0 && dj < 4 && current_body[di][dj])
          still_covered = true;

        if (!still_covered && px_old >= 0 && px_old < SPACE_WIDTH
            && py_old >= 0 && py_old < SPACE_HEIGHT) {
          uint32_t dead = space[getEncode(px_old, py_old)];
          if (dead != TFT_BLACK) drawRect(px_old, py_old, dead);
          else drawRect(px_old, py_old);
        }
      }

      if (current_body[i][j] && px_new >= 0 && px_new < SPACE_WIDTH
          && py_new >= 0 && py_new < SPACE_HEIGHT) {
        drawRect(px_new, py_new, current_color);
      }
    }
  }
  tft.endWrite();
}

void drawBody() {
  tft.startWrite();
  int x = current_pos[0];
  int y = current_pos[1];
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (current_body[i][j]) {
        int px = i + x - 1;
        int py = j + y - 1;
        if (px >= 0 && px < SPACE_WIDTH && py >= 0 && py < SPACE_HEIGHT)
          drawRect(px, py, current_color);
      }
  tft.endWrite();
}

void drawNext() {
  TFT_eSprite preview(&tft);
  preview.setColorDepth(16);
  if (!preview.createSprite(56, 26)) return;
  preview.fillSprite(TETRIS_CARD);
  int minX = 4, minY = 4, maxX = 0, maxY = 0;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (bodies[next_shape][i][j]) {
        minX = min(minX, i); maxX = max(maxX, i);
        minY = min(minY, j); maxY = max(maxY, j);
      }
  int left = (56 - (maxX - minX + 1) * 6) / 2;
  int top = (26 - (maxY - minY + 1) * 6) / 2;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
      if (!bodies[next_shape][i][j]) continue;
      int x = left + (i - minX) * 6, y = top + (j - minY) * 6;
      preview.fillRect(x, y, 5, 5, next_color);
      preview.drawFastHLine(x, y, 5, tetrisMix(next_color, 0xFFFF, 120));
      preview.drawFastHLine(x, y + 4, 5, tetrisMix(next_color, 0x0000, 100));
    }
  preview.pushSprite(92, TETRIS_HUD_Y + 19);
}

void redrawRowsAbove(int cleared_row) {
  tft.startWrite();
  for (int y = 0; y <= cleared_row; y++) {
    for (int x = 0; x < SPACE_WIDTH; x++) {
      uint32_t c = space[getEncode(x, y)];
      if (c != TFT_BLACK) drawRect(x, y, c);
      else drawRect(x, y);
    }
  }
  tft.endWrite();
}

void drawMenu(void) {
  const int cardX[] = {8, 84, 160};
  const char *labels[] = {"Score", "Next", "Time"};
  const uint16_t dots[] = {UI_YELLOW, UI_PURPLE, UI_CYAN};
  for (int i = 0; i < 3; i++) {
    tft.fillRoundRect(cardX[i], TETRIS_HUD_Y, 72, TETRIS_HUD_H, 12, TETRIS_CARD);
    tft.fillCircle(cardX[i] + 12, TETRIS_HUD_Y + 12, 3, dots[i]);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(0x6226, TETRIS_CARD);
    tft.drawString(labels[i], cardX[i] + 20, TETRIS_HUD_Y + 4, 2);
  }
}

static unsigned long tetrisScoreFlashAt = 0;

void drawTetrisScore() {
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", score);
  TFT_eSprite value(&tft);
  value.setColorDepth(16);
  if (!value.createSprite(56, 24)) return;
  value.fillSprite(TETRIS_CARD);
  value.setTextDatum(TL_DATUM);
  value.setTextFont(2);
  if (value.textWidth(buf) > 56) value.setTextFont(1);
  value.setTextColor(millis() - tetrisScoreFlashAt < 350 ? UI_CYAN : 0x2104);
  value.drawString(buf, 0, 2);
  value.pushSprite(17, TETRIS_HUD_Y + 21);
}

void updateScore() {
  score += 10;
  tetrisScoreFlashAt = millis();
  drawTetrisScore();
}

void updateTime() {
  unsigned long totalSeconds = elapsedPlayMs / 1000;
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu", totalSeconds / 60, totalSeconds % 60);
  TFT_eSprite value(&tft);
  value.setColorDepth(16);
  if (!value.createSprite(56, 24)) return;
  value.fillSprite(TETRIS_CARD);
  value.setTextDatum(TL_DATUM);
  value.setTextFont(2);
  if (value.textWidth(buffer) > 56) value.setTextFont(1);
  value.setTextColor(0x2104);
  value.drawString(buffer, 0, 2);
  value.pushSprite(169, TETRIS_HUD_Y + 21);
}

void addToDead() {
  int x = current_pos[0];
  int y = current_pos[1];
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
      int px = x + i - 1;
      int py = y + j - 1;
      if (px >= 0 && px < SPACE_WIDTH && py >= 0 && py < SPACE_HEIGHT)
        if (current_body[i][j])
          space[getEncode(px, py)] = current_color;
    }
}

bool checkCollision(int dx, int dy) {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (current_body[i][j]) {
        int px = current_pos[0] + i - 1 + dx;
        int py = current_pos[1] + j - 1 + dy;
        if (px < 0 || px >= SPACE_WIDTH || py >= SPACE_HEIGHT) return true;
        if (py >= 0 && space[getEncode(px, py)] != TFT_BLACK) return true;
      }
  return false;
}

bool newTetro() {
  int cleared = 0;
  bool levelUp = false;
  current_color = next_color;
  current_shape = next_shape;
  next_shape = random(7);
  next_color = colors[next_shape];
  current_pos[0] = random(SPACE_WIDTH - bodySize() + 1) + 1;
  current_pos[1] = 1;
  drawNext();

  for (int j = 0; j < SPACE_HEIGHT; j++) {
    int fill = 0;
    for (int i = 0; i < SPACE_WIDTH; i++)
      if (space[getEncode(i, j)] != TFT_BLACK) fill++;

    if (fill == SPACE_WIDTH) {
      ++cleared;
      // Quick white flash on the completed row before it collapses.
      tft.startWrite();
      for (int x = 0; x < SPACE_WIDTH; x++) drawRect(x, j, 0xFFFF);
      tft.endWrite();
      delay(70);
      for (int y = j; y > 0; y--)
        for (int x = 0; x < SPACE_WIDTH; x++)
          space[getEncode(x, y)] = space[getEncode(x, y - 1)];

      for (int x = 0; x < SPACE_WIDTH; x++)
        space[getEncode(x, 0)] = TFT_BLACK;

      redrawRowsAbove(j);
      updateScore();
      // Every 5 lines: next level, slightly faster gravity.
      int level = ++tetrisLines / 5 + 1;
      if (level != tetrisLevel) {
        levelUp = true;
        tetrisLevel = level;
        drop_interval = max(150, 600 - (tetrisLevel - 1) * 50);
        drawTetrisHeader();
      }
      j--;
    }
  }

  if (cleared) audio::play(cleared >= 4 ? audio::Cue::FourLines :
                           levelUp ? audio::Cue::Level : audio::Cue::Clear);
  memcpy(current_body, bodies[current_shape], sizeof(current_body));
  memcpy(prev_pos, current_pos, sizeof(current_pos));
  memcpy(prev_body, current_body, sizeof(current_body));
  return !checkCollision(0, 0);
}

void tryRotate() {
  if (current_shape == 1) return; // Rotating the square should not move it.
  bool temp[4][4] = {};
  int size = bodySize();
  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++)
      temp[i][j] = current_body[size - 1 - j][i];

  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++)
      if (temp[i][j]) {
        int px = current_pos[0] + i - 1;
        int py = current_pos[1] + j - 1;
        if (px < 0 || px >= SPACE_WIDTH || py >= SPACE_HEIGHT) return;
        if (py >= 0 && space[getEncode(px, py)] != TFT_BLACK) return;
      }

  memcpy(prev_pos, current_pos, sizeof(current_pos));
  memcpy(prev_body, current_body, sizeof(current_body));
  memcpy(current_body, temp, sizeof(temp));
  audio::play(audio::Cue::Rotate);
  redrawChangedCells();
}

void updatePosition(int dx = 0, int dy = 1) {
  if (!checkCollision(dx, dy)) {
    memcpy(prev_pos, current_pos, sizeof(current_pos));
    memcpy(prev_body, current_body, sizeof(current_body));
    current_pos[0] += dx;
    current_pos[1] += dy;
    redrawChangedCells();
  } else if (dy == 1) {
    addToDead();
    playTetrisDropSound();
    if (!newTetro()) {
      audio::play(audio::Cue::GameOver);
      gameState = 2;
      menu_drawn = false;
      tetrisInputRequireRelease();
    } else drawBody();
  }
}




void getCommand() {
  if (tetrisPressed & 16) {
    audio::pauseMusic(true); gameState = 1;
    menu_drawn = false;
    tetrisPauseSelection = 0;
    tetrisInputRequireRelease();
    tetrisSideHeld = 0;
    return;
  }
  if (tetrisPressed & 1) tryRotate();
  uint8_t held = tetrisInputHeld();
  int side = ((held & 2) != 0) - ((held & 8) != 0);
  unsigned long now = millis();
  // Queued taps still count even when a redraw outlasted the whole press.
  int tappedSide = (tetrisPressed & 2) ? 1 : (tetrisPressed & 8) ? -1 : 0;
  if (tappedSide) {
    updatePosition(tappedSide, 0);
    tetrisSideHeld = side == tappedSide ? side : 0;
    tetrisSideAt = now;
    tetrisSideRepeating = false;
  } else if (side != tetrisSideHeld) {
    // The scanner may publish held state between this loop's event and held
    // reads. Wait for its press event before starting movement or repeats.
    tetrisSideHeld = 0;
  } else if (side && now - tetrisSideAt >= (tetrisSideRepeating ? 65UL : 180UL)) {
    updatePosition(side, 0);
    tetrisSideAt = now;
    tetrisSideRepeating = true;
  }
}


void restoreAfterPauseTetris() {
  showResumeCountdown([]() {
  if (!restoringCountdownDigit()) drawTetrisBricks(0, 0, 240, 320);
  drawTetrisHeader();
  if (!restoringCountdownDigit()) drawTetrisPlayfieldFrame();
  tft.startWrite();
  for (int y = 0; y < SPACE_HEIGHT; y++)
    for (int x = 0; x < SPACE_WIDTH; x++) {
      uint32_t c = space[getEncode(x, y)];
      if (c != TFT_BLACK) drawRect(x, y, c);
      else drawRect(x, y);
    }
  tft.endWrite();
  drawBody();
  drawMenu();
  drawTetrisScore();
  drawNext();
  updateTime();
  });
  lastDrop = millis();
  lastTick = lastDrop;
  lastSoftDrop = lastDrop;
  tetrisDownWasHeld = false;
  gameState = 0;
  menu_drawn = false;
  tetrisSideHeld = 0;
  tetrisInputRequireRelease();
}

void drawTetrisPauseOption(int item) {
  ui::pauseItem(TETRIS_PAUSE_ITEMS, 4, item, tetrisPauseSelection, UI_CYAN);
}

void drawTetrisPauseMenu() {
  ui::pauseSheet("Tetris", TETRIS_PAUSE_ITEMS, 4, tetrisPauseSelection, UI_CYAN);
}

void handleTetrisPause() {
  int pressed = (tetrisPressed & 16) ? 4 : (tetrisPressed & 1) ? 0 : (tetrisPressed & 4) ? 2 : -1;
  if (pressed == 0 || pressed == 2) {
    int oldSelection = tetrisPauseSelection;
    tetrisPauseSelection = (tetrisPauseSelection + (pressed == 0 ? 3 : 1)) % 4;
    drawTetrisPauseOption(oldSelection);
    drawTetrisPauseOption(tetrisPauseSelection);
    playNavClick();
  } else if (pressed == 4) {
    playSelectTone();
    if (tetrisPauseSelection == 0) restoreAfterPauseTetris();
    else if (tetrisPauseSelection == 1) tetrisSetup();
    else if (tetrisPauseSelection == 2) {
      showGameHighScores();
      tetrisPressed = 0;
      tetrisInputRequireRelease();
      drawTetrisBricks(0, 0, 240, 320);
      drawTetrisHeader();
      drawTetrisPlayfieldFrame();
      tft.startWrite();
      for (int y = 0; y < SPACE_HEIGHT; ++y)
        for (int x = 0; x < SPACE_WIDTH; ++x) {
          uint32_t color = space[getEncode(x, y)];
          if (color != TFT_BLACK) drawRect(x, y, color);
          else drawRect(x, y);
        }
      tft.endWrite();
      drawMenu();
      drawNext();
      drawTetrisScore();
      updateTime();
      drawBody();
      drawTetrisPauseMenu();
    } else switchGame(0);
  }
}

void drawTetrisGameOver() {
  currentGame.recordScore(score);
  ui::gameOverSheet("Tetris", score, currentGame.highScores[0], UI_CYAN);
}

void handleTetrisGameOver() {
  int pressed = (tetrisPressed & 16) ? 4 : (tetrisPressed & 1) ? 0 : -1;
  if (pressed == 4) tetrisSetup();
  else if (pressed == 0) switchGame(0);
}



void tetrisSetup() {
  audio::restartMusic();
  tetrisInputBegin();
  tetrisSideHeld = 0;
  TETRIS_GRID_DARK = tetrisMix(UI_BG, UI_PURPLE, 22);
  TETRIS_GRID_LIGHT = tetrisMix(UI_BG, UI_CYAN, 90);
  TETRIS_CARD = 0xDE32;  // cream-brown with dark text
  score = 0;
  tetrisLines = 0;
  tetrisLevel = 1;
  gameState = 0;
  menu_drawn = false;
  drop_interval = 600;
  elapsedPlayMs = 0;
  lastShownSecond = 0;
  tetrisPauseSelection = 0;
  lastDrop = millis();
  lastTick = lastDrop;
  lastSoftDrop = lastDrop;
  tetrisDownWasHeld = false;
  next_shape = random(7);
  next_color = colors[next_shape];
  for (int i = 0; i < SPACE_WIDTH * SPACE_HEIGHT; i++)
    space[i] = TFT_BLACK;

  tft.setRotation(2);
  drawTetrisBricks(0, 0, 240, 320);
  drawTetrisHeader();
  drawTetrisPlayfieldFrame();

  tft.startWrite();
  for (int i = 0; i < SPACE_WIDTH; i++)
    for (int j = 0; j < SPACE_HEIGHT; j++)
      drawRect(i, j);
  tft.endWrite();

  drawMenu();
  drawTetrisScore();
  updateTime();
  newTetro();
  drawBody();
  lastDrop = millis();
  lastTick = lastDrop;
}

void tetrisLoop() {
  audio::gameState(gameState);
  tetrisPressed = tetrisInputPoll();
  if (gameState == 0) {
    unsigned long currentTime = millis();
    elapsedPlayMs += currentTime - lastTick;
    lastTick = currentTime;
    getCommand();
    if (gameState != 0) return;
    currentTime = millis();

    bool downHeld = (tetrisInputHeld() & 4) != 0;
    if ((tetrisPressed & 4) || (downHeld && tetrisDownWasHeld &&
                     currentTime - lastSoftDrop >= TETRIS_SOFT_DROP_MS)) {
      updatePosition(0, 1);
      if (gameState != 0) return;
      lastSoftDrop = millis();
      lastDrop = lastSoftDrop;
      currentTime = lastSoftDrop;
    }
    tetrisDownWasHeld = downHeld && (tetrisDownWasHeld || (tetrisPressed & 4));

    unsigned long shownSecond = elapsedPlayMs / 1000;
    if (shownSecond != lastShownSecond) {
      lastShownSecond = shownSecond;
      updateTime();
    }

    if (currentTime - lastDrop >= drop_interval) {
      lastDrop = currentTime;
      updatePosition();
    }
  } else if (gameState == 1) {
    if (!menu_drawn) {
      drawTetrisPauseMenu();
      menu_drawn = true;
    }
    handleTetrisPause();
  } else if (gameState == 2) {
    if (!menu_drawn) {
      drawTetrisGameOver();
      menu_drawn = true;
    }
    handleTetrisGameOver();
  }
}
