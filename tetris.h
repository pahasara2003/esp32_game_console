#include <array>
#include <Preferences.h>
using namespace std;

#include <stdint.h>
#include <math.h>
#include <array>

enum gameStatus {
  MENU,
  START,
  PAUSE,
  PLAY,
  END
};


gameStatus game = MENU;
Direction joystick = IDLE;

#define WIDTH 220
#define HEIGHT 260
#define SIZE 10
uint32_t BlockSpace[WIDTH / SIZE][HEIGHT / SIZE];

uint32_t bg1 = tft.color565(4, 13, 48);
uint32_t bg2 = tft.color565(14, 3, 38);
uint32_t blockColorList[] = { TFT_GREEN, TFT_RED, TFT_YELLOW, TFT_CYAN, TFT_ORANGE, TFT_PINK, TFT_PURPLE };

int t = 0;
unsigned long Game_previousMillis = 0;
unsigned long Shift_previousMillis = 0;
unsigned long rotation_previousMillis = 0;

bool rotation = false;

long Game_interval = 500;
int tetrisHighScore = 0;
int linesCleared = 0;

// Next piece variables
int nextPieceType = 0;
int currentPieceType = 0;

// Visual effects
struct TetrisParticle {
  float x, y;
  float speedX, speedY;
  uint16_t color;
  int life;
};
TetrisParticle tetrisParticles[20];
int tetrisParticleCount = 0;

struct TetrisStar {
  int x, y;
  int speed;
  uint16_t color;
};

bool bodies[7][3][3] = {
  {
    { 0, 0, 0 },
    { 1, 1, 1 },
    { 0, 0, 0 } },
  {
    { 0, 0, 0 },
    { 0, 1, 1 },
    { 0, 1, 1 } },
  {
    { 0, 0, 0 },
    { 1, 1, 1 },
    { 0, 1, 0 } },
  {
    { 0, 0, 0 },
    { 1, 1, 1 },
    { 1, 0, 0 } },
  {
    { 0, 0, 0 },
    { 1, 1, 1 },
    { 0, 0, 1 } },
  {
    { 0, 0, 0 },
    { 0, 1, 1 },
    { 1, 1, 0 } },
  {
    { 0, 0, 0 },
    { 1, 1, 0 },
    { 0, 1, 1 } }
};

typedef struct Block {
  bool body[3][3];
  int pos[2];
  uint32_t color;
  int bottom;
  int type;
} Block;

array<int, 2> direction = { 0, 1 };
extern Block aliveBlock;


void createTetrisParticles(int x, int y, uint16_t color);
void updateTetrisParticles();
void showTetrisMenu();
void drawNextPiece();
void drawTetrisPauseScreen();

// =================================================================
//                         VISUAL EFFECTS
// =================================================================



void createTetrisParticles(int x, int y, uint16_t color) {
  tetrisParticleCount = 0;
  for (int i = 0; i < 15; i++) {
    tetrisParticles[i].x = x;
    tetrisParticles[i].y = y;
    float angle = (i / 15.0) * 6.28;
    tetrisParticles[i].speedX = cos(angle) * 3;
    tetrisParticles[i].speedY = sin(angle) * 3;
    tetrisParticles[i].color = color;
    tetrisParticles[i].life = 20;
    tetrisParticleCount++;
  }
}

void updateTetrisParticles() {
  for (int i = 0; i < tetrisParticleCount; i++) {
    if (tetrisParticles[i].life > 0) {
      tft.drawPixel((int)tetrisParticles[i].x, (int)tetrisParticles[i].y, bg1);
      
      tetrisParticles[i].x += tetrisParticles[i].speedX;
      tetrisParticles[i].y += tetrisParticles[i].speedY;
      tetrisParticles[i].speedY += 0.2;
      tetrisParticles[i].life--;
      
      if (tetrisParticles[i].life > 0 && 
          tetrisParticles[i].x >= 0 && tetrisParticles[i].x < SCREEN_W &&
          tetrisParticles[i].y >= 40 && tetrisParticles[i].y < SCREEN_H) {
        tft.drawPixel((int)tetrisParticles[i].x, (int)tetrisParticles[i].y, tetrisParticles[i].color);
      }
    }
  }
}

