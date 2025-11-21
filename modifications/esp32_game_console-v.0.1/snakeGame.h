#include <sys/_stdint.h>
#include <TFT_eSPI.h>
#include <Preferences.h>

// -----------------------------------------------------------------
// 1. GAME CONSTANTS
// -----------------------------------------------------------------
#define SNAKE_BLOCK_SIZE 10
#define BACKGROUND_COLOR TFT_BLACK
#define GREEN1 0x07E0  // Bright green
#define GREEN2 0x0540  // Medium green
#define SNAKE_HEAD_COLOR 0x07FF  // Cyan for head

// Calculated grid size
#define SNAKE_COLS (SCREEN_W / SNAKE_BLOCK_SIZE)
#define SNAKE_ROWS ((SCREEN_H-40) / SNAKE_BLOCK_SIZE)

// Maximum length of the snake
const int MAX_LENGTH = SNAKE_COLS * SNAKE_ROWS;
int iter = 1;

// Big food constants
#define BIG_FOOD_SCORE_MULTIPLIER 5
#define BIG_FOOD_COLOR TFT_YELLOW
#define BIG_FOOD_DURATION_MS 7000

// -----------------------------------------------------------------
// 2. DATA STRUCTURES
// -----------------------------------------------------------------
enum Direction {
  UP,
  DOWN,
  LEFT,
  RIGHT,
  IDLE,
  PUSH
};

struct Vec2 {
  int x;
  int y;
};

// Particle effects
struct SnakeParticle {
  float x, y;
  float speedX, speedY;
  uint16_t color;
  int life;
};

// Background stars
struct SnakeStar {
  int x, y;
  int speed;
  uint16_t color;
};

// -----------------------------------------------------------------
// 3. GAME STATE VARIABLES
// -----------------------------------------------------------------
Vec2 snakeBody[MAX_LENGTH];
int snakeLength = 3;
Direction currentDirection = RIGHT;
Direction lastDirection = RIGHT;
Vec2 foodPos;
bool gameOverSnake = false;
bool gameStarted = false;
bool gamePaused = false;
int highScore = 0;

// Timing
unsigned long lastMoveTime = 0;
int gameSpeed_ms = 250;

// Big food
Vec2 bigFoodPos = {-1, -1};
unsigned long bigFoodSpawnTime = 0; 
int bigFoodBonus = 0;

// Visual effects
SnakeParticle particles[30];
int particleCount = 0;
SnakeStar stars[40];
unsigned long lastStarUpdate = 0;

// Animation
int foodPulse = 0;
int foodPulseDir = 1;
int bigFoodPulse = 0;

// -----------------------------------------------------------------
// 4. FUNCTION PROTOTYPES
// -----------------------------------------------------------------
void setupSnake();
void loopSnake();
void showSnakeMenu();
void initGame();
void generateFood();
void drawBlock(Vec2 pos, uint16_t color);
void handleInput();
void moveSnake();
bool checkSelfCollision(Vec2 head);
void drawScore();
void showSnakeGameOver();
void setupBuzzer();
void playFoodEatenSound();
void playNokiaSweepSound();
void initStars();
void updateStars();
void createFoodParticles(int x, int y, uint16_t color);
void updateParticles();
void drawSnakeHead();
void drawFood();
void drawBigFood();
void drawPauseScreen();

// =================================================================
//                         SOUND IMPLEMENTATIONS
// =================================================================
void setupBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void playFoodEatenSound() {
  tone(BUZZER_PIN, 800, 50);
  delay(50);
  noTone(BUZZER_PIN);
}

void playNokiaSweepSound() {
  const int startFreq = 2000;
  const int endFreq = 500;
  const int stepSize = 50;
  const int del = 200;
  
  for (int freq = startFreq; freq >= endFreq; freq -= stepSize) {
    tone(BUZZER_PIN, freq);
    delayMicroseconds(del); 
  }
  noTone(BUZZER_PIN); 
}

