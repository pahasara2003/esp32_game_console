#include "game.h"
#include "ui_kit.h"
#include "arcade_style.h"
#include <Arduino.h>

// =================================================================
// NUMBER SLIDE PUZZLE / 15-, 24-, OR 35-PUZZLE
// =================================================================

constexpr int PUZZLE_MAX_SIZE = 6;
constexpr int PUZZLE_Y = 62;

static int puzzleSelectedSize = 4;
static int puzzleSize = 4;
static int puzzleCount = 16;
static int puzzleTile = 46;
static int puzzleGap = 4;
static int puzzleX = 22;
static uint8_t puzzleBoard[PUZZLE_MAX_SIZE * PUZZLE_MAX_SIZE];
static int puzzleBlank = 15;
static int puzzleMoves = 0;
static int puzzleLastMoved = -1;
static int puzzlePauseSelection = 0;
static const char *const PUZZLE_PAUSE_ITEMS[] = {"Resume", "High scores", "Main menu"};
static bool puzzleCompleteRecorded = false;

static int puzzleRow(int index) { return index / puzzleSize; }
static int puzzleCol(int index) { return index % puzzleSize; }

static int puzzleColorBand(int value, int size) {
  return (value - 1) / size;
}

static uint16_t puzzleBandColor(int band) {
  constexpr uint16_t colors[6] = {0xF98B, 0xFD24, 0xFE64, 0x268C, 0x263F, 0xA33F};
  return colors[band];
}

static void setPuzzleGeometry() {
  puzzleSize = puzzleSelectedSize;
  puzzleCount = puzzleSize * puzzleSize;
  puzzleTile = puzzleSize == 4 ? 46 : (puzzleSize == 5 ? 36 : 30);
  puzzleGap = puzzleSize == 4 ? 4 : 3;
  puzzleX = (240 - puzzleSize * puzzleTile - (puzzleSize - 1) * puzzleGap) / 2;
}

static void drawPuzzleGoalPreview() {
  int size = puzzleSelectedSize;
  int tile = size == 4 ? 20 : (size == 5 ? 16 : 13);
  constexpr int gap = 1;
  int gridWidth = size * tile + (size - 1) * gap;
  int startX = (240 - gridWidth) / 2;
  constexpr int startY = 183;

  tft.fillRoundRect(63, 162, 114, 111, 10, ui::GROUP);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::GROUP);
  tft.drawString("Goal", 120, 172, 2);
  tft.setTextFont(1);

  for (int index = 0; index < size * size; ++index) {
    int row = index / size;
    int col = index % size;
    int x = startX + col * (tile + gap);
    int y = startY + row * (tile + gap);
    if (index == size * size - 1) {
      tft.drawRoundRect(x, y, tile, tile, 3, ui::SEPARATOR);
      continue;
    }
    int band = puzzleColorBand(index + 1, size);
    uint16_t fill = puzzleBandColor(band);
    tft.fillRoundRect(x, y, tile, tile, 3, fill);
    tft.setTextColor(ui::inkOn(fill), fill);
    tft.drawString(String(index + 1), x + tile / 2, y + tile / 2 + 1);
  }
}

static void puzzleTilePosition(int index, int &x, int &y) {
  x = puzzleX + puzzleCol(index) * (puzzleTile + puzzleGap);
  y = PUZZLE_Y + puzzleRow(index) * (puzzleTile + puzzleGap);
}

static void restorePuzzleBackdrop(int x, int y, int w, int h) {
  int x0 = max(0, x), x1 = min(240, x + w);
  int y0 = max(48, y), y1 = min(291, y + h);
  if (x0 >= x1 || y0 >= y1) return;
  uint16_t row[240];
  bool swap = tft.getSwapBytes();
  tft.setSwapBytes(true);
  for (int yy = y0; yy < y1; ++yy) {
    for (int xx = x0; xx < x1; ++xx) row[xx - x0] = arcadeStyle::backdrop(xx, yy);
    tft.pushImage(x0, yy, x1 - x0, 1, row);
  }
  tft.setSwapBytes(swap);
}

