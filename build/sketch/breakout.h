#line 1 "/home/pahasara/Arduino/gameConsole/breakout.h"
// Breakout/Arkanoid Game Constants
#define BREAKOUT_PADDLE_WIDTH 50
#define BREAKOUT_PADDLE_HEIGHT 8
#define BREAKOUT_BALL_SIZE 6
#define BREAKOUT_PADDLE_SPEED 8
#define BREAKOUT_BALL_SPEED_INITIAL 3.5
#define BREAKOUT_BRICK_WIDTH 30
#define BREAKOUT_BRICK_HEIGHT 12
#define BREAKOUT_BRICK_ROWS 6
#define BREAKOUT_BRICK_COLS 10
#define BREAKOUT_BRICK_PADDING 2
#define BREAKOUT_MAX_LIVES 3
#define BREAKOUT_POWERUP_SIZE 12
#define BREAKOUT_POWERUP_SPEED 2

// Brick types and point values
#define BRICK_EMPTY 0
#define BRICK_RED 1      // 70 points
#define BRICK_ORANGE 2   // 60 points
#define BRICK_YELLOW 3   // 50 points
#define BRICK_GREEN 4    // 40 points
#define BRICK_CYAN 5     // 30 points
#define BRICK_BLUE 6     // 20 points
#define BRICK_PURPLE 7   // 10 points
#define BRICK_SILVER 8   // 50 points - takes 2 hits
#define BRICK_GOLD 9     // 100 points - indestructible until all others cleared

// Power-up types
#define POWERUP_NONE 0
#define POWERUP_EXPAND 1    // Wider paddle
#define POWERUP_LASER 2     // Shoot lasers
#define POWERUP_MULTI 3     // 3 balls
#define POWERUP_SLOW 4      // Slow ball
#define POWERUP_LIFE 5      // Extra life

// Game variables
int breakout_paddleX = (SCREEN_W - BREAKOUT_PADDLE_WIDTH) / 2;
int breakout_paddleY = SCREEN_H - 25;
int breakout_prevPaddleX = breakout_paddleX;

float breakout_ballX = SCREEN_W / 2;
float breakout_ballY = SCREEN_H - 40;
float breakout_prevBallX = breakout_ballX;
float breakout_prevBallY = breakout_ballY;

float breakout_ballSpeedX = BREAKOUT_BALL_SPEED_INITIAL;
float breakout_ballSpeedY = -BREAKOUT_BALL_SPEED_INITIAL;

int breakout_score = 0;
int breakout_prevScore = -1;
int breakout_lives = BREAKOUT_MAX_LIVES;
int breakout_prevLives = -1;
int breakout_level = 1;
int breakout_highScore = 0;

bool breakout_gameOver = false;
bool breakout_gameStarted = false;
bool breakout_ballLaunched = false;
bool breakout_levelComplete = false;

// Bricks array
uint8_t breakout_bricks[BREAKOUT_BRICK_ROWS][BREAKOUT_BRICK_COLS];
uint8_t breakout_brickHits[BREAKOUT_BRICK_ROWS][BREAKOUT_BRICK_COLS]; // Track hits for multi-hit bricks

// Power-ups
int breakout_activePowerup = POWERUP_NONE;
int breakout_powerupX = -100;
int breakout_powerupY = -100;
int breakout_powerupType = POWERUP_NONE;
bool breakout_powerupActive = false;
unsigned long breakout_powerupTimer = 0;
int breakout_paddleWidth = BREAKOUT_PADDLE_WIDTH;

// Particle effects
struct Particle {
  float x, y;
  float speedX, speedY;
  uint16_t color;
  int life;
};
Particle breakout_particles[20];
int breakout_particleCount = 0;

unsigned long breakout_lastUpdateTime = 0;
unsigned long breakout_frameDelay = 16; // ~60 FPS

// Color definitions with gradient effect
#define BREAKOUT_BG_COLOR BG_COLOR      // Dark blue
#define BREAKOUT_PADDLE_COLOR 0xFFFF   // White
#define BREAKOUT_BALL_COLOR 0xF81F     // Magenta/Pink
#define BREAKOUT_WALL_COLOR 0x4208     // Dark gray

