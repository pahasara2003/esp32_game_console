#include "rally.h"
#include "sd_game_music.h"
#include <Arduino.h>
#include <Preferences.h>
#include "game.h"
#include "ui_kit.h"
#include "tetris_input.h"
#include "buttons.h"
#include <Adafruit_ADS1X15.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorMP3a.h>
#include <AudioOutputI2S.h>
#include <new>
#include <string.h>
#include "images.h"


const char* gameNames[] = { "Menu", "Tetris", "Snake", "Breakout", "Run", "Galaga", "Puzzle",
                            "TicTacToe", "Pong", "Minesweeper", "Life", "Mandelbrot", "Rally" };
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
uint8_t displayBrightness = 100;
bool soundEnabled = true;
bool batterySaver = false;
static uint8_t savedBrightness = 100;
static bool savedGameMusic = true;

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
  displayBrightness = prefs.getUChar("brightness", 100);
  soundEnabled = prefs.getBool("sound", true);
  batterySaver = prefs.getBool("saver", false);
  savedBrightness = prefs.getUChar("saverbright", 100);
  savedGameMusic = prefs.getBool("savermusic", true);
  prefs.end();
  audio::load();
<<<<<<< HEAD
=======
  if (batterySaver) { displayBrightness = 10; audio::gameMusic = false; }
>>>>>>> codex/rally-3d
  applyThemePreset(savedTheme, false);
}

void setDisplayBrightness(uint8_t brightness, bool persist) {
  static bool backlightPwmReady = false;
  displayBrightness = constrain(int(brightness), 10, 100);
  if (!backlightPwmReady && !ledcSetup(0, 5000, 8)) {
    Serial.println("[Display] Backlight PWM setup failed");
    return;
  }
  backlightPwmReady = true;
  // Display initialization can restore GPIO output mode, disconnecting PWM.
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, (uint16_t(displayBrightness) * 255) / 100);
  if (persist) {
    prefs.begin("console", false);
    prefs.putUChar("brightness", displayBrightness);
    prefs.end();
  }
}

void setBatterySaver(bool enabled) {
  if (enabled == batterySaver) return;
  if (enabled) {
    savedBrightness = displayBrightness;
    savedGameMusic = audio::gameMusic;
  }
  batterySaver = enabled;
  setDisplayBrightness(enabled ? 10 : savedBrightness, true);
  audio::gameMusic = enabled ? false : savedGameMusic;
  audio::save();
  prefs.begin("console", false);
  prefs.putBool("saver", batterySaver);
  prefs.putUChar("saverbright", savedBrightness);
  prefs.putBool("savermusic", savedGameMusic);
  prefs.end();
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
    case 12:
      gameSetup = rallySetup;
      gameLoop = rallyLoop;
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
  if (score > highScores[0]) audio::play(audio::Cue::Record);
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
    tft.fillCircle(centerX, centerY, 30, ui::CARD);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4);
    tft.setTextSize(2);
    tft.setTextColor(ui::LABEL, ui::CARD);
    tft.drawString(String(number), centerX, centerY + 2);
    tft.setTextSize(1);
    audio::play(audio::Cue::Countdown);
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

// Display names for the leaderboard caption (NVS keys stay unchanged).
static const char *gameDisplayName(const char *key) {
  if (!strcmp(key, "TicTacToe")) return "Tic-tac-toe";
  if (!strcmp(key, "Minesweeper")) return "Mines";
  return key;
}

