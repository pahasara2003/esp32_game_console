#include "game.h"
#include <Arduino.h>

// Portrait games share the same 240 x 320 content frame and input conventions.
#include "ui_kit.h"

namespace mini {
enum Phase { INTRO, PLAY, PAUSE, FINISH };
static Phase phase = INTRO;
static int pauseChoice = 0;
static const char *title = "";  // game name shown on sheets
static const char *const PAUSE_ITEMS[] = {"Resume", "New game", "High scores", "Main menu"};
static unsigned long pressedAt = 0;
static bool longSent = false;
static bool chordSent = false;

static void resetInput() {
  pressedAt = 0;
  longSent = false;
  chordSent = false;
}

// A short Shoot press is 4; a hold is 5. The caller chooses what the hold does.
static int input() {
  int command = findCommand();
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'w' || c == 'W') return 0;
    if (c == 'd' || c == 'D') return 1;
    if (c == 's' || c == 'S') return 2;
    if (c == 'a' || c == 'A') return 3;
    if (c == 'q' || c == 'Q' || c == 'f' || c == 'F') return 5;
    if (c == ' ' || c == 'e' || c == 'E' || c == '\n') return 4;
  }
  bool down = (buttonsHeld() & 16) != 0;
  // Start short/long-press tracking from the queued, debounced event. Raw
  // GPIO edges and a later queued event must not count as two separate taps.
  if (command == 4 && pressedAt == 0) {
    pressedAt = millis() ? millis() : 1;
    longSent = chordSent = false;
  }
  if (down && command == 0 && !chordSent) {
    chordSent = longSent = true;
    if (!pressedAt) pressedAt = millis() ? millis() : 1;
    return 0;
  }
  if (down) {
    if (pressedAt && !longSent && millis() - pressedAt >= 850) {
      longSent = true;
      return 5;
    }
    return -1;
  }
  if (pressedAt) {
    bool shortPress = !longSent;
    resetInput();
    return shortPress ? 4 : -1;
  }
  return command == 4 ? -1 : command;
}

static void header(const char *name, const char *detail) {
  title = name;
  if (!restoringCountdownDigit()) tft.fillScreen(UI_BG);
  tft.fillRoundRect(10, 10, 220, 44, 12, ui::CARD);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString(name, 22, 33, 4);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString(detail, 218, 33, 2);
}

static void footer(const char *top, const char *bottom) {
  tft.fillRect(0, 277, 240, 36, UI_BG);
  ui::hint(286, top, UI_BG, ui::LABEL);
  ui::hint(303, bottom, UI_BG);
}

static constexpr uint16_t BOARD_TILE_COLOR = 0xBFE0;

// Both boards share one yellow-green face; bevels distinguish covered mines.
static void boardTile(int x, int y, int size, int radius,
                      bool selected, bool recessed = false) {
  tft.fillRect(x, y, size, size, UI_BG);
  tft.fillRoundRect(x, y, size, size, radius, BOARD_TILE_COLOR);
  tft.drawRoundRect(x, y, size, size, radius, 0x7C80);
  if (recessed) {
    tft.drawRoundRect(x + 2, y + 2, size - 4, size - 4, radius - 1, 0x9DC0);
  } else {
    tft.drawFastHLine(x + radius, y + 2, size - radius * 2, 0xDFF0);
    tft.drawFastHLine(x + radius, y + size - 3, size - radius * 2, 0x9DC0);
    tft.drawFastVLine(x + size - 3, y + radius, size - radius * 2, 0x9DC0);
  }
  if (selected) {
    for (int inset = 0; inset < 3; ++inset)
      tft.drawRoundRect(x + inset, y + inset, size - inset * 2,
                        size - inset * 2, radius - inset, TFT_MAGENTA);
  }
}

