#include <Arduino.h>
#include <Preferences.h>
#include "game.h"
#include "tetris_input.h"
#include "buttons.h"
#include <Adafruit_ADS1X15.h>
#include <string.h>
#include "images.h"


const char* gameNames[] = { "Menu", "Tetris", "Snake", "Breakout", "Run", "Galaga", "Puzzle",
                            "TicTacToe", "Pong", "Minesweeper", "Life", "Mandelbrot" };
const int GAME_COUNT = sizeof(gameNames) / sizeof(gameNames[0]);
Preferences prefs;

uint16_t uiBg = 0x0000;
uint16_t uiPanel = 0x1082;
uint16_t uiSurface = 0x2104;
uint16_t uiCyan = 0x2EDC;
uint16_t uiGreen = 0xA7E9;
uint16_t uiPurple = 0xB37F;
uint16_t uiYellow = 0xFE4A;
uint16_t uiRed = 0xFA8D;
uint16_t uiText = 0xEF7D;
uint16_t uiMuted = 0x8C51;
uint16_t uiIconBg = 0x18C3;
uint16_t uiHeaderStart = 0xB7FF;
uint16_t uiHeaderEnd = 0xD65F;
bool lightThemeEnabled = false;
uint8_t activeTheme = 0;
bool soundEnabled = true;

const char* themeName(uint8_t index) {
  static const char* names[] = {"AURORA", "OCEAN", "ORCHID", "ROSE",
                               "CITRUS", "SUNSET", "MINT", "SILVER"};
  return names[index % THEME_COUNT];
}

void applyThemePreset(uint8_t index, bool persist) {
  activeTheme = index % THEME_COUNT;
  lightThemeEnabled = false;
  // Dark neutral utility surfaces; themes color the launcher title and focus.
  // Pastel opaque headers retain contrast with the fixed dark title ink.
  static const uint16_t palettes[THEME_COUNT][4] = {
    {0x7DFF, 0xB39F, 0xB7FF, 0xD65F}, // Aurora: ice blue to lavender.
    {0x4DDF, 0x3CFF, 0xAFFF, 0x9E7F}, // Ocean: aqua to sky.
    {0xCB5F, 0x93BF, 0xF67F, 0xCE7F}, // Orchid: pink to lilac.
    {0xFB75, 0xD3BE, 0xFE39, 0xF65C}, // Rose: peach to blush.
    {0xCFE8, 0x6EF3, 0xF7F1, 0xCFF7}, // Citrus: lemon to lime.
    {0xFD8B, 0xF395, 0xFF32, 0xFE19}, // Sunset: apricot to coral.
    {0x67D4, 0x4DDF, 0xBFFA, 0xA79F}, // Mint: seafoam to aqua.
    {0xC638, 0x94B5, 0xF7DF, 0xCE79}  // Silver: pearl to cool gray.
  };
  uiBg = 0x0000; uiPanel = 0x1082; uiSurface = 0x2945; uiIconBg = 0x18C3;
  uiText = 0xF7DE; uiMuted = 0xAD55;
  uiCyan = palettes[activeTheme][0];
  uiPurple = palettes[activeTheme][1];
  uiHeaderStart = palettes[activeTheme][2];
  uiHeaderEnd = palettes[activeTheme][3];
  uiGreen = 0xA7E9; uiYellow = 0xFFE8; uiRed = 0xFA8D;
  if (persist) {
    prefs.begin("console", false);
    prefs.putUChar("theme", activeTheme);
    prefs.end();
  }
}

void loadThemePreference() {
  prefs.begin("console", true);
  uint8_t savedTheme = prefs.getUChar("theme", 0);
  soundEnabled = prefs.getBool("sound", true);
  prefs.end();
  applyThemePreset(savedTheme, false);
}

