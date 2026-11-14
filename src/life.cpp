#include "ui_kit.h"
#include <Arduino.h>
#include <new>
#include "game.h"
#include "life_model.h"
#include "life_guide.h"

namespace lifeApp {
enum Mode { RUNNING, PAUSED, OPTIONS, INFO };
static Mode mode = RUNNING, optionsReturn = RUNNING;
// ~32 KB of cell buffers, allocated in lifeSetup and freed by lifeRelease.
struct State {
  lifeModel::Board b;
  uint8_t d[lifeModel::CAPACITY];
};
static State *state = nullptr;
#define board (state->b)
#define drawn (state->d)
static constexpr int CELL_SIZE = 2;
static bool boardDrawn = false;
static constexpr int FIELD_W = 224, FIELD_H = 192;
static constexpr int LEFT = 8;
static constexpr int TOP = 64;
static constexpr uint16_t FIELD = 0x0803;
static constexpr uint16_t DEAD = 0x2109;
static constexpr uint16_t LIVE = 0xBFE0;
static constexpr int PERIODS[] = {1000, 500, 250, 125, 83, 50, 33};
static constexpr int RATES[] = {1, 2, 4, 8, 12, 20, 30};
static constexpr int SPEED_COUNT = sizeof(RATES) / sizeof(RATES[0]);
static int speed = 5;
static constexpr int OPTION_COUNT = 5;
static int option = 0;
static int guidePage = 0;
static lifeGuide::Diagram guideDiagram;
static uint8_t guideDrawn[lifeGuide::COUNT];
static unsigned long lastGuideFrame = 0;
static unsigned long lastStep = 0, lastStats = 0, pressedAt = 0;
static bool shootHeld = false, holdSent = false;
static bool stable = false;
static unsigned long rateWindowAt = 0, rateWindowGeneration = 0, lastFrameMs = 0;
static float measuredRate = 0;

static_assert(LEFT * 2 + FIELD_W == 240, "Life board must be centered");
static_assert(TOP + FIELD_H < 264, "Life board must clear footer");

static void resetInput() { shootHeld = holdSent = false; }

static void resetRate() {
  rateWindowAt = millis();
  rateWindowGeneration = board.generation;
  measuredRate = 0;
}

static int input() {
  // Menus and speed controls advance once per physical press. Requiring a
  // release prevents a normal button press from becoming a second step.
  int cmd = lifeInputPoll(false);
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'w' || c == 'W') return 0;
    if (c == 'd' || c == 'D') return 1;
    if (c == 's' || c == 'S') return 2;
    if (c == 'a' || c == 'A') return 3;
    if (c == ' ' || c == 'e' || c == 'E') return 4;
    if (c == 'q' || c == 'Q') return 5;
    if (c == 'p' || c == 'P') {
      Serial.printf("[Life] mode=%d %dx%d gen=%lu alive=%u target=%d/s measured=%.1f/s frame=%lums\n",
                    (int)mode, board.width, board.height, (unsigned long)board.generation, board.population,
                    RATES[speed], measuredRate, lastFrameMs);
      return -1;
    }
  }
  if (cmd == 4 && !shootHeld) {
    shootHeld = true;
    holdSent = false;
    pressedAt = millis();
  }
  if (buttonsHeld() & 16) {
    if (shootHeld && !holdSent && millis() - pressedAt >= 850) {
      holdSent = true;
      return 5;
    }
    return -1;
  }
  if (shootHeld) {
    bool tapped = !holdSent;
    resetInput();
    if (tapped) return 4;
  }
  return cmd == 4 ? -1 : cmd;
}

static void drawState() {
  tft.fillRect(140, 12, 86, 17, ui::CARD);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(mode == RUNNING ? LIVE : ui::SECONDARY, ui::CARD);
  tft.drawString(mode == RUNNING ? (board.population == 0 ? "Extinct" : stable ? "Stable" : "Running") :
                 mode == OPTIONS ? "Options" : "Paused", 220, 13, 2);
}

static void drawStats() {
  char text[24];
  tft.fillRect(16, 31, 208, 18, ui::CARD);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  snprintf(text, sizeof(text), "Generation %lu", (unsigned long)board.generation);
  tft.drawString(text, 18, 32, 2);
  snprintf(text, sizeof(text), "%u alive", board.population);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(LIVE, ui::CARD);
  tft.drawString(text, 220, 32, 2);
  lastStats = millis();
}

static void drawHeader() {
  tft.fillRoundRect(8, 8, 224, 46, 12, ui::CARD);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString("Conway's Life", 18, 13, 2);
  drawState();
  drawStats();
}