// Brick colors (vibrant palette)
const uint16_t BRICK_COLORS[] = {
  0x0000,           // Empty
  0xF800,           // Red
  0xFD20,           // Orange
  0xFFE0,           // Yellow
  0x07E0,           // Green
  0x07FF,           // Cyan
  0x001F,           // Blue
  0x780F,           // Purple
  0xC618,           // Silver
  0xFEA0            // Gold
};

const int BRICK_POINTS[] = {0, 70, 60, 50, 40, 30, 20, 10, 50, 100};

// Function declarations
void breakout_setup();
void breakout_loop();
void breakout_showMenu();
void breakout_initGame();
void breakout_initLevel();
void breakout_updateGame();
void breakout_handleInput();
void breakout_moveBall();
void breakout_checkCollisions();
void breakout_checkBrickCollision();
void breakout_drawPaddle(int x, uint16_t color);
void breakout_erasePaddle(int x);
void breakout_drawBall(float x, float y, uint16_t color);
void breakout_eraseBall(float x, float y);
void breakout_drawBricks();
void breakout_drawBrick(int row, int col);
void breakout_eraseBrick(int row, int col);
void breakout_drawUI();
void breakout_resetBall();
void breakout_showGameOver();
void breakout_showLevelComplete();
void breakout_drawWalls();
void breakout_spawnPowerup(int x, int y);
void breakout_updatePowerup();
void breakout_drawPowerup();
void breakout_activatePowerup(int type);
void breakout_createParticles(float x, float y, uint16_t color);
void breakout_updateParticles();
void breakout_drawParticles();

void breakout_setup() {
  Serial.begin(115200);
  
  // Initialize TFT display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(BREAKOUT_BG_COLOR);

  // Initialize preferences
  preferences.begin("breakout", false);
  breakout_highScore = preferences.getInt("highscore", 0);
  
  Serial.println("Stylish Breakout Starting...");
  
  breakout_showMenu();
}

void breakout_loop() {
  if (!breakout_gameStarted) {
    if (command == 4) { // SHOOT - Start game
      breakout_gameStarted = true;
      breakout_initGame();
      command = -1;
    }
    return;
  }
  
  if (breakout_gameOver) {
    breakout_showGameOver();
    return;
  }
  
  if (breakout_levelComplete) {
    breakout_showLevelComplete();
    return;
  }
  
  unsigned long currentTime = millis();
  if (currentTime - breakout_lastUpdateTime >= breakout_frameDelay) {
    breakout_handleInput();
    breakout_updateGame();
    breakout_lastUpdateTime = currentTime;
  }
}

void breakout_showMenu() {
  tft.fillScreen(BREAKOUT_BG_COLOR);
  
  // Draw decorative bricks at top
  for (int i = 0; i < 10; i++) {
    tft.fillRect(i * 32, 5, 30, 10, BRICK_COLORS[(i % 6) + 1]);
    tft.drawRect(i * 32, 5, 30, 10, BREAKOUT_BG_COLOR);
  }
  
  // Title with shadow effect
  tft.setTextColor(0x4208);
  tft.setTextSize(4);
  tft.setCursor(52, 42);
  tft.println("BREAKOUT");
  
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(50, 40);
  tft.println("BREAKOUT");
  
  // Subtitle
  tft.setTextSize(1);
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(110, 75);
  tft.println("ARKANOID STYLE");
  
  // Instructions box
  tft.drawRect(30, 95, 260, 95, TFT_CYAN);
  tft.drawRect(31, 96, 258, 93, TFT_CYAN);
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(40, 105);
  tft.println("HOW TO PLAY:");
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(40, 120);
  tft.println("LEFT/RIGHT: Move paddle");
  tft.setCursor(40, 133);
  tft.println("SHOOT: Launch ball");
  
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(40, 153);
  tft.println("Break all bricks to advance!");
  tft.setCursor(40, 166);
  tft.println("Catch power-ups for bonuses");
  
  // High score
  if (breakout_highScore > 0) {
    tft.setTextColor(TFT_ORANGE);
    tft.setTextSize(2);
    tft.setCursor(70, 200);
    tft.print("HIGH SCORE: ");
    tft.print(breakout_highScore);
  }
  
  // Start prompt with blink effect
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(75, 225);
  tft.println("PRESS SHOOT TO START!");
  
  Serial.println("Menu screen drawn");
}

