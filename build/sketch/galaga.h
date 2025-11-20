#line 1 "/home/pahasara/Arduino/gameConsole/galaga.h"
// Galaga Game Constants
#define GALAGA_PLAYER_WIDTH 20
#define GALAGA_PLAYER_HEIGHT 16
#define GALAGA_PLAYER_SPEED 6
#define GALAGA_BULLET_WIDTH 2
#define GALAGA_BULLET_HEIGHT 8
#define GALAGA_BULLET_SPEED 8
#define GALAGA_ENEMY_WIDTH 16
#define GALAGA_ENEMY_HEIGHT 14
#define GALAGA_ENEMY_ROWS 4
#define GALAGA_ENEMY_COLS 8
#define GALAGA_ENEMY_SPACING 8
#define GALAGA_MAX_BULLETS 5
#define GALAGA_MAX_ENEMY_BULLETS 10
#define GALAGA_MAX_LIVES 3
#define GALAGA_POWERUP_SIZE 12

// Enemy types
#define ENEMY_NONE 0
#define ENEMY_BEE 1      // 100 points
#define ENEMY_BUTTERFLY 2 // 160 points
#define ENEMY_BOSS 3     // 400 points

// Enemy states
#define ENEMY_STATE_FORMATION 0
#define ENEMY_STATE_DIVING 1
#define ENEMY_STATE_RETURNING 2

// Power-up types
#define POWERUP_NONE 0
#define POWERUP_DOUBLE 1    // Double shot
#define POWERUP_RAPID 2     // Rapid fire
#define POWERUP_SHIELD 3    // Temporary shield
#define POWERUP_LIFE 4      // Extra life

// Game variables
int galaga_playerX = (SCREEN_W - GALAGA_PLAYER_WIDTH) / 2;
int galaga_playerY = SCREEN_H - 30;
int galaga_prevPlayerX = galaga_playerX;

int galaga_score = 0;
int galaga_prevScore = -1;
int galaga_lives = GALAGA_MAX_LIVES;
int galaga_prevLives = -1;
int galaga_level = 1;
int galaga_highScore = 0;
int galaga_enemiesKilled = 0;

bool galaga_gameOver = false;
bool galaga_gameStarted = false;
bool galaga_levelComplete = false;
bool galaga_hasShield = false;

unsigned long galaga_lastShootTime = 0;
unsigned long galaga_shootCooldown = 300;
unsigned long galaga_lastEnemyShootTime = 0;
unsigned long galaga_lastUpdateTime = 0;
unsigned long galaga_frameDelay = 30; // ~33 FPS
unsigned long galaga_shieldTimer = 0;

// Player bullets
struct Bullet {
  float x, y;
  bool active;
};
Bullet galaga_bullets[GALAGA_MAX_BULLETS];

// Enemy bullets
Bullet galaga_enemyBullets[GALAGA_MAX_ENEMY_BULLETS];

// Enemies
struct Enemy {
  float x, y;
  float formationX, formationY;
  float diveSpeed;
  int type;
  int state;
  bool active;
  int divePhase;
  float diveAngle;
};
Enemy galaga_enemies[GALAGA_ENEMY_ROWS * GALAGA_ENEMY_COLS];
int galaga_enemyCount = 0;
int galaga_formationOffsetX = 0;
int galaga_formationDirection = 1;

// Power-up
int galaga_powerupX = -100;
int galaga_powerupY = -100;
int galaga_powerupType = POWERUP_NONE;
bool galaga_powerupActive = false;
bool galaga_doubleShot = false;
bool galaga_rapidFire = false;
unsigned long galaga_powerupTimer = 0;

// Particle effects
struct Galaga_Particle {
  float x, y;
  float speedX, speedY;
  uint16_t color;
  int life;
};
Galaga_Particle galaga_particles[30];
int galaga_particleCount = 0;

// Stars for background
struct Star {
  int x, y;
  int speed;
  uint16_t color;
};
Star galaga_stars[50];

// Color definitions
#define GALAGA_BG_COLOR TFT_BLACK      // Black
#define GALAGA_PLAYER_COLOR 0x07FF   // Cyan
#define GALAGA_BULLET_COLOR 0xFFE0   // Yellow
#define GALAGA_ENEMY_BULLET_COLOR 0xF800 // Red
#define GALAGA_BEE_COLOR 0xFFE0      // Yellow
#define GALAGA_BUTTERFLY_COLOR 0xF81F // Magenta
#define GALAGA_BOSS_COLOR 0x07FF     // Cyan
#define GALAGA_SHIELD_COLOR 0x07E0   // Green

const int ENEMY_POINTS[] = {0, 100, 160, 400};
const uint16_t ENEMY_COLORS[] = {0x0000, GALAGA_BEE_COLOR, GALAGA_BUTTERFLY_COLOR, GALAGA_BOSS_COLOR};

