#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include <Wire.h>
#include "config.h"
#include "buttons.h"
extern Adafruit_ADS1115 ads;
extern bool adsConnected;
namespace {
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
constexpr uint8_t CAPACITY = 16;
// Keep intentional taps through a slow redraw; screen transitions clear the queue.
constexpr unsigned long EVENT_LIFETIME_MS = 1500;
// ADC buttons are active-low: idle reads ~26300 (3.3 V pull-up), pressed ~0.
// A press is a reading below PRESS_LEVEL; release needs one above RELEASE_LEVEL.
constexpr int16_t PRESS_LEVEL = 9000, RELEASE_LEVEL = 17000;
// 860 SPS conversion is ~1.16 ms; check it no sooner than this.
constexpr unsigned long CONVERSION_US = 1200;
unsigned long queuedAt[CAPACITY];
uint8_t events[CAPACITY], head = 0, count = 0, held = 0;
uint8_t blocked = 0x1f;
uint8_t debugHeld = 0;
unsigned long debugUntil = 0;
int16_t lastRaw[4] = {};
uint32_t busFaults = 0, pressCount = 0, expiredCount = 0;
int16_t peakRaw[4] = {};
uint32_t recordedPresses[5] = {}, recordedReads[5] = {};
uint32_t recordingStarted = 0, recordingDuration = 0;
TaskHandle_t scanner = nullptr;
constexpr uint16_t MUX[] = {ADS1X15_REG_CONFIG_MUX_SINGLE_0,
  ADS1X15_REG_CONFIG_MUX_SINGLE_1, ADS1X15_REG_CONFIG_MUX_SINGLE_2,
  ADS1X15_REG_CONFIG_MUX_SINGLE_3};
// Command index per ADS1115 channel: A0 = Up, A1 = Shoot, A2 = Down, A3 = Left.
// Right is not on the ADC; it is read from GPIO RIGHT_BTN (D8).
constexpr uint8_t DIRECTION[] = {0, 4, 2, 3};
// Set by the scanner from the debounced Shoot bit (ADC A1).
volatile bool shootDown = false;
volatile uint8_t scanPeriodMs = 1;
void publish(uint8_t state, uint8_t released) {
  portENTER_CRITICAL(&lock);
  blocked &= ~released;
  uint8_t pressed = state & ~held & ~blocked;
  held = state;
  {
    for (uint8_t command = 0; command < 5; ++command) {
      if ((pressed & (1 << command)) && count < CAPACITY) {
        events[(head + count) % CAPACITY] = command;
        queuedAt[(head + count) % CAPACITY] = millis();
        ++pressCount;
        ++recordedPresses[command];
        ++count;
      }
    }
  }
  portEXIT_CRITICAL(&lock);
}
// The library's register helper discards transfer errors. Check every bus
// operation here so an incomplete read can never become a button sample.
bool readRegister(uint8_t reg, uint16_t &value) {
  Wire.beginTransmission(0x48);
  Wire.write(reg);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom(uint8_t(0x48), uint8_t(2)) != 2) return false;
  value = (uint16_t(Wire.read()) << 8) | uint16_t(Wire.read());
  return true;
}
uint16_t configFor(uint8_t channel) {
  return 0x8000 | MUX[channel] | ads.getGain() | 0x0100 |
         ads.getDataRate() | 0x0003;
}
bool startConversion(uint8_t channel) {
  uint16_t config = configFor(channel);
  Wire.beginTransmission(0x48);
  Wire.write(uint8_t(1));
  Wire.write(uint8_t(config >> 8));
  Wire.write(uint8_t(config));
  return Wire.endTransmission() == 0;
}
void scan(void *) {
  uint8_t channel = 0, high[4] = {}, low[4] = {}, directions = 0;
  bool settling = true, rightRaw = false, rightStable = false;
  unsigned long rightChanged = millis(), started = 0;  // started is in micros
  unsigned long rightPressed = millis() - 120;
  bool converting = false;
  for (;;) {
    unsigned long now = millis();
    bool raw = digitalRead(RIGHT_BTN) == LOW;
    if (raw != rightRaw) { rightRaw = raw; rightChanged = now; }
    // Qualify both edges and space presses so a noisy contact cannot double-tap.
    if (raw && !rightStable && now - rightChanged >= 12 && now - rightPressed >= 120) {
      rightStable = true;
      rightPressed = now;
    } else if (!raw && rightStable && now - rightChanged >= 40) {
      rightStable = false;
    }
    bool fault = false;
    if (adsConnected) {
      if (!converting) {
        converting = startConversion(channel);
        started = micros();
        fault = !converting;
      } else if (micros() - started >= CONVERSION_US) {
        uint16_t config = 0, result = 0;
        if (!readRegister(1, config) ||
            (config & 0x7fff) != (configFor(channel) & 0x7fff)) fault = true;
        else if (config & 0x8000) {
          if (!readRegister(0, result)) fault = true;
          else {
            int16_t value = int16_t(result);
            // Allow the external button network to settle after changing MUX.
            // A completed ADC conversion alone does not qualify a stable input.
            if (settling) settling = false;
            else {
              portENTER_CRITICAL(&lock);
              lastRaw[channel] = value;
              if (value > peakRaw[channel]) peakRaw[channel] = value;
              portEXIT_CRITICAL(&lock);
              uint8_t bit = 1 << DIRECTION[channel];
              // high[] counts pressed samples, low[] released samples. One
              // settled sample below PRESS_LEVEL is a press (the hysteresis
              // gap and the MUX settling conversion already filter noise);
              // release still needs two samples so a held key never chatters.
              if (value < PRESS_LEVEL) {
                low[channel] = 0;
                high[channel] = 1;
                directions |= bit;
              } else if (value > RELEASE_LEVEL) {
                high[channel] = 0;
                if (low[channel] < 2) ++low[channel];
                if (low[channel] == 2) directions &= ~bit;
              } else high[channel] = low[channel] = 0;
              channel = (channel + 1) % 4;
              settling = true;
            }
            // Start the next conversion now instead of losing a scheduler tick.
            converting = startConversion(channel);
            started = micros();
            if (!converting) fault = true;
          }
        } else if (micros() - started > 15000) fault = true;
      }
    }
    if (fault) {
      portENTER_CRITICAL(&lock);
      ++busFaults;
      portEXIT_CRITICAL(&lock);
      // A failed transfer must not leave a stuck direction or fabricate a new
      // press when the connection recovers while a key is still held.
      converting = false;
      settling = true;
      channel = directions = 0;
      for (int i = 0; i < 4; ++i) high[i] = low[i] = 0;
      // Gate only the ADC-driven bits (Up, Down, Left, Shoot). Right is on GPIO
      // and is not affected by bus faults. Erased presses are not restored; each
      // gated key rearms after its own confirmed release.
      portENTER_CRITICAL(&lock);
      blocked |= 0x1d;
      portEXIT_CRITICAL(&lock);
    }
    uint8_t released = (!rightRaw && !rightStable) ? 2 : 0;
    if (adsConnected)
      for (int i = 0; i < 4; ++i)
        if (low[i] == 2) released |= 1 << DIRECTION[i];
    shootDown = (directions & 16) != 0;
    publish(directions | (rightStable ? 2 : 0), released);
    // Never try to catch up with a burst of old scan deadlines after a stall.
    vTaskDelay(pdMS_TO_TICKS(scanPeriodMs));
  }
}
// USB logging runs separately at lower priority. The ADC scanner never waits
// for serial output, and recording does not consume the game's input events.
void record(void *) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(100));
    int16_t raw[4], peak[4];
    uint32_t presses[5], reads[5], faults, expired, start, duration;
    uint8_t state, gate, queued;
    portENTER_CRITICAL(&lock);
    start = recordingStarted; duration = recordingDuration;
    for (int i=0; i<4; ++i) {
      raw[i] = lastRaw[i]; peak[i] = peakRaw[i]; peakRaw[i] = 0;
    }
    for (int i=0; i<5; ++i) {
      presses[i] = recordedPresses[i]; reads[i] = recordedReads[i];
    }
    state=held; gate=blocked; queued=count; faults=busFaults; expired=expiredCount;
    portEXIT_CRITICAL(&lock);
    if (!duration || millis()-start >= duration) continue;
    char line[256];
    int n=snprintf(line,sizeof(line),
      "[Record] t=%lu raw=%d,%d,%d,%d peak=%d,%d,%d,%d held=%02x gate=%02x q=%u press=%lu,%lu,%lu,%lu,%lu read=%lu,%lu,%lu,%lu,%lu faults=%lu expired=%lu\n",
      (unsigned long)millis(), raw[0],raw[1],raw[2],raw[3],
      peak[0],peak[1],peak[2],peak[3],state,gate,queued,
      (unsigned long)presses[0],(unsigned long)presses[1],(unsigned long)presses[2],
      (unsigned long)presses[3],(unsigned long)presses[4],
      (unsigned long)reads[0],(unsigned long)reads[1],(unsigned long)reads[2],
      (unsigned long)reads[3],(unsigned long)reads[4],
      (unsigned long)faults,(unsigned long)expired);
    if(n>0 && n<int(sizeof(line)) && Serial.availableForWrite()>=n)
      Serial.write((const uint8_t *)line,n);
  }
}
}
bool readShoot() {
  return shootDown;
}