static void drawFooter() {
  tft.fillRoundRect(8, 264, 224, 48, 12, ui::CARD);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(LIVE, ui::CARD);
  tft.drawString(String(RATES[speed]) + " gen/s", 20, 270, 2);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString(mode == OPTIONS ? "Up/Down choose" : "Up/Down speed", 220, 270, 2);
  ui::hint(298, mode == OPTIONS ? "Hold Shoot to go back" :
                mode == RUNNING ? "Shoot pause, hold for menu" : "Shoot resume, hold for menu",
           ui::CARD, ui::LABEL);
}

static void drawBoard(bool force = false) {
  force |= !boardDrawn;
  tft.startWrite();
  if (force) tft.fillRect(LEFT, TOP, FIELD_W, FIELD_H, FIELD);
    for (int y = 0; y < board.height; ++y) {
      int x = 0;
      while (x < board.width) {
        int i = y * board.width + x;
        if (!force && drawn[i] == board.cells[i]) { ++x; continue; }
        uint8_t alive = board.cells[i];
        int start = x, end = x + 1, lastDirty = x + 1, gap = 0;
        // Merge one-color updates across at most one unchanged cell. Sending
        // that cell costs less than a second SPI window; trim unchanged tails.
        while (end < board.width && board.cells[y * board.width + end] == alive) {
          if (force || drawn[y * board.width + end] != alive) {
            lastDirty = end + 1;
            gap = 0;
          } else if (++gap > 1) break;
          ++end;
        }
        x = lastDirty;
        tft.fillRect(LEFT + start * CELL_SIZE, TOP + y * CELL_SIZE,
                      (x - start) * CELL_SIZE, CELL_SIZE, alive ? LIVE : DEAD);
        memset(drawn + y * board.width + start, alive, x - start);
      }
    }
  boardDrawn = true;
  tft.endWrite();
}

static void newRandom() {
  board.resize(FIELD_W / CELL_SIZE, FIELD_H / CELL_SIZE);
  for (int i = 0; i < board.count(); ++i)
    if (random(100) < 30) board.set(i % board.width, i / board.width, true);
  stable = false;
  lastStep = millis();
  resetRate();
}

static const ui::List LIFE_OPTIONS = {32, 126, 176, 24, OPTION_COUNT};
static void drawOption(int i) {
  const char *labels[] = {"Resume", "New random world", "", "Field guide", "Main menu"};
  String speedLabel = String("Speed: ") + String(RATES[speed]) + " gen/s";
  ui::listRow(LIFE_OPTIONS, i, i == 2 ? speedLabel.c_str() : labels[i], i == option, LIVE);
}

static void drawOptions() {
  ui::card(18, 73, 204, 184);
  ui::heading(32, 82, "Conway's Life", "Options");
  for (int i = 0; i < OPTION_COUNT; ++i) drawOption(i);
}

static void leaveOptions(Mode next) {
  mode = next;
  drawBoard(true); // Restore the cells covered by the one-time menu overlay.
  drawState();
  drawStats();
  drawFooter();
  lastStep = millis();
  resetRate();
}

static void drawGuideDiagram(bool force) {
  tft.startWrite();
  for (int y = 0; y < lifeGuide::HEIGHT; ++y) {
    for (int x = 0; x < lifeGuide::WIDTH; ++x) {
      int i = y * lifeGuide::WIDTH + x;
      if (!force && guideDrawn[i] == guideDiagram.cells[i]) continue;
      tft.fillRoundRect(70 + x * 5, 108 + y * 5, 4, 4, 1,
                        guideDiagram.cells[i] ? LIVE : DEAD);
      guideDrawn[i] = guideDiagram.cells[i];
    }
  }
  tft.endWrite();
}

static void drawGuidePage() {
  const lifeGuide::Page &page = lifeGuide::PAGES[guidePage];
  guideDiagram.reset(page);
  lastGuideFrame = millis();
  tft.fillScreen(UI_BG);
  tft.fillRoundRect(8, 8, 224, 46, 12, ui::CARD);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString("Field guide", 18, 13, 2);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString("Conway's Game of Life", 18, 32, 2);
  char counter[12];
  snprintf(counter, sizeof(counter), "%d of %d", guidePage + 1, lifeGuide::PAGE_COUNT);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(counter, 220, 13, 2);
  tft.fillRoundRect(8, 64, 224, 192, 12, ui::CARD);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString(page.title, 120, 82, 2);
  tft.setTextFont(1);
  tft.setTextColor(LIVE, ui::CARD);
  tft.drawString(page.category, 120, 99);
  if (page.rle) drawGuideDiagram(true);
  tft.setTextColor(ui::LABEL, ui::CARD);
  for (int i = 0; i < 7 && page.lines[i]; ++i)
    tft.drawString(page.lines[i], 120, page.rle ? 211 + i * 13 : 123 + i * 18);
  tft.fillRoundRect(8, 264, 224, 48, 12, ui::CARD);
  ui::hint(276, "Left / Right to turn pages", ui::CARD, ui::LABEL);
  ui::hint(293, "Shoot to go back", ui::CARD);
  tft.fillRoundRect(20, 303, 200, 3, 1, ui::GROUP);
  tft.fillRoundRect(20, 303, (guidePage + 1) * 200 / lifeGuide::PAGE_COUNT, 3, 1, LIVE);
}