// Function declarations
void galaga_setup();
void galaga_loop();
void galaga_showMenu();
void galaga_initGame();
void galaga_initLevel();
void galaga_updateGame();
void galaga_handleInput();
void galaga_updatePlayer();
void galaga_updateBullets();
void galaga_updateEnemies();
void galaga_updateEnemyBullets();
void galaga_updatePowerup();
void galaga_checkCollisions();
void galaga_drawPlayer(int x, uint16_t color);
void galaga_erasePlayer(int x);
void galaga_drawBullet(float x, float y, uint16_t color);
void galaga_eraseBullet(float x, float y);
void galaga_drawEnemy(int index);
void galaga_eraseEnemy(int index);
void galaga_drawUI();
void galaga_shootBullet();
void galaga_enemyShoot(int enemyIndex);
void galaga_createExplosion(float x, float y, uint16_t color);
void galaga_updateParticles();
void galaga_showGameOver();
void galaga_showLevelComplete();
void galaga_initStars();
void galaga_updateStars();
void galaga_spawnPowerup(float x, float y);
void galaga_activatePowerup(int type);

void galaga_setup() {
  Serial.begin(115200);
  
  // Initialize TFT display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(GALAGA_BG_COLOR);

  // Initialize preferences
  preferences.begin("galaga", false);
  galaga_highScore = preferences.getInt("highscore", 0);
  
  // Initialize stars
  galaga_initStars();
  
  Serial.println("Galaga Starting...");
  
  galaga_showMenu();
}

void galaga_loop() {
  if (!galaga_gameStarted) {
    if (command == 4) { // SHOOT - Start game
      galaga_gameStarted = true;
      galaga_initGame();
      command = -1;
    }
    return;
  }
  
  if (galaga_gameOver) {
    galaga_showGameOver();
    return;
  }
  
  if (galaga_levelComplete) {
    galaga_showLevelComplete();
    
    return;
  }
  
  unsigned long currentTime = millis();
  if (currentTime - galaga_lastUpdateTime >= galaga_frameDelay) {
    galaga_handleInput();
    galaga_updateGame();
    galaga_lastUpdateTime = currentTime;
  }
}

void galaga_showMenu() {
  tft.fillScreen(GALAGA_BG_COLOR);
  
  // Draw stars
  for (int i = 0; i < 50; i++) {
    tft.drawPixel(galaga_stars[i].x, galaga_stars[i].y, galaga_stars[i].color);
  }
  
  // Draw sample enemies at top
  for (int i = 0; i < 8; i++) {
    int x = 40 + i * 30;
    // Boss
    if (i < 2) {
      tft.fillRect(x, 15, 16, 12, GALAGA_BOSS_COLOR);
      tft.drawRect(x, 15, 16, 12, TFT_WHITE);
    }
    // Butterfly
    else if (i < 4) {
      tft.fillRect(x, 15, 16, 12, GALAGA_BUTTERFLY_COLOR);
      tft.fillTriangle(x, 27, x + 8, 15, x + 16, 27, GALAGA_BUTTERFLY_COLOR);
    }
    // Bee
    else {
      tft.fillRect(x, 15, 16, 12, GALAGA_BEE_COLOR);
      tft.fillCircle(x + 8, 21, 6, GALAGA_BEE_COLOR);
    }
  }
  
  // Title with shadow effect
  tft.setTextColor(0x4208);
  tft.setTextSize(4);
  tft.setCursor(82, 52);
  tft.println("GALAGA");
  
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(80, 50);
  tft.println("GALAGA");
  
  // Draw decorative lines
  tft.drawFastHLine(60, 85, 200, TFT_CYAN);
  tft.drawFastHLine(60, 87, 200, TFT_CYAN);
  
  // Instructions box
  tft.drawRect(30, 95, 260, 100, TFT_CYAN);
  tft.drawRect(31, 96, 258, 98, TFT_CYAN);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(40, 105);
  tft.println("HOW TO PLAY:");
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(40, 120);
  tft.println("LEFT/RIGHT: Move ship");
  tft.setCursor(40, 133);
  tft.println("SHOOT: Fire bullets");
  
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(40, 153);
  tft.println("Destroy all alien invaders!");
  tft.setCursor(40, 166);
  tft.println("Watch out for diving attacks!");
  
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(40, 179);
  tft.println("Collect power-ups for bonuses");
  
  // High score
  if (galaga_highScore > 0) {
    tft.setTextColor(TFT_ORANGE);
    tft.setTextSize(2);
    tft.setCursor(70, 205);
    tft.print("HIGH: ");
    tft.print(galaga_highScore);
  }
  
  // Start prompt
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(75, 225);
  tft.println("PRESS SHOOT TO START!");
  
  Serial.println("Menu screen drawn");
}

void galaga_initGame() {
  Serial.println("Initializing Galaga...");
  tft.fillScreen(GALAGA_BG_COLOR);
  
  galaga_score = 0;
  galaga_prevScore = -1;
  galaga_lives = GALAGA_MAX_LIVES;
  galaga_prevLives = -1;
  galaga_level = 1;
  galaga_enemiesKilled = 0;
  
  galaga_gameOver = false;
  galaga_levelComplete = false;
  
  // Initialize bullets
  for (int i = 0; i < GALAGA_MAX_BULLETS; i++) {
    galaga_bullets[i].active = false;
  }
  
  for (int i = 0; i < GALAGA_MAX_ENEMY_BULLETS; i++) {
    galaga_enemyBullets[i].active = false;
  }
  
  galaga_initStars();
  galaga_initLevel();
}