// =================================================================
//                         VISUAL EFFECTS
// =================================================================
void initStars() {
  for (int i = 0; i < 40; i++) {
    stars[i].x = random(SCREEN_W);
    stars[i].y = random(40, SCREEN_H);
    stars[i].speed = random(1, 3);
    int brightness = random(3);
    if (brightness == 0) stars[i].color = 0x4208;
    else if (brightness == 1) stars[i].color = 0x8410;
    else stars[i].color = 0xFFFF;
  }
}

void updateStars() {
  for (int i = 0; i < 40; i++) {
    uint16_t color = (((int)particles[i].x / 10) + ((int)particles[i].x / 10))%2 == 0 ? BG_COLOR : TILE_COLOR;
    tft.drawPixel(stars[i].x, stars[i].y, color);
    
    stars[i].y += stars[i].speed;
    
    if (stars[i].y >= SCREEN_H) {
      stars[i].y = 40;
      stars[i].x = random(SCREEN_W);
    }
    
    tft.drawPixel(stars[i].x, stars[i].y, stars[i].color);
  }
}

void createFoodParticles(int x, int y, uint16_t color) {
  particleCount = 0;
  for (int i = 0; i < 15; i++) {
    particles[i].x = x;
    particles[i].y = y;
    float angle = (i / 15.0) * 6.28;
    particles[i].speedX = cos(angle) * 3;
    particles[i].speedY = sin(angle) * 3;
    particles[i].color = color;
    particles[i].life = 20;
    particleCount++;
  }
}

void updateParticles() {
  for (int i = 0; i < particleCount; i++) {
    if (particles[i].life > 0) {
      uint16_t color = (((int)particles[i].x / 10) + ((int)particles[i].x / 10))%2 == 0 ? BG_COLOR : TILE_COLOR;
      tft.drawPixel((int)particles[i].x, (int)particles[i].y, color);
      
      particles[i].x += particles[i].speedX;
      particles[i].y += particles[i].speedY;
      particles[i].speedY += 0.2;
      particles[i].life--;
      
      if (particles[i].life > 0 && 
          particles[i].x >= 0 && particles[i].x < SCREEN_W &&
          particles[i].y >= 40 && particles[i].y < SCREEN_H) {
        tft.drawPixel((int)particles[i].x, (int)particles[i].y, particles[i].color);
      }
    }
  }
}

// =================================================================
//                         PAUSE SCREEN
// =================================================================
void drawPauseScreen() {
  // Semi-transparent overlay effect (draw lines with spacing)
  for (int y = 40; y < SCREEN_H; y += 4) {
    tft.drawFastHLine(0, y, SCREEN_W, 0x2104); // Dark gray
  }
  
  // Pause box
  int boxW = 180;
  int boxH = 100;
  int boxX = (SCREEN_W - boxW) / 2;
  int boxY = (SCREEN_H - boxH) / 2;
  
  tft.fillRect(boxX, boxY, boxW, boxH, BACKGROUND_COLOR);
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
  tft.drawString("PAUSED", SCREEN_W/2, boxY + 55);
  
  tft.setTextFont(1);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Press SHOOT to Resume", SCREEN_W/2, boxY + 75);
  
  tft.setTextColor(TFT_ORANGE);
  tft.drawString("Press UP for Menu", SCREEN_W/2, boxY + 88);
}

