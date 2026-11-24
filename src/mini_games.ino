#include "game.h"
#include <Arduino.h>

// Portrait games share the same 240 x 320 content frame and input conventions.
#include "ui_kit.h"
#include "arcade_style.h"

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
    if (pauseChoice == 0) { showResumeCountdown(draw); resetInput(); phase = PLAY; audio::play(audio::Cue::Resume); }
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
  audio::play(audio::Cue::Pause);
  buttonsRequireRelease();
  pauseChoice = 0;
  audio::pauseMusic(true);
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

template<class Surface>
static void paintTile(Surface &canvas, int i, int x, int y) {
  constexpr int size = 60;
  const uint16_t colors[3] = {0xF98B, 0xFD24, 0xFE64};
  uint16_t color = colors[i / 3];
  canvas.fillRoundRect(x, y, size, size, 9, color);
  canvas.drawRoundRect(x, y, size, size, 9, arcadeStyle::fade(color, 135));
  canvas.drawFastHLine(x + 7, y + 2, size - 14, arcadeStyle::mix(color, 0xFFFF, 125));
  canvas.drawFastHLine(x + 9, y + 4, 10, TFT_WHITE);
  canvas.drawFastHLine(x + 7, y + size - 3, size - 14, arcadeStyle::fade(color, 135));
  uint16_t ink = ui::inkOn(color);
  if (cells[i] == 1) {
    for (int offset = -2; offset <= 2; ++offset) {
      canvas.drawLine(x + 17 + offset, y + 18, x + 43 + offset, y + 44, ink);
      canvas.drawLine(x + 43 + offset, y + 18, x + 17 + offset, y + 44, ink);
    }
  } else if (cells[i] == 2) {
    for (int radius = 14; radius <= 18; ++radius)
      canvas.drawCircle(x + 30, y + 31, radius, ink);
  }
  if (i == cursor) {
    canvas.drawRoundRect(x, y, size, size, 9, 0x36FF);
    canvas.drawRoundRect(x + 1, y + 1, size - 2, size - 2, 8, 0x36FF);
    canvas.drawPixel(x + size - 7, y + 5, TFT_WHITE);
    canvas.drawPixel(x + size - 5, y + 7, TFT_WHITE);
  }
}

static void drawTile(int i) {
  const int size = 60, gap = 3;
  int x = 27 + (i % 3) * (size + gap);
  int y = 75 + (i / 3) * (size + gap);
  TFT_eSprite tile(&tft);
  tile.setColorDepth(16);
  if (tile.createSprite(size, size)) {
    tile.setOrigin(-x, -y);
    arcadeStyle::drawBackdrop(tile, y, y + size - 1);
    tic::paintTile(tile, i, x, y);
    tile.pushSprite(x, y);
    tile.deleteSprite();
  } else {
    tic::paintTile(tft, i, x, y);
  }
}

static void draw() {
  mini::header("Tic-tac-toe", "You are X");
  uint16_t row[240];
  bool swap = tft.getSwapBytes();
  tft.setSwapBytes(true);
  for (int y = 58; y <= 276; ++y) {
    for (int x = 0; x < 240; ++x) row[x] = arcadeStyle::backdrop(x, y);
    tft.pushImage(0, y, 240, 1, row);
  }
  tft.setSwapBytes(swap);
  for (int i = 0; i < 9; ++i) drawTile(i);
  mini::footer(cpuPending ? "Console is thinking..." : "Shoot to place",
               "Hold Shoot to pause");
}

static void start() {
  audio::restartMusic();
  audio::stopEnding();
  memset(cells, 0, sizeof(cells));
  cursor = 4;
  recorded = false;
  cpuPending = false;
  mini::phase = mini::PLAY;
  draw();
}