void showGameHighScores() {
  const uint16_t medal[4] = {0xFEA0, 0xC618, 0xCB0C, 0x4A69};
  const char *places[] = {"1st", "2nd", "3rd", "4th"};
  tft.fillScreen(UI_BG);
  ui::heading(20, 16, gameDisplayName(currentGame.name), "High scores", UI_BG);

  const int x = 16, y = 74, w = 208, rowH = 46;
  tft.fillRoundRect(x, y, w, rowH * 4, 12, ui::GROUP);
  for (int rank = 0; rank < 4; ++rank) {
    int top = y + rank * rowH, cy = top + rowH / 2;
    if (rank) tft.drawFastHLine(x + 52, top, w - 52, ui::SEPARATOR);
    tft.fillCircle(x + 26, cy, 12, medal[rank]);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(ui::inkOn(medal[rank]), medal[rank]);
    tft.drawString(String(rank + 1), x + 26, cy + 1, 2);

    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(ui::SECONDARY, ui::GROUP);
    tft.drawString(places[rank], x + 52, cy + 1, 2);

    int value = currentGame.highScores[rank];
    String text = value > 0 ? String(value) : String("-");
    if (value > 0 && !strcmp(currentGame.name, "Run")) text += " m";
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(value > 0 ? ui::LABEL : ui::SECONDARY, ui::GROUP);
    tft.drawString(text, x + w - 14, cy + 2, 4);
  }
  ui::pill(60, 274, 120, 30, "Done", UI_CYAN);

  // The leaderboard may be opened by several different input layers. First
  // wait out the Shoot press that selected HIGH SCORES, then accept the next
  // physical Shoot edge directly so a game's private queue cannot swallow it.
  buttonsRequireRelease();
  while (readShoot() != LOW) delay(2);
  delay(22);
  while (true) {
    int command = findCommand(false);
    bool shootPressed = readShoot() != LOW;
    if (shootPressed) {
      delay(22);
      if (readShoot() != LOW) break;
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
  while (readShoot() != LOW) delay(2);
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
  audio::scene(gameIndex);
<<<<<<< HEAD
=======
  audio::pauseMusic(true);
>>>>>>> codex/rally-3d
  audio::play(gameIndex == 0 ? audio::Cue::Back : audio::Cue::Open);
  lifeInputEnd();
  lifeRelease();
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

// MAX98357A I2S amplifier: a background task streams a square wave at the
// currently requested frequency (0 = silence).
#include <driver/i2s.h>
static volatile uint32_t i2sToneFreq = 0;
static TaskHandle_t i2sTask = nullptr;
static volatile bool i2sHoldReq = false, i2sHeld = false;  // music player owns I2S while held
static constexpr int I2S_RATE = 22050;

extern const uint8_t nokiaDataStart[] asm("_binary_assets_nokia_welcome_mp3_start");
extern const uint8_t nokiaDataEnd[] asm("_binary_assets_nokia_welcome_mp3_end");
static AudioFileSourcePROGMEM *nokiaFile = nullptr;
static AudioGeneratorMP3a *nokiaDecoder = nullptr;
static AudioOutputI2S *nokiaOutput = nullptr;
static bool nokiaAudioHeld = false;

static void i2sToneTask(void *) {
<<<<<<< HEAD
  int16_t buf[256];
=======
  int16_t buf[256], music[256];
>>>>>>> codex/rally-3d
  while (true) {
    if (i2sHoldReq) { sd_music::stop(); i2sHeld = true; vTaskDelay(pdMS_TO_TICKS(10)); continue; }
    i2sHeld = false;
<<<<<<< HEAD
    audio::render(buf, 256, i2sToneFreq, soundEnabled);
=======
    bool embeddedTrack = sd_music::fill(music, 256, soundEnabled && !i2sToneFreq);
    audio::render(buf, 256, i2sToneFreq, soundEnabled, embeddedTrack ? music : nullptr);
>>>>>>> codex/rally-3d
    size_t written;
    i2s_write(I2S_NUM_0, buf, sizeof(buf), &written, portMAX_DELAY);
  }
}

static bool installI2S();
static void initI2SAudio() {
  if (installI2S()) xTaskCreate(i2sToneTask, "i2sTone", 8192, nullptr, 3, &i2sTask);
}

// Hand I2S to the MP3 player: park the tone task, then free the driver.
void audioReleaseForMusic() {
  audio::external(true);
  i2sToneFreq = 0;
  i2sHoldReq = true;
  while (i2sTask && !i2sHeld) delay(5);
  i2s_driver_uninstall(I2S_NUM_0);
}

// Take I2S back after the player and resume the tone task.
void audioReclaimFromMusic() {
  i2s_driver_uninstall(I2S_NUM_0);
  installI2S();
<<<<<<< HEAD
=======
  // ESP8266Audio routes its unused MCLK to GPIO0, taking over the backlight.
  setDisplayBrightness(displayBrightness, false);
>>>>>>> codex/rally-3d
  audio::external(false);
  i2sHoldReq = false;
}

static bool installI2S() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = I2S_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 8;  // cover a full shared-SPI display transfer during SD playback
  cfg.dma_buf_len = 128;  // short buffers keep tone changes snappy
  cfg.tx_desc_auto_clear = true;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = I2S_BCLK_PIN;
  pins.ws_io_num = I2S_LRCLK_PIN;
  pins.data_out_num = I2S_DOUT_PIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) != ESP_OK ||
      i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) {
    Serial.println("[Audio] I2S init failed");
    return false;
  }
  return true;
}

void setBuzzerTone(unsigned int frequency) {
  i2sToneFreq = (soundEnabled && frequency > 0) ? frequency : 0;
}

void setSoundEnabled(bool enabled, bool persist) {
  soundEnabled = enabled;
  if (!soundEnabled) i2sToneFreq = 0;

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
  audio::play(audio::Cue::Move);
}

void playSelectTone() {
  audio::play(audio::Cue::Open);
}