void galaga_initLevel() {
  galaga_playerX = (SCREEN_W - GALAGA_PLAYER_WIDTH) / 2;
  galaga_hasShield = false;
  galaga_doubleShot = false;
  galaga_rapidFire = false;
  galaga_powerupActive = false;
  galaga_formationOffsetX = 0;
  galaga_formationDirection = 1;
  
  // Clear screen
  tft.fillScreen(GALAGA_BG_COLOR);
  
  // Initialize enemy formation
  galaga_enemyCount = 0;
  int startX = 50;
  int startY = 30;
  
  for (int row = 0; row < GALAGA_ENEMY_ROWS; row++) {
    for (int col = 0; col < GALAGA_ENEMY_COLS; col++) {
      int index = row * GALAGA_ENEMY_COLS + col;
      
      galaga_enemies[index].formationX = startX + col * (GALAGA_ENEMY_WIDTH + GALAGA_ENEMY_SPACING);
      galaga_enemies[index].formationY = startY + row * (GALAGA_ENEMY_HEIGHT + GALAGA_ENEMY_SPACING);
      galaga_enemies[index].x = galaga_enemies[index].formationX;
      galaga_enemies[index].y = galaga_enemies[index].formationY;
      galaga_enemies[index].state = ENEMY_STATE_FORMATION;
      galaga_enemies[index].active = true;
      galaga_enemies[index].diveSpeed = 0;
      galaga_enemies[index].divePhase = 0;
      galaga_enemies[index].diveAngle = 0;
      
      // Assign enemy types based on row
      if (row == 0) {
        galaga_enemies[index].type = ENEMY_BOSS;
      } else if (row < 2) {
        galaga_enemies[index].type = ENEMY_BUTTERFLY;
      } else {
        galaga_enemies[index].type = ENEMY_BEE;
      }
      
      galaga_enemyCount++;
    }
  }
  
  galaga_drawUI();
  galaga_drawPlayer(galaga_playerX, GALAGA_PLAYER_COLOR);
}

void galaga_initStars() {
  for (int i = 0; i < 50; i++) {
    galaga_stars[i].x = random(SCREEN_W);
    galaga_stars[i].y = random(SCREEN_H);
    galaga_stars[i].speed = random(1, 4);
    int brightness = random(3);
    if (brightness == 0) galaga_stars[i].color = 0x4208;
    else if (brightness == 1) galaga_stars[i].color = 0x8410;
    else galaga_stars[i].color = 0xFFFF;
  }
}

void galaga_updateStars() {
  for (int i = 0; i < 50; i++) {
    // Erase old position
    tft.drawPixel(galaga_stars[i].x, galaga_stars[i].y, GALAGA_BG_COLOR);
    
    // Update position
    galaga_stars[i].y += galaga_stars[i].speed;
    
    // Wrap around
    if (galaga_stars[i].y >= SCREEN_H) {
      galaga_stars[i].y = 0;
      galaga_stars[i].x = random(SCREEN_W);
    }
    
    // Draw new position
    tft.drawPixel(galaga_stars[i].x, galaga_stars[i].y, galaga_stars[i].color);
  }
}

void galaga_handleInput() {
  // Player movement
  if (command == 3 && galaga_playerX > 5) { // LEFT
    galaga_playerX -= GALAGA_PLAYER_SPEED;
    if (galaga_playerX < 5) galaga_playerX = 5;
  }
  if (command == 1 && galaga_playerX < SCREEN_W - GALAGA_PLAYER_WIDTH - 5) { // RIGHT
    galaga_playerX += GALAGA_PLAYER_SPEED;
    if (galaga_playerX > SCREEN_W - GALAGA_PLAYER_WIDTH - 5) {
      galaga_playerX = SCREEN_W - GALAGA_PLAYER_WIDTH - 5;
    }
  }
  
  // Shooting
  if (command == 4) {
    unsigned long currentTime = millis();
    unsigned long cooldown = galaga_rapidFire ? 150 : galaga_shootCooldown;
    
    if (currentTime - galaga_lastShootTime >= cooldown) {
      galaga_shootBullet();
      galaga_lastShootTime = currentTime;
    }
  }
}