static void intro(const char *name, const char *line1, const char *line2,
                  const char *line3) {
  header(name, "New game");
  ui::card(18, 66, 204, 206);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString(name, 32, 80, 4);
  tft.drawString(line1, 32, 118, 2);
  tft.drawString(line2, 32, 136, 2);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString(line3, 32, 164, 2);
  ui::pill(32, 226, 176, 30, "Press Shoot to start", UI_CYAN);
  tft.fillRect(0, 277, 240, 36, UI_BG);
  ui::hint(294, "Up for home screen", UI_BG);
}

static void pause(const char *) {
  ui::pauseSheet(title, PAUSE_ITEMS, 4, pauseChoice, UI_CYAN);
}

static void result(const char *outcome, const char *detail) {
  ui::card(24, 66, 192, 186);
  ui::heading(40, 80, title, outcome);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString(detail, 40, 134, 2);
  ui::pill(40, 188, 160, 30, "Play again", UI_CYAN);
  ui::hint(234, "Up for home screen", ui::CARD);
}

static bool commonIntro(int cmd, void (*start)()) {
  if (phase != INTRO) return false;
  if (cmd == 4) { playSelectTone(); start(); }
  else if (cmd == 0) switchGame(0);
  return true;
}

static bool commonPause(int cmd, void (*draw)(), void (*start)()) {
  if (phase != PAUSE) return false;
  if (cmd == 0 || cmd == 2) {
    pauseChoice = (pauseChoice + (cmd == 2 ? 1 : 3)) % 4;
    playNavClick();
    pause("PAUSED");
  } else if (cmd == 4) {
    playSelectTone();
    if (pauseChoice == 0) { showResumeCountdown(draw); resetInput(); phase = PLAY; }
    else if (pauseChoice == 1) start();
    else if (pauseChoice == 2) {
      showGameHighScores();
      draw();
      pause("PAUSED");
    } else switchGame(0);
  }
  return true;
}

static void enterPause() {
  buttonsRequireRelease();
  pauseChoice = 0;
  phase = PAUSE;
  pause("PAUSED");
}
} // namespace mini

// ---------------- Tic-tac-toe: human X versus an optimal computer O. ----------------
namespace tic {
static uint8_t cells[9];
static int cursor = 4;
static int outcome = 0;
static bool recorded = false;
static bool cpuPending = false;
static unsigned long cpuAt = 0;

static int winner() {
  const uint8_t lines[8][3] = {
    {0,1,2},{3,4,5},{6,7,8},{0,3,6},
    {1,4,7},{2,5,8},{0,4,8},{2,4,6}
  };
  for (const auto &line : lines) {
    uint8_t mark = cells[line[0]];
    if (mark && mark == cells[line[1]] && mark == cells[line[2]]) return mark;
  }
  for (uint8_t cell : cells) if (!cell) return 0;
  return 3;
}

static int solve(int turn, int depth) {
  int result = winner();
  if (result == 2) return 10 - depth;
  if (result == 1) return depth - 10;
  if (result == 3) return 0;
  int best = turn == 2 ? -100 : 100;
  for (int i = 0; i < 9; ++i) {
    if (cells[i]) continue;
    cells[i] = turn;
    int value = solve(turn == 2 ? 1 : 2, depth + 1);
    cells[i] = 0;
    if (turn == 2) best = max(best, value);
    else best = min(best, value);
  }
  return best;
}

static void drawTile(int i) {
  const int x0 = 27, y0 = 75, size = 60, gap = 3;
  int x = x0 + (i % 3) * (size + gap);
  int y = y0 + (i / 3) * (size + gap);
  mini::boardTile(x, y, size, 9, i == cursor);
  if (cells[i] == 1) {
    for (int offset = -2; offset <= 2; ++offset) {
      tft.drawLine(x + 17 + offset, y + 18, x + 43 + offset, y + 44, TFT_BLACK);
      tft.drawLine(x + 43 + offset, y + 18, x + 17 + offset, y + 44, TFT_BLACK);
    }
  } else if (cells[i] == 2) {
    for (int radius = 14; radius <= 18; ++radius)
      tft.drawCircle(x + 30, y + 31, radius, TFT_BLACK);
  }
}

static void draw() {
  mini::header("Tic-tac-toe", "You are X");
  if (restoringCountdownDigit()) {
    for (int gap = 0; gap < 2; ++gap) {
      tft.fillRect(87 + gap * 63, 75, 3, 186, UI_BG);
      tft.fillRect(27, 135 + gap * 63, 186, 3, UI_BG);
    }
  }
  for (int i = 0; i < 9; ++i) drawTile(i);
  mini::footer(cpuPending ? "Console is thinking..." : "Shoot to place",
               "Hold Shoot to pause");
}

static void start() {
  memset(cells, 0, sizeof(cells));
  cursor = 4;
  recorded = false;
  cpuPending = false;
  mini::phase = mini::PLAY;
  draw();
}

static void finish() {
  outcome = winner();
  if (!outcome) return;
  mini::phase = mini::FINISH;
  if (outcome == 1 && !recorded) {
    currentGame.recordScore(100);
    recorded = true;
  }
  mini::result(outcome == 1 ? "You win" : outcome == 2 ? "Console wins" : "Draw",
               outcome == 1 ? "Three in a row." : outcome == 2 ? "Try another round." : "Nobody got three.");
}

static int computerTurn() {
  int best = -100, choice = -1;
  for (int i = 0; i < 9; ++i) {
    if (cells[i]) continue;
    cells[i] = 2;
    int value = solve(1, 1);
    cells[i] = 0;
    if (value > best || choice < 0) { best = value; choice = i; }
  }
  if (choice >= 0) cells[choice] = 2;
  return choice;
}
}