// =================================================================
//                         PAUSE SCREEN
// =================================================================
void drawTetrisPauseScreen() {
  // Semi-transparent overlay effect
  for (int y = 40; y < SCREEN_W; y += 4) {
    tft.drawFastHLine(0, y, SCREEN_H, 0x2104);
  }
  
  // Pause box
  int boxW = 180;
  int boxH = 100;
  int boxX = (SCREEN_H - boxW) / 2;
  int boxY = (SCREEN_W - boxH) / 2;
  
  tft.fillRect(boxX, boxY, boxW, boxH, bg1);
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
  tft.drawString("PAUSED", SCREEN_H/2, boxY + 55);
  
  tft.setTextFont(1);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Press SHOOT to Resume", SCREEN_H/2, boxY + 75);
  
  tft.setTextColor(TFT_ORANGE);
  tft.drawString("Press UP for Menu", SCREEN_H/2, boxY + 88);
}

// =================================================================
//                         MENU SYSTEM
// =================================================================
void showTetrisMenu() {
  tft.fillScreen(bg1);
  

  
  // Draw sample blocks at top
  int blockY = 15;
  for (int i = 0; i < 7; i++) {
    int x = 30 + i * 25;
    tft.fillRect(x, blockY, 20, 8, blockColorList[i]);
    tft.drawRect(x, blockY, 20, 8, TFT_WHITE);
  }
  
  // Title with shadow
  tft.setTextFont(4);
  tft.setTextColor(0x4208);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("TETRIS", SCREEN_H / 2 + 2, 52);
  
  tft.setTextColor(TFT_CYAN);
  tft.drawString("TETRIS", SCREEN_H / 2, 50);
  
  // Decorative lines
  tft.drawFastHLine(40, 75, SCREEN_H - 80, TFT_CYAN);
  tft.drawFastHLine(40, 77, SCREEN_H - 80, TFT_MAGENTA);
  
  // Instructions box
  tft.drawRect(20, 85, SCREEN_H - 40, 150, TFT_CYAN);
  tft.drawRect(21, 86, SCREEN_H - 42, 148, 0x4208);
  
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.setCursor(30, 95);
  tft.println("HOW TO PLAY:");
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(30, 110);
  tft.println("LEFT/RIGHT: Move piece");
  tft.setCursor(30, 123);
  tft.println("DOWN: Rotate piece");
  tft.setCursor(30, 136);
  tft.println("UP: Hard drop");
  
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(30, 149);
  tft.println("SHOOT: Pause/Resume");
  
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(30, 169);
  tft.println("OBJECTIVE:");
  tft.setCursor(30, 182);
  tft.println("- Complete horizontal lines");
  tft.setCursor(30, 195);
  tft.println("- Lines clear for points");
  tft.setCursor(30, 208);
  tft.println("- Game ends when blocks");
  tft.setCursor(30, 221);
  tft.println("  reach the top");
  
  // High score
  if (tetrisHighScore > 0) {
    tft.setTextColor(TFT_ORANGE);
    tft.setTextDatum(TC_DATUM);
    tft.setTextFont(2);
    tft.drawString("HIGH SCORE: " + String(tetrisHighScore), SCREEN_W/2, 242);
  }
  
  // Start prompt
  tft.setTextFont(1);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("PRESS ANY KEY TO START!", SCREEN_H / 2, 260);
  
  Serial.println("Tetris menu displayed");
}

// =================================================================
//                         NEXT PIECE PREVIEW
// =================================================================
void drawNextPiece() {
  // Clear preview area
  tft.fillRect(SCREEN_W - 50, 50, 45, 45, bg1);
  
  // Draw border
  tft.drawRect(SCREEN_W - 50, 50, 45, 45, TFT_CYAN);
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("NEXT", SCREEN_W - 27, 40);
  
  // Draw next piece
  uint32_t color = blockColorList[nextPieceType];
  int offsetX = SCREEN_W - 45;
  int offsetY = 60;
  
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (bodies[nextPieceType][i][j]) {
        tft.fillRect(offsetX + j * 10, offsetY + i * 10, 9, 9, color);
        tft.drawRect(offsetX + j * 10, offsetY + i * 10, 9, 9, TFT_WHITE);
      }
    }
  }
}

