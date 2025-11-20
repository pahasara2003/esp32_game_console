#line 1 "/home/pahasara/Arduino/gameConsole/menu.h"
#include "logo.h"

#define BUZZER_PIN 10

// Define apps
struct App {
  const char* label;
  int x_coord;
  int y_coord;
  const uint16_t *img;
};

App Apps[] = {
  {"snake",0,0,snake},
  {"tetris",0,0,tetris},
  {"breakout",0,0,breakout},
  {"about",0,0,about},
  {"run",0,0,run},
  {"galaga",0,0,galga_img}
};

const int APP_COUNT = sizeof(Apps) / sizeof(App);

// Grid layout
const int COLS = 3;
const int ROWS = 2;
const int TILE_H = (SCREEN_H - 60) / ROWS - 10+5;
const int TILE_W = TILE_H-8;
const int START_Y = 50;

int selectedApp = 0;
unsigned long lastInputTime = 0;
unsigned long lastAnimTime = 0;

// Background stars for animation


// Particle effects for menu
struct MenuParticle {
  float x, y;
  float speedX, speedY;
  uint16_t color;
  int life;
};
MenuParticle menuParticles[15];
int menuParticleCount = 0;

// Animation variables
int selectionGlow = 0;
int glowDirection = 1;




void createMenuParticles(int x, int y) {
  menuParticleCount = 0;
  for (int i = 0; i < 12; i++) {
    menuParticles[i].x = x;
    menuParticles[i].y = y;
    float angle = (i / 12.0) * 6.28;
    menuParticles[i].speedX = cos(angle) * 2;
    menuParticles[i].speedY = sin(angle) * 2;
    menuParticles[i].color = 0x07FF; // Cyan
    menuParticles[i].life = 15;
    menuParticleCount++;
  }
}

void updateMenuParticles() {
  for (int i = 0; i < menuParticleCount; i++) {
    if (menuParticles[i].life > 0) {
      // Erase old position
      tft.drawPixel((int)menuParticles[i].x, (int)menuParticles[i].y, TFT_BLACK);
      
      // Update position
      menuParticles[i].x += menuParticles[i].speedX;
      menuParticles[i].y += menuParticles[i].speedY;
      menuParticles[i].speedY += 0.1; // Gravity
      menuParticles[i].life--;
      
      // Draw new position
      if (menuParticles[i].life > 0 && 
          menuParticles[i].x >= 0 && menuParticles[i].x < SCREEN_W &&
          menuParticles[i].y >= 0 && menuParticles[i].y < SCREEN_H) {
        tft.drawPixel((int)menuParticles[i].x, (int)menuParticles[i].y, 
                      menuParticles[i].color);
      }
    }
  }
}

void drawSelectionGlow(int index) {
  int col = index % COLS;
  int row = index / COLS;
  int x = 10 + col * (TILE_W + 30);
  int y = START_Y + row * (TILE_H + 10);
  
  // Create pulsing glow effect
  uint16_t glowColor = tft.color565(0, selectionGlow, 255);
  
  // Draw glow border (doesn't redraw tile content)
  tft.drawRoundRect(x - 2, y - 2, TILE_W + 4, TILE_H + 4, 10, glowColor);
  tft.drawRoundRect(x - 1, y - 1, TILE_W + 2, TILE_H + 2, 9, 0x07FF);
  
  // Draw corner highlights
  tft.fillCircle(x + 5, y + 5, 2, 0xF81F); // Magenta
  tft.fillCircle(x + TILE_W - 5, y + 5, 2, 0xF81F);
  tft.fillCircle(x + 5, y + TILE_H - 5, 2, 0xF81F);
  tft.fillCircle(x + TILE_W - 5, y + TILE_H - 5, 2, 0xF81F);
}

void eraseSelectionGlow(int index) {
  int col = index % COLS;
  int row = index / COLS;
  int x = 10 + col * (TILE_W + 30);
  int y = START_Y + row * (TILE_H + 10);
  
  // Erase glow border
  tft.drawRoundRect(x - 2, y - 2, TILE_W + 4, TILE_H + 4, 10, TFT_BLACK);
  tft.drawRoundRect(x - 1, y - 1, TILE_W + 2, TILE_H + 2, 9, TFT_BLACK);
  
  // Erase corner highlights
  tft.fillCircle(x + 5, y + 5, 2, TFT_BLACK);
  tft.fillCircle(x + TILE_W - 5, y + 5, 2, TFT_BLACK);
  tft.fillCircle(x + 5, y + TILE_H - 5, 2, TFT_BLACK);
  tft.fillCircle(x + TILE_W - 5, y + TILE_H - 5, 2, TFT_BLACK);
}