void ticTacToeSetup() {
  tft.setRotation(2);
  mini::phase = mini::INTRO;
  mini::resetInput();
  mini::intro("Tic-tac-toe", "Get three in a row.",
              "The console plays O.", "D-pad moves, Shoot places");
}

void ticTacToeLoop() {
  int cmd = mini::input();
  if (mini::commonIntro(cmd, tic::start)) return;
  if (mini::phase == mini::PAUSE) {
    mini::commonPause(cmd, tic::draw, tic::start);
    if (mini::phase == mini::PLAY && tic::cpuPending) tic::cpuAt = millis();
    return;
  }
  if (mini::phase == mini::FINISH) {
    if (cmd == 4) tic::start();
    else if (cmd == 0) switchGame(0);
    return;
  }
  if (cmd == 5) { mini::enterPause(); return; }
  if (tic::cpuPending) {
    if (millis() - tic::cpuAt >= 1000) {
      tic::cpuPending = false;
      int reply = tic::computerTurn();
      if (reply >= 0) { tic::drawTile(reply); playNavClick(); }
      mini::footer("Shoot to place", "Hold Shoot to pause");
      tic::finish();
    }
    return;
  }
  if (cmd >= 0 && cmd <= 3) {
    int row = tic::cursor / 3, col = tic::cursor % 3;
    if (cmd == 0 && row > 0) --row;
    if (cmd == 2 && row < 2) ++row;
    if (cmd == 3 && col > 0) --col;
    if (cmd == 1 && col < 2) ++col;
    int next = row * 3 + col;
    if (next != tic::cursor) {
      int previous = tic::cursor;
      tic::cursor = next;
      playNavClick();
      tic::drawTile(previous);
      tic::drawTile(next);
    }
  } else if (cmd == 4 && !tic::cells[tic::cursor]) {
    tic::cells[tic::cursor] = 1;
    playSelectTone();
    tic::drawTile(tic::cursor);
    tic::finish();
    if (mini::phase == mini::PLAY) {
      tic::cpuPending = true;
      tic::cpuAt = millis();
      mini::footer("Console is thinking...", "Hold Shoot to pause");
    }
  }
}

