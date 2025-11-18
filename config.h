#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "esp_system.h"

int score = 0;
int highscore = 0;

const int SCREEN_W = 320;
const int SCREEN_H = 240;
#define BG_COLOR     0x18E5
#define TILE_COLOR   0x18C4
#define TEXT_COLOR   TFT_CYAN
#define TITLE_COLOR  0x57E0
#define DISPLAY_PIN 6
#define BUZZER_PIN 10

TFT_eSPI tft = TFT_eSPI();
Adafruit_ADS1115 ads;

// Button configuration
struct Button {
  const char* name;
  int channel;       // ADS channel (0–3)
};

Button buttons[] = {
  {"UP",    0},
  {"RIGHT", 1},
  {"DOWN",  2},
  {"LEFT",  3}
};
int command = -1;
const int NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);
unsigned long lastUpdate = 0;
unsigned long UPDATE_INTERVAL = 50; // ms


void beginKeyboard(){
  Wire.begin(8, 9); // SDA=8, SCL=9
  if (!ads.begin(0x48)) {
    while (1);
  }
  ads.setGain(GAIN_ONE); // ±4.096V
}

// Creative sleep animation with sound effects
void goToSleep() {
  tft.fillScreen(BG_COLOR);
  
  // Draw sleeping character (cute pixel art)
  int centerX = SCREEN_W / 2;
  int centerY = SCREEN_H / 2;
  
  // === Stage 1: Character appears ===
  // Head
  tft.fillCircle(centerX, centerY - 20, 15, 0xFD20); // Peach skin
  
  // Body
  tft.fillRoundRect(centerX - 12, centerY - 5, 24, 25, 5, TFT_CYAN); // Pajamas
  
  // Arms (sleeping position)
  tft.fillRect(centerX - 18, centerY, 6, 12, 0xFD20);
  tft.fillRect(centerX + 12, centerY, 6, 12, 0xFD20);
  
  // Closed eyes
  tft.fillRect(centerX - 8, centerY - 22, 4, 2, TFT_BLACK);
  tft.fillRect(centerX + 4, centerY - 22, 4, 2, TFT_BLACK);
  
  // Smile
  tft.drawArc(centerX, centerY - 18, 5, 3, 0, 180, TFT_BLACK, TFT_BLACK);
  
  // Nightcap
  tft.fillTriangle(centerX - 15, centerY - 25, 
                   centerX + 15, centerY - 25,
                   centerX + 8, centerY - 40, TFT_PURPLE);
  tft.fillCircle(centerX + 8, centerY - 40, 3, TFT_WHITE); // Pom pom
  
  delay(300);
  
  // === Stage 2: Text with typewriter effect ===
  tft.setTextFont(2);
  tft.setTextColor(TEXT_COLOR);
  tft.setTextDatum(MC_DATUM);
  
  String sleepText = "Going to sleep...";
  int textX = centerX;
  int textY = centerY + 40;
  
  for (int i = 0; i <= sleepText.length(); i++) {
    tft.fillRect(textX - 80, textY - 8, 160, 20, BG_COLOR);
    tft.drawString(sleepText.substring(0, i), textX, textY);
    tone(BUZZER_PIN, 800, 50); // Soft beep for each letter
    delay(100);
  }
  
  delay(300);
  
  // === Stage 3: Z's animation (sleep symbols) ===
  for (int z = 0; z < 3; z++) {
    // Draw Z rising up
    for (int rise = 0; rise < 30; rise += 3) {
      // Erase old Z
      if (rise > 0) {
        tft.setTextColor(BG_COLOR);
        tft.drawString("Z", centerX + 25 + z * 8, centerY - 30 - rise + 3);
      }
      
      // Draw new Z
      tft.setTextColor(TFT_YELLOW);
      tft.setTextFont(2 + (z / 2)); // Bigger Z's as they go
      tft.drawString("Z", centerX + 25 + z * 8, centerY - 30 - rise);
      
      // Gentle beep that rises in pitch
      tone(BUZZER_PIN, 400 + rise * 10, 30);
      delay(50);
    }
    delay(200);
  }
  
  // === Stage 4: Stars twinkle ===
  int stars[8][2] = {
    {30, 40}, {50, 60}, {280, 50}, {290, 80},
    {40, 180}, {270, 190}, {150, 30}, {200, 200}
  };
  
  for (int blink = 0; blink < 3; blink++) {
    for (int i = 0; i < 8; i++) {
      // Draw star
      tft.fillCircle(stars[i][0], stars[i][1], 2, TFT_YELLOW);
      tft.drawLine(stars[i][0] - 4, stars[i][1], stars[i][0] + 4, stars[i][1], TFT_YELLOW);
      tft.drawLine(stars[i][0], stars[i][1] - 4, stars[i][0], stars[i][1] + 4, TFT_YELLOW);
    }
    tone(BUZZER_PIN, 1000, 100);
    delay(300);
    
    // Erase stars
    for (int i = 0; i < 8; i++) {
      tft.fillCircle(stars[i][0], stars[i][1], 4, BG_COLOR);
      tft.drawLine(stars[i][0] - 4, stars[i][1], stars[i][0] + 4, stars[i][1], BG_COLOR);
      tft.drawLine(stars[i][0], stars[i][1] - 4, stars[i][0], stars[i][1] + 4, BG_COLOR);
    }
    delay(200);
  }
  
  // === Stage 5: Moon appears ===
  tft.fillCircle(50, 50, 20, TFT_YELLOW);
  tft.fillCircle(58, 50, 20, BG_COLOR); // Crescent effect
  tone(BUZZER_PIN, 600, 200);
  delay(300);
  
  // === Stage 6: Countdown beeps ===
  tft.setTextFont(4);
  tft.setTextColor(TFT_CYAN);
  
  for (int count = 3; count > 0; count--) {
    tft.fillRect(centerX - 20, centerY + 60, 40, 30, BG_COLOR);
    tft.drawString(String(count), centerX, centerY + 75);
    
    // Beep beep sound
    tone(BUZZER_PIN, 1200, 100);
    delay(150);
    tone(BUZZER_PIN, 1200, 100);
    delay(650);
  }
  
  // === Stage 7: Fade out effect ===
  tft.fillRect(centerX - 20, centerY + 60, 40, 30, BG_COLOR);
  tft.setTextFont(2);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("Good Night!", centerX, centerY + 75);
  
  // Final melody
  tone(BUZZER_PIN, 800, 150);
  delay(200);
  tone(BUZZER_PIN, 600, 150);
  delay(200);
  tone(BUZZER_PIN, 400, 300);
  delay(400);
  
  // Dim effect - draw semi-transparent rectangles
  for (int fade = 0; fade < 5; fade++) {
    for (int y = 0; y < SCREEN_H; y += 10) {
      tft.drawFastHLine(0, y + fade * 2, SCREEN_W, BG_COLOR);
    }
    delay(100);
  }
  
  // Final screen
  tft.fillScreen(BG_COLOR);
  tft.setTextFont(2);
  tft.setTextColor(0x4208); // Dim gray
  tft.drawString("zzz...", centerX, centerY);
  
  delay(500);
  
  // Turn off display and sleep
  digitalWrite(DISPLAY_PIN, LOW);
  esp_deep_sleep_start();
}