// =================================================================
//                         MENU SYSTEM
// =================================================================
void showSnakeMenu() {
  tft.fillScreen(BACKGROUND_COLOR);
  
  // Initialize stars
  initStars();
  
  // Draw stars
  for (int i = 0; i < 40; i++) {
    tft.drawPixel(stars[i].x, stars[i].y, stars[i].color);
  }
  
  // Draw animated snake at top
  int snakeY = 15;
  for (int i = 0; i < 8; i++) {
    int x = 50 + i * 12;
    uint16_t color = (i == 0) ? SNAKE_HEAD_COLOR : (i % 2 == 0 ? GREEN1 : GREEN2);
    tft.fillRoundRect(x, snakeY, 10, 10, 2, color);
    if (i == 0) {
      // Eyes on head
      tft.fillCircle(x + 3, snakeY + 3, 1, TFT_WHITE);
      tft.fillCircle(x + 7, snakeY + 3, 1, TFT_WHITE);
    }
  }
  
  // Draw food next to snake
  tft.fillCircle(180, snakeY + 5, 4, TFT_RED);
  tft.drawCircle(180, snakeY + 5, 5, TFT_ORANGE);
  
  // Title with shadow
  tft.setTextFont(4);
  tft.setTextColor(0x4208);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("SNAKE GAME", SCREEN_W / 2 + 2, 52);
  
  tft.setTextColor(TFT_GREEN);
  tft.drawString("SNAKE GAME", SCREEN_W / 2, 50);
  
  // Decorative lines
  tft.drawFastHLine(40, 75, SCREEN_W - 80, TFT_GREEN);
  tft.drawFastHLine(40, 77, SCREEN_W - 80, 0x07FF);
  
  // Instructions box
  tft.drawRect(20, 85, SCREEN_W - 40, 125, TFT_GREEN);
  tft.drawRect(21, 86, SCREEN_W - 42, 123, 0x0540);
  
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.setCursor(30, 95);
  tft.println("HOW TO PLAY:");
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(30, 110);
  tft.println("Arrow Keys: Control snake direction");
  
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(30, 123);
  tft.println("SHOOT: Pause/Resume game");
  
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(30, 138);
  tft.println("Red Food: +1 point, grow snake");
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(30, 151);
  tft.println("Yellow Food: +5 bonus (7sec timer)");
  
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(30, 169);
  tft.println("RULES:");
  tft.setCursor(30, 182);
  tft.println("- Don't hit yourself!");
  tft.setCursor(30, 195);
  tft.println("- Screen wraps around edges");
  
  // High score
  if (highScore > 0) {
    tft.setTextColor(TFT_ORANGE);
    tft.setTextDatum(TC_DATUM);
    tft.setTextFont(2);
    tft.setCursor(SCREEN_W/2 - 50, 213);
    tft.print("HIGH SCORE: ");
    tft.print(highScore);
  }
  
  // Start prompt with animation
  tft.setTextFont(1);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("PRESS ANY KEY TO START!", SCREEN_W / 2, 230);
  
  Serial.println("Snake menu displayed");
}

// =================================================================
//                         SETUP
// =================================================================
void setupSnake() {
  tft.init();
  tft.setRotation(3);
  tft.setTextFont(2);
  
  setupBuzzer();
  
  // Load high score
  preferences.begin("snake", false);
  highScore = preferences.getInt("highscore", 0);
  preferences.end();
  
  initStars();
  showSnakeMenu();
}

void initGame() {
  gameOverSnake = false;
  gamePaused = false;
  snakeLength = 3;
  currentDirection = RIGHT;
  lastDirection = RIGHT;
  score = 0;
  bigFoodPos = {-1, -1};
  gameSpeed_ms = 250;
  particleCount = 0;
  
  // Draw checkerboard background
  for(int i = 0; i < SNAKE_COLS; i++) {
    for(int j = 0; j < SNAKE_ROWS; j++) {
      drawBlock({i, j}, (i + j) % 2 == 0 ? BG_COLOR : TILE_COLOR);
    }
  }
  
  // Initial snake position
  int start_x = SNAKE_COLS / 4;
  int start_y = SNAKE_ROWS / 2;
  
  snakeBody[0] = {start_x, start_y};
  snakeBody[1] = {start_x - 1, start_y};
  snakeBody[2] = {start_x - 2, start_y};
  
  // Draw initial snake
  for (int i = 0; i < snakeLength; i++) {
    if (i == 0) {
      drawBlock(snakeBody[i], SNAKE_HEAD_COLOR);
    } else {
      drawBlock(snakeBody[i], i % 2 == 0 ? GREEN1 : GREEN2);
    }
  }
  
  generateFood();
  drawScore();
  lastMoveTime = millis();
}