// ---------------- Pong: horizontal paddle, scoring against the wall. ----------------
namespace pong {
static float ballX, ballY, vx, vy;
static int paddleX, lives, score;
static int drawnX, drawnY, drawnPaddle, drawnScore, drawnLives;
static unsigned long lastFrame;
static unsigned long lastPaddleStep;
static int lastPaddleDir;
static bool shootWasDown;
static bool recorded;
static constexpr uint16_t ACCENT = 0xFD20;
static constexpr uint16_t ACCENT_LIGHT = 0xFFE0;
static constexpr uint16_t DARK_TEXT = 0x20A0;
static constexpr uint16_t FRAME = 0x6A20;
static constexpr int GRID = 10;
static constexpr int FIELD_TOP = 48;
static constexpr int FIELD_BOTTOM = 290;
static constexpr int GRID_ROWS = (FIELD_BOTTOM - FIELD_TOP + GRID) / GRID;
static const uint16_t GRID_COLORS[4] = {0x0003, 0x0005, 0x0007, 0x0808};
static uint8_t grid[24][GRID_ROWS];

static void initBackdrop() {
  for (int x = 0; x < 24; ++x)
    for (int y = 0; y < GRID_ROWS; ++y)
      grid[x][y] = random(4);
}

static void drawBackdrop() {
  for (int gy = 0; gy < GRID_ROWS; ++gy) {
    int y = FIELD_TOP + gy * GRID;
    int h = min(GRID, FIELD_BOTTOM - y + 1);
    for (int gx = 0; gx < 24; ++gx)
      tft.fillRect(gx * GRID, y, GRID, h, GRID_COLORS[grid[gx][gy]]);
  }
  for (int x = 0; x < 240; x += GRID)
    tft.drawFastVLine(x, FIELD_TOP, FIELD_BOTTOM - FIELD_TOP + 1, 0x0001);
  for (int y = FIELD_TOP; y <= FIELD_BOTTOM; y += GRID)
    tft.drawFastHLine(0, y, 240, 0x0001);
  tft.drawFastHLine(16, 107, 208, ACCENT_LIGHT);
}

static void restoreBackdropRect(int x, int y, int w, int h) {
  int x0 = max(0, x), x1 = min(240, x + w);
  int y0 = max(FIELD_TOP, y), y1 = min(FIELD_BOTTOM + 1, y + h);
  if (x1 <= x0 || y1 <= y0) return;
  for (int gy = (y0 - FIELD_TOP) / GRID; gy <= (y1 - 1 - FIELD_TOP) / GRID; ++gy) {
    int cellY = FIELD_TOP + gy * GRID;
    int sy = max(y0, cellY), ey = min(y1, cellY + GRID);
    for (int gx = x0 / GRID; gx <= (x1 - 1) / GRID; ++gx) {
      int cellX = gx * GRID;
      int sx = max(x0, cellX), ex = min(x1, cellX + GRID);
      tft.fillRect(sx, sy, ex - sx, ey - sy, GRID_COLORS[grid[gx][gy]]);
      if (sx == cellX) tft.drawFastVLine(cellX, sy, ey - sy, 0x0001);
      if (sy == cellY) tft.drawFastHLine(sx, cellY, ex - sx, 0x0001);
    }
  }
  if (y0 <= 107 && y1 > 107) {
    int sx = max(x0, 16), ex = min(x1, 224);
    if (ex > sx) tft.drawFastHLine(sx, 107, ex - sx, ACCENT_LIGHT);
  }
}

static void drawHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(224, 38)) return;
  header.fillSprite(UI_BG);
  header.fillRoundRect(0, 0, 224, 38, 12, ui::CARD);
  header.setTextDatum(ML_DATUM);
  header.setTextColor(ui::LABEL);
  header.drawString("Pong", 12, 20, 4);
  header.setTextDatum(TR_DATUM);
  header.setTextColor(ui::SECONDARY);
  header.drawString("Score", 150, 3, 2);
  header.drawString("Lives", 212, 3, 2);
  header.setTextColor(ui::LABEL);
  header.drawString(String(score), 150, 19, 2);
  for (int i = 0; i < 3; ++i)
    header.fillCircle(184 + i * 12, 27, 4, i < lives ? 0xF800 : 0x4208);
  header.pushSprite(8, 5);
  header.deleteSprite();
}