Game::Game(int i) {
  if (i < 0 || i >= GAME_COUNT) i = 0;
  index = i;
  name = gameNames[index];

  switch (index) {
    case 0:
      gameSetup = menuSetup;
      gameLoop =  menuLoop;
      break;
    case 1:
      gameSetup = tetrisSetup;
      gameLoop = tetrisLoop;
      break;
    case 2:
      gameSetup = setupSnake;
      gameLoop = loopSnake;
      break;
      // in your game registration
    case 3:
      gameSetup = breakoutSetup;
      gameLoop = breakoutLoop;
      break;
    case 4:
      gameSetup = runSetup;
      gameLoop = runLoop;
      break;
    case 5:
      gameSetup = galagaSetup;
      gameLoop = galagaLoop;
      break;
    case 6:
      gameSetup = puzzleSetup;
      gameLoop = puzzleLoop;
      break;
    case 7:
      gameSetup = ticTacToeSetup;
      gameLoop = ticTacToeLoop;
      break;
    case 8:
      gameSetup = pongSetup;
      gameLoop = pongLoop;
      break;
    case 9:
      gameSetup = minesweeperSetup;
      gameLoop = minesweeperLoop;
      break;
    case 10:
      gameSetup = lifeSetup;
      gameLoop = lifeLoop;
      break;
    case 11:
      gameSetup = mandelbrotSetup;
      gameLoop = mandelbrotLoop;
      break;
  }

  prefs.begin(name, false);
  for (int i = 0; i < 5; i++) {
    char key[50];
    snprintf(key, sizeof(key), "score_%d", i);
    highScores[i] = prefs.getInt(key);
  }
  prefs.end();
}


void Game::recordScore(int score) {
  if (score <= 0) return;
  for (int position = 0; position < 5; ++position) {
    if (score <= highScores[position]) continue;
    for (int move = 4; move > position; --move)
      highScores[move] = highScores[move - 1];
    highScores[position] = score;
    break;
  }

  prefs.begin(name, false);
  for (int i = 0; i < 5; i++) {
    char key[50];
    snprintf(key, sizeof(key), "score_%d", i);
    prefs.putInt(key, highScores[i]);
  }
  prefs.end();
}

static bool countdownDigitRestore = false;
bool restoringCountdownDigit() { return countdownDigitRestore; }

// Clip scene restoration to the pause card first, then only to the digit area.
// Keep the screen coordinate origin unchanged for every game's renderer.
void showResumeCountdown(void (*redrawScene)()) {
  buttonsRequireRelease();
  tft.setViewport(18, 58, 204, 200, false);
  redrawScene();
  tft.resetViewport();
  const int centerX = tft.width() / 2;
  const int centerY = tft.height() / 2;
  for (int number = 3; number >= 1; --number) {
    if (number != 3) {
      countdownDigitRestore = true;
      tft.setViewport(centerX - 32, centerY - 32, 64, 64, false);
      redrawScene();
      tft.resetViewport();
    }
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4);
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED);
    tft.drawString(String(number), centerX, centerY);
    tft.setTextSize(1);
    unsigned long started = millis();
    while (millis() - started < 1000) delay(5);
  }
  countdownDigitRestore = true;
  tft.setViewport(centerX - 32, centerY - 32, 64, 64, false);
  redrawScene();
  tft.resetViewport();
  countdownDigitRestore = false;
  buttonsRequireRelease();
}

void showGameHighScores() {
  const uint16_t medalColors[4] = {0xFEA0, 0xC618, 0xB3A7, UI_CYAN};
  tft.fillScreen(UI_BG);
  tft.fillRoundRect(14, 24, 212, 272, 12, UI_PANEL);
  tft.drawRoundRect(14, 24, 212, 272, 12, UI_SURFACE);

  tft.fillRoundRect(28, 38, 184, 34, 8, uiHeaderStart);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(0x18C5, uiHeaderStart);
  tft.drawString("ALL-TIME BEST", 120, 49);
  tft.setTextFont(1);
  tft.drawString(currentGame.name, 120, 63);

  for (int rank = 0; rank < 4; ++rank) {
    int y = 84 + rank * 45;
    uint16_t medal = medalColors[rank];
    tft.fillRoundRect(27, y, 186, 36, 8, UI_ICON_BG);
    tft.drawRoundRect(27, y, 186, 36, 8,
                      rank == 0 ? medal : UI_SURFACE);

    // Small ribbon tails and a circular medal face.
    tft.fillTriangle(37, y + 23, 43, y + 33, 46, y + 21, medal);
    tft.fillTriangle(46, y + 21, 50, y + 33, 55, y + 23, medal);
    tft.fillCircle(46, y + 16, 11, medal);
    tft.drawCircle(46, y + 16, 8, rank < 3 ? 0xFFFF : UI_TEXT);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(2);
    tft.setTextColor(rank < 3 ? 0x2104 : 0x0000, medal);
    tft.drawString(String(rank + 1), 46, y + 16);

    tft.setTextDatum(ML_DATUM);
    tft.setTextFont(1);
    tft.setTextColor(rank == 0 ? medal : UI_MUTED, UI_ICON_BG);
    const char* labels[] = {"GOLD", "SILVER", "BRONZE", "FOURTH"};
    tft.drawString(labels[rank], 67, y + 10);
    tft.setTextFont(2);
    tft.setTextColor(UI_TEXT, UI_ICON_BG);
    String value = currentGame.highScores[rank] > 0
        ? String(currentGame.highScores[rank]) : String("---");
    if (!strcmp(currentGame.name, "Run") && currentGame.highScores[rank] > 0)
      value += " M";
    tft.setTextDatum(MR_DATUM);
    tft.drawString(value, 201, y + 19);
  }

  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(1);
  tft.setTextColor(UI_CYAN, UI_PANEL);
  tft.drawString("SHOOT  RETURN", 120, 278);

  // The leaderboard may be opened by several different input layers. First
  // wait out the Shoot press that selected HIGH SCORES, then accept the next
  // physical Shoot edge directly so a game's private queue cannot swallow it.
  buttonsRequireRelease();
  while (digitalRead(SHOOT) != LOW) delay(2);
  delay(22);
  while (true) {
    int command = findCommand(false);
    bool shootPressed = digitalRead(SHOOT) != LOW;
    if (shootPressed) {
      delay(22);
      if (digitalRead(SHOOT) != LOW) break;
    }
    if (command == 4 || command == 0) break;
    if (Serial.available()) {
      char key = Serial.read();
      if (key == ' ' || key == 'e' || key == 'E' || key == '\n' ||
          key == '\r' || key == 'w' || key == 'W') break;
    }
    delay(4);
  }
  playSelectTone();
  while (digitalRead(SHOOT) != LOW) delay(2);
  buttonsRequireRelease();

}