// =================================================================
//                         CORE GAME LOGIC
// =================================================================
void populateBody(Block* block, bool array[3][3]) {
  int bottom = 0;

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      block->body[i][j] = array[i][j];
      if (block->body[i][j] && bottom < i) bottom = i;
    }
  }
  block->bottom = bottom;
}

void newBody(Block* block) {
  block->pos[0] = rand() % ((WIDTH / SIZE) - 3) + 1;
  
  // Use the next piece type
  block->type = nextPieceType;
  populateBody(block, bodies[nextPieceType]);
  block->color = blockColorList[nextPieceType];
  block->bottom = 0;
  
  // Generate new next piece
  nextPieceType = rand() % 7;
  drawNextPiece();
}

int drawBlock(Block block) {
  int x = block.pos[0];
  int y = block.pos[1];
  int bottom = block.bottom;

  // Clear previous position
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      int drawX = x - direction[0] + i;
      int drawY = y - direction[1] + j;
      if (drawX >= 0 && drawX < WIDTH / SIZE && drawY >= 0 && drawY < HEIGHT / SIZE) {
        if (SIZE * drawY < HEIGHT + SIZE)
          tft.fillRect(SIZE * (drawX + 1) + 1, SIZE * (drawY + 1) + 1 + 40, SIZE - 2, SIZE - 2, BlockSpace[drawX][drawY]);
      }
    }
  }

  // Draw current position with shadow effect
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (block.body[i][j]) {
        int drawX = x + i;
        int drawY = y + j;
        if (drawX >= 0 && drawX < WIDTH / SIZE && drawY >= 0 && drawY < HEIGHT / SIZE) {
          // Main block
          tft.fillRect(SIZE * (drawX + 1) + 1, SIZE * (drawY + 1) + 1 + 40, SIZE - 2, SIZE - 2, block.color);
          
        }
      }
    }
  }
  return bottom;
}

void checkGame(Block* block) {
  int x = block->pos[0];
  int y = block->pos[1];
  bool status = true;

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (block->body[i][j] == 1) {
        int checkX = x + i;
        int checkY = y + j;
        if (checkX >= 0 && checkX < WIDTH / SIZE && checkY >= 0 && checkY < HEIGHT / SIZE) {
          if (BlockSpace[checkX][checkY] != bg2) {
            status = false;
          }
        }
      }
    }
  }
  if (!status) game = END;
}

void newBlock(Block* prevBlock) {
  int x = prevBlock->pos[0];
  int y = prevBlock->pos[1];

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (prevBlock->body[i][j]) {
        int storeX = x + i;
        int storeY = y + j;
        if (storeX >= 0 && storeX < WIDTH / SIZE && storeY >= 0 && storeY < HEIGHT / SIZE) {
          BlockSpace[storeX][storeY] = prevBlock->color;
        }
      }
    }
  }
  prevBlock->pos[1] = 0;
  prevBlock->pos[0] = rand() % ((WIDTH / SIZE) - 3) + 1;
  newBody(prevBlock);
  checkGame(prevBlock);
}

bool Collide(Block* block, array<int, 2> shift) {
  int x = block->pos[0] + shift[0];
  int y = block->pos[1] + shift[1];
  bool collide = false;

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (block->body[i][j] == 1) {
        int checkX = x + i;
        int checkY = y + j;

        if (checkX < 0 || checkX >= WIDTH / SIZE || checkY >= HEIGHT / SIZE) {
          collide = true;
          break;
        }

        if (checkY >= 0 && BlockSpace[checkX][checkY] != bg2) {
          collide = true;
          break;
        }
      }
    }
    if (collide) break;
  }

  return collide;
}