static void drawFooter() {
  tft.fillRoundRect(52, 296, 136, 20, 10, ui::CARD);
  ui::hint(306, "Shoot to pause", ui::CARD);
}

static void serve() {
  ballX = 120; ballY = 170;
  vx = random(2) ? 1.5f : -1.5f;
  vy = -2.5f;
  paddleX = 94;
  lastFrame = millis();
}

static void draw() {
  if (!restoringCountdownDigit()) tft.fillScreen(UI_BG);
  drawBackdrop();
  tft.drawFastHLine(0, FIELD_TOP - 1, 240, UI_SURFACE);
  tft.drawFastHLine(0, FIELD_BOTTOM + 1, 240, UI_SURFACE);
  drawHeader();
  drawFooter();
  tft.fillRoundRect(paddleX, 249, 52, 7, 3, TFT_WHITE);
  tft.fillCircle((int)ballX, (int)ballY, 5, ACCENT_LIGHT);
  drawnX = (int)ballX; drawnY = (int)ballY;
  drawnPaddle = paddleX; drawnScore = score; drawnLives = lives;
}

static void drawMotion() {
  int nextX = (int)ballX, nextY = (int)ballY;
  if (nextX != drawnX || nextY != drawnY) {
    restoreBackdropRect(drawnX - 5, drawnY - 5, 11, 11);
    if (drawnY + 5 >= 249 && drawnY - 5 <= 255)
      tft.fillRoundRect(paddleX, 249, 52, 7, 3, TFT_WHITE);
    tft.fillCircle(nextX, nextY, 5, ACCENT_LIGHT);
  }
  if (paddleX != drawnPaddle) {
    restoreBackdropRect(drawnPaddle, 249, 52, 7);
    tft.fillRoundRect(paddleX, 249, 52, 7, 3, TFT_WHITE);
  }
  if (score != drawnScore || lives != drawnLives) drawHeader();
  drawnX = nextX; drawnY = nextY;
  drawnPaddle = paddleX; drawnScore = score; drawnLives = lives;
}

static void movePaddle(int direction) {
  if (!direction) { lastPaddleDir = 0; return; }
  unsigned long now = millis();
  if (direction == lastPaddleDir && now - lastPaddleStep < 14) return;
  lastPaddleDir = direction;
  lastPaddleStep = now;
  int next = constrain(paddleX + direction * 4, 19, 169);
  if (next == paddleX) return;
  restoreBackdropRect(drawnPaddle, 249, 52, 7);
  paddleX = next;
  tft.fillRoundRect(paddleX, 249, 52, 7, 3, TFT_WHITE);
  if ((int)ballY + 5 >= 249 && (int)ballY - 5 <= 255)
    tft.fillCircle((int)ballX, (int)ballY, 5, ACCENT_LIGHT);
  drawnPaddle = paddleX;
}

static void showIntro() {
  tft.fillScreen(UI_BG);
  drawHeader();
  static const ui::Control controls[] = {
    {"Left / Right", "Move"}, {"Shoot", "Pause"}, {"Lives", "3 misses"}};
  ui::startCard(52, "Pong", "Keep the ball in play", controls, 3,
                currentGame.highScores[0], ACCENT, nullptr, "Press Shoot to start");
}

static void drawPauseOption(int item) {
  ui::pauseItem(mini::PAUSE_ITEMS, 4, item, mini::pauseChoice, ACCENT);
}

static void drawPongPauseMenu() {
  ui::pauseSheet("Pong", mini::PAUSE_ITEMS, 4, mini::pauseChoice, ACCENT);
}

static void enterPause() {
  buttonsRequireRelease();
  mini::resetInput();
  mini::pauseChoice = 0;
  mini::phase = mini::PAUSE;
  drawPongPauseMenu();
}

static void start();