void resetAllGameScores() {
  // Launcher indices 1..9 are games. Life and Mandelbrot are science apps and
  // intentionally retain no score reset behavior here.
  for (int gameIndex = 1; gameIndex <= 9; ++gameIndex) {
    prefs.begin(gameNames[gameIndex], false);
    for (int rank = 0; rank < 5; ++rank) {
      char key[20];
      snprintf(key, sizeof(key), "score_%d", rank);
      prefs.remove(key);
    }
    prefs.end();
  }
}

extern Game currentGame;

void switchGame(int gameIndex) {
  lifeInputEnd();
  tetrisInputEnd();
  buttonsRequireRelease();
  Serial.printf("[Game] Switching to game %d (%s)\n", gameIndex, (gameIndex >= 0 && gameIndex < GAME_COUNT) ? gameNames[gameIndex] : "Unknown");
  currentGame = Game(gameIndex);
  currentGame.gameSetup();
}

Adafruit_ADS1115 ads;
TFT_eSPI tft = TFT_eSPI();
bool adsConnected = false;

void drawTransparentSpriteFast(int x, int y, int w, int h, const uint16_t *img, uint16_t transparentColor) {
  static uint16_t lineBuffer[128];
  tft.startWrite();
  for (int j = 0; j < h; j++) {
    int pixelsToDraw = 0;
    int startX = -1;
    for (int i = 0; i < w; i++) {
      uint16_t color = pgm_read_word(&img[j * w + i]);
      if (color != transparentColor) {
        if (startX == -1) startX = i;
        lineBuffer[pixelsToDraw++] = color;
      } else {
        if (pixelsToDraw > 0) {
          tft.setAddrWindow(x + startX, y + j, pixelsToDraw, 1);
          tft.pushColors(lineBuffer, pixelsToDraw);
          pixelsToDraw = 0;
          startX = -1;
        }
      }
    }
    if (pixelsToDraw > 0) {
      tft.setAddrWindow(x + startX, y + j, pixelsToDraw, 1);
      tft.pushColors(lineBuffer, pixelsToDraw);
    }
  }
  tft.endWrite();
}

int score = 0;
int gameState = 0;
int menu_drawn = false;
unsigned long lastCommand = 0;
unsigned long lastTimeUpdate = 0;
int selected_menu_item = 1;
const unsigned long debounce_delay = 180;

// Direct ESP32-C3 Hardware LEDC PWM for Buzzer
const int BUZZER_LEDC_CH = 0;

void setBuzzerTone(unsigned int frequency) {
  ledcWriteTone(BUZZER_LEDC_CH, (soundEnabled && frequency > 0) ? frequency : 0);
}

void setSoundEnabled(bool enabled, bool persist) {
  soundEnabled = enabled;
  if (!soundEnabled) ledcWriteTone(BUZZER_LEDC_CH, 0);

  if (persist) {
    prefs.begin("console", false);
    prefs.putBool("sound", soundEnabled);
    prefs.end();
  }
}

