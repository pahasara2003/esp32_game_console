#line 1 "/home/pahasara/Arduino/gameConsole/runGame/runGame.h"
//runGame - OPTIMIZED VERSION WITH MENUS
#include "run.h"
#include "jump.h"
#include "floor.h"
#include "obs.h"
#include "fonts.h"

// ===== Game States =====
enum RunGameState {
  RUN_MENU,
  RUN_PLAYING,
  RUN_PAUSED,
  RUN_GAMEOVER
};

RunGameState runGameState = RUN_MENU;

// ===== Sprite Constants =====
#define SPRITE_W 58
#define SPRITE_H 80
#define FRAME_COUNT 6

// ===== Physics =====
int posX = 30;
float posY = 100;
float velocityY = 0;
const float gravity = 6.0;
const float jumpStrength = -28;

// ===== Animation =====
int frame = 0;
unsigned long lastFrameTime = 0;
const int runFrameDelay = 120;
const int jumpFrameDelay = 40;
bool jump = false;

// ===== Previous positions =====
int prevX = posX;
float prevY = posY;
int prevFrame = 0;
bool prevJump = false;
int prevScore = -1;

// ===== Obstacles =====
#define MAX_OBS 5
struct Obstacle {
  int x, y, w, h;
  int prevX;
  bool active;
};
Obstacle obstacles[MAX_OBS];
unsigned long lastObstacleTime = 0;

// ===== Buzzer =====
bool playJump = false;
unsigned long jumpStartTime = 0;
const unsigned long jumpDuration = 120;

// ===== Game state =====
bool gameOver = false;
bool playedTone = false;
bool floorDrawn = false;

// ===== High Score =====
int runHighScore = 0;

// ===== Pause =====
unsigned long lastPauseToggle = 0;
const unsigned long PAUSE_DEBOUNCE = 300;

// ===== Sprite array for fast lookup =====
const uint16_t *runFrames[6] = {runFrame0, runFrame1, runFrame2, runFrame3, runFrame4, runFrame5};
const uint16_t *jumpFrames[6] = {jumpFrame0, jumpFrame1, jumpFrame2, jumpFrame3, jumpFrame4, jumpFrame5};

// ===== Animated stars for menu =====
struct MenuStar {
  int x, y;
  int speed;
  uint16_t color;
};
MenuStar menuStars[20];

void initMenuStars() {
  for (int i = 0; i < 20; i++) {
    menuStars[i].x = random(320);
    menuStars[i].y = random(240);
    menuStars[i].speed = random(1, 4);
    int brightness = random(3);
    if (brightness == 0) menuStars[i].color = 0x4208;
    else if (brightness == 1) menuStars[i].color = 0x8410;
    else menuStars[i].color = 0xFFFF;
  }
}

void updateMenuStars() {
  for (int i = 0; i < 20; i++) {
    tft.drawPixel(menuStars[i].x, menuStars[i].y, 0x12AE);
    
    menuStars[i].x += menuStars[i].speed;
    
    if (menuStars[i].x >= 320) {
      menuStars[i].x = 0;
      menuStars[i].y = random(240);
    }
    
    tft.drawPixel(menuStars[i].x, menuStars[i].y, menuStars[i].color);
  }
}

void startJumpTone() {
  tone(BUZZER_PIN, 1000);
  jumpStartTime = millis();
  playJump = true;
}

void updateBuzzer() {
  if (playJump && millis() - jumpStartTime >= jumpDuration) {
    noTone(BUZZER_PIN);
    playJump = false;
  }
}

// ===== TFT Sprite for double buffering =====
TFT_eSprite playerSprite = TFT_eSprite(&tft);
TFT_eSprite obstacleSprite = TFT_eSprite(&tft);