void breakout_initGame() {
  Serial.println("Initializing Breakout...");
  tft.fillScreen(BREAKOUT_BG_COLOR);
  
  breakout_score = 0;
  breakout_prevScore = -1;
  breakout_lives = BREAKOUT_MAX_LIVES;
  breakout_prevLives = -1;
  breakout_level = 1;
  
  breakout_gameOver = false;
  breakout_levelComplete = false;
  
  breakout_initLevel();
}

void breakout_initLevel() {
  breakout_paddleX = (SCREEN_W - BREAKOUT_PADDLE_WIDTH) / 2;
  breakout_paddleWidth = BREAKOUT_PADDLE_WIDTH;
  breakout_ballLaunched = false;
  breakout_powerupActive = false;
  breakout_activePowerup = POWERUP_NONE;
  
  // Clear screen
  tft.fillScreen(BREAKOUT_BG_COLOR);
  
  // Initialize bricks based on level
  for (int row = 0; row < BREAKOUT_BRICK_ROWS; row++) {
    for (int col = 0; col < BREAKOUT_BRICK_COLS; col++) {
      // Create pattern based on level
      if (breakout_level == 1) {
        breakout_bricks[row][col] = (row % 7) + 1;
      } else if (breakout_level == 2) {
        if (row < 2) breakout_bricks[row][col] = BRICK_SILVER;
        else breakout_bricks[row][col] = ((row + col) % 6) + 1;
      } else {
        // Random pattern for level 3+
        breakout_bricks[row][col] = (random(7) + 1);
        if (random(10) > 7) breakout_bricks[row][col] = BRICK_SILVER;
      }
      breakout_brickHits[row][col] = 0;
    }
  }
  
  breakout_drawWalls();
  breakout_drawBricks();
  breakout_drawUI();
  breakout_resetBall();
  breakout_drawPaddle(breakout_paddleX, BREAKOUT_PADDLE_COLOR);
}

void breakout_drawWalls() {
  // Top wall with gradient
  tft.fillRect(0, 0, SCREEN_W, 3, TFT_CYAN);
  tft.fillRect(0, 3, SCREEN_W, 2, 0x4208);
}

void breakout_resetBall() {
  breakout_ballX = breakout_paddleX + breakout_paddleWidth / 2 - BREAKOUT_BALL_SIZE / 2;
  breakout_ballY = breakout_paddleY - BREAKOUT_BALL_SIZE - 1;
  breakout_prevBallX = breakout_ballX;
  breakout_prevBallY = breakout_ballY;
  
  breakout_ballSpeedX = BREAKOUT_BALL_SPEED_INITIAL * (random(2) == 0 ? 1 : -1);
  breakout_ballSpeedY = -BREAKOUT_BALL_SPEED_INITIAL;
  
  breakout_ballLaunched = false;
}

void breakout_handleInput() {
  // Paddle movement
  if (command == 3 && breakout_paddleX > 5) { // LEFT
    breakout_paddleX -= BREAKOUT_PADDLE_SPEED;
    if (breakout_paddleX < 5) breakout_paddleX = 5;
  }
  if (command == 1 && breakout_paddleX < SCREEN_W - breakout_paddleWidth - 5) { // RIGHT
    breakout_paddleX += BREAKOUT_PADDLE_SPEED;
    if (breakout_paddleX > SCREEN_W - breakout_paddleWidth - 5) {
      breakout_paddleX = SCREEN_W - breakout_paddleWidth - 5;
    }
  }
  
  // Launch ball
  if (command == 4 && !breakout_ballLaunched) {
    breakout_ballLaunched = true;
    command = -1;
  }
}