static void finish() {
  audio::pauseMusic(true);
  outcome = winner();
  if (!outcome) return;
<<<<<<< HEAD
  audio::play(outcome == 1 ? audio::Cue::Win : outcome == 2 ? audio::Cue::Death : audio::Cue::Draw);
=======
  audio::play(outcome == 1 ? audio::Cue::Win : outcome == 2 ? audio::Cue::Lose : audio::Cue::Draw);
>>>>>>> codex/rally-3d
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
  audio::pauseMusic(mini::phase != mini::PLAY);
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
      if (reply >= 0) { tic::drawTile(reply); audio::play(audio::Cue::CpuMove); }
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
    audio::play(audio::Cue::PlayerMove);
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
static constexpr int FIELD_TOP = 48;
static constexpr int FIELD_BOTTOM = 290;
static float trailX[8], trailY[8];
static int trailCount = 0, impactLife = 0, impactX = 0, impactY = 0;
static uint16_t dirtyBands = 0;

static void dirty(int y, int h) {
  int first = max(y, FIELD_TOP), last = min(y + h - 1, FIELD_BOTTOM);
  if (first > last) return;
  for (int i = (first - FIELD_TOP) / 16; i <= (last - FIELD_TOP) / 16; ++i)
    dirtyBands |= uint16_t(1) << i;
}

static void dirtyObjects() {
  dirty((int)ballY - 6, 13);
  for (int i = 0; i < trailCount; ++i) dirty((int)trailY[i] - 3, 7);
  if (impactLife) dirty(impactY - 20, 41);
}

static void impact(int x, int y) {
  impactX = x; impactY = y; impactLife = 7;
}

template<class Surface>
static void compose(Surface &canvas, int top, int bottom) {
  arcadeStyle::drawBackdrop(canvas, top, bottom);
  canvas.drawFastHLine(16, 107, 208, 0x36FF);
  if (impactLife) {
    int radius = 3 + (7 - impactLife) * 2;
    canvas.drawCircle(impactX, impactY, radius, arcadeStyle::fade(0x36FF, impactLife * 36));
    canvas.fillCircle(impactX - radius, impactY, 1, arcadeStyle::fade(ACCENT, impactLife * 36));
    canvas.fillCircle(impactX + radius, impactY, 1, arcadeStyle::fade(ACCENT, impactLife * 36));
  }
  for (int i = trailCount - 1; i >= 0; --i) {
    uint16_t c = arcadeStyle::mix(ACCENT, 0x36FF, i * 255 / 7);
    canvas.fillCircle((int)trailX[i], (int)trailY[i], i < 3 ? 2 : 1,
                      arcadeStyle::fade(c, 165 - i * 18));
  }
  canvas.fillRoundRect(paddleX, 249, 52, 7, 3, 0xDEFB);
  canvas.fillRoundRect(paddleX + 1, 250, 50, 5, 2, TFT_WHITE);
  canvas.fillRoundRect(paddleX + 1, 250, 6, 5, 2, ACCENT);
  canvas.fillRoundRect(paddleX + 45, 250, 6, 5, 2, ACCENT);
  canvas.drawFastHLine(paddleX + 9, 254, 34, 0x9CF3);
  canvas.fillCircle((int)ballX, (int)ballY, 5, arcadeStyle::fade(ACCENT, 75));
  canvas.fillCircle((int)ballX, (int)ballY, 3, 0xFFE8);
  canvas.drawPixel((int)ballX - 1, (int)ballY - 1, TFT_WHITE);
}

static void render(bool full = false) {
  if (full) dirtyBands = 0xFFFF;
  if (!dirtyBands) return;
  TFT_eSprite band(&tft);
  band.setColorDepth(16);
  if (!band.createSprite(240, 16)) {
    int x = tft.getViewportX(), y = tft.getViewportY();
    int w = tft.getViewportWidth(), h = tft.getViewportHeight();
    int top = max(y, FIELD_TOP), bottom = min(y + h - 1, FIELD_BOTTOM);
    if (top <= bottom) {
      tft.setViewport(x, top, w, bottom - top + 1, false);
      pong::compose(tft, FIELD_TOP, FIELD_BOTTOM);
    }
    tft.setViewport(x, y, w, h, false);
  } else {
    for (int i = 0; i < 16; ++i) {
      if (!(dirtyBands & (uint16_t(1) << i))) continue;
      int y = FIELD_TOP + i * 16;
      int h = min(16, FIELD_BOTTOM - y + 1);
      band.setViewport(0, 0, 240, h);
      band.setOrigin(0, -y);
      pong::compose(band, y, y + h - 1);
      band.pushSprite(0, y, 0, 0, 240, h);
    }
    band.deleteSprite();
  }
  dirtyBands = 0;
}

static unsigned long scoreFlashAt = 0;
static bool drawnScoreFlash = false;
static void drawHeader() {
  TFT_eSprite header(&tft);
  header.setColorDepth(16);
  if (!header.createSprite(224, 38)) return;
  header.fillSprite(UI_BG);
  header.fillRoundRect(0, 0, 224, 38, 12, ui::CARD);
  header.setTextDatum(ML_DATUM);
  header.setTextColor(ui::LABEL);
  header.drawString("Pong", 12, 20, 2);
  header.setTextDatum(TR_DATUM);
  header.setTextColor(ACCENT);
  header.drawString("Score", 150, 3, 2);
  header.setTextColor(ui::SECONDARY);
  header.drawString("Lives", 212, 3, 2);
  header.setTextColor(scoreFlashAt && millis() - scoreFlashAt < 320 ? ACCENT_LIGHT : ui::LABEL);
  header.drawString(String(score), 150, 19, 2);
  for (int i = 0; i < 3; ++i)
    header.fillCircle(184 + i * 12, 27, 4, i < lives ? 0xF800 : 0x4208);
  header.pushSprite(8, 5);
  header.deleteSprite();
}

static void drawFooter() {
  tft.fillRect(0, 294, 240, 26, UI_BG);
  ui::hint(306, "Shoot to pause", UI_BG);
}

static void serve() {
  ballX = 120; ballY = 170;
  vx = random(2) ? 1.5f : -1.5f;
  vy = -2.5f;
  paddleX = 94;
  trailCount = 0; impactLife = 0;
  lastFrame = millis();
}

static void draw() {
  if (!restoringCountdownDigit()) tft.fillScreen(UI_BG);
  render(true);
  tft.drawFastHLine(0, FIELD_TOP - 1, 240, UI_SURFACE);
  tft.drawFastHLine(0, FIELD_BOTTOM + 1, 240, UI_SURFACE);
  drawHeader();
  drawFooter();
  drawnX = (int)ballX; drawnY = (int)ballY;
  drawnPaddle = paddleX; drawnScore = score; drawnLives = lives;
  drawnScoreFlash = scoreFlashAt && millis() - scoreFlashAt < 320;
}

static void drawMotion() {
  dirtyObjects();
  for (int i = min(trailCount, 7); i > 0; --i) {
    trailX[i] = trailX[i - 1]; trailY[i] = trailY[i - 1];
  }
  trailX[0] = drawnX; trailY[0] = drawnY;
  trailCount = min(8, trailCount + 1);
  if (impactLife) --impactLife;
  dirtyObjects();
  render();
  bool flash = scoreFlashAt && millis() - scoreFlashAt < 320;
  if (score != drawnScore || lives != drawnLives || flash != drawnScoreFlash) drawHeader();
  drawnScoreFlash = flash;
  drawnX = (int)ballX; drawnY = (int)ballY;
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
  dirty(245, 15);
  paddleX = next;
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
  audio::play(audio::Cue::Pause);
  buttonsRequireRelease();
  mini::resetInput();
  mini::pauseChoice = 0;
  audio::pauseMusic(true);
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
      audio::play(audio::Cue::Resume);
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
  audio::restartMusic();
  audio::stopEnding();
  lives = 3; score = 0; recorded = false;
  scoreFlashAt = 0; drawnScoreFlash = false;
  lastPaddleDir = 0;
  lastPaddleStep = 0;
  shootWasDown = false;
  serve();
  mini::phase = mini::PLAY;
  draw();
}

static void end() {
<<<<<<< HEAD
  audio::play(audio::Cue::Death);
=======
  audio::pauseMusic(true);
  audio::play(audio::Cue::GameOver);
>>>>>>> codex/rally-3d
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
  audio::pauseMusic(mini::phase != mini::PLAY);
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
  pong::dirtyObjects();
  pong::ballX += pong::vx * steps;
  pong::ballY += pong::vy * steps;
  if (pong::ballX <= 20 || pong::ballX >= 220) {
    pong::vx = -pong::vx;
    pong::ballX = constrain(pong::ballX, 20.0f, 220.0f);
<<<<<<< HEAD
=======
    pong::impact((int)pong::ballX, (int)pong::ballY);
>>>>>>> codex/rally-3d
    audio::play(audio::Cue::Wall);
  }
  if (pong::ballY <= 112) {
    pong::ballY = 112;
    pong::vy = abs(pong::vy);
    ++pong::score;
<<<<<<< HEAD
=======
    pong::scoreFlashAt = millis();
    pong::impact((int)pong::ballX, 107);
>>>>>>> codex/rally-3d
    audio::play(audio::Cue::Pickup);
  }
  if (pong::vy > 0 && pong::ballY >= 243 && pong::ballY <= 256 &&
      pong::ballX >= pong::paddleX - 4 && pong::ballX <= pong::paddleX + 56) {
    pong::ballY = 243;
    pong::vy = -min(5.0f, 2.5f + pong::score * 0.08f);
    pong::vx = constrain((pong::ballX - (pong::paddleX + 26)) / 10.0f, -3.5f, 3.5f);
    if (abs(pong::vx) < 0.6f) pong::vx = 0.6f;
<<<<<<< HEAD
=======
    pong::impact((int)pong::ballX, 249);
>>>>>>> codex/rally-3d
    audio::play(audio::Cue::Paddle);
  }
  if (pong::ballY > 264) {
    if (--pong::lives <= 0) { pong::end(); return; }
<<<<<<< HEAD
    audio::play(audio::Cue::Hit);
=======
    audio::play(audio::Cue::Lose);
>>>>>>> codex/rally-3d
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
  audio::restartMusic();
  audio::stopEnding();
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
<<<<<<< HEAD
  audio::play(won ? audio::Cue::Win : audio::Cue::Death);
=======
  audio::pauseMusic(true);
  audio::play(won ? audio::Cue::Win : audio::Cue::Lose);
>>>>>>> codex/rally-3d
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
  audio::pauseMusic(mini::phase != mini::PLAY);
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
    audio::play(flag ? audio::Cue::Flag : audio::Cue::Unflag);
    mines::drawTile(mines::cursor);
  } else if (cmd == 4 && !mines::flagged[mines::cursor] && !mines::revealed[mines::cursor]) {
    if (!mines::planted) mines::plant(mines::cursor);
    if (mines::bomb[mines::cursor]) {
      mines::revealed[mines::cursor] = true;
      mines::drawTile(mines::cursor);
      mines::finish(false);
    } else {
      mines::uncover(mines::cursor);
      audio::play(audio::Cue::Reveal);
      mines::drawDirty();
      if (mines::exposed == mines::COUNT - mines::BOMBS) mines::finish(true);
    }
  }
}