// ===== FASTEST: Push sprite with masking =====
void drawTransparentSpriteFast(int x, int y, int w, int h, const uint16_t *img, uint16_t transparentColor = 0xF81F) {
  // Create temporary buffer
  static uint16_t lineBuffer[SPRITE_W];
  
  tft.startWrite(); // Begin SPI transaction (CRITICAL for speed)
  
  for (int j = 0; j < h; j++) {
    int pixelsToDraw = 0;
    int startX = -1;
    
    // Scan line and find continuous non-transparent pixels
    for (int i = 0; i < w; i++) {
      uint16_t color = pgm_read_word(&img[j * w + i]);
      
      if (color != transparentColor) {
        if (startX == -1) startX = i;
        lineBuffer[pixelsToDraw++] = color;
      } else {
        // Draw accumulated pixels
        if (pixelsToDraw > 0) {
          tft.setAddrWindow(x + startX, y + j, pixelsToDraw, 1);
          tft.pushColors(lineBuffer, pixelsToDraw);
          pixelsToDraw = 0;
          startX = -1;
        }
      }
    }
    
    // Draw remaining pixels in line
    if (pixelsToDraw > 0) {
      tft.setAddrWindow(x + startX, y + j, pixelsToDraw, 1);
      tft.pushColors(lineBuffer, pixelsToDraw);
    }
  }
  
  tft.endWrite(); // End SPI transaction
}

// ===== METHOD 3: Block-based rendering (10x faster) =====
void drawTransparentSpriteBlock(int x, int y, int w, int h, const uint16_t *img, uint16_t transparentColor = 0xF81F) {
  tft.startWrite();
  
  // Draw in horizontal strips
  for (int j = 0; j < h; j++) {
    bool inBlock = false;
    int blockStart = 0;
    uint16_t blockBuffer[SPRITE_W];
    int blockLen = 0;
    
    for (int i = 0; i < w; i++) {
      uint16_t color = pgm_read_word(&img[j * w + i]);
      
      if (color != transparentColor) {
        if (!inBlock) {
          blockStart = i;
          inBlock = true;
          blockLen = 0;
        }
        blockBuffer[blockLen++] = color;
      } else {
        if (inBlock) {
          // Flush block
          tft.setAddrWindow(x + blockStart, y + j, blockLen, 1);
          tft.pushColors(blockBuffer, blockLen);
          inBlock = false;
        }
      }
    }
    
    // Flush remaining block
    if (inBlock) {
      tft.setAddrWindow(x + blockStart, y + j, blockLen, 1);
      tft.pushColors(blockBuffer, blockLen);
    }
  }
  
  tft.endWrite();
}

// ===== Optimized sprite drawing with clipping =====
void drawSpriteOptimized(int x, int y, int w, int h, const uint16_t *img) {
  // Only draw if visible on screen
  if (x + w < 0 || x > 320 || y + h < 0 || y > 240) return;
  
  // Use the FASTEST method (block-based)
  drawTransparentSpriteBlock(x, y, w, h, img);
}

// ===== Obstacles =====
void initObstacles() {
  for (int i = 0; i < MAX_OBS; i++) {
    obstacles[i].active = false;
    obstacles[i].prevX = 0;
  }
}

void spawnObstacle() {
  // Check if there's already an obstacle too close
  for (int i = 0; i < MAX_OBS; i++) {
    if (obstacles[i].active && obstacles[i].x > (320 - SCREEN_W/2)) {
      return; // Too close, don't spawn
    }
  }
  
  for (int i = 0; i < MAX_OBS; i++) {
    if (!obstacles[i].active) {
      obstacles[i].w = 34;
      obstacles[i].h = 27;
      obstacles[i].x = 320;
      obstacles[i].prevX = 320;
      obstacles[i].y = 180 - 32;
      obstacles[i].active = true;
      
      // Set random next spawn time with minimum gap
      lastObstacleTime = millis();
      break;
    }
  }
}

void updateObstacles() {
  // Update and draw obstacles
  for (int i = 0; i < MAX_OBS; i++) {
    if (obstacles[i].active) {
      // Only erase and move if game is not over
      if (!gameOver) {
        // Erase only the previous position (not full width)
        tft.fillRect(obstacles[i].prevX, obstacles[i].y + 5, obstacles[i].w, obstacles[i].h, 0x12AE);
        
        // Store previous position
        obstacles[i].prevX = obstacles[i].x;
        
        // Move
        obstacles[i].x -= 15;

        // Deactivate if off-screen
        if (obstacles[i].x + obstacles[i].w < 0) {
          obstacles[i].active = false;
          continue;
        }
      }
      
      // Draw obstacle at new position
      drawSpriteOptimized(obstacles[i].x, obstacles[i].y + 5, obstacles[i].w, obstacles[i].h, ob0);
    }
  }
}