void breakout_updateGame() {
  // Update and draw paddle
  if (breakout_prevPaddleX != breakout_paddleX) {
    breakout_erasePaddle(breakout_prevPaddleX);
    breakout_drawPaddle(breakout_paddleX, BREAKOUT_PADDLE_COLOR);
    breakout_prevPaddleX = breakout_paddleX;
    
    // Move ball with paddle if not launched
    if (!breakout_ballLaunched) {
      breakout_eraseBall(breakout_prevBallX, breakout_prevBallY);
      breakout_ballX = breakout_paddleX + breakout_paddleWidth / 2 - BREAKOUT_BALL_SIZE / 2;
      breakout_prevBallX = breakout_ballX;
      breakout_drawBall(breakout_ballX, breakout_ballY, BREAKOUT_BALL_COLOR);
    }
  }
  
  // Move and draw ball
  if (breakout_ballLaunched) {
    breakout_moveBall();
    breakout_checkCollisions();
  }
  
  // Update power-up
  breakout_updatePowerup();
  
  // Update particles
  breakout_updateParticles();
  
  // Update UI
  breakout_drawUI();
}

void breakout_moveBall() {
  breakout_eraseBall(breakout_prevBallX, breakout_prevBallY);
  
  breakout_ballX += breakout_ballSpeedX;
  breakout_ballY += breakout_ballSpeedY;
  
  breakout_drawBall(breakout_ballX, breakout_ballY, BREAKOUT_BALL_COLOR);
  
  breakout_prevBallX = breakout_ballX;
  breakout_prevBallY = breakout_ballY;
}

void breakout_checkCollisions() {
  // Top wall
  if (breakout_ballY <= 5) {
    breakout_ballSpeedY = abs(breakout_ballSpeedY);
    breakout_ballY = 5;
  }
  
  // Side walls
  if (breakout_ballX <= 0 || breakout_ballX >= SCREEN_W - BREAKOUT_BALL_SIZE) {
    breakout_ballSpeedX = -breakout_ballSpeedX;
    breakout_ballX = constrain(breakout_ballX, 0, SCREEN_W - BREAKOUT_BALL_SIZE);
  }
  
  // Paddle collision
  if (breakout_ballY + BREAKOUT_BALL_SIZE >= breakout_paddleY &&
      breakout_ballY < breakout_paddleY + BREAKOUT_PADDLE_HEIGHT &&
      breakout_ballX + BREAKOUT_BALL_SIZE >= breakout_paddleX &&
      breakout_ballX <= breakout_paddleX + breakout_paddleWidth) {
    
    breakout_ballSpeedY = -abs(breakout_ballSpeedY);
    
    // Add angle based on where ball hits paddle
    float hitPos = (breakout_ballX + BREAKOUT_BALL_SIZE / 2) - (breakout_paddleX + breakout_paddleWidth / 2);
    breakout_ballSpeedX = hitPos / 6.0;
    
    breakout_ballY = breakout_paddleY - BREAKOUT_BALL_SIZE;
  }
  
  // Bottom - lose life
  if (breakout_ballY >= SCREEN_H) {
    breakout_lives--;
    if (breakout_lives <= 0) {
      breakout_gameOver = true;
    } else {
      breakout_resetBall();
      breakout_drawPaddle(breakout_paddleX, BREAKOUT_PADDLE_COLOR);
    }
  }
  
  // Check brick collisions
  breakout_checkBrickCollision();
}