// =================================================================
//                         DRAWING FUNCTIONS
// =================================================================
void drawBlock(Vec2 pos, uint16_t color) {
  tft.fillRect(
    pos.x * SNAKE_BLOCK_SIZE,
    pos.y * SNAKE_BLOCK_SIZE + 40,
    SNAKE_BLOCK_SIZE,
    SNAKE_BLOCK_SIZE,
    color);
}

void drawSnakeHead() {
  Vec2 head = snakeBody[0];
  int x = head.x * SNAKE_BLOCK_SIZE;
  int y = head.y * SNAKE_BLOCK_SIZE + 40;
  
  // Head with color
  tft.fillRect(x, y, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, SNAKE_HEAD_COLOR);
  
  // Eyes based on direction
  int eye1X = x + 3, eye1Y = y + 3;
  int eye2X = x + 7, eye2Y = y + 3;
  
  switch(currentDirection) {
    case UP:
      eye1Y = y + 2; eye2Y = y + 2;
      break;
    case DOWN:
      eye1Y = y + 7; eye2Y = y + 7;
      break;
    case LEFT:
      eye1X = x + 2; eye2X = x + 2;
      eye1Y = y + 3; eye2Y = y + 6;
      break;
    case RIGHT:
      eye1X = x + 7; eye2X = x + 7;
      eye1Y = y + 3; eye2Y = y + 6;
      break;
  }
  
  tft.fillCircle(eye1X, eye1Y, 1, TFT_WHITE);
  tft.fillCircle(eye2X, eye2Y, 1, TFT_WHITE);
}

void drawFood() {
  int x = foodPos.x * SNAKE_BLOCK_SIZE;
  int y = foodPos.y * SNAKE_BLOCK_SIZE + 40;
  
  // Pulsing animation (brightness, not size)
  foodPulse += foodPulseDir;
  if (foodPulse > 2) foodPulseDir = -1;
  if (foodPulse < 0) foodPulseDir = 1;
  
  // Draw food filling the entire grid cell
  tft.fillRect(x, y, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, TFT_RED);
  
  // Add visual detail with alternating brightness
  if (foodPulse > 1) {
    // Brighter center
    tft.fillRect(x + 2, y + 2, SNAKE_BLOCK_SIZE - 4, SNAKE_BLOCK_SIZE - 4, TFT_ORANGE);
    tft.fillRect(x + 4, y + 4, SNAKE_BLOCK_SIZE - 8, SNAKE_BLOCK_SIZE - 8, TFT_RED);
  }
  
  // Border for definition
  tft.drawRect(x, y, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, TFT_ORANGE);
}

void drawBigFood() {
  if (bigFoodPos.x == -1) return;
  
  int x = bigFoodPos.x * SNAKE_BLOCK_SIZE;
  int y = bigFoodPos.y * SNAKE_BLOCK_SIZE + 40;
  
  // Animated big food (brightness cycling)
  bigFoodPulse = (millis() / 100) % 3;
  
  // Fill entire grid cell with yellow
  tft.fillRect(x, y, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, BIG_FOOD_COLOR);
  
  // Add star/diamond pattern within the grid
  if (bigFoodPulse > 0) {
    // Draw brighter inner diamond
    tft.fillTriangle(x + 5, y, x + 10, y + 5, x + 5, y + 10, TFT_ORANGE);
    tft.fillTriangle(x + 5, y, x, y + 5, x + 5, y + 10, TFT_ORANGE);
  }
  
  // Border
  tft.drawRect(x, y, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, TFT_ORANGE);
  
  // Timer indicator bar at bottom (within grid cell)
  unsigned long elapsed = millis() - bigFoodSpawnTime;
  int barWidth = map(BIG_FOOD_DURATION_MS - elapsed, 0, BIG_FOOD_DURATION_MS, 0, SNAKE_BLOCK_SIZE);
  
  if (barWidth > 0) {
    // Background for timer bar
    tft.fillRect(x, y + SNAKE_BLOCK_SIZE - 3, SNAKE_BLOCK_SIZE, 3, TFT_BLACK);
    // Actual timer bar
    tft.fillRect(x, y + SNAKE_BLOCK_SIZE - 3, barWidth, 3, TFT_RED);
  }
}