void updateBody(Block* block, array<int, 2> shift) {
  bool collide = Collide(block, shift);

  if (!collide) {
    block->pos[0] += shift[0];
    block->pos[1] += shift[1];
  } else if (shift[1] > 0 && shift[0] == 0) {
    // Only create new block if moving downward without horizontal movement
    newBlock(block);
  }
  // If colliding horizontally (shift[0] != 0), just don't move - don't create new block
}

void transpose(bool matrix[3][3]) {
  for (int i = 0; i < 3; i++) {
    for (int j = i + 1; j < 3; j++) {
      swap(matrix[i][j], matrix[j][i]);
    }
  }
}

void rotate(Block* block) {
  transpose(block->body);

  for (int i = 0; i < 3; i++) {
    swap(block->body[i][0], block->body[i][2]);
  }

  int bottom = 0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (block->body[i][j] && bottom < i) bottom = i;
    }
  }
  block->bottom = bottom;
}

void rotateBlock(Block* block) {
  Block test;

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      test.body[i][j] = block->body[i][j];
    }
  }
  test.pos[0] = block->pos[0];
  test.pos[1] = block->pos[1];
  test.bottom = block->bottom;

  rotate(&test);

  if (!Collide(&test, { 0, 0 })) {
    rotate(block);
  }
}

void gameSetup() {
  for (int i = 0; i < WIDTH / SIZE; i++) {
    for (int j = 0; j < HEIGHT / SIZE; j++) {
      BlockSpace[i][j] = bg2;
    }
  }

  for (int j = 0; j < HEIGHT / SIZE; j++) {
    for (int i = 0; i < WIDTH / SIZE; i++) {
      tft.fillRect(SIZE * (i + 1) + 1, SIZE * (j + 1) + 1 + 40, SIZE - 2, SIZE - 2, BlockSpace[i][j]);
    }
  }

  // Draw score with style
  tft.fillRect(0, 0, 170, 40, bg1);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.setCursor(10, 12);
  tft.print("SCORE: ");
  tft.setTextColor(TFT_YELLOW);
  tft.print(score);
  
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 28);
  tft.print("LINES: ");
  tft.setTextColor(TFT_GREEN);
  tft.print(linesCleared);

  // Initialize pieces
  nextPieceType = rand() % 7;
  aliveBlock.pos[0] = rand() % ((WIDTH / SIZE) - 3) + 1;
  aliveBlock.pos[1] = 0;
  newBody(&aliveBlock);
  
  drawNextPiece();
  drawBlock(aliveBlock);
}

void updateScore() {
  tft.fillRect(0, 0, 170, 40, bg1);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.setCursor(10, 12);
  tft.print("SCORE: ");
  tft.setTextColor(TFT_YELLOW);
  tft.print(score);
  
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 28);
  tft.print("LINES: ");
  tft.setTextColor(TFT_GREEN);
  tft.print(linesCleared);
}