void galaga_updateGame() {
  // Update stars
  galaga_updateStars();
  
  // Update and draw player
  // Erase player first
  galaga_erasePlayer(galaga_prevPlayerX);
  
  // Determine player color (changes when shielded)
  uint16_t playerColor = GALAGA_PLAYER_COLOR;
  if (galaga_hasShield) {
    // Pulsing effect when shielded
    if (millis() % 400 < 200) {
      playerColor = TFT_GREEN;  // Shield color
    } else {
      playerColor = TFT_YELLOW; // Alternate color
    }
  }
  
  // Always redraw player (for shield pulsing and engine animation)
  galaga_drawPlayer(galaga_playerX, playerColor);
  galaga_prevPlayerX = galaga_playerX;
  
  // Update bullets
  galaga_updateBullets();
  
  // Update enemies
  galaga_updateEnemies();
  
  // Update enemy bullets
  galaga_updateEnemyBullets();
  
  // Update power-up
  galaga_updatePowerup();
  
  // Check collisions
  galaga_checkCollisions();
  
  // Update particles
  galaga_updateParticles();
  
  // Update UI
  galaga_drawUI();
  
  // Check shield timer
  if (galaga_hasShield && millis() - galaga_shieldTimer > 10000) {
    galaga_hasShield = false;
  }
  
  // Check power-up timers
  if (galaga_doubleShot && millis() - galaga_powerupTimer > 15000) {
    galaga_doubleShot = false;
  }
  if (galaga_rapidFire && millis() - galaga_powerupTimer > 15000) {
    galaga_rapidFire = false;
  }
}

void galaga_updatePlayer() {
  // Player is updated in handleInput
}

void galaga_shootBullet() {
  for (int i = 0; i < GALAGA_MAX_BULLETS; i++) {
    if (!galaga_bullets[i].active) {
      galaga_bullets[i].x = galaga_playerX + GALAGA_PLAYER_WIDTH / 2 - GALAGA_BULLET_WIDTH / 2;
      galaga_bullets[i].y = galaga_playerY;
      galaga_bullets[i].active = true;
      
      // Double shot
      if (galaga_doubleShot && i < GALAGA_MAX_BULLETS - 1) {
        galaga_bullets[i + 1].x = galaga_bullets[i].x + 8;
        galaga_bullets[i + 1].y = galaga_playerY;
        galaga_bullets[i + 1].active = true;
      }
      
      break;
    }
  }
}

void galaga_updateBullets() {
  for (int i = 0; i < GALAGA_MAX_BULLETS; i++) {
    if (galaga_bullets[i].active) {
      // Erase old position
      galaga_eraseBullet(galaga_bullets[i].x, galaga_bullets[i].y);
      
      // Update position
      galaga_bullets[i].y -= GALAGA_BULLET_SPEED;
      
      // Check if off screen
      if (galaga_bullets[i].y < 0) {
        galaga_bullets[i].active = false;
      } else {
        // Draw new position
        galaga_drawBullet(galaga_bullets[i].x, galaga_bullets[i].y, GALAGA_BULLET_COLOR);
      }
    }
  }
}

void galaga_updateEnemies() {
  // Move formation
  galaga_formationOffsetX += galaga_formationDirection;
  if (galaga_formationOffsetX > 20 || galaga_formationOffsetX < -20) {
    galaga_formationDirection = -galaga_formationDirection;
  }
  
  int activeCount = 0;
  
  for (int i = 0; i < GALAGA_ENEMY_ROWS * GALAGA_ENEMY_COLS; i++) {
    if (galaga_enemies[i].active) {
      activeCount++;
      
      // Erase old position
      galaga_eraseEnemy(i);
      
      if (galaga_enemies[i].state == ENEMY_STATE_FORMATION) {
        // Move in formation with offset
        galaga_enemies[i].x = galaga_enemies[i].formationX + galaga_formationOffsetX;
        
        // Random dive attack
        if (random(1000) < 5 + galaga_level * 2) {
          galaga_enemies[i].state = ENEMY_STATE_DIVING;
          galaga_enemies[i].divePhase = 0;
          galaga_enemies[i].diveAngle = 0;
          galaga_enemies[i].diveSpeed = 3 + galaga_level;
        }
        
        // Random shooting
        if (random(1000) < 3) {
          galaga_enemyShoot(i);
        }
      }
      else if (galaga_enemies[i].state == ENEMY_STATE_DIVING) {
        // Diving attack pattern
        galaga_enemies[i].divePhase++;
        galaga_enemies[i].diveAngle += 0.1;
        
        galaga_enemies[i].x += sin(galaga_enemies[i].diveAngle) * 3;
        galaga_enemies[i].y += galaga_enemies[i].diveSpeed;
        
        // Shoot during dive
        if (galaga_enemies[i].divePhase % 20 == 0) {
          galaga_enemyShoot(i);
        }
        
        // Check if reached bottom
        if (galaga_enemies[i].y > SCREEN_H) {
          galaga_enemies[i].state = ENEMY_STATE_RETURNING;
        }
      }
      else if (galaga_enemies[i].state == ENEMY_STATE_RETURNING) {
        // Return to formation
        float dx = galaga_enemies[i].formationX - galaga_enemies[i].x;
        float dy = galaga_enemies[i].formationY - galaga_enemies[i].y;
        float dist = sqrt(dx * dx + dy * dy);
        
        if (dist < 5) {
          galaga_enemies[i].state = ENEMY_STATE_FORMATION;
          galaga_enemies[i].x = galaga_enemies[i].formationX;
          galaga_enemies[i].y = galaga_enemies[i].formationY;
        } else {
          galaga_enemies[i].x += (dx / dist) * 4;
          galaga_enemies[i].y += (dy / dist) * 4;
        }
      }
      
      // Draw new position
      galaga_drawEnemy(i);
    }
  }
  
  // Check level complete
  if (activeCount == 0) {
    galaga_levelComplete = true;
  }
}