void drawScore() {
  tft.fillRect(0, 0, SCREEN_W, 40, BACKGROUND_COLOR);
  
  // Score with glow effect
  tft.setTextFont(4);
  tft.setTextColor(0x4208);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("SCORE", SCREEN_W/2 - 40 + 1, 21);
  
  tft.setTextColor(TFT_GREEN);
  tft.drawString("SCORE", SCREEN_W/2 - 40, 20);
  
  tft.setTextColor(0x4208);
  tft.drawString(String(score), SCREEN_W/2 + 40 + 1, 21);
  
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(String(score), SCREEN_W/2 + 40, 20);
  
  // Decorative line
  tft.drawFastHLine(0, 39, SCREEN_W, 0x07FF);
  
  tft.setTextFont(1);
}

// =================================================================
//                         GAME LOGIC
// =================================================================
void generateFood() {
  // Clear big food
  if (bigFoodPos.x != -1) {
    drawBlock(bigFoodPos, (bigFoodPos.x + bigFoodPos.y) % 2 == 0 ? BG_COLOR : TILE_COLOR);
    bigFoodPos = {-1, -1};
    bigFoodSpawnTime = 0;
  }
  
  // 10% chance for big food
  if (random(10) == 0 && snakeLength >= 5) { 
    bool placed = false;
    while (!placed) {
      bigFoodPos.x = random(0, SNAKE_COLS);
      bigFoodPos.y = random(0, SNAKE_ROWS);
      
      bool overlap = false;
      for (int i = 0; i < snakeLength; i++) {
        if (bigFoodPos.x == snakeBody[i].x && bigFoodPos.y == snakeBody[i].y) {
          overlap = true;
          break;
        }
      }
      
      if (!overlap && (bigFoodPos.x != foodPos.x || bigFoodPos.y != foodPos.y)) {
        bigFoodSpawnTime = millis();
        bigFoodBonus = 5 + (MAX_LENGTH - snakeLength) / 5; 
        placed = true;
      }
    }
  } 
  else {
    bool placed = false;
    while (!placed) {
      foodPos.x = random(0, SNAKE_COLS);
      foodPos.y = random(0, SNAKE_ROWS);
      
      bool overlap = false;
      for (int i = 0; i < snakeLength; i++) {
        if (foodPos.x == snakeBody[i].x && foodPos.y == snakeBody[i].y) {
          overlap = true;
          break;
        }
      }
      
      if (!overlap) {
        placed = true;
      }
    }
  }
}

void handleInput() {
  Direction intendedDirection = currentDirection;
  
  if (command == 0) {
    intendedDirection = UP;
    command = -1;
  }
  else if (command == 2) {
    intendedDirection = DOWN;
    command = -1;
  }
  else if (command == 3) {
    intendedDirection = LEFT;
    command = -1;
  }
  else if (command == 1) {
    intendedDirection = RIGHT;
    command = -1;
  }
  
  bool isReverse = (intendedDirection == UP && lastDirection == DOWN) ||
                   (intendedDirection == DOWN && lastDirection == UP) ||
                   (intendedDirection == LEFT && lastDirection == RIGHT) ||
                   (intendedDirection == RIGHT && lastDirection == LEFT);
  
  if (!isReverse) {
    currentDirection = intendedDirection;
  }
}

