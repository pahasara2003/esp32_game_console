// Standalone video player: plays /videos/better_call_saul_the_win/ from SD.
// video.mjpeg = concatenated 320x240 JPEG frames, audio.wav = mono 16-bit PCM.
// Audio is the master clock; video frames are skipped if decoding falls behind.
// Shoot button: pause / resume.
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <JPEGDEC.h>
#include <driver/i2s.h>

#define VIDEO_DIR   "/videos/better_call_saul_the_win"
#define VIDEO_FPS   12          // must match --fps used in yt_to_console.py
#define ROTATION    1           // landscape; use 3 if the picture is upside down
#define VOLUME_PCT  70          // 0..100
#ifndef DITHER
#define DITHER      0           // 1 = smooth gradients on the 16-bit panel (slower)
#endif

#define SD_CS   9
#define SHOOT   8
#define I2S_BCLK 6
#define I2S_LRC  7
#define I2S_DOUT 5

TFT_eSPI tft;
JPEGDEC jpeg;
File vfile, afile;

static const size_t FRAME_CAP = 60 * 1024;
static uint8_t *frameBuf;
static uint8_t inBuf[4096];
static size_t inPos = 0, inLen = 0;

static uint32_t sampleRate = 22050;
static uint64_t samplesWritten = 0;
static int16_t monoBuf[512];
static int16_t stereoBuf[1024];
static size_t pendOff = 0, pendLen = 0;   // bytes of stereoBuf not yet sent
static bool audioDone = false;
static uint64_t decodeUs = 0;
static uint32_t underruns = 0;

static void fail(const char *msg) {
  Serial.println(msg);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString(msg, 10, 110, 2);
  while (true) delay(1000);
}

// ---------- MJPEG stream ----------
static int nextByte() {
  if (inPos >= inLen) {
    inLen = vfile.read(inBuf, sizeof(inBuf));
    inPos = 0;
    if (inLen == 0) return -1;
  }
  return inBuf[inPos++];
}

// Reads one JPEG (SOI..EOI). If buf is null the frame is skipped. Returns length, 0 at EOF.
static size_t readFrame(uint8_t *buf) {
  int c, prev = -1;
  // find SOI FF D8
  while ((c = nextByte()) >= 0) {
    if (prev == 0xFF && c == 0xD8) break;
    prev = c;
  }
  if (c < 0) return 0;
  size_t n = 0;
  if (buf) { buf[n++] = 0xFF; buf[n++] = 0xD8; } else n = 2;
  prev = 0xD8;
  while ((c = nextByte()) >= 0) {
    if (buf && n < FRAME_CAP) buf[n] = c;
    n++;
    if (prev == 0xFF && c == 0xD9) break;
    prev = c;
  }
  if (n > FRAME_CAP) return buf ? 1 : n;  // oversize: flag as bad
  return n;
}