void galaga_enemyShoot(int enemyIndex) {
  for (int i = 0; i < GALAGA_MAX_ENEMY_BULLETS; i++) {
    if (!galaga_enemyBullets[i].active) {
      galaga_enemyBullets[i].x = galaga_enemies[enemyIndex].x + GALAGA_ENEMY_WIDTH / 2;
      galaga_enemyBullets[i].y = galaga_enemies[enemyIndex].y + GALAGA_ENEMY_HEIGHT;
      galaga_enemyBullets[i].active = true;
      break;
    }
  }
}

void galaga_updateEnemyBullets() {
  for (int i = 0; i < GALAGA_MAX_ENEMY_BULLETS; i++) {
    if (galaga_enemyBullets[i].active) {
      // Erase old position
      galaga_eraseBullet(galaga_enemyBullets[i].x, galaga_enemyBullets[i].y);
      
      // Update position
      galaga_enemyBullets[i].y += 5;
      
      // Check if off screen
      if (galaga_enemyBullets[i].y > SCREEN_H) {
        galaga_enemyBullets[i].active = false;
      } else {
        // Draw new position
        galaga_drawBullet(galaga_enemyBullets[i].x, galaga_enemyBullets[i].y, GALAGA_ENEMY_BULLET_COLOR);
      }
    }
  }
}

void galaga_checkCollisions() {
  // Player bullets vs enemies
  for (int i = 0; i < GALAGA_MAX_BULLETS; i++) {
    if (galaga_bullets[i].active) {
      for (int e = 0; e < GALAGA_ENEMY_ROWS * GALAGA_ENEMY_COLS; e++) {
        if (galaga_enemies[e].active) {
          if (galaga_bullets[i].x + GALAGA_BULLET_WIDTH >= galaga_enemies[e].x &&
              galaga_bullets[i].x <= galaga_enemies[e].x + GALAGA_ENEMY_WIDTH &&
              galaga_bullets[i].y <= galaga_enemies[e].y + GALAGA_ENEMY_HEIGHT &&
              galaga_bullets[i].y + GALAGA_BULLET_HEIGHT >= galaga_enemies[e].y) {
            
            // Hit!
            galaga_bullets[i].active = false;
            galaga_enemies[e].active = false;
            galaga_enemyCount--;
            galaga_enemiesKilled++;
            
            // Add score
            galaga_score += ENEMY_POINTS[galaga_enemies[e].type];
            tone(BUZZER_PIN,300);
            
            // Create explosion
            galaga_createExplosion(galaga_enemies[e].x + GALAGA_ENEMY_WIDTH / 2,
                                  galaga_enemies[e].y + GALAGA_ENEMY_HEIGHT / 2,
                                  ENEMY_COLORS[galaga_enemies[e].type]);
            
            // Erase enemy
            galaga_eraseEnemy(e);
            
            // Random power-up spawn
            if (random(100) < 10) {
              galaga_spawnPowerup(galaga_enemies[e].x, galaga_enemies[e].y);
            }
            
            break;
          }
        }
      }
    }
  }
  
  // Enemy bullets vs player
  for (int i = 0; i < GALAGA_MAX_ENEMY_BULLETS; i++) {
    if (galaga_enemyBullets[i].active) {
      if (galaga_enemyBullets[i].x + GALAGA_BULLET_WIDTH >= galaga_playerX &&
          galaga_enemyBullets[i].x <= galaga_playerX + GALAGA_PLAYER_WIDTH &&
          galaga_enemyBullets[i].y + GALAGA_BULLET_HEIGHT >= galaga_playerY &&
          galaga_enemyBullets[i].y <= galaga_playerY + GALAGA_PLAYER_HEIGHT) {
        
        galaga_enemyBullets[i].active = false;
        galaga_eraseBullet(galaga_enemyBullets[i].x, galaga_enemyBullets[i].y);
        
        if (!galaga_hasShield) {
          // Player hit! Decrease life
          galaga_lives--;
          galaga_prevLives = -1; // Force UI update
          tone(BUZZER_PIN,1200);
          
          galaga_createExplosion(galaga_playerX + GALAGA_PLAYER_WIDTH / 2,
                                galaga_playerY + GALAGA_PLAYER_HEIGHT / 2,
                                GALAGA_PLAYER_COLOR);
          
          // Check if game over
          if (galaga_lives <= 0) {
            galaga_gameOver = true;
          } else {
            // Give temporary shield after being hit
            galaga_hasShield = true;
            galaga_shieldTimer = millis();
          }
        }
      }
    }
  }
  
  // Enemies vs player (collision)
  for (int e = 0; e < GALAGA_ENEMY_ROWS * GALAGA_ENEMY_COLS; e++) {
    if (galaga_enemies[e].active) {
      if (galaga_enemies[e].x + GALAGA_ENEMY_WIDTH >= galaga_playerX &&
          galaga_enemies[e].x <= galaga_playerX + GALAGA_PLAYER_WIDTH &&
          galaga_enemies[e].y + GALAGA_ENEMY_HEIGHT >= galaga_playerY &&
          galaga_enemies[e].y <= galaga_playerY + GALAGA_PLAYER_HEIGHT) {
        
        if (!galaga_hasShield) {
          // Player collided with enemy! Decrease life
          galaga_lives--;
          galaga_prevLives = -1; // Force UI update
          galaga_enemies[e].active = false;
          
          galaga_createExplosion(galaga_playerX + GALAGA_PLAYER_WIDTH / 2,
                                galaga_playerY + GALAGA_PLAYER_HEIGHT / 2,
                                GALAGA_PLAYER_COLOR);
          
          // Check if game over
          if (galaga_lives <= 0) {
            galaga_gameOver = true;
          } else {
            // Give temporary shield after being hit
            galaga_hasShield = true;
            galaga_shieldTimer = millis();
          }
        }
      }
    }
  }
}