// ===== Optimized Collision with early exit =====
bool checkCollision() {
  int playerRight = posX + SPRITE_W - 10;
  int playerBottom = posY + SPRITE_H;
  
  for (int i = 0; i < MAX_OBS; i++) {
    if (!obstacles[i].active) continue;

    int obsX = obstacles[i].x;
    int obsY = obstacles[i].y;
    int obsW = obstacles[i].w;
    int obsH = obstacles[i].h;
    
    // AABB collision with early rejection
    if (playerRight > obsX && 
        posX < obsX + obsW && 
        playerBottom > obsY + obsH && 
        posY < obsY + obsH + obstacles[i].h) {
      return true;
    }
  }
  return false;
}

// ===== Animation =====
inline void updateAnimation() {
  int currentDelay = jump ? jumpFrameDelay : runFrameDelay;
  if (millis() - lastFrameTime > currentDelay) {
    frame = (frame + 1) % FRAME_COUNT;
    lastFrameTime = millis();
  }
}

// ===== Play Nokia Ringtone =====
void playNokiaTone() {
  if (playedTone) return;
  playedTone = true;

  const int melody[] = {659, 659, 0, 659, 0, 523, 659, 0, 784};
  const int duration[] = {125, 125, 125, 125, 125, 125, 125, 125, 250};

  for (int i = 0; i < 9; i++) {
    if (melody[i] == 0) {
      noTone(BUZZER_PIN);
    } else {
      tone(BUZZER_PIN, melody[i]);
    }
    delay(duration[i]);
  }
  noTone(BUZZER_PIN);
}

// ===== PAUSE SCREEN =====
void showRunPauseScreen() {
  // Semi-transparent overlay effect
  for (int y = 0; y < 240; y += 4) {
    tft.drawFastHLine(0, y, 320, 0x2104);
  }
  
  // Pause box
  int boxW = 200;
  int boxH = 100;
  int boxX = (320 - boxW) / 2;
  int boxY = (240 - boxH) / 2;
  
  tft.fillRect(boxX, boxY, boxW, boxH, 0x12AE);
  tft.drawRect(boxX, boxY, boxW, boxH, TFT_CYAN);
  tft.drawRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, TFT_CYAN);
  tft.drawRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, 0x07FF);
  
  // Pause icon (two vertical bars)
  int iconX = boxX + boxW/2 - 15;
  int iconY = boxY + 15;
  tft.fillRect(iconX, iconY, 10, 25, TFT_CYAN);
  tft.fillRect(iconX + 20, iconY, 10, 25, TFT_CYAN);
  
  // Text
  tft.setTextFont(4);
  tft.setTextColor(TFT_CYAN);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("PAUSED", 160, boxY + 55);
  
  tft.setTextFont(2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Press SHOOT to Resume", 160, boxY + 75);
  
  tft.setTextFont(1);
  tft.setTextColor(TFT_ORANGE);
  tft.drawString("Press UP for Menu", 160, boxY + 90);
}

// ===== MENU SCREEN =====
void showRunMenu() {
  tft.fillScreen(0x12AE);
  
  initMenuStars();
  
  // Draw stars
  for (int i = 0; i < 20; i++) {
    tft.drawPixel(menuStars[i].x, menuStars[i].y, menuStars[i].color);
  }
  
  // Draw animated runner preview
  static int previewFrame = 0;
  static unsigned long lastPreviewUpdate = 0;
  if (millis() - lastPreviewUpdate > 150) {
    previewFrame = (previewFrame + 1) % FRAME_COUNT;
    lastPreviewUpdate = millis();
  }
  drawSpriteOptimized(130, 50, SPRITE_W, SPRITE_H, runFrames[previewFrame]);
  
  // Title with shadow
  tft.setTextFont(4);
  tft.setTextColor(0x4208);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("ENDLESS RUNNER", 162, 152);
  
  tft.setTextColor(TFT_CYAN);
  tft.drawString("ENDLESS RUNNER", 160, 150);
  
  // Instructions box
  tft.drawRect(30, 170, 260, 50, TFT_YELLOW);
  tft.drawRect(31, 171, 258, 48, 0x4208);
  
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Press UP to JUMP", 160, 185);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("Avoid obstacles!", 160, 205);
  
  // High score
  if (runHighScore > 0) {
    tft.setTextFont(2);
    tft.setTextColor(TFT_ORANGE);
    tft.drawString("HIGH SCORE: " + String(runHighScore), 160, 10);
  }
  
  // Start prompt with blink
  static bool blinkState = false;
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 500) {
    blinkState = !blinkState;
    lastBlink = millis();
  }
  
  if (blinkState) {
    tft.setTextFont(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("PRESS ANY KEY TO START!", 160, 228);
  }
  
  Serial.println("Run game menu displayed");
}