void playToneDirect(unsigned int freq, unsigned long durationMs) {
  if (freq > 0) {
    setBuzzerTone(freq);
  } else {
    setBuzzerTone(0);
  }
  delay(durationMs);
  setBuzzerTone(0);
}

void playNavClick() {
  // Crisp retro click/blip via hardware PWM
  setBuzzerTone(1800);
  delay(20);
  setBuzzerTone(0);
}

void playSelectTone() {
  // Two-tone cheerful affirmation
  setBuzzerTone(1319); // E6
  delay(50);
  setBuzzerTone(1760); // A6
  delay(80);
  setBuzzerTone(0);
}

void playTetrisDropSound() {
  // A short descending click that gives the landed piece a little weight.
  playToneDirect(880, 9);
  playToneDirect(440, 13);
}

void drawBootHand(int tip, bool fromLeft, uint16_t handColor) {
  // A simple original pixel-style hand silhouette, mirrored around the center.
  auto handX = [tip, fromLeft](int distance, int width) {
    return fromLeft ? tip - distance - width : tip + distance;
  };
  tft.fillRoundRect(handX(57, 56), 138, 56, 23, 5, UI_SURFACE);
  tft.fillRect(handX(91, 8), 138, 8, 23, UI_MUTED);
  tft.fillRoundRect(handX(25, 37), 130, 37, 36, 7, handColor);
  tft.fillRoundRect(handX(0, 31), 126, 31, 6, 2, handColor);
  tft.fillRoundRect(handX(0, 34), 134, 34, 7, 2, handColor);
  tft.fillRoundRect(handX(0, 33), 143, 33, 7, 2, handColor);
  tft.fillRoundRect(handX(4, 27), 152, 27, 6, 2, handColor);
}

void drawBootSpark(int x, int y, uint16_t color) {
  tft.drawFastHLine(x - 5, y, 11, color);
  tft.drawFastVLine(x, y - 5, 11, color);
  tft.drawPixel(x - 3, y - 3, color);
  tft.drawPixel(x + 3, y + 3, color);
}

uint16_t bootRgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void fillBootGradient(int x, int y, int w, int h,
                      uint8_t r1, uint8_t g1, uint8_t b1,
                      uint8_t r2, uint8_t g2, uint8_t b2) {
  for (int row = 0; row < h; row++) {
    float t = h <= 1 ? 0.0f : (float)row / (float)(h - 1);
    uint8_t r = r1 + (int)((r2 - r1) * t);
    uint8_t g = g1 + (int)((g2 - g1) * t);
    uint8_t b = b1 + (int)((b2 - b1) * t);
    tft.drawFastHLine(x, y + row, w, bootRgb565(r, g, b));
  }
}