void galaga_drawPlayer(int x, uint16_t color) {
  int y = galaga_playerY;
  
  // Main rocket body (cylinder)
  tft.fillRect(x + 7, y + 4, 6, 10, color);
  tft.drawRect(x + 7, y + 4, 6, 10, TFT_WHITE);
  
  // Nose cone (pointed top)
  tft.fillTriangle(x + 7, y + 4, x + 13, y + 4, x + 10, y, color);
  tft.drawLine(x + 7, y + 4, x + 10, y, TFT_WHITE);
  tft.drawLine(x + 13, y + 4, x + 10, y, TFT_WHITE);
  
  // Left fin
  tft.fillTriangle(x + 2, y + 14, x + 7, y + 8, x + 7, y + 14, TFT_RED);
  tft.drawTriangle(x + 2, y + 14, x + 7, y + 8, x + 7, y + 14, TFT_WHITE);
  
  // Right fin
  tft.fillTriangle(x + 18, y + 14, x + 13, y + 8, x + 13, y + 14, TFT_RED);
  tft.drawTriangle(x + 18, y + 14, x + 13, y + 8, x + 13, y + 14, TFT_WHITE);
  
  // Cockpit window
  tft.fillRect(x + 8, y + 6, 4, 3, TFT_CYAN);
  tft.drawRect(x + 8, y + 6, 4, 3, TFT_WHITE);
  
  // Engine nozzle
  tft.fillRect(x + 8, y + 14, 4, 2, 0x4208); // Dark gray
  
  // Engine flames (animated)
  if (millis() % 200 < 100) {
    // Flame 1
    tft.fillRect(x + 9, y + 16, 2, 2, TFT_YELLOW);
    tft.drawPixel(x + 10, y + 18, TFT_ORANGE);
  } else {
    // Flame 2 (alternating pattern)
    tft.fillRect(x + 8, y + 16, 4, 1, TFT_ORANGE);
    tft.fillRect(x + 9, y + 17, 2, 2, TFT_YELLOW);
  }
  
  // Detail lines on body
  tft.drawFastVLine(x + 10, y + 4, 10, 0xC618); // Silver line down center
}

void galaga_erasePlayer(int x) {
  // Erase larger area to account for all rocket parts including flame animation
  tft.fillRect(x - 5, galaga_playerY - 3, 30, 25, GALAGA_BG_COLOR);
}

void galaga_drawBullet(float x, float y, uint16_t color) {
  tft.fillRect((int)x, (int)y, GALAGA_BULLET_WIDTH, GALAGA_BULLET_HEIGHT, color);
}

void galaga_eraseBullet(float x, float y) {
  tft.fillRect((int)x, (int)y, GALAGA_BULLET_WIDTH, GALAGA_BULLET_HEIGHT, GALAGA_BG_COLOR);
}

void galaga_drawEnemy(int index) {
  int x = (int)galaga_enemies[index].x;
  int y = (int)galaga_enemies[index].y;
  uint16_t color = ENEMY_COLORS[galaga_enemies[index].type];
  
  switch (galaga_enemies[index].type) {
    case ENEMY_BEE:
      // Round body
      tft.fillCircle(x + 8, y + 7, 6, color);
      // Wings
      tft.fillRect(x, y + 5, 4, 4, color);
      tft.fillRect(x + 12, y + 5, 4, 4, color);
      // Eyes
      tft.fillCircle(x + 6, y + 6, 1, TFT_WHITE);
      tft.fillCircle(x + 10, y + 6, 1, TFT_WHITE);
      break;
      
    case ENEMY_BUTTERFLY:
      // Body
      tft.fillRect(x + 6, y + 4, 4, 8, color);
      // Wings
      tft.fillTriangle(x, y + 8, x + 6, y, x + 6, y + 12, color);
      tft.fillTriangle(x + 10, y, x + 10, y + 12, x + 16, y + 8, color);
      // Outline
      tft.drawRect(x + 6, y + 4, 4, 8, TFT_WHITE);
      break;
      
    case ENEMY_BOSS:
      // Large body
      tft.fillRect(x + 2, y + 2, 12, 10, color);
      tft.fillRect(x, y + 6, 16, 4, color);
      // Cockpit
      tft.fillRect(x + 6, y + 4, 4, 4, TFT_RED);
      // Outline
      tft.drawRect(x + 2, y + 2, 12, 10, TFT_WHITE);
      break;
  }
}