// ===== SETUP =====
void run_setup() {
  tft.init();
  tft.setRotation(3);
  tft.setSwapBytes(true);

  // Load high score
  preferences.begin("runner", false);
  runHighScore = preferences.getInt("highscore", 0);
  preferences.end();
  
  // If in menu state, show menu
  if (runGameState == RUN_MENU) {
    showRunMenu();
    return;
  }

  // Background
  tft.fillScreen(0x12AE);

  // Draw floor once using fast method
  tft.startWrite();
  for (int i = 0; i < 10; i++) {
    drawTransparentSpriteBlock(i * 32, 180, 32, 32, floorFrame0);
    drawTransparentSpriteBlock(i * 32, 212, 32, 32, floorFrame1);
  }
  tft.endWrite();
  floorDrawn = true;

  // Initialize obstacles
  initObstacles();
  
  // Reset game state
  prevScore = -1;
  gameOver = false;
  playedTone = false;
}

// ===== GAME OVER SCREEN =====
void showRunGameOver() {
  static bool gameOverDrawn = false;
  
  if (!gameOverDrawn) {
    // Update high score
    if (score > runHighScore) {
      runHighScore = score;
      preferences.begin("runner", false);
      preferences.putInt("highscore", runHighScore);
      preferences.end();
    }
    
    // Semi-transparent overlay
    for (int y = 0; y < 240; y += 4) {
      tft.drawFastHLine(0, y, 320, 0x2104);
    }
    
    // Game over box
    tft.fillRect(60, 60, 200, 120, 0x12AE);
    tft.drawRect(60, 60, 200, 120, TFT_RED);
    tft.drawRect(61, 61, 198, 118, TFT_RED);
    tft.drawRect(62, 62, 196, 116, TFT_ORANGE);
    
    // Title
    tft.setTextFont(4);
    tft.setTextColor(0x4208);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("GAME OVER", 162, 82);
    
    tft.setTextColor(TFT_RED);
    tft.drawString("GAME OVER", 160, 80);
    
    // Score
    tft.setTextFont(2);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("SCORE: " + String(score), 160, 110);
    
    // High score
    if (score >= runHighScore) {
      tft.setTextColor(TFT_GREEN);
      tft.drawString("NEW HIGH SCORE!", 160, 130);
    } else {
      tft.setTextColor(TFT_WHITE);
      tft.drawString("HIGH: " + String(runHighScore), 160, 130);
    }
    
    // Options
    tft.setTextFont(1);
    tft.setTextColor(TFT_CYAN);
    tft.drawString("UP: Retry", 160, 150);
    tft.setTextColor(TFT_ORANGE);
    tft.drawString("DOWN: Main Menu", 160, 165);
    
    gameOverDrawn = true;
  }
  
  playNokiaTone();
  
  // Handle input
  if (command == 0) { // SHOOT - Retry
    command = -1;
    gameOverDrawn = false;
    runGameState = RUN_PLAYING;
    
    // Reset game
    score = 0;
    posY = 100;
    velocityY = 0;
    jump = false;
    gameOver = false;
    playedTone = false;
    initObstacles();
    lastObstacleTime = millis();
    
    // Redraw game
    run_setup();
  }
  
  if (command == 2) { // UP - Main menu
    command = -1;
    gameOverDrawn = false;
    esp_restart();
  }
}