void drawAppTile(int index, bool selected = false) {
  int col = index % COLS;
  int row = index / COLS;

  int x = 10 + col * (TILE_W + 30);
  int y = START_Y + row * (TILE_H + 10);

  uint16_t fill = selected ? TEXT_COLOR : TFT_BLACK;

  Apps[index].x_coord = x;
  Apps[index].y_coord = y;

  // Draw shadow for depth
  tft.fillRoundRect(x + 2, y + 2, TILE_W, TILE_H, 8, 0x2104); // Dark shadow

  // Draw tile background
  tft.fillRoundRect(x, y, TILE_W, TILE_H, 8, fill);

  // Draw icon - ONLY ONCE when tile is drawn
  drawTransparentSpriteFast(x+7, y+5, 60, 60, Apps[index].img);

  // Label
  tft.setTextColor(selected ? TILE_COLOR : TEXT_COLOR, fill);
  tft.setTextDatum(TC_DATUM);
  
  int iconW = TILE_W / 2;
  int iconH = TILE_H / 3;
  int iconY = y + 10;
  
  tft.drawString(Apps[index].label, x + TILE_W / 2, iconY + iconH + 35);
  
  // If selected, draw initial glow
  if (selected) {
    drawSelectionGlow(index);
  }
}

void displayMenu() {
  tft.fillScreen(TFT_BLACK);
  
  // Initialize stars
  
  // Draw title with decorative elements
  tft.setTextFont(4);
  tft.setTextColor(TITLE_COLOR, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  
  // Title shadow
  tft.setTextColor(0x2104); // Dark shadow
  tft.drawString("GAME CONSOLE", SCREEN_W / 2 + 2, 27);
  
  // Main title
  tft.setTextColor(TITLE_COLOR);
  tft.drawString("GAME CONSOLE", SCREEN_W / 2, 25);
  
  // Decorative lines
  tft.drawFastHLine(20, 42, SCREEN_W - 40, 0x07FF); // Cyan
  tft.drawFastHLine(20, 43, SCREEN_W - 40, 0xF81F); // Magenta
  
  tft.setTextFont(1);

  // Draw all app tiles ONCE
  for (int i = 0; i < APP_COUNT; i++) {
    drawAppTile(i, i == selectedApp);
  }
}

void updateMenuAnimation() {
  // Update glow animation
  selectionGlow += glowDirection * 10;
  if (selectionGlow >= 255) {
    selectionGlow = 255;
    glowDirection = -1;
  } else if (selectionGlow <= 100) {
    selectionGlow = 100;
    glowDirection = 1;
  }
  
  // Update stars
  
  // Update particles
  updateMenuParticles();
  
  // Redraw ONLY the glow effect, not the entire tile
  // eraseSelectionGlow(selectedApp);
  // drawSelectionGlow(selectedApp);
}

void menu_setup(){
  tft.init();
  tft.setRotation(3);
  tft.setSwapBytes(true);
  displayMenu();
  lastAnimTime = millis();
}

void menu_loop(){
  // Handle animations
  unsigned long currentTime = millis();
  if (currentTime - lastAnimTime > 50) {
    updateMenuAnimation();
    lastAnimTime = currentTime;
  }
  
  switch (command) {
    case 0: 
      eraseSelectionGlow(selectedApp);
      drawAppTile(selectedApp, false);
      selectedApp = (selectedApp - 3 + APP_COUNT) % APP_COUNT;
      drawAppTile(selectedApp, true);
      createMenuParticles(Apps[selectedApp].x_coord + TILE_W/2, 
                         Apps[selectedApp].y_coord + TILE_H/2);
      command = -1;
      break;
      
    case 2:
      eraseSelectionGlow(selectedApp);
      drawAppTile(selectedApp, false);
      selectedApp = (selectedApp + 3 + APP_COUNT) % APP_COUNT;
      drawAppTile(selectedApp, true);
      createMenuParticles(Apps[selectedApp].x_coord + TILE_W/2, 
                         Apps[selectedApp].y_coord + TILE_H/2);
      command = -1;
      break;
      
    case 1:
      eraseSelectionGlow(selectedApp);
      drawAppTile(selectedApp, false);
      selectedApp = (selectedApp + 1 + APP_COUNT) % APP_COUNT;
      drawAppTile(selectedApp, true);
      createMenuParticles(Apps[selectedApp].x_coord + TILE_W/2, 
                         Apps[selectedApp].y_coord + TILE_H/2);
      command = -1;
      break;
      
    case 3:
      eraseSelectionGlow(selectedApp);
      drawAppTile(selectedApp, false);
      selectedApp = (selectedApp - 1 + APP_COUNT) % APP_COUNT;
      drawAppTile(selectedApp, true);
      createMenuParticles(Apps[selectedApp].x_coord + TILE_W/2, 
                         Apps[selectedApp].y_coord + TILE_H/2);
      command = -1;
      break;
      
    case 4:
      // Launch animation - flash effect
      
      command = -1;
      preferences.begin("gameConsole", false);
      preferences.putString("app", Apps[selectedApp].label); 
      preferences.putBool("getApp", true);
      preferences.end();
      esp_restart();
      break;
      
    default:
      break;
  } 
  
  delay(10);
}