void breakout_checkBrickCollision() {
  int brickStartY = 10;
  int brickStartX = 5;
  
  for (int row = 0; row < BREAKOUT_BRICK_ROWS; row++) {
    for (int col = 0; col < BREAKOUT_BRICK_COLS; col++) {
      if (breakout_bricks[row][col] != BRICK_EMPTY) {
        int brickX = brickStartX + col * (BREAKOUT_BRICK_WIDTH + BREAKOUT_BRICK_PADDING);
        int brickY = brickStartY + row * (BREAKOUT_BRICK_HEIGHT + BREAKOUT_BRICK_PADDING);
        
        // Check collision
        if (breakout_ballX + BREAKOUT_BALL_SIZE >= brickX &&
            breakout_ballX <= brickX + BREAKOUT_BRICK_WIDTH &&
            breakout_ballY + BREAKOUT_BALL_SIZE >= brickY &&
            breakout_ballY <= brickY + BREAKOUT_BRICK_HEIGHT) {
          
          // Determine collision side
          float ballCenterX = breakout_ballX + BREAKOUT_BALL_SIZE / 2;
          float ballCenterY = breakout_ballY + BREAKOUT_BALL_SIZE / 2;
          float brickCenterX = brickX + BREAKOUT_BRICK_WIDTH / 2;
          float brickCenterY = brickY + BREAKOUT_BRICK_HEIGHT / 2;
          
          if (abs(ballCenterX - brickCenterX) > abs(ballCenterY - brickCenterY)) {
            breakout_ballSpeedX = -breakout_ballSpeedX;
          } else {
            breakout_ballSpeedY = -breakout_ballSpeedY;
          }
          
          // Handle brick hit
          uint8_t brickType = breakout_bricks[row][col];
          
          if (brickType == BRICK_SILVER) {
            breakout_brickHits[row][col]++;
            if (breakout_brickHits[row][col] >= 2) {
              breakout_bricks[row][col] = BRICK_EMPTY;
              breakout_score += BRICK_POINTS[brickType];
              breakout_createParticles(brickX + BREAKOUT_BRICK_WIDTH/2, brickY + BREAKOUT_BRICK_HEIGHT/2, BRICK_COLORS[brickType]);
              breakout_eraseBrick(row, col);
              
              // Random power-up spawn
              if (random(10) > 7) {
                breakout_spawnPowerup(brickX, brickY);
              }
            } else {
              // Change color to show damage
              breakout_drawBrick(row, col);
            }
          } else {
            breakout_bricks[row][col] = BRICK_EMPTY;
            breakout_score += BRICK_POINTS[brickType];
            breakout_createParticles(brickX + BREAKOUT_BRICK_WIDTH/2, brickY + BREAKOUT_BRICK_HEIGHT/2, BRICK_COLORS[brickType]);
            breakout_eraseBrick(row, col);
            
            // Random power-up spawn
            if (random(10) > 7) {
              breakout_spawnPowerup(brickX, brickY);
            }
          }
          
          // Check level complete
          bool allCleared = true;
          for (int r = 0; r < BREAKOUT_BRICK_ROWS && allCleared; r++) {
            for (int c = 0; c < BREAKOUT_BRICK_COLS && allCleared; c++) {
              if (breakout_bricks[r][c] != BRICK_EMPTY) {
                allCleared = false;
              }
            }
          }
          
          if (allCleared) {
            breakout_levelComplete = true;
          }
          
          return; // Only process one brick per frame
        }
      }
    }
  }
}

void breakout_drawBricks() {
  for (int row = 0; row < BREAKOUT_BRICK_ROWS; row++) {
    for (int col = 0; col < BREAKOUT_BRICK_COLS; col++) {
      breakout_drawBrick(row, col);
    }
  }
}

void breakout_drawBrick(int row, int col) {
  if (breakout_bricks[row][col] != BRICK_EMPTY) {
    int brickX = 5 + col * (BREAKOUT_BRICK_WIDTH + BREAKOUT_BRICK_PADDING);
    int brickY = 10 + row * (BREAKOUT_BRICK_HEIGHT + BREAKOUT_BRICK_PADDING);
    
    uint16_t color = BRICK_COLORS[breakout_bricks[row][col]];
    
    // Damaged silver bricks show darker
    if (breakout_bricks[row][col] == BRICK_SILVER && breakout_brickHits[row][col] > 0) {
      color = 0x8410; // Darker silver
    }
    
    // Draw brick with 3D effect
    tft.fillRect(brickX, brickY, BREAKOUT_BRICK_WIDTH, BREAKOUT_BRICK_HEIGHT, color);
    
    // Highlight
    tft.drawLine(brickX, brickY, brickX + BREAKOUT_BRICK_WIDTH - 1, brickY, 0xFFFF);
    tft.drawLine(brickX, brickY, brickX, brickY + BREAKOUT_BRICK_HEIGHT - 1, 0xFFFF);
    
    // Shadow
    tft.drawLine(brickX + BREAKOUT_BRICK_WIDTH - 1, brickY + 1, 
                 brickX + BREAKOUT_BRICK_WIDTH - 1, brickY + BREAKOUT_BRICK_HEIGHT - 1, 0x0000);
    tft.drawLine(brickX + 1, brickY + BREAKOUT_BRICK_HEIGHT - 1, 
                 brickX + BREAKOUT_BRICK_WIDTH - 1, brickY + BREAKOUT_BRICK_HEIGHT - 1, 0x0000);
  }
}