bool buttonsBegin() {
  if (scanner) return true;
  if (xTaskCreate(scan, "buttons", 3072, nullptr, 2, &scanner) != pdPASS) return false;
  if (xTaskCreate(record, "input-record", 3072, nullptr, 1, nullptr) != pdPASS)
    Serial.println("[Input] Recorder unavailable");
  return true;
}
uint8_t buttonsHeld() {
  portENTER_CRITICAL(&lock);
  uint8_t state = (held & ~blocked) | (millis() < debugUntil ? debugHeld : 0);
  portEXIT_CRITICAL(&lock);
  return state;
}
// The scanner's I2C traffic costs real CPU time; CPU-bound apps (video) slow it down.
void buttonsSetScanPeriod(uint8_t ms) { scanPeriodMs = ms ? ms : 1; }
void buttonsDebugKey(char key) {
  int command = -1;
  switch (key) {
    case 'w': case 'W': command = 0; break;
    case 'd': case 'D': command = 1; break;
    case 's': case 'S': command = 2; break;
    case 'a': case 'A': command = 3; break;
    case 'e': case 'E': case ' ': command = 4; break;
  }
  if (command < 0) return;
  portENTER_CRITICAL(&lock);
  if (count < CAPACITY) {
    events[(head + count) % CAPACITY] = command;
    queuedAt[(head + count) % CAPACITY] = millis();
    ++count;
  }
  debugHeld = 1 << command;
  debugUntil = millis() + 80;
  portEXIT_CRITICAL(&lock);
}
int buttonsNextPress() {
  portENTER_CRITICAL(&lock);
  int command = -1;
  while (count && millis() - queuedAt[head] > EVENT_LIFETIME_MS) {
    ++expiredCount;
    head = (head + 1) % CAPACITY;
    --count;
  }
  if (count) {
    command = events[head];
    ++recordedReads[command];
    head = (head + 1) % CAPACITY;
    --count;
  }
  portEXIT_CRITICAL(&lock);
  return command;
}
bool buttonsPendingPress() {
  portENTER_CRITICAL(&lock);
  bool pending = count != 0;
  portEXIT_CRITICAL(&lock);
  return pending;
}
void buttonsRequireRelease() {
  portENTER_CRITICAL(&lock);
  head = count = 0;
  blocked = 0x1f;
  debugHeld = 0;
  debugUntil = 0;
  portEXIT_CRITICAL(&lock);
}

