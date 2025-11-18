#include "esp32-hal.h"


// ===== CUSTOMIZE THESE =====
String recipientName = "Chamathka";
String customMessage1 = "Wishing you a day filled with";     // First line of custom message
String customMessage2 = "joy, laughter & amazing moments!";  // Second line
String customMessage3 = "With love, Pahasara";               // Third line (leave empty if not needed)
// ============================

// Birthday state
enum BirthdayState {
  SURPRISE_MENU,
  BIRTHDAY_ANIMATION,
  JOURNEY_PROMPT
};

BirthdayState birthdayState = SURPRISE_MENU;

// Visual effects
struct Confetti {
  float x, y;
  float speedX, speedY;
  uint16_t color;
  int size;
};

struct Balloon {
  float x, y;
  float speed;
  uint16_t color;
  int wobble;
};

struct BirthdayStar {
  int x, y;
  int brightness;
  uint16_t color;
};

Confetti confetti[40];
Balloon balloons[8];
BirthdayStar bdayStars[50];

// Music control (NON-BLOCKING)
int currentNote = 0;
unsigned long lastNoteTime = 0;
unsigned long noteStartTime = 0;
bool musicPlaying = false;
bool noteIsPlaying = false;
int noteCount = 0;
int songLoopCount = 0;

// Animation control
unsigned long animationStartTime = 0;
unsigned long lastAnimUpdate = 0;
int textPulse = 0;
int textPulseDir = 1;
int cakeFlameFrame = 0;
unsigned long lastFlameUpdate = 0;

// Colors
uint16_t birthdayColors[] = {
  TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN,
  TFT_CYAN, TFT_BLUE, TFT_MAGENTA, TFT_PINK
};

// TFT object (Assuming its initialization is done externally or in setup)

// =================================================================
//                         FUNCTION PROTOTYPES
// =================================================================
void setupBirthday();
void loopBirthday();
void showSurpriseMenu();
void initBirthdayEffects();
void updateConfetti();
void updateBalloons();
void updateStarsBd();
void drawCake();
void drawBirthdayText();
void showBirthdayAnimation();
void showJourneyPrompt();

// =================================================================
//                    MUSIC LOGIC (RE-ADDED FOR COMPLETENESS)
// =================================================================

void playBirthdayMusicNonBlocking() {
  playHappyBirthday();
}


// =================================================================
//                         VISUAL EFFECTS INIT
// =================================================================
void initBirthdayEffects() {
  // Initialize confetti
  for (int i = 0; i < 40; i++) {
    confetti[i].x = random(SCREEN_W);
    confetti[i].y = random(-200, 0);
    confetti[i].speedX = random(-2, 3) * 0.5;
    confetti[i].speedY = random(1, 4);
    confetti[i].color = birthdayColors[random(8)];
    confetti[i].size = random(2, 5);
  }

  // Initialize balloons
  for (int i = 0; i < 8; i++) {
    balloons[i].x = 20 + i * 35;
    balloons[i].y = SCREEN_H + random(50, 150);
    balloons[i].speed = random(5, 15) * 0.1;
    balloons[i].color = birthdayColors[i];
    balloons[i].wobble = 0;
  }

  // Initialize stars
  for (int i = 0; i < 50; i++) {
    bdayStars[i].x = random(SCREEN_W);
    bdayStars[i].y = random(SCREEN_H);
    bdayStars[i].brightness = random(100, 255);
    bdayStars[i].color = TFT_WHITE;
  }

  noteCount = sizeof(happyBirthdayMelody) / sizeof(happyBirthdayMelody[0]);
  currentNote = 0;
  musicPlaying = false;
  noteIsPlaying = false;
  songLoopCount = 0;
}

// =================================================================
//                         VISUAL EFFECTS
// =================================================================
void updateConfetti() {
  for (int i = 0; i < 40; i++) {
    // Clear old position
    tft.fillCircle((int)confetti[i].x, (int)confetti[i].y, confetti[i].size, TFT_BLACK);

    // Update position
    confetti[i].y += confetti[i].speedY;
    confetti[i].x += confetti[i].speedX;

    // Wrap around
    if (confetti[i].y > SCREEN_H) {
      confetti[i].y = -10;
      confetti[i].x = random(SCREEN_W);
    }
    if (confetti[i].x < 0) confetti[i].x = SCREEN_W;
    if (confetti[i].x > SCREEN_W) confetti[i].x = 0;

    // Draw new position
    tft.fillCircle((int)confetti[i].x, (int)confetti[i].y, confetti[i].size, confetti[i].color);
  }
}