void breakout_eraseBrick(int row, int col) {
  int brickX = 5 + col * (BREAKOUT_BRICK_WIDTH + BREAKOUT_BRICK_PADDING);
  int brickY = 10 + row * (BREAKOUT_BRICK_HEIGHT + BREAKOUT_BRICK_PADDING);
  tft.fillRect(brickX, brickY, BREAKOUT_BRICK_WIDTH, BREAKOUT_BRICK_HEIGHT, BREAKOUT_BG_COLOR);
}

void breakout_drawPaddle(int x, uint16_t color) {
  // Draw paddle with gradient effect
  tft.fillRect(x, breakout_paddleY, breakout_paddleWidth, BREAKOUT_PADDLE_HEIGHT, color);
  tft.fillRect(x, breakout_paddleY, breakout_paddleWidth, 2, TFT_CYAN);
  tft.drawRect(x, breakout_paddleY, breakout_paddleWidth, BREAKOUT_PADDLE_HEIGHT, TFT_CYAN);
}

void breakout_erasePaddle(int x) {
  tft.fillRect(x, breakout_paddleY, BREAKOUT_PADDLE_WIDTH + 30, BREAKOUT_PADDLE_HEIGHT, BREAKOUT_BG_COLOR);
}

void breakout_drawBall(float x, float y, uint16_t color) {
  // Draw ball with glow effect
  tft.fillCircle((int)x + BREAKOUT_BALL_SIZE/2, (int)y + BREAKOUT_BALL_SIZE/2, BREAKOUT_BALL_SIZE/2 + 1, 0x8010);
  tft.fillCircle((int)x + BREAKOUT_BALL_SIZE/2, (int)y + BREAKOUT_BALL_SIZE/2, BREAKOUT_BALL_SIZE/2, color);
}

void breakout_eraseBall(float x, float y) {
  tft.fillRect((int)x - 2, (int)y - 2, BREAKOUT_BALL_SIZE + 4, BREAKOUT_BALL_SIZE + 4, BREAKOUT_BG_COLOR);
}

void breakout_drawUI() {
  if (breakout_prevScore != breakout_score) {
    tft.fillRect(5, SCREEN_H - 15, 100, 12, BREAKOUT_BG_COLOR);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(5, SCREEN_H - 15);
    tft.print("SCORE: ");
    tft.print(breakout_score);
    breakout_prevScore = breakout_score;
  }
  
  if (breakout_prevLives != breakout_lives) {
    tft.fillRect(SCREEN_W - 80, SCREEN_H - 15, 75, 12, BREAKOUT_BG_COLOR);
    tft.setTextColor(TFT_RED);
    tft.setCursor(SCREEN_W - 80, SCREEN_H - 15);
    tft.print("LIVES: ");
    for (int i = 0; i < breakout_lives; i++) {
      tft.fillCircle(SCREEN_W - 45 + i * 12, SCREEN_H - 9, 3, TFT_RED);
    }
    breakout_prevLives = breakout_lives;
  }
}

void breakout_spawnPowerup(int x, int y) {
  if (!breakout_powerupActive) {
    breakout_powerupX = x + BREAKOUT_BRICK_WIDTH / 2 - BREAKOUT_POWERUP_SIZE / 2;
    breakout_powerupY = y;
    breakout_powerupType = random(1, 6); // Random power-up type
    breakout_powerupActive = true;
  }
}