void gamePlayLoop() {
  unsigned long currentMillis = millis();
  
  // Handle horizontal movement (LEFT/RIGHT)
  if (joystick == LEFT || joystick == RIGHT) {
    if (currentMillis - Shift_previousMillis >= 120) {  // Debounce for smooth movement
      direction[0] = (joystick == RIGHT) ? 1 : -1;
      direction[1] = 0;
      
      if (!Collide(&aliveBlock, direction)) {
        aliveBlock.pos[0] += direction[0];
        drawBlock(aliveBlock);
      }
      
      Shift_previousMillis = currentMillis;
      joystick = IDLE;  // Clear after processing
    }
  }
  
  // Handle rotation (DOWN)
  if (joystick == DOWN) {
    if (currentMillis - rotation_previousMillis >= 200) {  // Increased debounce for rotation
      rotateBlock(&aliveBlock);
      drawBlock(aliveBlock);
      rotation_previousMillis = currentMillis;
      joystick = IDLE;  // Clear after processing
    }
  }
  
  // Handle hard drop (UP)
  if (joystick == UP) {
    // Drop piece all the way down
    while (!Collide(&aliveBlock, {0, 1})) {
      aliveBlock.pos[1] += 1;
    }
    
    // Lock the piece immediately
    newBlock(&aliveBlock);
    drawBlock(aliveBlock);
    Game_previousMillis = currentMillis;  // Reset drop timer
    joystick = IDLE;  // Clear after processing
  }

  // Automatic downward movement (gravity)
  if (currentMillis - Game_previousMillis >= Game_interval) {
    Game_previousMillis = currentMillis;
    
    // Check for completed lines FIRST
    for (int j = 0; j < HEIGHT / SIZE; j++) {
      int count = 0;
      for (int i = 0; i < WIDTH / SIZE; i++) {
        if (BlockSpace[i][j] != bg2)
          count += 1;
        tft.fillRect(SIZE * (i + 1) + 1, SIZE * (j + 1) + 1 + 40, SIZE - 2, SIZE - 2, BlockSpace[i][j]);
      }

      if (count == WIDTH / SIZE) {
        score += 10;
        linesCleared++;
        
        // Create particles for line clear
        createTetrisParticles(SCREEN_W/2, j * SIZE + 40, TFT_YELLOW);
        
        tone(BUZZER_PIN, 800, 100);
        
        updateScore();
        
        // Shift all rows down
        for (int J = j; J >= 0; J--) {
          for (int i = 0; i < WIDTH / SIZE; i++) {
            if (J > 0)
              BlockSpace[i][J] = BlockSpace[i][J - 1];
            else
              BlockSpace[i][J] = bg2;
          }
        }
      }
    }
    
    // Then move piece down
    direction[0] = 0;
    direction[1] = 1;
    updateBody(&aliveBlock, direction);
    aliveBlock.bottom = drawBlock(aliveBlock);
  }
  
  // Update particle effects
  updateTetrisParticles();
}

void showTetrisGameOver() {
  static bool gameOverDrawn = false;
  
  if (!gameOverDrawn) {
    // Update high score
    if (score > tetrisHighScore) {
      tetrisHighScore = score;
      preferences.begin("tetris", false);
      preferences.putInt("highscore", tetrisHighScore);
      preferences.end();
    }
    
    // Game over box
    tft.fillRect(20, 80, 200, 120, bg1);
    tft.drawRect(20, 80, 200, 120, TFT_RED);
    tft.drawRect(21, 81, 198, 118, TFT_RED);
    tft.drawRect(22, 82, 196, 116, TFT_ORANGE);
    
    // Title
    tft.setTextFont(4);
    tft.setTextColor(0x4208);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("GAME OVER", SCREEN_H/2 + 2, 102);
    
    tft.setTextColor(TFT_RED);
    tft.drawString("GAME OVER", SCREEN_H/2, 100);
    
    // Scores
    tft.setTextFont(2);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("SCORE: " + String(score), SCREEN_H/2, 125);
    
    tft.setTextFont(1);
    tft.setTextColor(TFT_CYAN);
    tft.drawString("LINES: " + String(linesCleared), SCREEN_H/2, 142);
    
    // High score
    if (score >= tetrisHighScore) {
      tft.setTextColor(TFT_GREEN);
      tft.drawString("NEW HIGH SCORE!", SCREEN_H/2, 157);
    } else {
      tft.setTextColor(TFT_WHITE);
      tft.drawString("HIGH: " + String(tetrisHighScore), SCREEN_H/2, 157);
    }
    
    // Options
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SHOOT: Retry", SCREEN_W/2, 172);
    tft.setTextColor(TFT_ORANGE);
    tft.drawString("UP: Main Menu", SCREEN_W/2, 185);
    
    gameOverDrawn = true;
  }
  
  // Handle input
  if (command == 4) { // SHOOT - Retry
    command = -1;
    gameOverDrawn = false;
    score = 0;
    linesCleared = 0;
    joystick = IDLE;  // Clear joystick state
    game = START;
  }
  
  if (command == 1) { // UP - Main menu
    command = -1;
    joystick = IDLE;
    esp_restart();
  }
}

Block aliveBlock = {
  .body = {
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0}
  },
  .pos = {WIDTH/SIZE/2, 0},
  .color = TFT_WHITE,
  .bottom = 0,
  .type = 0
};