void galaga_eraseEnemy(int index) {
  int x = (int)galaga_enemies[index].x;
  int y = (int)galaga_enemies[index].y;
  tft.fillRect(x - 2, y - 2, GALAGA_ENEMY_WIDTH + 4, GALAGA_ENEMY_HEIGHT + 4, GALAGA_BG_COLOR);
}

void galaga_drawUI() {
  if (galaga_prevScore != galaga_score) {
    tft.fillRect(5, 3, 120, 12, GALAGA_BG_COLOR);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(5, 3);
    tft.print("SCORE: ");
    tft.print(galaga_score);
    galaga_prevScore = galaga_score;
  }
  
  if (galaga_prevLives != galaga_lives) {
    tft.fillRect(SCREEN_W - 100, 3, 95, 12, GALAGA_BG_COLOR);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(SCREEN_W - 100, 3);
    tft.print("SHIPS: ");
    
    for (int i = 0; i < galaga_lives; i++) {
      int shipX = SCREEN_W - 55 + i * 15;
      tft.fillTriangle(shipX, 13, shipX + 10, 13, shipX + 5, 5, TFT_GREEN);
    }
    galaga_prevLives = galaga_lives;
  }
  
  // Draw level
  tft.fillRect(SCREEN_W / 2 - 30, 3, 60, 12, GALAGA_BG_COLOR);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(SCREEN_W / 2 - 25, 3);
  tft.print("LEVEL ");
  tft.print(galaga_level);
}

void galaga_spawnPowerup(float x, float y) {
  if (!galaga_powerupActive) {
    galaga_powerupX = x;
    galaga_powerupY = y;
    galaga_powerupType = random(1, 5);
    galaga_powerupActive = true;
  }
}

void galaga_updatePowerup() {
  if (galaga_powerupActive) {
    // Erase old position
    tft.fillRect(galaga_powerupX - 1, galaga_powerupY - 3, 
                 GALAGA_POWERUP_SIZE + 2, GALAGA_POWERUP_SIZE + 4, 
                 GALAGA_BG_COLOR);
    
    galaga_powerupY += 2;
    
    // Check if caught by player
    if (galaga_powerupY + GALAGA_POWERUP_SIZE >= galaga_playerY &&
        galaga_powerupY <= galaga_playerY + GALAGA_PLAYER_HEIGHT &&
        galaga_powerupX + GALAGA_POWERUP_SIZE >= galaga_playerX &&
        galaga_powerupX <= galaga_playerX + GALAGA_PLAYER_WIDTH) {
      
      galaga_activatePowerup(galaga_powerupType);
      galaga_powerupActive = false;
    }
    
    // Check if missed
    if (galaga_powerupY > SCREEN_H) {
      galaga_powerupActive = false;
    }
    
    if (galaga_powerupActive) {
      // Draw power-up
      uint16_t color = TFT_GREEN;
      switch (galaga_powerupType) {
        case POWERUP_DOUBLE: color = TFT_CYAN; break;
        case POWERUP_RAPID: color = TFT_YELLOW; break;
        case POWERUP_SHIELD: color = TFT_GREEN; break;
        case POWERUP_LIFE: color = TFT_MAGENTA; break;
      }
      
      tft.fillRect(galaga_powerupX, galaga_powerupY, GALAGA_POWERUP_SIZE, GALAGA_POWERUP_SIZE, color);
      tft.drawRect(galaga_powerupX, galaga_powerupY, GALAGA_POWERUP_SIZE, GALAGA_POWERUP_SIZE, TFT_WHITE);
      
      // Draw letter
      tft.setTextSize(1);
      tft.setTextColor(TFT_BLACK);
      tft.setCursor(galaga_powerupX + 3, galaga_powerupY + 2);
      switch (galaga_powerupType) {
        case POWERUP_DOUBLE: tft.print("D"); break;
        case POWERUP_RAPID: tft.print("R"); break;
        case POWERUP_SHIELD: tft.print("S"); break;
        case POWERUP_LIFE: tft.print("L"); break;
      }
    }
  }
}

void galaga_activatePowerup(int type) {
  switch (type) {
    case POWERUP_DOUBLE:
      galaga_doubleShot = true;
      galaga_powerupTimer = millis();
      galaga_score += 200;
      break;
    case POWERUP_RAPID:
      galaga_rapidFire = true;
      galaga_powerupTimer = millis();
      galaga_score += 200;
      break;
    case POWERUP_SHIELD:
      galaga_hasShield = true;
      galaga_shieldTimer = millis();
      galaga_score += 300;
      break;
    case POWERUP_LIFE:
      if (galaga_lives < GALAGA_MAX_LIVES) {
        galaga_lives++;
        galaga_prevLives = -1;
      }
      galaga_score += 500;
      break;
  }
}