void updateBalloons() {
  for (int i = 0; i < 8; i++) {
    // Update position
    balloons[i].y -= balloons[i].speed;
    balloons[i].wobble += 5;

    // Reset when off screen
    if (balloons[i].y < -50) {
      balloons[i].y = SCREEN_H + 50;
      balloons[i].speed = random(5, 15) * 0.1;
    }

    // Calculate wobble
    int wobbleOffset = sin(balloons[i].wobble * 0.05) * 3;
    int x = balloons[i].x + wobbleOffset;
    int y = balloons[i].y;

    // Draw balloon
    tft.fillCircle(x, y, 12, balloons[i].color);
    tft.fillCircle(x - 3, y - 3, 3, TFT_WHITE);  // Highlight

    // Draw string
    tft.drawLine(x, y + 12, x + 2, y + 25, TFT_WHITE);
  }
}

void updateStarsBd() {
  for (int i = 0; i < 50; i++) {
    // Twinkling effect
    bdayStars[i].brightness += random(-20, 21);
    if (bdayStars[i].brightness > 255) bdayStars[i].brightness = 255;
    if (bdayStars[i].brightness < 50) bdayStars[i].brightness = 50;

    uint16_t color = tft.color565(bdayStars[i].brightness, bdayStars[i].brightness, bdayStars[i].brightness);
    tft.drawPixel(bdayStars[i].x, bdayStars[i].y, color);
  }
}

void drawCake() {
  int cakeX = SCREEN_W / 2 - 30;
  int cakeY = SCREEN_H - 65;

  // Cake layers
  // Bottom layer
  tft.fillRoundRect(cakeX, cakeY + 20, 60, 30, 3, TFT_ORANGE);
  tft.drawRoundRect(cakeX, cakeY + 20, 60, 30, 3, TFT_RED);

  // Middle layer
  tft.fillRoundRect(cakeX + 5, cakeY + 5, 50, 20, 3, TFT_PINK);
  tft.drawRoundRect(cakeX + 5, cakeY + 5, 50, 20, 3, TFT_MAGENTA);

  // Top layer
  tft.fillRoundRect(cakeX + 10, cakeY - 10, 40, 20, 3, TFT_YELLOW);
  tft.drawRoundRect(cakeX + 10, cakeY - 10, 40, 20, 3, TFT_ORANGE);

  // Decorations (frosting dots)
  for (int i = 0; i < 5; i++) {
    tft.fillCircle(cakeX + 10 + i * 10, cakeY + 35, 2, TFT_CYAN);
    tft.fillCircle(cakeX + 15 + i * 8, cakeY + 15, 2, TFT_WHITE);
  }

  // Candles
  int candlePos[] = { cakeX + 15, cakeX + 30, cakeX + 45 };
  for (int i = 0; i < 3; i++) {
    // Candle body
    tft.fillRect(candlePos[i], cakeY - 25, 4, 15, TFT_WHITE);
    tft.drawRect(candlePos[i], cakeY - 25, 4, 15, TFT_BLUE);

    // Flame (animated)
    uint16_t flameColor = (cakeFlameFrame % 2 == 0) ? TFT_YELLOW : TFT_ORANGE;
    tft.fillCircle(candlePos[i] + 2, cakeY - 28, 3, flameColor);
    tft.fillCircle(candlePos[i] + 2, cakeY - 30, 2, TFT_RED);
  }
}