static bool handlePause(int cmd) {
  if (mini::phase != mini::PAUSE) return false;
  if (cmd == 0 || cmd == 2) {
    int old = mini::pauseChoice;
    mini::pauseChoice = (mini::pauseChoice + (cmd == 2 ? 1 : 3)) % 4;
    drawPauseOption(old);
    drawPauseOption(mini::pauseChoice);
    playNavClick();
  } else if (cmd == 4) {
    playSelectTone();
    if (mini::pauseChoice == 0) {
      showResumeCountdown(draw);
      mini::resetInput();
      mini::phase = mini::PLAY;
      shootWasDown = false;
      lastFrame = millis();
    } else if (mini::pauseChoice == 1) start();
    else if (mini::pauseChoice == 2) {
      showGameHighScores();
      draw();
      drawPongPauseMenu();
    } else switchGame(0);
  }
  return true;
}

static void drawGameOver() {
  ui::gameOverSheet("Pong", score, currentGame.highScores[0], ACCENT);
}

static void start() {
  lives = 3; score = 0; recorded = false;
  lastPaddleDir = 0;
  lastPaddleStep = 0;
  shootWasDown = false;
  initBackdrop();
  serve();
  mini::phase = mini::PLAY;
  draw();
}

static void end() {
  mini::phase = mini::FINISH;
  if (!recorded) { currentGame.recordScore(score); recorded = true; }
  drawGameOver();
}
}

void pongSetup() {
  tft.setRotation(2);
  mini::phase = mini::INTRO;
  mini::resetInput();
  pong::score = 0;
  pong::lives = 3;
  pong::showIntro();
}

void pongLoop() {
  // During play, use cached held controls without menu key-repeat.
  int cmd = mini::phase == mini::PLAY ? -1 : mini::input();
  if (mini::commonIntro(cmd, pong::start)) return;
  if (pong::handlePause(cmd)) return;
  if (mini::phase == mini::FINISH) {
    if (cmd == 4) pong::start();
    else if (cmd == 0) switchGame(0);
    return;
  }
  bool shootDown = readShoot();
  if (shootDown) pong::shootWasDown = true;
  else if (pong::shootWasDown) {
    pong::shootWasDown = false;
    mini::resetInput();
    pong::enterPause();
    return;
  }
  if (Serial.available()) {
    char c = Serial.peek();
    if (c == 'q' || c == 'Q' || c == ' ' || c == 'e' || c == 'E') {
      Serial.read();
      pong::enterPause();
      return;
    }
  }
  int direction = getDirectHorizontalInput();
  pong::movePaddle(direction);
  unsigned long now = millis();
  if (now - pong::lastFrame < 28) return;
  float steps = min(3.0f, (now - pong::lastFrame) / 28.0f);
  pong::lastFrame = now;
  pong::ballX += pong::vx * steps;
  pong::ballY += pong::vy * steps;
  if (pong::ballX <= 20 || pong::ballX >= 220) { pong::vx = -pong::vx; pong::ballX = constrain(pong::ballX, 20.0f, 220.0f); }
  if (pong::ballY <= 112) {
    pong::ballY = 112;
    pong::vy = abs(pong::vy);
    ++pong::score;
    playNavClick();
  }
  if (pong::vy > 0 && pong::ballY >= 243 && pong::ballY <= 256 &&
      pong::ballX >= pong::paddleX - 4 && pong::ballX <= pong::paddleX + 56) {
    pong::ballY = 243;
    pong::vy = -min(5.0f, 2.5f + pong::score * 0.08f);
    pong::vx = constrain((pong::ballX - (pong::paddleX + 26)) / 10.0f, -3.5f, 3.5f);
    if (abs(pong::vx) < 0.6f) pong::vx = 0.6f;
    playNavClick();
  }
  if (pong::ballY > 264) {
    if (--pong::lives <= 0) { pong::end(); return; }
    pong::serve();
  }
  pong::drawMotion();
}