void galaga_createExplosion(float x, float y, uint16_t color) {
  galaga_particleCount = 0;
  for (int i = 0; i < 20; i++) {
    galaga_particles[i].x = x;
    galaga_particles[i].y = y;
    float angle = (i / 20.0) * 6.28;
    galaga_particles[i].speedX = cos(angle) * 3;
    galaga_particles[i].speedY = sin(angle) * 3;
    galaga_particles[i].color = color;
    galaga_particles[i].life = 20;
    galaga_particleCount++;
  }
}

void galaga_updateParticles() {
  for (int i = 0; i < galaga_particleCount; i++) {
    if (galaga_particles[i].life > 0) {
      // Erase old position
      tft.drawPixel((int)galaga_particles[i].x, (int)galaga_particles[i].y, GALAGA_BG_COLOR);
      
      // Update position
      galaga_particles[i].x += galaga_particles[i].speedX;
      galaga_particles[i].y += galaga_particles[i].speedY;
      galaga_particles[i].speedY += 0.2; // Gravity
      galaga_particles[i].life--;
      
      // Draw new position
      if (galaga_particles[i].life > 0 && 
          galaga_particles[i].x >= 0 && galaga_particles[i].x < SCREEN_W &&
          galaga_particles[i].y >= 0 && galaga_particles[i].y < SCREEN_H) {
        tft.drawPixel((int)galaga_particles[i].x, (int)galaga_particles[i].y, 
                      galaga_particles[i].color);
      }
    }
  }
}

void galaga_showGameOver() {
  static bool gameOverDrawn = false;
  
  if (!gameOverDrawn) {
    // Draw semi-transparent overlay
    for (int y = 60; y < 180; y += 2) {
      tft.drawFastHLine(30, y, 260, GALAGA_BG_COLOR);
    }
    
    // Game over box - made taller for extra option
    tft.fillRect(35, 55, 250, 130, GALAGA_BG_COLOR);
    tft.drawRect(35, 55, 250, 130, TFT_RED);
    tft.drawRect(36, 56, 248, 128, TFT_RED);
    tft.drawRect(37, 57, 246, 126, TFT_ORANGE);
    
    // Title
    tft.setTextColor(0x4208);
    tft.setTextSize(3);
    tft.setCursor(72, 72);
    tft.println("GAME OVER");
    
    tft.setTextColor(TFT_RED);
    tft.setCursor(70, 70);
    tft.println("GAME OVER");
    
    // Final score
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(60, 105);
    tft.print("SCORE: ");
    tft.print(galaga_score);
    
    // Enemies killed
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(100, 125);
    tft.print("KILLED: ");
    tft.print(galaga_enemiesKilled);
    
    // High score check
    if (galaga_score > galaga_highScore) {
      galaga_highScore = galaga_score;
      preferences.putInt("highscore", galaga_highScore);
      
      tft.setTextColor(TFT_GREEN);
      tft.setCursor(85, 137);
      tft.println("NEW HIGH SCORE!");
    }
    
    // Option 1: Retry
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(70, 155);
    tft.println("SHOOT: Retry Game");
    
    // Option 2: Main Menu
    tft.setTextColor(TFT_ORANGE);
    tft.setCursor(70, 168);
    tft.println("UP: Back to Main Menu");
    command = -1;
    gameOverDrawn = true;
  }
  playNokiaTone();

  
  // Check for SHOOT button - Retry game
  if (command == 4) {
    command = -1;
    gameOverDrawn = false;
    galaga_gameStarted = false;
    galaga_showMenu();
  }
  
  // Check for UP button - Go to main menu (restart ESP32)
  if (command == 0) { // UP button
    command = -1;
    esp_restart(); // Restart ESP32 to go back to main menu
  }
}

void galaga_showLevelComplete() {
  static bool levelCompleteDrawn = false;
  
  if (!levelCompleteDrawn) {
    // Victory box
    tft.fillRect(35, 75, 250, 90, GALAGA_BG_COLOR);
    tft.drawRect(35, 75, 250, 90, TFT_GREEN);
    tft.drawRect(36, 76, 248, 88, TFT_GREEN);
    tft.drawRect(37, 77, 246, 86, TFT_YELLOW);
    
    // Title
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(3);
    tft.setCursor(55, 85);
    tft.println("WAVE CLEAR!");
    
    // Bonus
    int bonus = galaga_enemiesKilled * 50;
    galaga_score += bonus;
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(80, 120);
    tft.print("WAVE BONUS: +");
    tft.print(bonus);
    
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(90, 135);
    tft.print("TOTAL: ");
    tft.print(galaga_score);
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(65, 150);
    tft.println("PRESS SHOOT FOR NEXT!");
    
    levelCompleteDrawn = true;
    command = -1;
  }
  delay(500);
  
  if (command == 4) {
    command = -1;
    levelCompleteDrawn = false;
    galaga_levelComplete = false;
    galaga_level++;
    galaga_enemiesKilled = 0;
    
    // Clear bullets
    for (int i = 0; i < GALAGA_MAX_BULLETS; i++) {
      galaga_bullets[i].active = false;
    }
    for (int i = 0; i < GALAGA_MAX_ENEMY_BULLETS; i++) {
      galaga_enemyBullets[i].active = false;
    }
    
    galaga_initLevel();
  }
}