void drawBirthdayText() {
  // Pulsing effect
  textPulse += textPulseDir;
  if (textPulse > 10) textPulseDir = -1;
  if (textPulse < 0) textPulseDir = 1;

  int yOffset = 30 + textPulse / 2;

  // Draw "HAPPY BIRTHDAY" with rainbow colors
  tft.setTextFont(4);
  tft.setTextDatum(MC_DATUM);

  String text1 = "HAPPY";
  String text2 = "BIRTHDAY";

  // Shadow
  tft.setTextColor(0x4208);
  tft.drawString(text1, SCREEN_W / 2 + 2, yOffset + 2);
  tft.drawString(text2, SCREEN_W / 2 + 2, yOffset + 32);

  // Main text (rainbow cycling)
  int colorIndex = (millis() / 200) % 8;
  tft.setTextColor(birthdayColors[colorIndex]);
  tft.drawString(text1, SCREEN_W / 2, yOffset);

  tft.setTextColor(birthdayColors[(colorIndex + 4) % 8]);
  tft.drawString(text2, SCREEN_W / 2, yOffset + 30);

  // Recipient name
  tft.setTextFont(2);
  tft.setTextColor(TFT_PINK);
  tft.drawString("Dear " + recipientName, SCREEN_W / 2, yOffset + 55);

  // ===== CUSTOM MESSAGE SECTION =====
  tft.setTextFont(1);
  int customY = yOffset + 75;

  // Custom message line 1
  if (customMessage1.length() > 0) {
    tft.setTextColor(TFT_YELLOW);
    tft.drawString(customMessage1, SCREEN_W / 2, customY);
    customY += 12;
  }

  // Custom message line 2
  if (customMessage2.length() > 0) {
    tft.setTextColor(TFT_CYAN);
    tft.drawString(customMessage2, SCREEN_W / 2, customY);
    customY += 12;
  }

  // Custom message line 3
  if (customMessage3.length() > 0) {
    tft.setTextColor(TFT_GREEN);
    tft.drawString(customMessage3, SCREEN_W / 2, customY);
  }
  // ==================================

  // Decorative stars around text
  for (int i = 0; i < 6; i++) {
    int starX = SCREEN_W / 2 - 80 + random(160);
    int starY = yOffset - 10 + random(70);
    tft.fillCircle(starX, starY, 1, TFT_YELLOW);
  }
}

// =================================================================
//                         MENU
// =================================================================
void showSurpriseMenu() {
  tft.fillScreen(TFT_BLACK);

  // Draw mysterious background with stars
  for (int i = 0; i < 30; i++) {
    int x = random(SCREEN_W);
    int y = random(SCREEN_H);
    int brightness = random(100, 255);
    tft.drawPixel(x, y, tft.color565(brightness, brightness, brightness));
  }

  // Mystery box icon
  int boxX = SCREEN_W / 2 - 25;
  int boxY = 60;

  // Gift box
  tft.fillRect(boxX, boxY, 50, 40, TFT_RED);
  tft.drawRect(boxX, boxY, 50, 40, TFT_ORANGE);

  // Ribbon vertical
  tft.fillRect(boxX + 20, boxY, 10, 40, TFT_YELLOW);

  // Lid
  tft.fillRect(boxX - 5, boxY - 10, 60, 10, TFT_MAGENTA);
  tft.drawRect(boxX - 5, boxY - 10, 60, 10, TFT_PINK);

  // Bow
  tft.fillCircle(boxX + 15, boxY - 15, 5, TFT_YELLOW);
  tft.fillCircle(boxX + 35, boxY - 15, 5, TFT_YELLOW);
  tft.fillCircle(boxX + 25, boxY - 15, 4, TFT_ORANGE);

  // Sparkles around gift
  for (int i = 0; i < 8; i++) {
    int sx = boxX + 25 + cos(i * 0.785) * 40;
    int sy = boxY + 20 + sin(i * 0.785) * 40;
    tft.fillCircle(sx, sy, 2, birthdayColors[i]);
  }

  // Title with mystery
  tft.setTextFont(4);
  tft.setTextColor(0x4208);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("SURPRISE", SCREEN_W / 2 + 2, 132);

  tft.setTextColor(TFT_YELLOW);
  tft.drawString("SURPRISE", SCREEN_W / 2, 130);

  tft.setTextColor(0x4208);
  tft.drawString("AWAITS!", SCREEN_W / 2 + 2, 162);

  tft.setTextColor(TFT_CYAN);
  tft.drawString("AWAITS!", SCREEN_W / 2, 160);

  // Decorative lines
  tft.drawFastHLine(30, 185, SCREEN_W - 60, TFT_MAGENTA);
  tft.drawFastHLine(30, 187, SCREEN_W - 60, TFT_PINK);

  // Mystery hint
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Something special is waiting...", SCREEN_W / 2, 200);

  // Animated dots
  String dots = "";
  int dotCount = (millis() / 500) % 4;
  for (int i = 0; i < dotCount; i++) {
    dots += ".";
  }
  tft.setTextColor(TFT_GREEN);
  tft.drawString("Preparing" + dots, SCREEN_W / 2, 215);

  // Action prompt with animation
  tft.setTextFont(2);
  int promptBrightness = 100 + (sin(millis() * 0.005) + 1) * 77;
  uint16_t promptColor = tft.color565(promptBrightness, promptBrightness, 0);
  tft.setTextColor(promptColor);
  tft.drawString("PRESS SHOOT", SCREEN_W / 2, 235);

  tft.setTextFont(1);
  tft.setTextColor(TFT_ORANGE);
  tft.drawString("to be surprised!", SCREEN_W / 2, 255);

  Serial.println("Surprise menu displayed");
}