// ---------------- Minesweeper: 8 x 8 board, safe first reveal. ----------------
namespace mines {
static constexpr int SIDE = 8;
static constexpr int COUNT = SIDE * SIDE;
static constexpr int BOMBS = 10;
static uint8_t counts[COUNT];
static bool bomb[COUNT], revealed[COUNT], flagged[COUNT];
static bool dirty[COUNT];
static int cursor, exposed, flags, result;
static bool planted, recorded;


static void neighbors(int index, void (*visit)(int)) {
  int row = index / SIDE, col = index % SIDE;
  for (int dr = -1; dr <= 1; ++dr)
    for (int dc = -1; dc <= 1; ++dc) {
      if (!dr && !dc) continue;
      int nr = row + dr, nc = col + dc;
      if (nr >= 0 && nr < SIDE && nc >= 0 && nc < SIDE) visit(nr * SIDE + nc);
    }
}

static void plant(int safe) {
  memset(bomb, 0, sizeof(bomb));
  memset(counts, 0, sizeof(counts));
  int placed = 0;
  while (placed < BOMBS) {
    int index = random(COUNT);
    if (index == safe || bomb[index]) continue;
    bomb[index] = true;
    ++placed;
  }
  for (int i = 0; i < COUNT; ++i)
    if (bomb[i]) neighbors(i, [](int n) { ++counts[n]; });
  planted = true;
}

static void uncover(int index) {
  uint8_t pending[COUNT];
  int head = 0, tail = 0;
  auto reveal = [&](int cell) {
    if (revealed[cell] || flagged[cell] || bomb[cell]) return;
    // Mark before enqueueing: every square enters the fixed queue at most once.
    revealed[cell] = true;
    dirty[cell] = true;
    ++exposed;
    pending[tail++] = cell;
  };
  reveal(index);
  while (head < tail) {
    int cell = pending[head++];
    if (counts[cell]) continue;
    int row = cell / SIDE, col = cell % SIDE;
    for (int dr = -1; dr <= 1; ++dr)
      for (int dc = -1; dc <= 1; ++dc) {
        int nr = row + dr, nc = col + dc;
        if (nr >= 0 && nr < SIDE && nc >= 0 && nc < SIDE)
          reveal(nr * SIDE + nc);
      }
  }
}

static void drawTile(int i) {
  const int tile = 25, gap = 1, left = 16, top = 61;
  int x = left + (i % SIDE) * (tile + gap);
  int y = top + (i / SIDE) * (tile + gap);
  const uint16_t fill = revealed[i] ? 0xEF7D : TFT_WHITE;
  // A one-pixel shadow fits in the grid gap without touching another tile.
  tft.fillRect(x, y, tile + 1, tile + 1, UI_BG);
  tft.fillRoundRect(x + 1, y + 1, tile, tile, 4, 0x6B4D);
  tft.fillRoundRect(x, y, tile, tile, 4, fill);
  tft.drawRoundRect(x, y, tile, tile, 4, revealed[i] ? 0xADB5 : 0xD6BA);
  if (!revealed[i]) {
    tft.drawFastHLine(x + 5, y + tile - 3, tile - 10, 0xDEFB);
    tft.drawFastVLine(x + tile - 3, y + 5, tile - 10, 0xDEFB);
  }
  if (i == cursor)
    for (int inset = 0; inset < 3; ++inset)
      tft.drawRoundRect(x + inset, y + inset, tile - inset * 2,
                        tile - inset * 2, 4 - inset, TFT_MAGENTA);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  if (flagged[i] && !revealed[i]) {
    tft.drawFastVLine(x + 8, y + 6, 14, TFT_BLACK);
    tft.drawFastHLine(x + 5, y + 19, 11, TFT_BLACK);
    tft.fillTriangle(x + 9, y + 6, x + 9, y + 14, x + 18, y + 10, 0xF800);
  } else if (revealed[i] && bomb[i]) {
    tft.fillCircle(x + 12, y + 13, 5, 0x18C3);
    tft.drawLine(x + 15, y + 9, x + 18, y + 5, 0x18C3);
    tft.fillCircle(x + 18, y + 5, 1, 0xF800);
  } else if (revealed[i] && counts[i]) {
    const uint16_t numberColors[] = {0x0000, 0x023F, 0x0440, 0xF800,
                                     0x3012, 0x9800, 0x03EF, 0x18C3, 0x6B4D};
    tft.setTextColor(numberColors[counts[i]], fill);
    tft.drawString(String(counts[i]), x + 12, y + 12);
  } else if (revealed[i]) {
    // The selection border covers the bevel, so empty cells need their own cue.
    tft.fillCircle(x + 12, y + 12, 2, 0x94B2);
  }
}

static void drawDirty() {
  for (int i = 0; i < COUNT; ++i)
    if (dirty[i]) { drawTile(i); dirty[i] = false; }
}

static void draw() {
  mini::header("Mines", "10 mines");
  for (int i = 0; i < COUNT; ++i) drawTile(i);
  mini::footer("Shoot reveals, hold to flag", "Shoot + Up to pause");
}

static void start() {
  memset(revealed, 0, sizeof(revealed));
  memset(flagged, 0, sizeof(flagged));
  memset(bomb, 0, sizeof(bomb));
  memset(counts, 0, sizeof(counts));
  memset(dirty, 0, sizeof(dirty));
  cursor = 27; exposed = 0; flags = 0; result = 0;
  planted = false; recorded = false;
  mini::phase = mini::PLAY;
  draw();
}

static void finish(bool won) {
  result = won ? 1 : -1;
  mini::phase = mini::FINISH;
  if (won && !recorded) { currentGame.recordScore(max(1, 1000 - flags * 10)); recorded = true; }
  mini::result(won ? "Cleared" : "Boom", won ? "Every safe tile found." : "You hit a mine.");
}
}