// ===== MAIN LOOP =====
void run_loop() {
  switch (runGameState) {
    case RUN_MENU:
      {
        // Animate menu
        static unsigned long lastMenuUpdate = 0;
        if (millis() - lastMenuUpdate > 100) {
          updateMenuStars();
          
          // Redraw animated runner
          static int previewFrame = 0;
          static unsigned long lastPreviewUpdate = 0;
          if (millis() - lastPreviewUpdate > 150) {
            // Clear previous frame
            tft.fillRect(130, 50, SPRITE_W, SPRITE_H, 0x12AE);
            previewFrame = (previewFrame + 1) % FRAME_COUNT;
            drawSpriteOptimized(130, 50, SPRITE_W, SPRITE_H, runFrames[previewFrame]);
            lastPreviewUpdate = millis();
          }
          
          // Blink text
          static bool blinkState = false;
          static unsigned long lastBlink = 0;
          if (millis() - lastBlink > 500) {
            blinkState = !blinkState;
            lastBlink = millis();
            
            tft.fillRect(60, 220, 200, 20, 0x12AE);
            if (blinkState) {
              tft.setTextFont(1);
              tft.setTextColor(TFT_WHITE);
              tft.setTextDatum(MC_DATUM);
              tft.drawString("PRESS ANY KEY TO START!", 160, 228);
            }
          }
          
          lastMenuUpdate = millis();
        }
        
        // Wait for any key to start
        if (command >= 0 && command != -1) {
          Serial.println("Starting game...");
          runGameState = RUN_PLAYING;
          command = -1;
          run_setup();
        }
        break;
      }
      
    case RUN_PLAYING:
      {
        // Handle pause toggle
        if (command == 4) { // SHOOT - Pause
          unsigned long currentTime = millis();
          if (currentTime - lastPauseToggle >= PAUSE_DEBOUNCE) {
            runGameState = RUN_PAUSED;
            lastPauseToggle = currentTime;
            showRunPauseScreen();
            tone(BUZZER_PIN, 1000, 100); // Pause sound
            Serial.println("Game paused");
          }
          command = -1;
          break;  // Don't process game loop this frame
        }
        
        // Only update score display if changed
        if (score != prevScore) {
          tft.fillRect(0, 0, 320, 35, TFT_BLACK);
          tft.setTextColor(TFT_WHITE);
          tft.setCursor(100, 23);
          tft.setFreeFont(FF21);
          tft.print("SCORE ");
          tft.setTextColor(TFT_BLUE);
          tft.print(score);
          prevScore = score;
        }

        // Jump input
        if (command == 0 && !jump) {
          velocityY = jumpStrength;
          jump = true;
          startJumpTone();
        }
        command = -1;

        updateBuzzer();

        // Physics
        velocityY += gravity;
        posY += velocityY;
        if (posY >= 100) {
          posY = 100;
          velocityY = 0;
          jump = false;
        }

        // Animation
        updateAnimation();

        // Erase previous frame
        tft.fillRect(prevX, (int)prevY, SPRITE_W, SPRITE_H, 0x12AE);

        // Get current frame from array
        const uint16_t *currentFrame = jump ? jumpFrames[frame] : runFrames[frame];

        // Increment score only on frame 0 of run cycle
        if (!jump && frame == 0) score++;

        // Draw player at new position
        drawSpriteOptimized(posX, (int)posY, SPRITE_W, SPRITE_H, currentFrame);

        // Store current state for next frame
        prevX = posX;
        prevY = posY;
        prevFrame = frame;
        prevJump = jump;

        // Spawn obstacles with random timing (minimum gap enforced in spawnObstacle)
        int randomDelay = random(1000, 2500); // Random delay between 1-2.5 seconds
        if (millis() - lastObstacleTime > randomDelay) {
          spawnObstacle();
        }

        // Update & render obstacles
        updateObstacles();

        // Collision check
        if (checkCollision()) {
          runGameState = RUN_GAMEOVER;
          gameOver = true;
        }
        break;
      }
      
    case RUN_PAUSED:
      {
        // Handle pause input
        if (command == 4) { // SHOOT - Resume
          unsigned long currentTime = millis();
          if (currentTime - lastPauseToggle >= PAUSE_DEBOUNCE) {
            runGameState = RUN_PLAYING;
            lastPauseToggle = currentTime;
            
            Serial.println("Resuming game");
            
            // Redraw the game screen
            tft.fillScreen(0x12AE);
            
            // Redraw floor
            tft.startWrite();
            for (int i = 0; i < 10; i++) {
              drawTransparentSpriteBlock(i * 32, 180, 32, 32, floorFrame0);
              drawTransparentSpriteBlock(i * 32, 212, 32, 32, floorFrame1);
            }
            tft.endWrite();
            
            // Force redraw of everything
            prevScore = -1; // Force score redraw
            
            tone(BUZZER_PIN, 1200, 100); // Resume sound
          }
          command = -1;
        }
        
        // Check for menu return
        if (command == 0) { // UP - Main menu
          Serial.println("Returning to menu from pause");
          command = -1;
          esp_restart();
        }
        
        // Clear any other inputs while paused
        if (command != -1 && command != 4 && command != 0) {
          command = -1;
        }
        break;
      }
      
    case RUN_GAMEOVER:
      {
        showRunGameOver();
        break;
      }
  }
}