static void closeGuide() {
  mode = OPTIONS;
  lifeInputRequireRelease();
  tft.fillScreen(UI_BG);
  drawHeader();
  drawBoard(true);
  drawOptions();
  drawFooter();
}

} // namespace lifeApp

void lifeRelease() {
  delete lifeApp::state;
  lifeApp::state = nullptr;
}

void lifeSetup() {
  if (!lifeApp::state) lifeApp::state = new (std::nothrow) lifeApp::State();
  if (!lifeApp::state) { switchGame(0); return; }
  tft.setRotation(2);
  lifeApp::mode = lifeApp::RUNNING;
  lifeApp::resetInput();
  // The menu launches on press: consume that press through its release.
  if (readShoot()) lifeApp::shootHeld = lifeApp::holdSent = true;
  lifeApp::speed = 5;
  lifeApp::boardDrawn = false;
  lifeInputBegin();
  lifeApp::newRandom();
  tft.fillScreen(UI_BG);
  lifeApp::drawHeader(); lifeApp::drawBoard(true); lifeApp::drawFooter();
  lifeApp::lastStep = millis();
  lifeApp::resetRate();
}

void lifeLoop() {
  using namespace lifeApp;
  audio::pauseMusic(mode != RUNNING);
  int cmd = input();
  if (mode == INFO) {
    if (cmd == 4 || cmd == 5) { closeGuide(); return; }
    if (cmd == 1 || cmd == 3) {
      int next = constrain(guidePage + (cmd == 1 ? 1 : -1),
                           0, lifeGuide::PAGE_COUNT - 1);
      if (next != guidePage) { guidePage = next; drawGuidePage(); playNavClick(); }
    }
    const lifeGuide::Page &page = lifeGuide::PAGES[guidePage];
    if (page.frames > 1 && millis() - lastGuideFrame >= 450) {
      lastGuideFrame = millis();
      guideDiagram.advance(page);
      drawGuideDiagram(false);
    }
    return;
  }
  if (mode == OPTIONS) {
    if (cmd == 0 || cmd == 2) {
      int previous = option;
      option = (option + (cmd == 2 ? 1 : OPTION_COUNT - 1)) % OPTION_COUNT;
      drawOption(previous); drawOption(option); playNavClick();
    } else if (cmd == 5) leaveOptions(optionsReturn);
    else if (cmd == 4) {
      playSelectTone();
      if (option == 4) { switchGame(0); return; }
      if (option == 0) leaveOptions(RUNNING);
      else if (option == 1) { newRandom(); audio::play(audio::Cue::Sparkle); leaveOptions(RUNNING); }
      else if (option == 2) { speed = (speed + 1) % SPEED_COUNT; drawOption(option); drawFooter(); }
      else if (option == 3) {
        mode = INFO; guidePage = 0;
        lifeInputRequireRelease();
        drawGuidePage();
      }
    }
    return;
  }
  if (cmd == 5) {
    audio::play(audio::Cue::Pause);
    optionsReturn = mode;
    mode = OPTIONS; option = 0;
    lifeInputRequireRelease();
    drawState(); drawOptions(); drawFooter();
    return;
  }
  if (cmd == 4) {
    mode = mode == RUNNING ? PAUSED : RUNNING;
    audio::pauseMusic(mode != RUNNING);
    audio::play(mode == RUNNING ? audio::Cue::Resume : audio::Cue::Pause);
    lastStep = millis();
    resetRate();
    drawState(); drawStats(); drawFooter();
    return;
  }
  if (cmd == 0 || cmd == 2) {
    speed = constrain(speed + (cmd == 0 ? 1 : -1), 0, SPEED_COUNT - 1);
    drawFooter();
    lastStep = millis();
    resetRate();
  }
  if (mode == RUNNING) {
    unsigned long now = millis();
    if (now - lastStep >= (unsigned long)PERIODS[speed]) {
      // The frame's work counts toward the period rather than adding to it.
      lastStep = now;
      bool wasStable = stable;
      uint16_t previousPopulation = board.population;
      stable = !board.step();
      if (!wasStable && stable) audio::play(audio::Cue::Draw);
      drawBoard();
      if (wasStable != stable || (previousPopulation == 0) != (board.population == 0)) drawState();
      if (millis() - lastStats >= 200) drawStats();
      lastFrameMs = millis() - now;
      unsigned long elapsed = millis() - rateWindowAt;
      if (elapsed >= 1000) {
        measuredRate = (board.generation - rateWindowGeneration) * 1000.0f / elapsed;
        rateWindowAt = millis();
        rateWindowGeneration = board.generation;
      }
    }
  }
}