// =================================================================
//                         ANIMATION
// =================================================================
void showBirthdayAnimation() {
  unsigned long currentTime = millis();

  // Check if 30 seconds have passed
  if (currentTime - animationStartTime >= 10000) {
    musicPlaying = false;
    noTone(BUZZER_PIN);
    birthdayState = JOURNEY_PROMPT;
    return;
  }

  // Update confetti
  if (currentTime - lastAnimUpdate >= 50) {
    updateConfetti();
    lastAnimUpdate = currentTime;
  }

  // Update stars (twinkling)
  if (currentTime % 100 == 0) {
    updateStarsBd();
  }

  // Update cake flame animation
  if (currentTime - lastFlameUpdate >= 200) {
    cakeFlameFrame++;
    lastFlameUpdate = currentTime;
  }

  // Clear and redraw main elements
  tft.fillRect(0, 0, SCREEN_W, 150, TFT_BLACK);  // Increased height for custom message
  tft.fillRect(SCREEN_W / 2 - 60, SCREEN_H - 110, 120, 90, TFT_BLACK);

  // Draw elements
  drawBirthdayText();
  drawCake();

  // Draw balloon layer (over confetti)
  for (int i = 0; i < 8; i++) {
    if (balloons[i].y < SCREEN_H - 50) {
      int wobbleOffset = sin(balloons[i].wobble * 0.05) * 3;
      int x = balloons[i].x + wobbleOffset;
      int y = balloons[i].y;

      tft.fillCircle(x, y, 12, balloons[i].color);
      tft.fillCircle(x - 3, y - 3, 3, TFT_WHITE);
      tft.drawLine(x, y + 12, x + 2, y + 25, TFT_WHITE);
    }
  }

  // Update balloons
  for (int i = 0; i < 8; i++) {
    balloons[i].y -= balloons[i].speed;
    balloons[i].wobble += 5;

    if (balloons[i].y < -50) {
      balloons[i].y = SCREEN_H + 50;
      balloons[i].speed = random(5, 15) * 0.1;
    }
  }

  // Play music (non-blocking)
  playBirthdayMusicNonBlocking();
}