// The panel is RGB565, so smooth gradients band into flat steps. Decode to
// 24-bit and apply 4x4 ordered (Bayer) dither while converting to 565.
static const uint8_t bayer[4][4] = {
  {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
static uint16_t lineBuf[320 * 16];
// Per-threshold lookup tables, already byte-swapped for the panel.
static uint16_t lutR[16][256], lutG[16][256], lutB[16][256];

static void buildDitherLuts() {
  for (int t = 0; t < 16; t++)
    for (int v = 0; v < 256; v++) {
      int r = min(255, v + (t >> 1));          // 5-bit step 8 -> +0..7
      int g = min(255, v + (t >> 2));          // 6-bit step 4 -> +0..3
      uint16_t cr = (r & 0xF8) << 8, cg = (g & 0xFC) << 3, cb = r >> 3;
      lutR[t][v] = (cr >> 8) | (cr << 8);
      lutG[t][v] = (cg >> 8) | (cg << 8);
      lutB[t][v] = (cb >> 8) | (cb << 8);
    }
}

static int drawBlock(JPEGDRAW *d) {
  if (!DITHER) {
    tft.pushImage(d->x, d->y, d->iWidth, d->iHeight, d->pPixels);
    return 1;
  }
  const uint32_t *src = (const uint32_t *)d->pPixels;
  int w = d->iWidth, h = d->iHeight;
  if (w * h > (int)(sizeof(lineBuf) / 2)) h = sizeof(lineBuf) / 2 / w;
  uint16_t *out = lineBuf;
  for (int y = 0; y < h; y++) {
    const uint8_t *brow = bayer[(d->y + y) & 3];
    int x0 = d->x;
    for (int x = 0; x < w; x++) {
      uint32_t p = *src++;
      int t = brow[(x0 + x) & 3];
      *out++ = lutR[t][p & 0xFF] | lutG[t][(p >> 8) & 0xFF] | lutB[t][(p >> 16) & 0xFF];
    }
  }
  tft.pushImage(d->x, d->y, w, h, lineBuf);
  return 1;
}

// ---------- audio ----------
static bool openWav() {
  afile = SD.open(VIDEO_DIR "/audio.wav");
  if (!afile) return false;
  uint8_t h[12];
  if (afile.read(h, 12) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) return false;
  while (afile.available()) {
    uint8_t ch[8];
    if (afile.read(ch, 8) != 8) return false;
    uint32_t sz = ch[4] | ch[5] << 8 | ch[6] << 16 | (uint32_t)ch[7] << 24;
    if (!memcmp(ch, "fmt ", 4)) {
      uint8_t f[16];
      afile.read(f, 16);
      uint16_t chans = f[2] | f[3] << 8;
      sampleRate = f[4] | f[5] << 8 | f[6] << 16 | (uint32_t)f[7] << 24;
      uint16_t bits = f[14] | f[15] << 8;
      if (chans != 1 || bits != 16) return false;
      afile.seek(afile.position() + sz - 16 + (sz & 1));
    } else if (!memcmp(ch, "data", 4)) {
      return true;
    } else {
      afile.seek(afile.position() + sz + (sz & 1));
    }
  }
  return false;
}

static void initI2S() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = sampleRate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 16;
  cfg.dma_buf_len = 1024;          // ~0.7 s of buffer at 22050 Hz
  cfg.tx_desc_auto_clear = true;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = I2S_BCLK;
  pins.ws_io_num = I2S_LRC;
  pins.data_out_num = I2S_DOUT;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  i2s_driver_uninstall(I2S_NUM_0);
  i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

// Push as much audio as the DMA buffers accept without blocking.
static void pumpAudio() {
  static bool primed = false;
  size_t total = 0;
  while (!audioDone) {
    if (pendLen == 0) {
      int got = afile.read((uint8_t *)monoBuf, sizeof(monoBuf));
      if (got <= 0) { audioDone = true; return; }
      int n = got / 2;
      for (int i = 0; i < n; i++) {
        int16_t s = (int32_t)monoBuf[i] * VOLUME_PCT / 100;
        stereoBuf[2 * i] = s;
        stereoBuf[2 * i + 1] = s;
      }
      pendOff = 0;
      pendLen = n * 4;
    }
    size_t w = 0;
    i2s_write(I2S_NUM_0, (uint8_t *)stereoBuf + pendOff, pendLen, &w, 0);
    if (w == 0) {                  // DMA full
      // If nearly the whole DMA ring was free, it ran dry: an audible gap.
      if (primed && total > 15 * 1024 * 4) underruns++;
      primed = true;
      return;
    }
    total += w;
    pendOff += w;
    pendLen -= w;
    samplesWritten += w / 4;
  }
}

// ---------- main ----------
void setup() {
  Serial.begin(115200);
  pinMode(SHOOT, INPUT_PULLUP);
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);            // backlight
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  tft.init();
  tft.setRotation(ROTATION);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(false);

  SPI.end();
  SPI.begin(1, 21, 2, SD_CS);
  digitalWrite(TFT_CS, HIGH);
  if (!SD.begin(SD_CS, SPI, 40000000)) fail("SD card error");

  buildDitherLuts();
  frameBuf = (uint8_t *)malloc(FRAME_CAP);
  if (!frameBuf) fail("Out of memory");

  vfile = SD.open(VIDEO_DIR "/video.mjpeg");
  if (!vfile) fail("video.mjpeg not found");
  if (!openWav()) fail("audio.wav missing or not mono 16-bit");
  Serial.printf("Audio %u Hz, video %d fps\n", sampleRate, VIDEO_FPS);

  initI2S();
  pumpAudio();
}

void loop() {
  static uint32_t frameNo = 0, shown = 0, t0 = millis();
  static bool paused = false, lastBtn = false;

  bool btn = !digitalRead(SHOOT);
  if (btn && !lastBtn) {
    paused = !paused;
    if (paused) i2s_stop(I2S_NUM_0); else i2s_start(I2S_NUM_0);
    delay(30);
  }
  lastBtn = btn;
  if (paused) { delay(10); return; }

  pumpAudio();

  // Playback clock: samples handed to I2S minus what is still queued (~buffer size).
  int64_t played = (int64_t)samplesWritten - 16 * 1024;
  if (played < 0) played = 0;
  uint32_t due = audioDone ? frameNo + 1 : (uint32_t)(played * VIDEO_FPS / sampleRate);

  if (frameNo > due) { delay(2); return; }   // video ahead of audio: wait

  // Behind by more than one frame: skip without decoding.
  while (frameNo + 1 < due) {
    if (readFrame(nullptr) == 0) goto finished;
    frameNo++;
    pumpAudio();
  }

  {
    size_t len = readFrame(frameBuf);
    if (len == 0) goto finished;
    frameNo++;
    if (len > 4 && jpeg.openRAM(frameBuf, len, drawBlock)) {
      jpeg.setPixelType(DITHER ? RGB8888 : RGB565_BIG_ENDIAN);
      uint32_t td = micros();
      jpeg.decode(0, 0, 0);
      decodeUs += micros() - td;
      jpeg.close();
      shown++;
    }
  }

  if (frameNo % (VIDEO_FPS * 5) == 0)
  {
    Serial.printf("frame %u, shown %u, fps %.1f, decode %u ms, underruns %u\n",
                  frameNo, shown, shown * 1000.0f / (millis() - t0),
                  shown ? (unsigned)(decodeUs / shown / 1000) : 0, underruns);
  }
  return;

finished:
  while (!audioDone) { pumpAudio(); delay(5); }
  delay(800);
  i2s_zero_dma_buffer(I2S_NUM_0);
  Serial.println("Done. Restarting in 2 s");
  delay(2000);
  ESP.restart();
}