void setupTetris() {
  score = 0;
  linesCleared = 0;

  tft.begin();
  tft.setRotation(2);
  
  // Load high score
  preferences.begin("tetris", false);
  tetrisHighScore = preferences.getInt("highscore", 0);
  preferences.end();
  
  showTetrisMenu();
}

void loopTetris() {
  switch (game) {
    case MENU:
      {
        // Wait for any key to start
        if (command >= 0 && command != -1) {
          Serial.printf("Menu: Command %d received, starting game\n", command);
          game = START;
          command = -1;
          joystick = IDLE;
        }
        break;
      }
    case START:
      {
        tft.fillScreen(bg1);
        tft.setTextColor(TFT_WHITE, bg1);
        gameSetup();
        aliveBlock.bottom = drawBlock(aliveBlock);
        game = PLAY;
        
        // Clear any pending inputs
        command = -1;
        joystick = IDLE;
        direction[0] = 0;
        direction[1] = 1;
        
        Serial.println("Game started");
        break;
      }
    case PAUSE:
      {
        // Handle pause input
        if (command == 4) { // SHOOT - Resume
          unsigned long currentTime = millis();
          if (currentTime - lastPauseToggle >= PAUSE_DEBOUNCE) {
            game = PLAY;
            lastPauseToggle = currentTime;
            
            Serial.println("Resuming game");
            
            // Redraw the game screen
            tft.fillScreen(bg1);
            
            // Redraw all blocks
            for (int j = 0; j < HEIGHT / SIZE; j++) {
              for (int i = 0; i < WIDTH / SIZE; i++) {
                tft.fillRect(SIZE * (i + 1) + 1, SIZE * (j + 1) + 1 + 40, SIZE - 2, SIZE - 2, BlockSpace[i][j]);
              }
            }
            
            // Redraw current piece
            drawBlock(aliveBlock);
            
            // Redraw score
            updateScore();
            
            // Redraw next piece
            drawNextPiece();
            
            tone(BUZZER_PIN, 1200, 100); // Resume sound
          }
          command = -1;
          joystick = IDLE;
        }
        
        // Check for menu return
        if (command == 1) { // UP - Main menu
          Serial.println("Returning to menu from pause");
          command = -1;
          joystick = IDLE;
          esp_restart();
        }
        
        // Clear any other inputs while paused
        if (command != -1 && command != 4 && command != 1) {
          command = -1;
        }
        break;
      }
    case PLAY:
      {
        // Handle pause toggle FIRST, before any other input processing
        if (command == 4) { // SHOOT - Pause
          unsigned long currentTime = millis();
          if (currentTime - lastPauseToggle >= PAUSE_DEBOUNCE) {
            game = PAUSE;
            lastPauseToggle = currentTime;
            drawTetrisPauseScreen();
            tone(BUZZER_PIN, 1000, 100); // Pause sound
            Serial.println("Game paused");
          }
          command = -1;
          joystick = IDLE;
          break;  // Don't process game loop this frame
        }
        
        // Map commands to joystick directions ONLY if no joystick action is pending
        if (command >= 0 && joystick == IDLE) {
          switch (command) {
            case 0:  // UP button - mapped to RIGHT movement
              joystick = RIGHT;
              Serial.println("RIGHT");
              break;
            case 1:  // RIGHT button - mapped to UP (Hard drop)
              joystick = UP;
              Serial.println("UP (Hard Drop)");
              break;
            case 2:  // DOWN button - mapped to LEFT movement
              joystick = LEFT;
              Serial.println("LEFT");
              break;
            case 3:  // LEFT button - mapped to DOWN (Rotate)
              joystick = DOWN;
              Serial.println("DOWN (Rotate)");
              break;
            default:
              Serial.println("Unknown command");
              break;
          }
          command = -1;  // Consume command after mapping
        }
        
        // Run the game loop
        gamePlayLoop();
        break;
      }
    case END:
      {
        showTetrisGameOver();
        break;
      }
    default:
      break;
  }
}