template<class Surface>
static void drawPuzzleTileAt(Surface &canvas, int x, int y, uint8_t value, bool highlighted) {
  if (value == 0) {
    canvas.fillRoundRect(x, y, puzzleTile, puzzleTile, 7, 0x10C9);
    canvas.drawRoundRect(x, y, puzzleTile, puzzleTile, 7, 0x3A52);
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextFont(1);
    canvas.setTextColor(UI_MUTED, 0x10C9);
    canvas.drawString(puzzleSize == 6 ? "GAP" : "EMPTY", x + puzzleTile / 2, y + puzzleTile / 2);
    return;
  }

  uint16_t tileColor = puzzleBandColor(puzzleColorBand(value, puzzleSize));
  canvas.fillRoundRect(x, y, puzzleTile, puzzleTile, 7, tileColor);
  canvas.drawRoundRect(x, y, puzzleTile, puzzleTile, 7, arcadeStyle::fade(tileColor, 135));
  canvas.drawFastHLine(x + 5, y + 2, puzzleTile - 10, arcadeStyle::mix(tileColor, 0xFFFF, 125));
  canvas.drawFastHLine(x + 7, y + 4, min(10, puzzleTile - 14), TFT_WHITE);
  canvas.drawFastHLine(x + 6, y + puzzleTile - 3, puzzleTile - 12, arcadeStyle::fade(tileColor, 135));

  char number[4];
  snprintf(number, sizeof(number), "%u", value);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextFont(puzzleSize == 6 ? 2 : 4);
  canvas.setTextColor(ui::inkOn(tileColor), tileColor);
  canvas.drawString(number, x + puzzleTile / 2, y + puzzleTile / 2 + 2);

  if (highlighted) {
    canvas.drawRoundRect(x, y, puzzleTile, puzzleTile, 7, 0x36FF);
    canvas.drawPixel(x + puzzleTile - 7, y + 5, TFT_WHITE);
    canvas.drawPixel(x + puzzleTile - 5, y + 7, TFT_WHITE);
  }
}

static void drawPuzzleTile(int index) {
  int x, y;
  puzzleTilePosition(index, x, y);
  TFT_eSprite tile(&tft);
  tile.setColorDepth(16);
  if (tile.createSprite(puzzleTile, puzzleTile)) {
    tile.setOrigin(-x, -y);
    for (int yy = y; yy < y + puzzleTile; ++yy)
      for (int xx = x; xx < x + puzzleTile; ++xx)
        tile.drawPixel(xx, yy, arcadeStyle::backdrop(xx, yy));
    drawPuzzleTileAt(tile, x, y, puzzleBoard[index], index == puzzleLastMoved);
    tile.pushSprite(x, y);
    tile.deleteSprite();
  } else {
    drawPuzzleTileAt(tft, x, y, puzzleBoard[index], index == puzzleLastMoved);
  }
}

static void animatePuzzleSlide(int fromIndex, int toIndex, uint8_t value) {
  int fromX, fromY, toX, toY;
  puzzleTilePosition(fromIndex, fromX, fromY);
  puzzleTilePosition(toIndex, toX, toY);
  int dirtyX = min(fromX, toX), dirtyY = min(fromY, toY);
  int dirtyW = abs(toX - fromX) + puzzleTile;
  int dirtyH = abs(toY - fromY) + puzzleTile;
  TFT_eSprite frame(&tft);
  frame.setColorDepth(16);
  bool buffered = frame.createSprite(dirtyW, dirtyH);
  for (int step = 1; step <= 5; ++step) {
    int x = fromX + (toX - fromX) * step / 5;
    int y = fromY + (toY - fromY) * step / 5;
    if (buffered) {
      frame.setOrigin(-dirtyX, -dirtyY);
      for (int yy = dirtyY; yy < dirtyY + dirtyH; ++yy)
        for (int xx = dirtyX; xx < dirtyX + dirtyW; ++xx)
          frame.drawPixel(xx, yy, arcadeStyle::backdrop(xx, yy));
      drawPuzzleTileAt(frame, fromX, fromY, 0, false);
      drawPuzzleTileAt(frame, toX, toY, 0, false);
      drawPuzzleTileAt(frame, x, y, value, step >= 3);
      frame.pushSprite(dirtyX, dirtyY);
    } else {
      restorePuzzleBackdrop(dirtyX, dirtyY, dirtyW, dirtyH);
      drawPuzzleTileAt(tft, fromX, fromY, 0, false);
      drawPuzzleTileAt(tft, toX, toY, 0, false);
      drawPuzzleTileAt(tft, x, y, value, step >= 3);
    }
    delay(6);
  }
  if (buffered) frame.deleteSprite();
}

static void drawPuzzleMoves() {
  TFT_eSprite count(&tft);
  count.setColorDepth(16);
  if (count.createSprite(62, 16)) {
    count.fillSprite(ui::CARD);
    count.setTextDatum(TR_DATUM);
    count.setTextColor(ui::LABEL, ui::CARD);
    count.drawString(String(puzzleMoves), 62, 0, 2);
    count.pushSprite(158, 24);
    count.deleteSprite();
    return;
  }
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.setTextPadding(40);
  tft.drawString(String(puzzleMoves), 220, 24, 2);
  tft.setTextPadding(0);
}

