#include "game.h"
#include <Arduino.h>

// Portrait games share the same 240 x 320 content frame and input conventions.
namespace mini {
enum Phase { INTRO, PLAY, PAUSE, FINISH };
static Phase phase = INTRO;
static int pauseChoice = 0;
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

static void header(const char *title, const char *detail) {
  tft.fillScreen(UI_BG);
  tft.fillRoundRect(10, 10, 220, 44, 10, UI_PANEL);
  tft.drawRoundRect(10, 10, 220, 44, 10, UI_SURFACE);
  tft.fillRoundRect(18, 19, 4, 26, 2, UI_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString(title, 32, 16);
  tft.setTextFont(1);
  tft.setTextColor(UI_MUTED, UI_PANEL);
  tft.drawString(detail, 32, 37);
}

static void footer(const char *top, const char *bottom) {
  tft.fillRoundRect(10, 277, 220, 34, 8, UI_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(1);
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString(top, 120, 287);
  tft.setTextColor(UI_CYAN, UI_PANEL);
  tft.drawString(bottom, 120, 301);
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

static void intro(const char *title, const char *line1, const char *line2,
                  const char *line3) {
  header(title, "NEW GAME");
  tft.fillRoundRect(19, 89, 202, 153, 12, UI_PANEL);
  tft.drawRoundRect(19, 89, 202, 153, 12, UI_SURFACE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(strlen(title) > 9 ? 2 : 4);
  tft.setTextColor(UI_CYAN, UI_PANEL);
  tft.drawString(title, 120, 117);
  tft.setTextFont(1);
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString(line1, 120, 160);
  tft.drawString(line2, 120, 179);
  tft.setTextColor(UI_MUTED, UI_PANEL);
  tft.drawString(line3, 120, 217);
  footer("SHOOT  START", "UP  MAIN MENU");
}

static void pause(const char *title) {
  tft.fillRoundRect(23, 84, 194, 157, 12, UI_PANEL);
  tft.drawRoundRect(23, 84, 194, 157, 12, UI_SURFACE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString(title, 120, 111);
  const char *choices[] = {"RESUME", "NEW GAME", "MAIN MENU"};
  for (int i = 0; i < 3; ++i) {
    int y = 141 + i * 31;
    tft.fillRoundRect(38, y, 164, 26, 6, UI_ICON_BG);
    tft.drawRoundRect(38, y, 164, 26, 6,
                      i == pauseChoice ? UI_CYAN : UI_SURFACE);
    if (i == pauseChoice)
      tft.drawRoundRect(39, y + 1, 162, 24, 5, UI_CYAN);
    tft.setTextFont(2);
    tft.setTextColor(i == pauseChoice ? UI_TEXT : UI_MUTED, UI_ICON_BG);
    tft.drawString(choices[i], 120, y + 13);
  }
}

static void result(const char *title, const char *detail) {
  tft.fillRoundRect(23, 88, 194, 150, 12, UI_PANEL);
  tft.drawRoundRect(23, 88, 194, 150, 12, UI_SURFACE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(UI_YELLOW, UI_PANEL);
  tft.drawString(title, 120, 119);
  tft.setTextFont(2);
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString(detail, 120, 167);
  tft.setTextFont(1);
  tft.setTextColor(UI_CYAN, UI_PANEL);
  tft.drawString("SHOOT  PLAY AGAIN", 120, 204);
  tft.setTextColor(UI_MUTED, UI_PANEL);
  tft.drawString("UP  MAIN MENU", 120, 221);
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
    pauseChoice = (pauseChoice + (cmd == 2 ? 1 : 2)) % 3;
    playNavClick();
    pause("PAUSED");
  } else if (cmd == 4) {
    playSelectTone();
    if (pauseChoice == 0) { phase = PLAY; draw(); }
    else if (pauseChoice == 1) start();
    else switchGame(0);
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
  mini::header("TIC-TAC-TOE", "YOU: X   /   CPU: O");
  for (int i = 0; i < 9; ++i) drawTile(i);
  mini::footer(cpuPending ? "CPU THINKING..." : "D-PAD  CHOOSE   SHOOT  PLACE",
               "HOLD SHOOT  PAUSE");
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
  mini::result(outcome == 1 ? "YOU WIN!" : outcome == 2 ? "CPU WINS" : "DRAW",
               outcome == 1 ? "Great move!" : outcome == 2 ? "Try another round" : "Well played");
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
  mini::intro("TIC-TAC-TOE", "Get three X marks in a row.",
              "The console plays O.", "D-PAD  MOVE   SHOOT  PLACE");
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
      mini::footer("D-PAD  CHOOSE   SHOOT  PLACE", "HOLD SHOOT  PAUSE");
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
      mini::footer("CPU THINKING...", "HOLD SHOOT  PAUSE");
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

static void serve() {
  ballX = 120; ballY = 170;
  vx = random(2) ? 1.5f : -1.5f;
  vy = -2.5f;
  paddleX = 94;
  lastFrame = millis();
}

static void draw() {
  mini::header("PONG", "KEEP THE BALL ALIVE");
  tft.fillRoundRect(13, 61, 214, 205, 8, UI_ICON_BG);
  tft.drawRoundRect(13, 61, 214, 205, 8, UI_SURFACE);
  tft.drawFastHLine(25, 107, 190, UI_SURFACE);
  tft.fillRoundRect(paddleX, 249, 52, 7, 3, UI_CYAN);
  tft.fillCircle((int)ballX, (int)ballY, 5, UI_YELLOW);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(UI_TEXT, UI_ICON_BG);
  tft.drawString(String(score), 25, 74);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(UI_RED, UI_ICON_BG);
  tft.drawString(String(lives) + " LIVES", 215, 74);
  mini::footer("LEFT / RIGHT  MOVE", "SHOOT  PAUSE");
  drawnX = (int)ballX; drawnY = (int)ballY;
  drawnPaddle = paddleX; drawnScore = score; drawnLives = lives;
}

static void drawMotion() {
  int nextX = (int)ballX, nextY = (int)ballY;
  if (nextX != drawnX || nextY != drawnY) {
    tft.fillCircle(drawnX, drawnY, 5, UI_ICON_BG);
    if (drawnY <= 112) tft.drawFastHLine(25, 107, 190, UI_SURFACE);
    tft.fillCircle(nextX, nextY, 5, UI_YELLOW);
  }
  if (paddleX != drawnPaddle) {
    tft.fillRoundRect(drawnPaddle, 249, 52, 7, 3, UI_ICON_BG);
    tft.fillRoundRect(paddleX, 249, 52, 7, 3, UI_CYAN);
  }
  if (score != drawnScore) {
    tft.fillRect(23, 72, 80, 29, UI_ICON_BG);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(2);
    tft.setTextColor(UI_TEXT, UI_ICON_BG);
    tft.drawString(String(score), 25, 74);
  }
  if (lives != drawnLives) {
    tft.fillRect(130, 72, 87, 29, UI_ICON_BG);
    tft.setTextDatum(TR_DATUM);
    tft.setTextFont(2);
    tft.setTextColor(UI_RED, UI_ICON_BG);
    tft.drawString(String(lives) + " LIVES", 215, 74);
  }
  drawnX = nextX; drawnY = nextY;
  drawnPaddle = paddleX; drawnScore = score; drawnLives = lives;
}

static void movePaddle(int direction) {
  if (!direction) { lastPaddleDir = 0; return; }
  unsigned long now = millis();
  if (direction == lastPaddleDir && now - lastPaddleStep < 25) return;
  lastPaddleDir = direction;
  lastPaddleStep = now;
  int next = constrain(paddleX + direction * 12, 19, 169);
  if (next == paddleX) return;
  tft.fillRoundRect(drawnPaddle, 249, 52, 7, 3, UI_ICON_BG);
  paddleX = next;
  tft.fillRoundRect(paddleX, 249, 52, 7, 3, UI_CYAN);
  drawnPaddle = paddleX;
}

static void start() {
  lives = 3; score = 0; recorded = false;
  lastPaddleDir = 0;
  lastPaddleStep = 0;
  shootWasDown = false;
  serve();
  mini::phase = mini::PLAY;
  draw();
}

static void end() {
  mini::phase = mini::FINISH;
  if (!recorded) { currentGame.recordScore(score); recorded = true; }
  mini::result("GAME OVER", (String(score) + " RETURNS").c_str());
}
}

void pongSetup() {
  tft.setRotation(2);
  mini::phase = mini::INTRO;
  mini::resetInput();
  mini::intro("PONG", "Move the paddle left and right.",
              "Keep the ball above the floor.", "THREE LIVES   /   BEAT YOUR BEST");
}

void pongLoop() {
  // During play, use cached held controls without menu key-repeat.
  int cmd = mini::phase == mini::PLAY ? -1 : mini::input();
  if (mini::commonIntro(cmd, pong::start)) return;
  if (mini::commonPause(cmd, pong::draw, pong::start)) { pong::lastFrame = millis(); return; }
  if (mini::phase == mini::FINISH) {
    if (cmd == 4) pong::start();
    else if (cmd == 0) switchGame(0);
    return;
  }
  bool shootDown = digitalRead(SHOOT);
  if (shootDown) pong::shootWasDown = true;
  else if (pong::shootWasDown) {
    pong::shootWasDown = false;
    mini::resetInput();
    mini::enterPause();
    return;
  }
  if (Serial.available()) {
    char c = Serial.peek();
    if (c == 'q' || c == 'Q' || c == ' ' || c == 'e' || c == 'E') {
      Serial.read();
      mini::enterPause();
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
  if (pong::ballY <= 112) { pong::vy = abs(pong::vy); ++pong::score; playNavClick(); }
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
  mini::header("MINESWEEPER", "10 MINES  /  8 x 8 BOARD");
  for (int i = 0; i < COUNT; ++i) drawTile(i);
  mini::footer("SHOOT  REVEAL   HOLD  FLAG", "SHOOT + UP  PAUSE");
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
  mini::result(won ? "CLEARED!" : "BOOM!", won ? "All safe tiles found" : "You hit a mine");
}
}

void minesweeperSetup() {
  tft.setRotation(2);
  mini::phase = mini::INTRO;
  mini::resetInput();
  mini::intro("MINES", "Reveal every safe square.",
              "Numbers count nearby mines.", "FIRST REVEAL IS ALWAYS SAFE");
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
  if (digitalRead(SHOOT) && cmd == 0) { mini::enterPause(); return; }
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
