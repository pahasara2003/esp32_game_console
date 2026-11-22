#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <TFT_eSPI.h>
#include "../../src/ui_kit.h"

constexpr uint8_t SDA_PIN = 10;
constexpr uint8_t SCL_PIN = 20;
constexpr uint8_t DIGITAL_PIN = 8;
constexpr uint8_t ADS1115_FIRST_ADDRESS = 0x48;
constexpr uint8_t ADS1115_LAST_ADDRESS = 0x4B;

Adafruit_ADS1115 ads;
TFT_eSPI tft;
bool adsDetected = false;
uint8_t adsAddress = 0;
constexpr int PLOT_LEFT = 48;
constexpr int PLOT_RIGHT = 222;
constexpr int TRACK_TOP = 62;
constexpr int TRACK_STEP = 49;
constexpr int TRACK_HEIGHT = 38;
constexpr uint16_t TRACK_COLORS[5] = {
    TFT_CYAN, TFT_ORANGE, TFT_GREEN, TFT_MAGENTA, TFT_YELLOW};
int plotX = PLOT_LEFT;
int previousY[5] = {};

// Reuse the console's shared UI helpers in this standalone test build.
#include "../../src/ui_kit.cpp"

void showStatus(const char *title, const char *detail, uint16_t color) {
  tft.fillScreen(TFT_BLACK);
  ui::card(14, 54, 212, 204);
  ui::heading(30, 72, "I2C test", "ADS1115");
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(color, ui::CARD);
  tft.drawString(title, 30, 140, 4);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString(detail, 30, 180, 2);
  ui::hint(282, "SDA 10  SCL 20", TFT_BLACK);
}

void showGraph() {
  tft.fillScreen(TFT_BLACK);
  ui::heading(16, 10, "Live input", "ADS1115", TFT_BLACK);
  for (uint8_t channel = 0; channel < 5; ++channel) {
    const int top = TRACK_TOP + channel * TRACK_STEP;
    tft.fillRect(14, top - 5, 212, TRACK_HEIGHT + 10, ui::CARD);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TRACK_COLORS[channel], ui::CARD);
    char label[3] = {channel == 4 ? 'D' : 'A',
                     channel == 4 ? '8' : char('0' + channel), '\0'};
    tft.drawString(label, 20, top + TRACK_HEIGHT / 2, 2);
    previousY[channel] = -1;
  }
  plotX = PLOT_LEFT;
}

int traceY(uint8_t channel, int16_t reading) {
  const int top = TRACK_TOP + channel * TRACK_STEP;
  const int32_t value = constrain(int32_t(reading), 0, 32767);
  return top + TRACK_HEIGHT - 1 - value * (TRACK_HEIGHT - 1) / 32767;
}

int16_t readSettledADC(uint8_t channel) {
  // Match the console button scanner: the first conversion after a MUX
  // change can still reflect the previous channel's external button network.
  ads.readADC_SingleEnded(channel);
  return ads.readADC_SingleEnded(channel);
}

void detectAds() {
  adsDetected = false;
  for (uint8_t address = ADS1115_FIRST_ADDRESS;
       address <= ADS1115_LAST_ADDRESS; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("I2C device responded at 0x%02X\n", address);
      if (ads.begin(address, &Wire)) {
        adsAddress = address;
        adsDetected = true;
        break;
      }
    }
  }

  if (adsDetected) {
    ads.setGain(GAIN_ONE);
    ads.setDataRate(RATE_ADS1115_860SPS);
    showGraph();
    Serial.printf("DETECTED: device at address 0x%02X\n", adsAddress);
  } else {
    showStatus("Not found", "Checked 0x48 to 0x4B", TFT_RED);
    Serial.println("NOT DETECTED: no device at 0x48-0x4B.");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(DIGITAL_PIN, INPUT_PULLUP);
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);  // Keep the SD card off the shared SPI bus.
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  delay(150);
  tft.init();
  tft.setRotation(2);
  showStatus("Checking", "Scanning I2C bus...", TFT_WHITE);

  Serial.println();
  Serial.println("ADS1115 detection test");
  Serial.printf("I2C: SDA GPIO%u, SCL GPIO%u\n", SDA_PIN, SCL_PIN);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  Wire.setTimeOut(20);

  detectAds();
}

void loop() {
  if (!adsDetected) {
    delay(2000);
    detectAds();
    return;
  }

  if (plotX > PLOT_RIGHT) showGraph();

  for (uint8_t channel = 0; channel < 4; ++channel) {
    const int y = traceY(channel, readSettledADC(channel));
    if (previousY[channel] >= 0)
      tft.drawLine(plotX - 1, previousY[channel], plotX, y,
                   TRACK_COLORS[channel]);
    else
      tft.drawPixel(plotX, y, TRACK_COLORS[channel]);
    previousY[channel] = y;
  }

  const int digitalTop = TRACK_TOP + 4 * TRACK_STEP;
  const int digitalY = digitalRead(DIGITAL_PIN) == HIGH
                           ? digitalTop + 4
                           : digitalTop + TRACK_HEIGHT - 5;
  if (previousY[4] >= 0) {
    tft.drawFastHLine(plotX - 1, previousY[4], 2, TRACK_COLORS[4]);
    if (previousY[4] != digitalY)
      tft.drawLine(plotX, previousY[4], plotX, digitalY, TRACK_COLORS[4]);
  } else {
    tft.drawPixel(plotX, digitalY, TRACK_COLORS[4]);
  }
  previousY[4] = digitalY;
  ++plotX;
}