void breakout_updatePowerup() {
  if (breakout_powerupActive) {
    // Erase old position
    tft.fillRect(breakout_powerupX - 1, breakout_powerupY - BREAKOUT_POWERUP_SPEED - 1, 
                 BREAKOUT_POWERUP_SIZE + 2, BREAKOUT_POWERUP_SIZE + BREAKOUT_POWERUP_SPEED + 2, 
                 BREAKOUT_BG_COLOR);
    
    breakout_powerupY += BREAKOUT_POWERUP_SPEED;
    
    // Check if caught by paddle
    if (breakout_powerupY + BREAKOUT_POWERUP_SIZE >= breakout_paddleY &&
        breakout_powerupY < breakout_paddleY + BREAKOUT_PADDLE_HEIGHT &&
        breakout_powerupX + BREAKOUT_POWERUP_SIZE >= breakout_paddleX &&
        breakout_powerupX <= breakout_paddleX + breakout_paddleWidth) {
      
      breakout_activatePowerup(breakout_powerupType);
      breakout_powerupActive = false;
    }
    
    // Check if missed
    if (breakout_powerupY > SCREEN_H) {
      breakout_powerupActive = false;
    }
    
    if (breakout_powerupActive) {
      breakout_drawPowerup();
    }
  }
}

void breakout_drawPowerup() {
  uint16_t color = TFT_GREEN;
  switch (breakout_powerupType) {
    case POWERUP_EXPAND: color = TFT_CYAN; break;
    case POWERUP_LASER: color = TFT_RED; break;
    case POWERUP_MULTI: color = TFT_YELLOW; break;
    case POWERUP_SLOW: color = TFT_BLUE; break;
    case POWERUP_LIFE: color = TFT_MAGENTA; break;
  }
  
  tft.fillCircle(breakout_powerupX + BREAKOUT_POWERUP_SIZE/2, 
                 breakout_powerupY + BREAKOUT_POWERUP_SIZE/2, 
                 BREAKOUT_POWERUP_SIZE/2, color);
  tft.drawCircle(breakout_powerupX + BREAKOUT_POWERUP_SIZE/2, 
                 breakout_powerupY + BREAKOUT_POWERUP_SIZE/2, 
                 BREAKOUT_POWERUP_SIZE/2, TFT_WHITE);
}

void breakout_activatePowerup(int type) {
  switch (type) {
    case POWERUP_EXPAND:
      breakout_erasePaddle(breakout_paddleX);
      breakout_paddleWidth = BREAKOUT_PADDLE_WIDTH + 30;
      breakout_drawPaddle(breakout_paddleX, BREAKOUT_PADDLE_COLOR);
      breakout_score += 50;
      break;
    case POWERUP_LASER:
      breakout_score += 75;
      break;
    case POWERUP_MULTI:
      breakout_score += 100;
      break;
    case POWERUP_SLOW:
      breakout_ballSpeedX *= 0.7;
      breakout_ballSpeedY *= 0.7;
      breakout_score += 50;
      break;
    case POWERUP_LIFE:
      if (breakout_lives < BREAKOUT_MAX_LIVES) {
        breakout_lives++;
        breakout_prevLives = -1;
      }
      breakout_score += 100;
      break;
  }
}

void breakout_createParticles(float x, float y, uint16_t color) {
  breakout_particleCount = 0;
  for (int i = 0; i < 15; i++) {
    breakout_particles[i].x = x;
    breakout_particles[i].y = y;
    breakout_particles[i].speedX = (random(-20, 20)) / 10.0;
    breakout_particles[i].speedY = (random(-20, 20)) / 10.0;
    breakout_particles[i].color = color;
    breakout_particles[i].life = 15;
    breakout_particleCount++;
  }
}

