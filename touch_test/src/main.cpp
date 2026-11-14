#include <SPI.h>
#include <TFT_eSPI.h>

#define CS_PIN 21
TFT_eSPI screen;

// Read a single 12-bit XPT2046 channel at 100 kHz. This is deliberately much
// slower than the normal library driver, so it exposes signal-integrity faults.
static uint16_t readTouchChannel(uint8_t command) {
  SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(command);
  const uint16_t value = SPI.transfer16(0x0000) >> 3;
  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();
  return value;
}

void setup() {
  Serial.begin(115200);
  digitalWrite(9, HIGH);
  pinMode(9, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TFT_CS, OUTPUT);

  screen.init();
  screen.setRotation(2);
  screen.fillScreen(TFT_BLACK);
  screen.setTextDatum(MC_DATUM);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.drawString("SLOW TOUCH DIAGNOSTIC", 120, 65, 2);
  screen.drawString("100 kHz direct SPI read", 120, 110, 2);
  screen.drawString("Readings sent over USB", 120, 140, 2);
  screen.drawString("CS21 / MISO7", 120, 190, 2);

  SPI.end();
  SPI.begin(4, TFT_MISO, 0, -1);
  while (!Serial && (millis() <= 1000));
  Serial.printf("Slow raw test ready: CLK4 MOSI0 MISO%d CS21\n", TFT_MISO);
}

void loop() {
  const uint16_t x = readTouchChannel(0xD0);
  const uint16_t y = readTouchChannel(0x90);
  const uint16_t z1 = readTouchChannel(0xB0);
  const uint16_t z2 = readTouchChannel(0xC0);
  Serial.printf("X=%4u  Y=%4u  Z1=%4u  Z2=%4u\n", x, y, z1, z2);
  delay(100);
}