static void drawPuzzleHeader() {
  tft.fillRoundRect(8, 5, 224, 38, 12, ui::CARD);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString("Puzzle", 20, 25, 4);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString("Grid", 158, 8, 2);
  tft.drawString("Moves", 220, 8, 2);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.setTextPadding(40);
  tft.drawString(String(puzzleSize) + "x" + String(puzzleSize), 158, 24, 2);
  tft.setTextPadding(0);
  drawPuzzleMoves();
}

static void drawPuzzleBottomBar() {
  tft.fillRect(0, 288, 240, 32, UI_BG);
  ui::hint(304, "Shoot to pause", UI_BG);
}

static void drawPuzzleBoard() {
  if (!restoringCountdownDigit()) tft.fillScreen(UI_BG);
  restorePuzzleBackdrop(0, 48, 240, 243);
  drawPuzzleHeader();
  for (int i = 0; i < puzzleCount; i++) {
    int x, y;
    puzzleTilePosition(i, x, y);
    drawPuzzleTileAt(tft, x, y, puzzleBoard[i], i == puzzleLastMoved);
  }
  drawPuzzleBottomBar();
}

static bool slidePuzzleGap(int command, bool countMove) {
  int row = puzzleRow(puzzleBlank);
  int col = puzzleCol(puzzleBlank);
  int target = puzzleBlank;

  if (command == 0 && row > 0) target = puzzleBlank - puzzleSize;
  else if (command == 1 && col < puzzleSize - 1) target = puzzleBlank + 1;
  else if (command == 2 && row < puzzleSize - 1) target = puzzleBlank + puzzleSize;
  else if (command == 3 && col > 0) target = puzzleBlank - 1;
  else return false;

  int oldBlank = puzzleBlank;
  uint8_t moved = puzzleBoard[target];
  int oldHighlight = puzzleLastMoved;
  if (countMove) animatePuzzleSlide(target, oldBlank, moved);
  puzzleBoard[puzzleBlank] = moved;
  puzzleBoard[target] = 0;
  puzzleBlank = target;
  puzzleLastMoved = oldBlank;
  if (countMove) puzzleMoves++;

  if (countMove) {
    if (oldHighlight >= 0 && oldHighlight != oldBlank && oldHighlight != target)
      drawPuzzleTile(oldHighlight);
    drawPuzzleMoves();
  }
  return true;
}

static bool puzzleIsSolved() {
  for (int i = 0; i < puzzleCount - 1; i++)
    if (puzzleBoard[i] != i + 1) return false;
  return puzzleBoard[puzzleCount - 1] == 0;
}

static void shufflePuzzle() {
  for (int i = 0; i < puzzleCount - 1; i++) puzzleBoard[i] = i + 1;
  puzzleBoard[puzzleCount - 1] = 0;
  puzzleBlank = puzzleCount - 1;

  int previous = -1;
  for (int i = 0; i < 220 + (puzzleSize - 4) * 90; i++) {
    int choices[4];
    int choiceCount = 0;
    for (int command = 0; command < 4; command++) {
      if (command == previous) continue;
      int row = puzzleRow(puzzleBlank);
      int col = puzzleCol(puzzleBlank);
      bool valid = (command == 0 && row > 0) ||
                   (command == 1 && col < puzzleSize - 1) ||
                   (command == 2 && row < puzzleSize - 1) ||
                   (command == 3 && col > 0);
      if (valid) choices[choiceCount++] = command;
    }
    int command = choices[random(choiceCount)];
    int opposite = (command == 0) ? 2 : (command == 2 ? 0 : (command == 1 ? 3 : 1));
    slidePuzzleGap(command, false);
    previous = opposite;
  }
  if (puzzleIsSolved()) slidePuzzleGap(0, false);
  puzzleMoves = 0;
  puzzleLastMoved = -1;
}

static int puzzleSerialCommand() {
  if (!Serial.available()) return -1;
  char c = Serial.read();
  if (c == 'w' || c == 'W') return 0;
  if (c == 'd' || c == 'D') return 1;
  if (c == 's' || c == 'S') return 2;
  if (c == 'a' || c == 'A') return 3;
  if (c == ' ' || c == '\n' || c == '\r' || c == 'e' || c == 'E' || c == 'x' || c == 'X') return 4;
  return -1;
}