void minesweeperSetup() {
  tft.setRotation(2);
  mini::phase = mini::INTRO;
  mini::resetInput();
  mini::intro("Mines", "Reveal every safe tile.",
              "Numbers count nearby mines.", "Your first tap is always safe");
}

void minesweeperLoop() {
  int cmd = mini::input();
  if (mini::commonIntro(cmd, mines::start)) return;
  if (mini::commonPause(cmd, mines::draw, mines::start)) return;
  if (mini::phase == mini::FINISH) {
    if (cmd == 4) mines::start();
    else if (cmd == 0) switchGame(0);
    return;
  }
  // Holding Shoot while pressing Up opens the pause menu; holding alone flags.
  if (readShoot() && cmd == 0) { mini::enterPause(); return; }
  if (cmd >= 0 && cmd <= 3) {
    int row = mines::cursor / mines::SIDE, col = mines::cursor % mines::SIDE;
    if (cmd == 0 && row > 0) --row;
    if (cmd == 2 && row < mines::SIDE - 1) ++row;
    if (cmd == 3 && col > 0) --col;
    if (cmd == 1 && col < mines::SIDE - 1) ++col;
    int next = row * mines::SIDE + col;
    if (next != mines::cursor) {
      int previous = mines::cursor;
      mines::cursor = next;
      playNavClick();
      mines::drawTile(previous);
      mines::drawTile(next);
    }
  } else if (cmd == 5 && !mines::revealed[mines::cursor]) {
    bool &flag = mines::flagged[mines::cursor];
    flag = !flag;
    mines::flags += flag ? 1 : -1;
    playNavClick();
    mines::drawTile(mines::cursor);
  } else if (cmd == 4 && !mines::flagged[mines::cursor] && !mines::revealed[mines::cursor]) {
    if (!mines::planted) mines::plant(mines::cursor);
    if (mines::bomb[mines::cursor]) {
      mines::revealed[mines::cursor] = true;
      mines::drawTile(mines::cursor);
      mines::finish(false);
    } else {
      mines::uncover(mines::cursor);
      playSelectTone();
      mines::drawDirty();
      if (mines::exposed == mines::COUNT - mines::BOMBS) mines::finish(true);
    }
  }
}
