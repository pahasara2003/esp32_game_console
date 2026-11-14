#include "game.h"
#include <Arduino.h>

// =================================================================
// NUMBER SLIDE PUZZLE / 15-PUZZLE
// =================================================================

constexpr int PUZZLE_SIZE = 4;
constexpr int PUZZLE_COUNT = PUZZLE_SIZE * PUZZLE_SIZE;
constexpr int PUZZLE_TILE = 46;
constexpr int PUZZLE_GAP = 4;
constexpr int PUZZLE_X = 22;
constexpr int PUZZLE_Y = 62;

static uint8_t puzzleBoard[PUZZLE_COUNT];
static int puzzleBlank = PUZZLE_COUNT - 1;
static int puzzleMoves = 0;
static int puzzleLastMoved = -1;
static int puzzlePauseSelection = 0;
static bool puzzleCompleteRecorded = false;

static int puzzleRow(int index) { return index / PUZZLE_SIZE; }
static int puzzleCol(int index) { return index % PUZZLE_SIZE; }

static void drawPuzzleGoalPreview() {
  constexpr int tile = 18;
  constexpr int gap = 2;
  constexpr int startX = 81;
  constexpr int startY = 171;
  constexpr uint16_t rowColors[4] = {0xCFE0, 0xFFE0, 0xFC60, 0xF81F};

  tft.fillRoundRect(61, 150, 118, 103, 8, UI_ICON_BG);
  tft.drawRoundRect(61, 150, 118, 103, 8, UI_SURFACE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(1);
  tft.setTextColor(UI_CYAN, UI_ICON_BG);
  tft.drawString("FINAL GRID", 120, 161);

  for (int index = 0; index < PUZZLE_COUNT; ++index) {
    int x = startX + puzzleCol(index) * (tile + gap);
    int y = startY + puzzleRow(index) * (tile + gap);
    if (index == PUZZLE_COUNT - 1) {
      tft.fillRoundRect(x, y, tile, tile, 3, UI_BG);
      tft.drawRoundRect(x, y, tile, tile, 3, UI_SURFACE);
      continue;
    }
    uint16_t fill = rowColors[puzzleRow(index)];
    tft.fillRoundRect(x, y, tile, tile, 3, fill);
    tft.drawRoundRect(x, y, tile, tile, 3, UI_PANEL);
    tft.setTextColor(puzzleRow(index) == 3 ? TFT_WHITE : TFT_BLACK, fill);
    tft.drawString(String(index + 1), x + tile / 2, y + tile / 2 + 1);
  }
}

static void puzzleTilePosition(int index, int &x, int &y) {
  x = PUZZLE_X + puzzleCol(index) * (PUZZLE_TILE + PUZZLE_GAP);
  y = PUZZLE_Y + puzzleRow(index) * (PUZZLE_TILE + PUZZLE_GAP);
}

static void drawPuzzleTileAt(int x, int y, uint8_t value, bool highlighted) {
  if (value == 0) {
    tft.fillRoundRect(x, y, PUZZLE_TILE, PUZZLE_TILE, 7, UI_BG);
    tft.drawRoundRect(x, y, PUZZLE_TILE, PUZZLE_TILE, 7, UI_SURFACE);
    tft.drawRoundRect(x + 2, y + 2, PUZZLE_TILE - 4, PUZZLE_TILE - 4, 5, UI_ICON_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(1);
    tft.setTextColor(UI_MUTED, UI_BG);
    tft.drawString("EMPTY", x + PUZZLE_TILE / 2, y + PUZZLE_TILE / 2);
    return;
  }

  uint16_t tileColor;
  uint16_t numberColor;
  switch ((value - 1) / PUZZLE_SIZE) {
    case 0:
      tileColor = 0xCFE0;  // vivid yellow-green
      numberColor = TFT_BLACK;
      break;
    case 1:
      tileColor = 0xFFE0;  // bright yellow
      numberColor = TFT_BLACK;
      break;
    case 2:
      tileColor = 0xFC60;  // bright orange
      numberColor = TFT_BLACK;
      break;
    default:
      tileColor = 0xF81F;  // vivid magenta
      numberColor = TFT_WHITE;
      break;
  }

  tft.fillRoundRect(x, y, PUZZLE_TILE, PUZZLE_TILE, 7, tileColor);
  tft.drawRoundRect(x, y, PUZZLE_TILE, PUZZLE_TILE, 7, UI_PANEL);
  tft.fillRoundRect(x + 3, y + 3, PUZZLE_TILE - 6, 4, 2, 0xFFFF);

  char number[4];
  snprintf(number, sizeof(number), "%u", value);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(numberColor, tileColor);
  tft.drawString(number, x + PUZZLE_TILE / 2, y + PUZZLE_TILE / 2 + 2);

  if (highlighted) {
    tft.drawRoundRect(x, y, PUZZLE_TILE, PUZZLE_TILE, 7, UI_CYAN);
    tft.drawRoundRect(x + 1, y + 1, PUZZLE_TILE - 2, PUZZLE_TILE - 2, 6, UI_PURPLE);
  }
}

static void drawPuzzleTile(int index) {
  int x, y;
  puzzleTilePosition(index, x, y);
  drawPuzzleTileAt(x, y, puzzleBoard[index], index == puzzleLastMoved);
}

static void animatePuzzleSlide(int fromIndex, int toIndex, uint8_t value) {
  int fromX, fromY, toX, toY;
  puzzleTilePosition(fromIndex, fromX, fromY);
  puzzleTilePosition(toIndex, toX, toY);
  int dirtyX = min(fromX, toX);
  int dirtyY = min(fromY, toY);
  int dirtyW = abs(toX - fromX) + PUZZLE_TILE;
  int dirtyH = abs(toY - fromY) + PUZZLE_TILE;

  // The moving tile crosses only its own cell, the gap, and the empty cell.
  for (int step = 1; step < 5; ++step) {
    int x = fromX + (toX - fromX) * step / 5;
    int y = fromY + (toY - fromY) * step / 5;
    tft.fillRect(dirtyX, dirtyY, dirtyW, dirtyH, UI_BG);
    drawPuzzleTileAt(toX, toY, 0, false);
    drawPuzzleTileAt(x, y, value, false);
    delay(6);
  }
  // Remove the last in-between tile before drawing both cells at rest.
  tft.fillRect(dirtyX, dirtyY, dirtyW, dirtyH, UI_BG);
}

static void drawPuzzleHeader() {
  tft.fillRoundRect(8, 5, 224, 38, 10, UI_PANEL);
  tft.drawRoundRect(8, 5, 224, 38, 10, UI_SURFACE);
  tft.fillRoundRect(16, 12, 54, 24, 6, UI_SURFACE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(1);
  tft.setTextColor(UI_CYAN, UI_SURFACE);
  tft.drawString("PUZZLE", 43, 24);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(UI_MUTED, UI_PANEL);
  tft.drawString("MOVES", 86, 11);
  tft.setTextFont(2);
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString(String(puzzleMoves), 86, 21);
  tft.setTextFont(1);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(UI_MUTED, UI_PANEL);
  tft.drawString("15-TILE", 218, 11);
  tft.setTextColor(UI_GREEN, UI_PANEL);
  tft.drawString("4 x 4", 218, 23);
}

static void drawPuzzleBottomBar() {
  tft.fillRoundRect(8, 288, 224, 28, 7, UI_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(1);
  tft.setTextColor(UI_MUTED, UI_PANEL);
  tft.drawString("D-PAD  MOVE GAP", 120, 297);
  tft.setTextColor(UI_CYAN, UI_PANEL);
  tft.drawString("SHOOT  PAUSE", 120, 309);
}

static void drawPuzzleBoard() {
  tft.fillScreen(UI_BG);
  drawPuzzleHeader();
  for (int i = 0; i < PUZZLE_COUNT; i++) drawPuzzleTile(i);
  drawPuzzleBottomBar();
}

static bool slidePuzzleGap(int command, bool countMove) {
  int row = puzzleRow(puzzleBlank);
  int col = puzzleCol(puzzleBlank);
  int target = puzzleBlank;

  if (command == 0 && row > 0) target = puzzleBlank - PUZZLE_SIZE;
  else if (command == 1 && col < PUZZLE_SIZE - 1) target = puzzleBlank + 1;
  else if (command == 2 && row < PUZZLE_SIZE - 1) target = puzzleBlank + PUZZLE_SIZE;
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
    drawPuzzleTile(oldBlank);
    drawPuzzleTile(target);
    if (oldHighlight >= 0 && oldHighlight != oldBlank && oldHighlight != target)
      drawPuzzleTile(oldHighlight);
    drawPuzzleHeader();
  }
  return true;
}

static bool puzzleIsSolved() {
  for (int i = 0; i < PUZZLE_COUNT - 1; i++)
    if (puzzleBoard[i] != i + 1) return false;
  return puzzleBoard[PUZZLE_COUNT - 1] == 0;
}

static void shufflePuzzle() {
  for (int i = 0; i < PUZZLE_COUNT - 1; i++) puzzleBoard[i] = i + 1;
  puzzleBoard[PUZZLE_COUNT - 1] = 0;
  puzzleBlank = PUZZLE_COUNT - 1;

  int previous = -1;
  for (int i = 0; i < 220; i++) {
    int choices[4];
    int choiceCount = 0;
    for (int command = 0; command < 4; command++) {
      if (command == previous) continue;
      int row = puzzleRow(puzzleBlank);
      int col = puzzleCol(puzzleBlank);
      bool valid = (command == 0 && row > 0) ||
                   (command == 1 && col < PUZZLE_SIZE - 1) ||
                   (command == 2 && row < PUZZLE_SIZE - 1) ||
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

static void drawPuzzleStartScreen() {
  tft.fillScreen(UI_BG);
  tft.fillRoundRect(14, 34, 212, 254, 12, UI_PANEL);
  tft.drawRoundRect(14, 34, 212, 254, 12, UI_SURFACE);
  tft.fillRoundRect(66, 49, 108, 22, 6, UI_SURFACE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(1);
  tft.setTextColor(UI_CYAN, UI_SURFACE);
  tft.drawString("NUMBER PUZZLE", 120, 60);
  tft.setTextFont(4);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString("15 PUZZLE", 120, 94);
  tft.setTextFont(1);
  tft.setTextColor(UI_MUTED, UI_PANEL);
  tft.drawString("Slide every number into order", 120, 121);
  tft.drawString("by moving the empty space.", 120, 136);
  drawPuzzleGoalPreview();
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString("D-PAD  MOVE SPACE", 120, 264);
  tft.setTextColor(UI_GREEN, UI_PANEL);
  tft.drawString("PRESS SHOOT TO BEGIN", 120, 280);
}

static void drawPuzzlePause() {
  tft.fillRoundRect(25, 87, 190, 145, 12, UI_PANEL);
  tft.drawRoundRect(25, 87, 190, 145, 12, UI_SURFACE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString("PAUSED", 41, 103);
  const char* labels[] = {"RESUME", "MAIN MENU"};
  for (int i = 0; i < 2; i++) {
    int y = 153 + i * 34;
    bool selected = i == puzzlePauseSelection;
    tft.fillRoundRect(40, y, 160, 28, 6, UI_ICON_BG);
    if (selected) {
      tft.drawRoundRect(40, y, 160, 28, 6, UI_PURPLE);
      tft.drawRoundRect(41, y + 1, 158, 26, 5, UI_CYAN);
    }
    tft.setTextDatum(ML_DATUM);
    tft.setTextFont(2);
    tft.setTextColor(selected ? UI_TEXT : UI_MUTED, UI_ICON_BG);
    tft.drawString(labels[i], 52, y + 14);
  }
}

static void drawPuzzleComplete() {
  tft.fillRoundRect(25, 83, 190, 160, 12, UI_PANEL);
  tft.drawRoundRect(25, 83, 190, 160, 12, UI_SURFACE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(UI_GREEN, UI_PANEL);
  tft.drawString("SOLVED!", 42, 100);
  tft.setTextFont(2);
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString(String(puzzleMoves) + " MOVES", 42, 145);
  tft.setTextFont(1);
  tft.setTextColor(UI_CYAN, UI_PANEL);
  tft.drawString("SHOOT  PLAY AGAIN", 42, 185);
  tft.setTextColor(UI_MUTED, UI_PANEL);
  tft.drawString("UP  MAIN MENU", 42, 207);
}

static void startPuzzle() {
  gameState = 0;
  menu_drawn = false;
  puzzleCompleteRecorded = false;
  shufflePuzzle();
  drawPuzzleBoard();
  delay(180);
}

void puzzleSetup() {
  tft.setRotation(2);
  gameState = -1;
  puzzlePauseSelection = 0;
  drawPuzzleStartScreen();
}

void puzzleLoop() {
  int command = findCommand();
  int serialCommand = puzzleSerialCommand();
  if (serialCommand >= 0) command = serialCommand;

  if (gameState == -1) {
    if (command == 4) { playSelectTone(); startPuzzle(); }
    else if (command == 0) switchGame(0);
    return;
  }

  if (gameState == 0) {
    if (command >= 0 && command <= 3) {
      if (slidePuzzleGap(command, true)) {
        playNavClick();
        if (puzzleIsSolved()) {
          gameState = 2;
          menu_drawn = false;
          playSelectTone();
        }
      }
    } else if (command == 4) {
      puzzlePauseSelection = 0;
      gameState = 1;
      menu_drawn = false;
    }
  } else if (gameState == 1) {
    if (!menu_drawn) { drawPuzzlePause(); menu_drawn = true; }
    if (command == 0 || command == 2) {
      puzzlePauseSelection = 1 - puzzlePauseSelection;
      drawPuzzlePause();
      playNavClick();
    } else if (command == 4) {
      playSelectTone();
      if (puzzlePauseSelection == 0) {
        gameState = 0;
        menu_drawn = false;
        drawPuzzleBoard();
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