void buttonsAllowHeldDirections() {
  // For continuous-steering games: a direction already held when a screen
  // changed (e.g. through a countdown) works immediately instead of waiting
  // for a release. Shoot stays gated so it cannot re-trigger a menu.
  portENTER_CRITICAL(&lock);
  blocked &= ~0x0f;
  portEXIT_CRITICAL(&lock);
}

void buttonsPrintDiagnostics() {
  int16_t raw[4];
  uint8_t state;
  uint8_t gated;
  uint32_t faults, presses, expired;
  portENTER_CRITICAL(&lock);
  for (int i = 0; i < 4; ++i) raw[i] = lastRaw[i];
  state = held; gated = blocked;
  faults = busFaults; presses = pressCount; expired = expiredCount;
  portEXIT_CRITICAL(&lock);
  Serial.printf("[Input] raw=%d,%d,%d,%d held=%02x gate=%d faults=%lu presses=%lu expired=%lu\n",
    raw[0], raw[1], raw[2], raw[3], state, gated,
    (unsigned long)faults, (unsigned long)presses, (unsigned long)expired);
}

void buttonsRecordFor(uint32_t durationMs) {
  portENTER_CRITICAL(&lock);
  recordingStarted = millis();
  recordingDuration = durationMs;
  portEXIT_CRITICAL(&lock);
}