// =================================================================
//                         JOURNEY PROMPT
// =================================================================
void showJourneyPrompt() {
  static bool promptDrawn = false;

  if (!promptDrawn) {
    tft.fillScreen(TFT_BLACK);

    // Draw starfield
    for (int i = 0; i < 50; i++) {
      int x = random(SCREEN_W);
      int y = random(SCREEN_H);
      int brightness = random(100, 255);
      tft.drawPixel(x, y, tft.color565(brightness, brightness, brightness));
    }

    // Draw portal/door effect
    int cx = SCREEN_W / 2;
    int cy = SCREEN_H / 2;

    for (int r = 60; r > 0; r -= 10) {
      int colorVal = map(r, 0, 60, 0, 255);
      uint16_t color = tft.color565(colorVal / 4, colorVal / 2, colorVal);
      tft.drawCircle(cx, cy, r, color);
    }

    // Main message box
    tft.fillRect(20, 80, SCREEN_W - 40, 140, tft.color565(10, 0, 30));
    tft.drawRect(20, 80, SCREEN_W - 40, 140, TFT_CYAN);
    tft.drawRect(21, 81, SCREEN_W - 42, 138, TFT_MAGENTA);

    // Title
    tft.setTextFont(4);
    tft.setTextColor(0x4208);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("READY TO", SCREEN_W / 2 + 2, 102);

    tft.setTextColor(TFT_GREEN);
    tft.drawString("READY TO", SCREEN_W / 2, 100);

    tft.setTextColor(0x4208);
    tft.drawString("START THE", SCREEN_W / 2 + 2, 132);

    tft.setTextColor(TFT_YELLOW);
    tft.drawString("START THE", SCREEN_W / 2, 130);

    tft.setTextColor(0x4208);
    tft.drawString("JOURNEY?", SCREEN_W / 2 + 2, 162);

    tft.setTextColor(TFT_CYAN);
    tft.drawString("JOURNEY?", SCREEN_W / 2, 160);

    // Prompt
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("PRESS SHOOT", SCREEN_W / 2, 190);

    tft.setTextFont(1);
    tft.setTextColor(TFT_ORANGE);
    tft.drawString("to begin your adventure!", SCREEN_W / 2, 210);

    // Sparkles
    for (int i = 0; i < 15; i++) {
      int sx = random(SCREEN_W);
      int sy = random(SCREEN_H);
      tft.fillCircle(sx, sy, 1, birthdayColors[random(8)]);
    }

    promptDrawn = true;
  }


  // Handle input
  if (command == 4) {  // SHOOT - Start journey
    command = -1;

    preferences.begin("gameConsole", false);  // 'false' means read-write mode

    // Clear the preferences memory
    preferences.putBool("firstBoot", false);

    // Print a message to confirm clearing

    // End preferences session
    preferences.end();
      // Note: esp_restart() requires ESP32/ESP8266 board
      // For other boards, you might need a different command.
      // For the context of this game console, we'll keep it.
      esp_restart();
  }
}

// =================================================================
//                         SETUP & LOOP
// =================================================================
void setupBirthday() {
  tft.init();
  tft.setRotation(3);
  pinMode(BUZZER_PIN, OUTPUT);

  showSurpriseMenu();
}

void loopBirthday() {
  last_action = millis();
  switch (birthdayState) {
    case SURPRISE_MENU:
      {
        // Animate the menu
        static unsigned long lastMenuUpdate = 0;
        if (millis() - lastMenuUpdate >= 100) {

          // --- FIX: CLEAR PREPARING... AREA ---
          // Use a fixed width clearing area large enough for "Preparing..." and the pulsing dots.
          int prep_x = SCREEN_W / 2;
          int prep_y = 215;
          int clear_w = 120;  // Width to clear
          int clear_h = 10;

          // Clear the old "Preparing..." string area
          tft.fillRect(prep_x - (clear_w / 2), prep_y - (clear_h / 2), clear_w, clear_h, TFT_BLACK);

          // Animate dots
          String dots = "";
          int dotCount = (millis() / 500) % 4;
          for (int i = 0; i < dotCount; i++) {
            dots += ".";
          }
          tft.setTextFont(1);
          tft.setTextColor(TFT_GREEN);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("Preparing" + dots, prep_x, prep_y);

          // --- FIX: CLEAR PRESS SHOOT AREA ---
          int prompt_x = SCREEN_W / 2;
          int prompt_y = 235;

          // Clear the old PRESS SHOOT string area
          tft.fillRect(prompt_x - 70, prompt_y - 10, 140, 20, TFT_BLACK);  // Clear area wide enough for text

          // Animate prompt brightness
          int promptBrightness = 100 + (sin(millis() * 0.005) + 1) * 77;
          uint16_t promptColor = tft.color565(promptBrightness, promptBrightness, 0);
          tft.setTextFont(2);
          tft.setTextColor(promptColor);
          tft.drawString("PRESS SHOOT", prompt_x, prompt_y);

          lastMenuUpdate = millis();
        }

        // Wait for SHOOT button
        if (command == 4) {
          command = -1;
          tft.fillScreen(TFT_BLACK);
          initBirthdayEffects();
          musicPlaying = true;
          noteIsPlaying = false;
          currentNote = 0;
          animationStartTime = millis();
          birthdayState = BIRTHDAY_ANIMATION;
        }
        break;
      }

    case BIRTHDAY_ANIMATION:
      {
        showBirthdayAnimation();
        break;
      }

    case JOURNEY_PROMPT:
      {
        showJourneyPrompt();
        break;
      }
  }
}
// END OF FILE