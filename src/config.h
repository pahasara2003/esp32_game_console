#include <cstdint>
#ifndef CONFIG_H
#define CONFIG_H


// Runtime palette. Settings can switch every screen between dark and light.
extern uint16_t uiBg;
extern uint16_t uiPanel;
extern uint16_t uiSurface;
extern uint16_t uiCyan;
extern uint16_t uiGreen;
extern uint16_t uiPurple;
extern uint16_t uiYellow;
extern uint16_t uiRed;
extern uint16_t uiText;
extern uint16_t uiMuted;
extern uint16_t uiIconBg;
extern uint16_t uiHeaderStart;
extern uint16_t uiHeaderEnd;
extern bool lightThemeEnabled;
extern uint8_t activeTheme;
extern uint8_t displayBrightness;
extern bool soundEnabled;
constexpr uint8_t THEME_COUNT = 8;

#define UI_BG        uiBg
#define UI_PANEL     uiPanel
#define UI_SURFACE   uiSurface
#define UI_CYAN      uiCyan
#define UI_GREEN     uiGreen
#define UI_PURPLE    uiPurple
#define UI_YELLOW    uiYellow
#define UI_RED       uiRed
#define UI_TEXT      uiText
#define UI_MUTED     uiMuted
#define UI_ICON_BG   uiIconBg

#define MARGIN_X 10
#define MARGIN_Y 10
#define BG1 UI_BG
#define BG2 UI_PANEL
#define SCREEN_H 240
#define SCREEN_W 320

#define TILE_COLOR   UI_SURFACE
#define TEXT_COLOR   UI_CYAN
#define TITLE_COLOR  UI_YELLOW
#define BG_COLOR     UI_BG

#define RIGHT_BTN 8   // D8, Right: 10k pull-up, pressed = LOW
#define SD_CS_PIN 9
#define I2S_BCLK_PIN 6
#define I2S_LRCLK_PIN 7
#define I2S_DOUT_PIN 5
#define I2C_SDA_PIN 10
#define I2C_SCL_PIN 20
// Returns true while Shoot is pressed (ADS1115 A1, debounced in buttons.cpp).
bool readShoot();

inline uint16_t bg() {
  switch (random(5)) {
    case 0: return UI_BG;
    case 1: return UI_PANEL;
    case 2: return UI_ICON_BG;
    default: return UI_SURFACE;
  }
}
extern int score;
extern int gameState;
extern int menu_drawn;

#endif