static void stopNokiaMusic() {
  if (nokiaDecoder) nokiaDecoder->stop();
  delete nokiaDecoder; nokiaDecoder = nullptr;
  delete nokiaFile; nokiaFile = nullptr;
  delete nokiaOutput; nokiaOutput = nullptr;
  if (nokiaAudioHeld) {
    audioReclaimFromMusic();
    nokiaAudioHeld = false;
  }
}

void playNokiaSweepSound() {
  if (!soundEnabled) return;
  audioReleaseForMusic();
  nokiaAudioHeld = true;
  nokiaFile = new (std::nothrow) AudioFileSourcePROGMEM(
      nokiaDataStart, nokiaDataEnd - nokiaDataStart);
  nokiaDecoder = new (std::nothrow) AudioGeneratorMP3a();
  nokiaOutput = new (std::nothrow) AudioOutputI2S(0, AudioOutputI2S::EXTERNAL_I2S, 16);
  if (!nokiaFile || !nokiaFile->isOpen() || !nokiaDecoder ||
      !nokiaOutput) {
    Serial.println("[Audio] System music open failed");
    stopNokiaMusic();
    return;
  }
  nokiaOutput->SetPinout(I2S_BCLK_PIN, I2S_LRCLK_PIN, I2S_DOUT_PIN);
  nokiaOutput->SetGain(audio::mediaGain(0.35f));
  if (!nokiaDecoder->begin(nokiaFile, nokiaOutput)) {
    Serial.println("[Audio] System music decode failed");
    stopNokiaMusic();
    return;
  }
  Serial.println("[Audio] Playing embedded Nokia welcome tune (no SD required)");
<<<<<<< HEAD
=======
  setDisplayBrightness(displayBrightness, false);
>>>>>>> codex/rally-3d
}

static void serviceNokiaMusic() {
  if (nokiaDecoder && nokiaDecoder->isRunning() && !nokiaDecoder->loop())
    stopNokiaMusic();
}

void playTetrisDropSound() {
  audio::play(audio::Cue::Lock);
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

// ---- Boot splash ------------------------------------------------------------
static uint16_t bootLerp(uint16_t a, uint16_t b, float t) {
  t = fminf(1.0f, fmaxf(0.0f, t));
  const int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  const int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)(((ar + (int)((br - ar) * t)) << 11) |
                    ((ag + (int)((bg - ag) * t)) << 5) | (ab + (int)((bb - ab) * t)));
}