void moveSnake() {
  Vec2 oldTail = snakeBody[snakeLength - 1];
  
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeBody[i] = snakeBody[i - 1];
  }
  
  Vec2 newHead = snakeBody[0];
  
  switch (currentDirection) {
    case UP: newHead.y--; break;
    case DOWN: newHead.y++; break;
    case LEFT: newHead.x--; break;
    case RIGHT: newHead.x++; break;
  }
  
  // Wrapping
  if (newHead.x < 0) newHead.x = SNAKE_COLS - 1;
  if (newHead.x >= SNAKE_COLS) newHead.x = 0;
  if (newHead.y < 0) newHead.y = SNAKE_ROWS - 1;
  if (newHead.y >= SNAKE_ROWS) newHead.y = 0;
  
  snakeBody[0] = newHead;
  lastDirection = currentDirection;
  
  // Self collision
  if (checkSelfCollision(newHead)) {
    gameOverSnake = true;
    return;
  }
  
  // Big food collision
  if (bigFoodPos.x != -1 && newHead.x == bigFoodPos.x && newHead.y == bigFoodPos.y) {
    snakeBody[snakeLength] = oldTail;
    snakeLength += BIG_FOOD_SCORE_MULTIPLIER;
    score += bigFoodBonus;
    
    // Create explosion particles at grid center
    int px = bigFoodPos.x * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE/2;
    int py = bigFoodPos.y * SNAKE_BLOCK_SIZE + 40 + SNAKE_BLOCK_SIZE/2;
    createFoodParticles(px, py, BIG_FOOD_COLOR);
    
    playNokiaSweepSound();
    
    bigFoodPos = {-1, -1};
    bigFoodSpawnTime = 0;
    
    gameSpeed_ms = max(80, gameSpeed_ms - 80);
    drawScore();
    generateFood();
    
    drawSnakeHead();
    iter++;
    return;
  }
  
  // Normal food collision
  if (newHead.x == foodPos.x && newHead.y == foodPos.y) {
    snakeBody[snakeLength] = oldTail;
    snakeLength++;
    score++;
    
    // Create particles at grid center
    int px = foodPos.x * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE/2;
    int py = foodPos.y * SNAKE_BLOCK_SIZE + 40 + SNAKE_BLOCK_SIZE/2;
    createFoodParticles(px, py, TFT_RED);
    
    playFoodEatenSound();
    
    if (gameSpeed_ms > 120) {
      gameSpeed_ms -= 40;
    }
    
    drawScore();
    generateFood();
  } else {
    drawBlock(oldTail, (oldTail.x + oldTail.y) % 2 == 0 ? BG_COLOR : TILE_COLOR);
  }
  
  // Draw snake
  drawSnakeHead();
  for (int i = 1; i < snakeLength; i++) {
    drawBlock(snakeBody[i], i % 2 == 0 ? GREEN1 : GREEN2);
  }
  iter++;
}

bool checkSelfCollision(Vec2 head) {
  for (int i = 1; i < snakeLength; i++) {
    if (head.x == snakeBody[i].x && head.y == snakeBody[i].y) {
      return true;
    }
  }
  return false;
}