void breakout_updateParticles() {
  for (int i = 0; i < breakout_particleCount; i++) {
    if (breakout_particles[i].life > 0) {
      // Erase old position
      tft.drawPixel((int)breakout_particles[i].x, (int)breakout_particles[i].y, BREAKOUT_BG_COLOR);
      
      // Update position
      breakout_particles[i].x += breakout_particles[i].speedX;
      breakout_particles[i].y += breakout_particles[i].speedY;
      breakout_particles[i].speedY += 0.3; // Gravity
      breakout_particles[i].life--;
      
      // Draw new position
      if (breakout_particles[i].life > 0 && 
          breakout_particles[i].x >= 0 && breakout_particles[i].x < SCREEN_W &&
          breakout_particles[i].y >= 0 && breakout_particles[i].y < SCREEN_H) {
        tft.drawPixel((int)breakout_particles[i].x, (int)breakout_particles[i].y, 
                      breakout_particles[i].color);
      }
    }
  }
}

void breakout_drawParticles() {
  for (int i = 0; i < breakout_particleCount; i++) {
    if (breakout_particles[i].life > 0) {
      tft.drawPixel((int)breakout_particles[i].x, (int)breakout_particles[i].y, 
                    breakout_particles[i].color);
    }
  }
}

void breakout_showGameOver() {
  static bool gameOverDrawn = false;

  
  if (!gameOverDrawn) {
        command = -1;

    // Draw semi-transparent overlay effect
    for (int y = 60; y < 180; y += 2) {
      tft.drawFastHLine(30, y, 260, BREAKOUT_BG_COLOR);
    }
    
    // Game over box with double border
    tft.fillRect(35, 65, 250, 110, BREAKOUT_BG_COLOR);
    tft.drawRect(35, 65, 250, 110, TFT_RED);
    tft.drawRect(36, 66, 248, 108, TFT_RED);
    tft.drawRect(37, 67, 246, 106, TFT_ORANGE);
    
    // Title with shadow
    tft.setTextColor(0x4208);
    tft.setTextSize(3);
    tft.setCursor(72, 82);
    tft.println("GAME OVER");
    
    tft.setTextColor(TFT_RED);
    tft.setCursor(70, 80);
    tft.println("GAME OVER");
    
    // Final score
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(60, 115);
    tft.print("FINAL SCORE: ");
    tft.print(breakout_score);
    
    // High score check
    if (breakout_score > breakout_highScore) {
      breakout_highScore = breakout_score;
      preferences.putInt("highscore", breakout_highScore);
      
      tft.setTextSize(1);
      tft.setTextColor(TFT_GREEN);
      tft.setCursor(85, 135);
      tft.println("NEW HIGH SCORE!");
    }
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(70, 140);
    tft.println("PRESS SHOOT TO RETRY");
    tft.setCursor(70, 150);
    tft.println("PRESS UP FOR MENU");
    gameOverDrawn = true;
  }
  playNokiaTone();
  
  if (command == 4) {
    command = -1;
    gameOverDrawn = false;
    breakout_gameStarted = false;
    breakout_showMenu();
  }
   if (command == 0) {
    esp_restart();
  }
}

void breakout_showLevelComplete() {
  static bool levelCompleteDrawn = false;
  
  if (!levelCompleteDrawn) {
    // Victory box
    tft.fillRect(35, 75, 250, 90, BREAKOUT_BG_COLOR);
    tft.drawRect(35, 75, 250, 90, TFT_GREEN);
    tft.drawRect(36, 76, 248, 88, TFT_GREEN);
    tft.drawRect(37, 77, 246, 86, TFT_YELLOW);
    
    // Title
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(3);
    tft.setCursor(55, 85);
    tft.println("LEVEL CLEAR!");
    
    // Bonus points
    int bonus = breakout_lives * 500;
    breakout_score += bonus;
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(80, 115);
    tft.print("LIVES BONUS: +");
    tft.print(bonus);
    
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(90, 130);
    tft.print("TOTAL: ");
    tft.print(breakout_score);
    
    tft.setTextColor(TFT_WHITE);
    tft.println("PRESS SHOOT FOR NEXT");
        tft.setCursor(70, 150);
    tft.println("PRESS SHOOT FOR NEXT");

    levelCompleteDrawn = true;
  }
  
  if (command == 4) {
    command = -1;
    levelCompleteDrawn = false;
    breakout_levelComplete = false;
    breakout_level++;
    breakout_initLevel();
  }
   if (command == 0) {
    esp_restart();
  }
}