void playBootAnimation() {
  // Local boot palette stays vivid regardless of the selected menu theme.
  const uint16_t ink = bootRgb565(12, 14, 22);
  const uint16_t muted = bootRgb565(115, 123, 140);
  const uint16_t lime = bootRgb565(200, 255, 56);
  const int centerX = tft.width() / 2;
  const int top = (tft.height() - 240) / 2;
  TFT_eSprite logo(&tft);
  logo.setColorDepth(16);
  const bool buffered = logo.createSprite(144, 128) != nullptr;
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  // Fixed, separated zones: logo 0..127, title 156, status 180, bar 211.
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("GAME CONSOLE", centerX, top + 156);
  tft.setTextFont(1);
  tft.setTextColor(muted, TFT_BLACK);
  tft.drawString("A LITTLE WORLD OF PLAY", centerX, top + 180);
  tft.fillRoundRect(centerX - 48, top + 211, 96, 4, 2, ink);

  for (int frame = 0; frame <= 64; ++frame) {
    const unsigned long frameStart = millis();
    if (buffered) {
      logo.fillSprite(TFT_BLACK);
      // Ease the controller up by 12 pixels as its color fades in.
      const int remaining = max(0, 24 - frame);
      const int lift = remaining * remaining / 48;
      const int brightness = min(255, frame * 255 / 24);
      // Rounded 100 x 60 body; each scanline blends cyan into violet.
      for (int row = 0; row < 60; ++row) {
        const int edge = min(row, 59 - row);
        const int inset = edge < 14
            ? 14 - (int)sqrtf(196.0f - (14 - edge) * (14 - edge)) : 0;
        const int r = 48 + row * 158 / 59;
        const int g = 218 - row * 124 / 59;
        const int b = 255;
        logo.drawFastHLine(22 + inset, 38 + lift + row, 100 - inset * 2,
            bootRgb565(r * brightness / 255, g * brightness / 255,
                       b * brightness / 255));
      }
      // Controls are centered within the body, with space between groups.
      logo.fillRoundRect(36, 61 + lift, 28, 10, 2, ink);
      logo.fillRoundRect(45, 52 + lift, 10, 28, 2, ink);
      logo.fillCircle(99, 59 + lift, 6, ink);
      logo.fillCircle(88, 73 + lift, 6, ink);
      if (frame >= 24) {
        logo.fillCircle(99, 59 + lift, 3, lime);
        logo.fillCircle(88, 73 + lift, 3, TFT_WHITE);
      }
      logo.fillRoundRect(67, 83 + lift, 10, 3, 1, ink);
      // Small settling sparkles stay clear of the controller and text.
      if (frame >= 24 && frame < 48) {
        const int radius = frame < 36 ? (frame - 24) / 3 : (48 - frame) / 3;
        logo.drawFastHLine(113 - radius, 24, radius * 2 + 1, lime);
        logo.drawFastVLine(113, 24 - radius, radius * 2 + 1, lime);
        logo.drawPixel(18, 105, TFT_WHITE);
      }
      logo.pushSprite(centerX - 72, top);
    }
    const int fill = frame * 92 / 64;
    if (fill > 0) tft.fillRect(centerX - 46, top + 212, fill, 2, lime);
    // Brief notes run alongside the frames and respect system sound settings.
    if (frame == 24) setBuzzerTone(659);
    if (frame == 29 || frame == 39 || frame == 54) setBuzzerTone(0);
    if (frame == 34) setBuzzerTone(988);
    if (frame == 44) setBuzzerTone(1319);
    const unsigned long elapsed = millis() - frameStart;
    if (elapsed < 28) delay(28 - elapsed);
  }
  setBuzzerTone(0);
  if (buffered) logo.deleteSprite();
  tft.fillRect(centerX - 100, top + 172, 200, 16, TFT_BLACK);
  tft.setTextColor(lime, TFT_BLACK);
  tft.drawString("READY TO PLAY", centerX, top + 180);
  delay(280);
}
void init_interface() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(2);
  loadThemePreference();
  tft.fillScreen(TFT_BLACK);
  Serial.println("[Game] Starting interface initialization...");

  ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CH);
  setBuzzerTone(0);
  playBootAnimation();

  Wire.begin(8, 9);
  Wire.setClock(100000);
  Wire.setTimeOut(20);
  if (!ads.begin(0x48)) {
    Serial.println("[Game] Warning: ADS1115 not detected at 0x48. Check I2C wiring (SDA=8, SCL=9).");
    adsConnected = false;
  } else {
    ads.setGain(GAIN_ONE);
    ads.setDataRate(RATE_ADS1115_860SPS);
    adsConnected = true;
    Serial.println("[Game] ADS1115 ADC initialized at 0x48.");
  }

  pinMode(SHOOT, INPUT_PULLDOWN);
  if (!buttonsBegin()) Serial.println("[Input] ERROR: button scanner could not start");
}

static int lastActiveCommand = -1;

bool isDownHeld() { return (buttonsHeld() & 4) != 0; }

int findCommand(bool repeat) {
  unsigned long now = millis();
  int command = buttonsNextPress();
  if (command >= 0) {
    lastActiveCommand = command;
    lastCommand = now;
    return command;
  }
  uint8_t held = buttonsHeld();
  if (lastActiveCommand < 0 || !(held & (1 << lastActiveCommand))) {
    lastActiveCommand = -1;
    return -1;
  }
  // Direction keys repeat. Select is always one event per physical press.
  if (repeat && lastActiveCommand < 4 && now - lastCommand >= 350) {
    lastCommand = now;
    return lastActiveCommand;
  }
  return -1;
}

void PauseMenuControls() {
  int pressed = findCommand();
  int x = (tft.width() > 250 ? 1 : 0);


  if (pressed == 0 + x) updatePauseMenu(1);
  if (pressed == 2 + x) updatePauseMenu(-1);

  if (pressed == 5) {
    if (gameState == 0) {
      gameState = 1;
      menu_drawn = false;
    } else if (gameState == 1) {
      switch (selected_menu_item) {
        case 1:
          restoreAfterPauseTetris();
        case 0:
          ESP.restart();
        case 2:
          ESP.restart();
      }
    }
  }
}