// =================================================================
//                         GAME OVER
// =================================================================
void showSnakeGameOver() {
  static bool gameOverDrawn = false;
  
  if (!gameOverDrawn) {
    // Update high score
    if (score > highScore) {
      highScore = score;
      preferences.begin("snake", false);
      preferences.putInt("highscore", highScore);
      preferences.end();
    }
    
    // Game over box
    tft.fillRect(35, 70, 250, 110, BACKGROUND_COLOR);
    tft.drawRect(35, 70, 250, 110, TFT_RED);
    tft.drawRect(36, 71, 248, 108, TFT_RED);
    tft.drawRect(37, 72, 246, 106, TFT_ORANGE);
    
    // Title
    tft.setTextFont(4);
    tft.setTextColor(0x4208);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("GAME OVER", SCREEN_W/2 + 2, 92);
    
    tft.setTextColor(TFT_RED);
    tft.drawString("GAME OVER", SCREEN_W/2, 90);
    
    // Score
    tft.setTextFont(2);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("SCORE: " + String(score), SCREEN_W/2, 115);
    
    tft.setTextFont(1);
    tft.setTextColor(TFT_CYAN);
    tft.drawString("LENGTH: " + String(snakeLength), SCREEN_W/2, 130);
    
    // High score
    if (score >= highScore) {
      tft.setTextColor(TFT_GREEN);
      tft.drawString("NEW HIGH SCORE!", SCREEN_W/2, 145);
    } else {
      tft.setTextColor(TFT_WHITE);
      tft.drawString("HIGH: " + String(highScore), SCREEN_W/2, 145);
    }
    
    // Options
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SHOOT: Retry", SCREEN_W/2, 160);
    tft.setTextColor(TFT_ORANGE);
    tft.drawString("UP: Main Menu", SCREEN_W/2, 172);
    
    gameOverDrawn = true;
  }

  playNokiaTone();
  
  // Handle input
  if (command == 4) { // SHOOT - Retry
    command = -1;
    gameOverDrawn = false;
    gameStarted = true;
    tft.fillScreen(BACKGROUND_COLOR);
    initGame();
  }
  
  if (command == 0) { // UP - Main menu
    command = -1;
    esp_restart();
  }
}

// =================================================================
//                         MAIN LOOP
// =================================================================
void loopSnake() {
  if (!gameStarted) {
    // Wait for any key press to start
    if (command >= 0) {
      command = -1;
      gameStarted = true;
      tft.fillScreen(BACKGROUND_COLOR);
      initGame();
    }
    return;
  }
  
  if (gameOverSnake) {
    showSnakeGameOver();
    return;
  }
  
  // Handle pause toggle
  if (command == 4) { // SHOOT button
    unsigned long currentTime = millis();
    if (currentTime - lastPauseToggle >= PAUSE_DEBOUNCE) {
      gamePaused = !gamePaused;
      lastPauseToggle = currentTime;
      
      if (gamePaused) {
        drawPauseScreen();
        tone(BUZZER_PIN, 1000, 100); // Pause sound
      } else {
        // Resume - redraw the game
        tft.fillRect(0, 40, SCREEN_W, SCREEN_H - 40, BACKGROUND_COLOR);
        
        // Redraw checkerboard
        for(int i = 0; i < SNAKE_COLS; i++) {
          for(int j = 0; j < SNAKE_ROWS; j++) {
            drawBlock({i, j}, (i + j) % 2 == 0 ? BG_COLOR : TILE_COLOR);
          }
        }
        
        // Redraw snake
        for (int i = 0; i < snakeLength; i++) {
          if (i == 0) {
            drawSnakeHead();
          } else {
            drawBlock(snakeBody[i], i % 2 == 0 ? GREEN1 : GREEN2);
          }
        }
        
        // Redraw food
        drawFood();
        if (bigFoodPos.x != -1) {
          drawBigFood();
        }
        
        tone(BUZZER_PIN, 1200, 100); // Resume sound
      }
    }
    command = -1;
  }
  
  // Check for menu return while paused
  if (gamePaused && command == 0) { // UP key
    command = -1;
    esp_restart();
  }
  
  // Exit early if paused
  if (gamePaused) return;
  
  // Big food timer
  if (bigFoodPos.x != -1 && (millis() - bigFoodSpawnTime) >= BIG_FOOD_DURATION_MS) {
    drawBlock(bigFoodPos, (bigFoodPos.x + bigFoodPos.y) % 2 == 0 ? BG_COLOR : TILE_COLOR);
    bigFoodPos = {-1, -1};
  }
  
  // Update animations
  if (millis() - lastStarUpdate > 100) {
    updateStars();
    lastStarUpdate = millis();
  }
  
  updateParticles();
  drawFood();
  drawBigFood();
  
  // Handle input
  handleInput();
  
  // Move snake
  if (millis() - lastMoveTime >= gameSpeed_ms) {
    moveSnake();
    lastMoveTime = millis();
  }
}