// iOS-style segmented control for the grid size.
static void drawPuzzleStartChoices() {
  tft.fillRect(28, 92, 184, 64, ui::CARD);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString("Grid size", 30, 96, 2);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString(String(puzzleSelectedSize * puzzleSelectedSize - 1) + " tiles", 210, 96, 2);
  tft.fillRoundRect(30, 118, 180, 32, 10, ui::GROUP);
  for (int i = 0; i < 3; ++i) {
    int size = i + 4, x = 32 + i * 59;
    bool selected = size == puzzleSelectedSize;
    if (selected) tft.fillRoundRect(x, 120, 58, 28, 8, UI_CYAN);
    else if (i > 0 && size - 1 != puzzleSelectedSize) tft.drawFastVLine(x - 1, 126, 16, ui::SEPARATOR);
    tft.setTextDatum(MC_DATUM);
    uint16_t fill = selected ? UI_CYAN : ui::GROUP;
    tft.setTextColor(selected ? ui::inkOn(UI_CYAN) : ui::LABEL, fill);
    tft.drawString(String(size) + " x " + String(size), x + 29, 135, 2);
  }
  drawPuzzleGoalPreview();
}

static void drawPuzzleStartScreen() {
  tft.fillScreen(UI_BG);
  restorePuzzleBackdrop(0, 48, 240, 243);
  ui::card(14, 24, 212, 284);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString("Puzzle", 30, 38, 4);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString("Slide the tiles into order", 30, 66, 2);
  drawPuzzleStartChoices();
  ui::hint(286, "Shoot to start", ui::CARD);
}

static void drawPuzzlePause() {
  ui::pauseSheet("Puzzle", PUZZLE_PAUSE_ITEMS, 3, puzzlePauseSelection, UI_CYAN);
}

static void drawPuzzleComplete() {
  ui::card(24, 66, 192, 186);
  ui::heading(40, 80, "Puzzle", "Solved");
  const ui::List stats = {32, 130, 176, 24, 2};
  ui::infoRow(stats, 0, "Moves", String(puzzleMoves).c_str());
  String grid = String(puzzleSize) + "x" + String(puzzleSize);
  ui::infoRow(stats, 1, "Grid", grid.c_str());
  ui::pill(40, 188, 160, 30, "Play again", UI_CYAN);
  ui::hint(234, "Up for home screen", ui::CARD);
}

static void startPuzzle() {
  audio::restartMusic();
  gameState = 0;
  menu_drawn = false;
  puzzleCompleteRecorded = false;
  setPuzzleGeometry();
  shufflePuzzle();
  drawPuzzleBoard();
  delay(180);
}

void puzzleSetup() {
  tft.setRotation(2);
  gameState = -1;
  puzzleSelectedSize = 4;
  setPuzzleGeometry();
  puzzlePauseSelection = 0;
  drawPuzzleStartScreen();
}

void puzzleLoop() {
  audio::gameState(gameState);
  int command = findCommand();
  int serialCommand = puzzleSerialCommand();
  if (serialCommand >= 0) command = serialCommand;

  if (gameState == -1) {
    if (command == 4) { playSelectTone(); startPuzzle(); }
    else if (command == 0) switchGame(0);
    else if (command == 1 && puzzleSelectedSize < 6) {
      puzzleSelectedSize++;
      playNavClick();
      drawPuzzleStartChoices();
    } else if (command == 3 && puzzleSelectedSize > 4) {
      puzzleSelectedSize--;
      playNavClick();
      drawPuzzleStartChoices();
    }
    return;
  }

  if (gameState == 0) {
    if (command >= 0 && command <= 3) {
      if (slidePuzzleGap(command, true)) {
        audio::play(audio::Cue::Slide);
        if (puzzleIsSolved()) {
          gameState = 2;
          menu_drawn = false;
          audio::play(audio::Cue::Win);
        }
      } else audio::play(audio::Cue::Error);
    } else if (command == 4) {
      puzzlePauseSelection = 0;
      audio::pauseMusic(true); gameState = 1;
      menu_drawn = false;
    }
  } else if (gameState == 1) {
    if (!menu_drawn) { drawPuzzlePause(); menu_drawn = true; }
    if (command == 0 || command == 2) {
      puzzlePauseSelection = (puzzlePauseSelection + (command == 0 ? 2 : 1)) % 3;
      drawPuzzlePause();
      playNavClick();
    } else if (command == 4) {
      playSelectTone();
      if (puzzlePauseSelection == 0) {
        showResumeCountdown(drawPuzzleBoard);
        gameState = 0;
        menu_drawn = false;
      } else if (puzzlePauseSelection == 1) {
        showGameHighScores();
        drawPuzzleBoard();
        drawPuzzlePause();
      } else switchGame(0);
    }
  } else if (gameState == 2) {
    if (!puzzleCompleteRecorded) {
      currentGame.recordScore(max(1, 10000 - puzzleMoves));
      puzzleCompleteRecorded = true;
      drawPuzzleComplete();
    }
    if (command == 4) { playSelectTone(); startPuzzle(); }
    else if (command == 0) switchGame(0);
  }
}