void updatePauseMenu(int dx) {
  const char* labels[] = {"RETRY", "RESUME", "MENU"};
  int new_item = selected_menu_item + dx;
  if (new_item >= 0 && new_item < 3) selected_menu_item = new_item;

  for (int i = 0; i < 3; i++) {
    int y = 140 + i * 34;
    bool sel = (i == selected_menu_item);
    uint16_t fill = UI_ICON_BG;
    tft.fillRoundRect(36, y, 168, 28, 6, fill);
    if (sel) {
      tft.drawRoundRect(36, y, 168, 28, 6, UI_PURPLE);
      tft.drawRoundRect(37, y + 1, 166, 26, 5, UI_PURPLE);
      tft.drawRoundRect(38, y + 2, 164, 24, 4, UI_CYAN);
    }
    tft.setTextDatum(ML_DATUM);
    tft.setTextFont(2);
    tft.setTextColor(sel ? UI_TEXT : UI_MUTED, fill);
    tft.drawString(labels[i], 48, y + 14);
    if (sel) {
      tft.setTextDatum(MR_DATUM);
      tft.drawString(">", 192, y + 14);
    }
  }
}

void drawPauseMenu(void) {
  tft.fillRoundRect(24, 75, 192, 175, 12, UI_PANEL);
  tft.drawRoundRect(24, 75, 192, 175, 12, UI_SURFACE);
  tft.fillRect(38, 91, 24, 3, 0xFFFF);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(UI_TEXT, UI_PANEL);
  tft.drawString("PAUSED", 38, 99);
  updatePauseMenu(0);
}

// =================================================================
//                         GAME OVER
// =================================================================
void showGameOver() {
  static bool gameOverDrawn = false;
  currentGame.recordScore(score);

  if (!gameOverDrawn) {
    tft.fillRoundRect(24, 65, 192, 190, 12, UI_PANEL);
    tft.drawRoundRect(24, 65, 192, 190, 12, UI_SURFACE);

    tft.fillRect(38, 81, 24, 3, UI_RED);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(1);
    tft.setTextColor(UI_MUTED, UI_PANEL);
    tft.drawString("GAME / FINISHED", 38, 93);

    tft.setTextFont(4);
    tft.setTextColor(UI_TEXT, UI_PANEL);
    tft.drawString("GAME OVER", 38, 107);

    tft.setTextFont(1);
    tft.setTextColor(UI_MUTED, UI_PANEL);
    tft.drawString("SCORE", 38, 145);
    tft.drawString("BEST", 136, 145);

    tft.setTextFont(2);
    tft.setTextColor(UI_TEXT, UI_PANEL);
    tft.drawString(String(score), 38, 158);
    tft.setTextColor(0xFFFF, UI_PANEL);
    tft.drawString(String(currentGame.highScores[0]), 136, 158);

    tft.drawFastHLine(38, 186, 164, UI_SURFACE);
    tft.setTextFont(1);
    tft.setTextColor(0xFFFF, UI_PANEL);
    tft.drawString("SHOOT  RETRY", 38, 198);
    tft.setTextColor(UI_MUTED, UI_PANEL);
    tft.drawString("UP  MAIN MENU", 38, 218);

    gameOverDrawn = true;
  }
}

void drawDefaultBg(){
   int ymax = tft.width() <= 240 ? 32:24;
      int xmax = tft.width() > 240 ? 32:24;


   for (int y = 0; y < ymax; y++) {
      for (int x = 0; x < 32; x++) {
        tft.fillRect(x * 10, y * 10, 10, 10, bg());
      }
    }
}

void drawDefaultBg2(){
   int ymax = tft.width() <= 240 ? 32:24;
      int xmax = tft.width() > 240 ? 32:24;


   for (int y = 0; y < ymax; y++) {
      for (int x = 0; x < 32; x++) {
        tft.fillRect(x * 10, y * 10, 10, 10, (x+y) % 2 == 0? BG1:BG2);
      }
    }
}

int getDirectHorizontalInput() {
  // These games handle Shoot directly and consume held paddle movement.
  // Drain unused edges so they cannot later navigate a pause menu.
  while (buttonsNextPress() >= 0) {}
  uint8_t held = buttonsHeld();
  int direction = ((held & 2) != 0) - ((held & 8) != 0);
  if (direction) return direction;
  if (Serial.available()) {
    char c = Serial.peek();
    if (c == 'd' || c == 'l') { Serial.read(); return 1; }
    if (c == 'a' || c == 'j') { Serial.read(); return -1; }
  }
  return 0;
}