// Ease-out cubic on a frame window [from, from + len].
static float bootEase(int frame, int from, int len) {
  float t = (frame - from) / (float)len;
  t = fminf(1.0f, fmaxf(0.0f, t));
  return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

// Rounded-square app icon: soft indigo gradient with a white glyph,
// faded toward black by `alpha`.
static void bootDrawIcon(TFT_eSprite *spr, int cx, int cy, int size, float alpha) {
  auto hline = [&](int x, int y, int w, uint16_t c) {
    if (spr) spr->drawFastHLine(x, y, w, c); else tft.drawFastHLine(x, y, w, c);
  };
  const int r = size * 28 / 100;
  for (int row = 0; row < size; ++row) {
    const int edge = min(row, size - 1 - row);
    const int inset = edge < r ? r - (int)sqrtf((float)(r * r - (r - edge) * (r - edge))) : 0;
    const float t = row / (float)max(1, size - 1);
    const uint16_t top = bootRgb565(110, 120, 255), bottom = bootRgb565(70, 60, 200);
    hline(cx - size / 2 + inset, cy - size / 2 + row, size - inset * 2,
          bootLerp(TFT_BLACK, bootLerp(top, bottom, t), alpha));
  }
  const uint16_t ink = bootLerp(TFT_BLACK, TFT_WHITE, alpha);
  const int u = max(2, size / 22);
  const int lx = cx - size / 5, rx = cx + size / 5;
  auto rr = [&](int x, int y, int w, int h) {
    if (spr) spr->fillRoundRect(x, y, w, h, u / 2, ink); else tft.fillRoundRect(x, y, w, h, u / 2, ink);
  };
  auto cc = [&](int x, int y, int rad) {
    if (spr) spr->fillCircle(x, y, rad, ink); else tft.fillCircle(x, y, rad, ink);
  };
  rr(lx - u * 3, cy - u, u * 6, u * 2);
  rr(lx - u, cy - u * 3, u * 2, u * 6);
  cc(rx + u + u / 2, cy - u, u + u / 3);
  cc(rx - u / 2, cy + u + u / 2, u + u / 3);
}

void playBootAnimation() {
  // Minimal splash: black screen, the app icon scales and fades in, the
  // name rises into place while the welcome tune plays. Service the decoder
  // between drawing operations and continuously during each frame's wait.
  const int cx = tft.width() / 2;
  const int top = (tft.height() - 240) / 2;
  const int iconCY = top + 88;
  const int titleY = top + 160;
  const uint16_t grey = bootRgb565(140, 146, 160);
  const int kFrames = 64, kFrameMs = 25;  // 1.6 s of motion
  const int S = 100;

  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);

  TFT_eSprite icon(&tft);
  icon.setColorDepth(16);
  const bool buffered = icon.createSprite(S, S) != nullptr;
  playNokiaSweepSound();
  const uint32_t musicStarted = millis();

  for (int frame = 0; frame <= kFrames; ++frame) {
    const unsigned long frameStart = millis();
    serviceNokiaMusic();

    // Icon: 0..20 scale 0.86 -> 1.0 and fade in.
    if (frame <= 22) {
      const float t = bootEase(frame, 0, 20);
      const int size = (int)(76 * (0.86f + 0.14f * t)) & ~1;
      if (buffered) {
        icon.fillSprite(TFT_BLACK);
        bootDrawIcon(&icon, S / 2, S / 2, size, t);
        icon.pushSprite(cx - S / 2, iconCY - S / 2);
      } else if (frame == 22) {
        bootDrawIcon(nullptr, cx, iconCY, 76, 1.0f);
      }
    }

    serviceNokiaMusic();
    // Title: 12..32 rises 8 px while fading in.
    if (frame >= 12 && frame <= 34) {
      const float t = bootEase(frame, 12, 20);
      const int lift = (int)(8 * (1.0f - t));
      tft.fillRect(cx - 100, titleY - 16, 200, 34, TFT_BLACK);
      tft.setTextColor(bootLerp(TFT_BLACK, TFT_WHITE, t), TFT_BLACK);
      tft.drawString("Game Console", cx, titleY + lift, 4);
    }

    serviceNokiaMusic();
    // Hairline: 24..44 grows outward from the centre.
    if (frame >= 24 && frame <= 44) {
      const int half = (int)(28 * bootEase(frame, 24, 20));
      tft.drawFastHLine(cx - half, titleY + 24, half * 2, grey);
    }

    // Caption: 30..50 fades in.
    if (frame >= 30 && frame <= 50 && (frame & 1) == 0) {
      tft.setTextColor(bootLerp(TFT_BLACK, grey, bootEase(frame, 30, 20)), TFT_BLACK);
      tft.drawString("Ready to play", cx, titleY + 44, 2);
    }

    while (millis() - frameStart < (unsigned long)kFrameMs) {
      serviceNokiaMusic();
      if (nokiaDecoder) yield();
      else delay(1);
    }
  }
  setBuzzerTone(0);
  if (buffered) icon.deleteSprite();
  while (nokiaDecoder && millis() - musicStarted < 10000) {
    serviceNokiaMusic();
    yield();
  }
  stopNokiaMusic();

  // Fade the whole composition to black (redraw icon and text darker).
  for (int step = 1; step <= 10; ++step) {
    const float a = 1.0f - step / 10.0f;
    bootDrawIcon(nullptr, cx, iconCY, 76, a);
    tft.setTextColor(bootLerp(TFT_BLACK, TFT_WHITE, a), TFT_BLACK);
    tft.drawString("Game Console", cx, titleY, 4);
    tft.drawFastHLine(cx - 28, titleY + 24, 56, bootLerp(TFT_BLACK, grey, a));
    tft.setTextColor(bootLerp(TFT_BLACK, grey, a), TFT_BLACK);
    tft.drawString("Ready to play", cx, titleY + 44, 2);
    delay(22);
  }
  tft.fillScreen(TFT_BLACK);
}
void init_interface() {
  Serial.begin(115200);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);  // deselect SD before initializing the shared SPI bus
  delay(150);  // let the display power stabilize before its software reset
  tft.init();
  tft.setRotation(2);
  loadThemePreference();
  setDisplayBrightness(displayBrightness, false);
  tft.fillScreen(TFT_BLACK);
  Serial.println("[Game] Starting interface initialization...");

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);  // keep SD card off the shared SPI bus
  sd_music::begin();
  initI2SAudio();
  setBuzzerTone(0);
#ifndef BIRTHDAY_DEV
  playBootAnimation();
#endif

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);  // ADS1115 supports 400 kHz fast mode
  Wire.setTimeOut(20);
  if (!ads.begin(0x48)) {
    Serial.println("[Game] Warning: ADS1115 not detected at 0x48. Check I2C wiring (SDA=10, SCL=20).");
    adsConnected = false;
  } else {
    ads.setGain(GAIN_ONE);
    ads.setDataRate(RATE_ADS1115_860SPS);
    adsConnected = true;
    Serial.println("[Game] ADS1115 ADC initialized at 0x48.");
  }

  pinMode(RIGHT_BTN, INPUT_PULLUP);  // external 10k pull-up as well
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
      audio::pauseMusic(true); gameState = 1;
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
