// Videos app: playlist of /videos/<name>/ folders, landscape MJPEG + WAV playback.
// Each folder holds video.mjpeg (320x240 baseline JPEG frames), audio.wav
// (mono 16-bit PCM) and optionally info.txt (line 1 title, line 2 fps),
// all written by tools/yt_to_console.py.
//
// Controls
//   Playlist: Up/Down browse, Shoot plays, Right opens the player, and Left
//             returns home, matching the Music app.
//   Playing:  Shoot pauses and shows the control bar.
//   Paused:   Left/Right move between Playlist, Previous, Play, Next;
//             Shoot presses the focused button. Focus starts on Play, so a
//             second Shoot hides the bar and resumes.
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <JPEGDEC.h>
#include <driver/i2s.h>
#include <new>
#include "buttons.h"
#include "config.h"
#include "audio_kit.h"

extern TFT_eSPI tft;
extern JPEGDEC jpeg;   // shared with the music player (never used at the same time)

namespace {

constexpr int MAX_VIDEOS = 32;
constexpr size_t FRAME_CAP = 32 * 1024;     // converted frames peak ~27 KB
constexpr int DMA_BUFS = 8, DMA_LEN = 1024;           // ~0.37 s at 22050 Hz
constexpr int DMA_SAMPLES = DMA_BUFS * DMA_LEN;
constexpr int VOLUME_PCT = 70;
constexpr int LAND_ROT = 3, PORT_ROT = 2;
// Button scan step while a video plays: the 1 ms default steals decode time.
constexpr int PLAY_SCAN_MS = 5;

// Colours match the music player.
constexpr uint16_t BG = 0x0000, WHITE = 0xFFFF, MUTED = 0x9492, ACCENT = 0x1DCA;
constexpr uint16_t SELECTED = 0x2124, SURFACE = 0x18E3, PROG_BG = 0x2124;

// Playlist layout (portrait 240x320), same grid as the music library.
constexpr int LIST_Y = 66, ROW_H = 44, LIST_ROWS = 5;
constexpr int FOOTER_Y = LIST_Y + ROW_H * LIST_ROWS;

enum Key { K_UP = 0, K_RIGHT, K_DOWN, K_LEFT, K_SHOOT };
enum Ctrl { C_LIST = 0, C_PREV, C_PLAY, C_NEXT, C_COUNT };

struct Video {
  String dir, title;
  float fps = 12;
  uint32_t seconds = 0;
};

Video videos[MAX_VIDEOS];
int videoCount = 0;
int listSel = 0, listTop = 0;

uint8_t *frameBuf = nullptr;
File vfile, afile;
size_t inPos = 0, inLen = 0;

int current = 0;
uint32_t sampleRate = 22050, audioBytes = 0;
uint64_t samplesWritten = 0;
// Stream buffers (~7 KB) are allocated only while the app is open.
struct Buffers {
  uint8_t in[4096];
  int16_t mono[512];
  int16_t stereo[1024];
};
Buffers *bufs = nullptr;
#define inBuf (bufs->in)
#define monoBuf (bufs->mono)
#define stereoBuf (bufs->stereo)
size_t pendOff = 0, pendLen = 0;
bool audioDone = false;
uint32_t frameNo = 0;
bool paused = false;
int focus = C_PLAY;

// ---------------------------------------------------------------- helpers
uint16_t lerp565(uint16_t a, uint16_t b, uint8_t t) {
  int r = ((a >> 11) * (255 - t) + (b >> 11) * t) / 255;
  int g = (((a >> 5) & 63) * (255 - t) + ((b >> 5) & 63) * t) / 255;
  int bl = ((a & 31) * (255 - t) + (b & 31) * t) / 255;
  return (r << 11) | (g << 5) | bl;
}

String fitText(const String &s, int font, int w) {
  if (tft.textWidth(s, font) <= w) return s;
  String t = s;
  while (t.length() && tft.textWidth(t + "...", font) > w) t.remove(t.length() - 1);
  return t + "...";
}

String prettyName(String n) {
  n.replace('_', ' ');
  if (n.length()) n.setCharAt(0, toupper(n[0]));
  return n;
}

void fmtTime(char *buf, uint32_t s) { sprintf(buf, "%u:%02u", s / 60, s % 60); }

int readKey() {
  int k = buttonsNextPress();
  if (k < 0 && Serial.available()) {
    switch (tolower(Serial.read())) {
      case 'w': k = K_UP;    break; case 'd': k = K_RIGHT; break;
      case 's': k = K_DOWN;  break; case 'a': k = K_LEFT;  break;
      case 'e': case ' ': k = K_SHOOT; break;
    }
  }
  // Playback and its controls are rotated 90 degrees relative to the buttons.
  if (k == K_UP) return K_LEFT;
  if (k == K_LEFT) return K_DOWN;
  if (k == K_DOWN) return K_RIGHT;
  if (k == K_RIGHT) return K_UP;
  return k;
}

// Bytes of PCM data after the WAV header; leaves the file at the data start.
bool openWav(File &f, uint32_t &rate, uint32_t &bytes) {
  uint8_t h[12];
  if (f.read(h, 12) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) return false;
  bool fmtOk = false;
  while (f.available()) {
    uint8_t ch[8];
    if (f.read(ch, 8) != 8) return false;
    uint32_t sz = ch[4] | ch[5] << 8 | ch[6] << 16 | (uint32_t)ch[7] << 24;
    if (!memcmp(ch, "fmt ", 4)) {
      uint8_t fm[16];
      f.read(fm, 16);
      uint16_t chans = fm[2] | fm[3] << 8, bits = fm[14] | fm[15] << 8;
      rate = fm[4] | fm[5] << 8 | fm[6] << 16 | (uint32_t)fm[7] << 24;
      fmtOk = chans == 1 && bits == 16;
      f.seek(f.position() + sz - 16 + (sz & 1));
    } else if (!memcmp(ch, "data", 4)) {
      bytes = sz;
      return fmtOk;
    } else {
      f.seek(f.position() + sz + (sz & 1));
    }
  }
  return false;
}

// ---------------------------------------------------------------- library
bool mountSd() {
  if (SD.cardType() != CARD_NONE) return true;
  SPI.end();
  SPI.begin(1, 21, 2, SD_CS_PIN);
  digitalWrite(TFT_CS, HIGH);
  return SD.begin(SD_CS_PIN, SPI, 40000000);
}

void loaderTick(bool force = false);

void scanVideos() {
  videoCount = 0;
  File root = SD.open("/videos");
  if (!root || !root.isDirectory()) return;
  for (File d = root.openNextFile(); d && videoCount < MAX_VIDEOS; d = root.openNextFile()) {
    if (!d.isDirectory()) { d.close(); continue; }
    String dir = String("/videos/") + d.name();
    d.close();
    if (!SD.exists(dir + "/video.mjpeg") || !SD.exists(dir + "/audio.wav")) continue;
    Video &v = videos[videoCount];
    v = Video();
    v.dir = dir;
    v.title = prettyName(dir.substring(dir.lastIndexOf('/') + 1));
    File info = SD.open(dir + "/info.txt");
    if (info) {
      String t = info.readStringUntil('\n'); t.trim();
      String f = info.readStringUntil('\n'); f.trim();
      if (t.length()) v.title = t;
      if (f.toFloat() > 0) v.fps = f.toFloat();
      info.close();
    }
    File a = SD.open(dir + "/audio.wav");
    uint32_t rate = 0, bytes = 0;
    if (a && openWav(a, rate, bytes) && rate) v.seconds = bytes / 2 / rate;
    a.close();
    videoCount++;
    loaderTick();
  }
  root.close();
  // Alphabetical by title.
  for (int i = 1; i < videoCount; i++)
    for (int j = i; j > 0 && videos[j].title.compareTo(videos[j - 1].title) < 0; j--) {
      Video t = videos[j]; videos[j] = videos[j - 1]; videos[j - 1] = t;
    }
}

// ---------------------------------------------------------------- playlist
uint16_t avatarColor(const String &s) {
  static const uint16_t pal[] = {0xF9F0, 0xFCA0, 0x1DCA, 0x2BDF, 0x903F, 0x0657};
  uint32_t h = 0;
  for (unsigned i = 0; i < s.length(); ++i) h = h * 31 + (uint8_t)s[i];
  return pal[h % 6];
}

void drawHeader() {
  for (int y = 0; y < LIST_Y; ++y)
    tft.drawFastHLine(0, y, 240, lerp565(0x4810, BG, y * 255 / (LIST_Y - 1)));
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WHITE);
  tft.drawString("Videos", 16, 12, 4);
  char sub[24];
  snprintf(sub, sizeof(sub), videoCount == 1 ? "%d video" : "%d videos", videoCount);
  tft.setTextColor(0xC618);
  tft.drawString(sub, 17, 40, 2);
  tft.fillCircle(210, 30, 17, ACCENT);
  tft.fillTriangle(204, 21, 204, 39, 220, 30, BG);
}

void drawRow(int slot) {
  int i = listTop + slot, y = LIST_Y + slot * ROW_H;
  tft.fillRect(0, y, 236, ROW_H, BG);
  if (i >= videoCount) return;
  bool sel = i == listSel;
  if (sel) tft.fillRoundRect(6, y + 2, 222, ROW_H - 4, 10, SELECTED);
  // Thumbnail tile with a play glyph.
  uint16_t c = avatarColor(videos[i].title);
  tft.fillRoundRect(14, y + 6, 40, 32, 6, c);
  tft.fillTriangle(29, y + 14, 29, y + 30, 42, y + 22, WHITE);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(WHITE);
  tft.drawString(fitText(videos[i].title, 2, 150), 64, y + 14, 2);
  char t[12];
  fmtTime(t, videos[i].seconds);
  tft.setTextColor(MUTED);
  tft.drawString(t, 64, y + 31, 2);
  if (sel) {
    tft.drawLine(212, y + 16, 217, y + 22, MUTED); tft.drawLine(217, y + 22, 212, y + 28, MUTED);
    tft.drawLine(213, y + 16, 218, y + 22, MUTED); tft.drawLine(218, y + 22, 213, y + 28, MUTED);
  }
}

void drawScrollbar() {
  tft.fillRect(236, LIST_Y, 4, ROW_H * LIST_ROWS, BG);
  if (videoCount <= LIST_ROWS) return;
  int trackH = ROW_H * LIST_ROWS - 8;
  int thumbH = max(16, trackH * LIST_ROWS / videoCount);
  int thumbY = (trackH - thumbH) * listTop / (videoCount - LIST_ROWS);
  tft.fillRoundRect(235, LIST_Y + 4 + thumbY, 3, thumbH, 1, 0x4A69);
}

void drawFooter() {
  tft.fillRect(0, FOOTER_Y, 240, 320 - FOOTER_Y, BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(MUTED);
  tft.drawString("< Home   Shoot play   Open >", 120, FOOTER_Y + 17, 2);
}

void drawPlaylist() {
  tft.setRotation(PORT_ROT);
  tft.fillScreen(BG);
  drawHeader();
  if (!videoCount) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(MUTED);
    tft.drawString("No videos in /videos", 120, 150, 2);
    tft.drawString("Use tools/yt_to_console.py", 120, 172, 2);
  }
  for (int k = 0; k < LIST_ROWS; ++k) drawRow(k);
  drawScrollbar();
  drawFooter();
}

void moveSelection(int delta) {
  if (!videoCount) return;
  int old = listSel, oldTop = listTop;
  listSel = (listSel + delta + videoCount) % videoCount;
  if (listSel < listTop) listTop = listSel;
  if (listSel >= listTop + LIST_ROWS) listTop = listSel - LIST_ROWS + 1;
  if (listTop != oldTop) {
    for (int k = 0; k < LIST_ROWS; ++k) drawRow(k);
    drawScrollbar();
  } else {
    drawRow(old - listTop);
    drawRow(listSel - listTop);
  }
}

// ---------------------------------------------------------------- stream
int nextByte() {
  if (inPos >= inLen) {
    inLen = vfile.read(inBuf, sizeof(inBuf));
    inPos = 0;
    if (inLen == 0) return -1;
  }
  return inBuf[inPos++];
}

// One JPEG (SOI..EOI) into buf, or skipped when buf is null. 0 at end of file.
size_t readFrame(uint8_t *buf) {
  int c, prev = -1;
  while ((c = nextByte()) >= 0) {
    if (prev == 0xFF && c == 0xD8) break;
    prev = c;
  }
  if (c < 0) return 0;
  size_t n = 2;
  if (buf) { buf[0] = 0xFF; buf[1] = 0xD8; }
  prev = 0xD8;
  while ((c = nextByte()) >= 0) {
    if (buf && n < FRAME_CAP) buf[n] = c;
    n++;
    if (prev == 0xFF && c == 0xD9) break;
    prev = c;
  }
  return (buf && n > FRAME_CAP) ? 1 : n;   // 1 = oversized, not drawn
}

// Per-stage timing, printed every few seconds: "[Video] 11.2 fps ...".
uint32_t stShown = 0, stSkipped = 0, stReadUs = 0, stDecodeUs = 0, stPushUs = 0, stSince = 0;

int drawBlock(JPEGDRAW *d) {
  uint32_t t = micros();
  tft.pushImage(d->x, d->y, d->iWidth, d->iHeight, d->pPixels);
  stPushUs += micros() - t;
  return 1;
}

void printStats() {
  uint32_t ms = millis() - stSince;
  if (ms < 3000 || !stShown) return;
  Serial.printf("[Video] %.1f fps, %u skipped, per frame: read %u ms, decode %u ms, push %u ms\n",
                stShown * 1000.0f / ms, (unsigned)stSkipped, (unsigned)(stReadUs / stShown / 1000),
                (unsigned)((stDecodeUs - stPushUs) / stShown / 1000), (unsigned)(stPushUs / stShown / 1000));
  stShown = stSkipped = stReadUs = stDecodeUs = stPushUs = 0;
  stSince = millis();
}

void installI2S(uint32_t rate) {
  i2s_driver_uninstall(I2S_NUM_0);
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = rate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = DMA_BUFS;
  cfg.dma_buf_len = DMA_LEN;
  cfg.tx_desc_auto_clear = true;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = I2S_BCLK_PIN;
  pins.ws_io_num = I2S_LRCLK_PIN;
  pins.data_out_num = I2S_DOUT_PIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

void pumpAudio() {
  while (!audioDone) {
    if (pendLen == 0) {
      int got = afile.read((uint8_t *)monoBuf, sizeof(monoBuf));
      if (got <= 0) { audioDone = true; return; }
      int n = got / 2;
      for (int i = 0; i < n; i++) {
        int16_t s = (int32_t)monoBuf[i] * VOLUME_PCT / 100 * audio::musicVolume / 100;
        stereoBuf[2 * i] = stereoBuf[2 * i + 1] = s;
      }
      pendOff = 0;
      pendLen = n * 4;
    }
    size_t w = 0;
    i2s_write(I2S_NUM_0, (uint8_t *)stereoBuf + pendOff, pendLen, &w, 0);
    if (w == 0) return;
    pendOff += w;
    pendLen -= w;
    samplesWritten += w / 4;
  }
}

uint32_t playedSamples() {
  int64_t p = (int64_t)samplesWritten - DMA_SAMPLES;
  return p < 0 ? 0 : (uint32_t)p;
}

void closeVideo() {
  buttonsSetScanPeriod(1);
  if (vfile) vfile.close();
  if (afile) afile.close();
  i2s_zero_dma_buffer(I2S_NUM_0);
}

bool openVideo(int index) {
  closeVideo();
  current = index;
  const Video &v = videos[index];
  vfile = SD.open(v.dir + "/video.mjpeg");
  afile = SD.open(v.dir + "/audio.wav");
  if (!vfile || !afile || !openWav(afile, sampleRate, audioBytes)) {
    closeVideo();
    return false;
  }
  inPos = inLen = 0;
  samplesWritten = 0;
  pendOff = pendLen = 0;
  audioDone = false;
  frameNo = 0;
  stShown = stSkipped = stReadUs = stDecodeUs = stPushUs = 0;
  stSince = millis();
  paused = false;
  focus = C_PLAY;
  buttonsSetScanPeriod(PLAY_SCAN_MS);
  tft.setRotation(LAND_ROT);
  tft.fillScreen(BG);
  installI2S(sampleRate);
  pumpAudio();
  Serial.printf("[Video] %s (%u Hz, %.0f fps)\n", v.title.c_str(), sampleRate, v.fps);
  return true;
}

// ---------------------------------------------------------------- controls
// Landscape overlay drawn over the paused frame: title strip on top,
// progress + Playlist / Previous / Play / Next along the bottom.
constexpr int BAR_Y = 176, BAR_H = 64, BTN_Y = 218;
constexpr int BTN_X[C_COUNT] = {52, 120, 200, 268};
float controlScale[C_COUNT] = {1, 1, 1.15f, 1};

float controlTarget(int c) {
  return focus == c ? (c == C_PLAY ? 1.15f : 1.4f) : 1.0f;
}

void drawControlButton(int c, float scale) {
  bool f = focus == c;
  int x = BTN_X[c], y = BTN_Y;
  uint16_t ink = f ? ACCENT : WHITE;
  auto S = [scale](float value) { return (int)lroundf(value * scale * 0.82f); };
  tft.fillRect(x - 27, y - 23, 55, 47, BG);
  switch (c) {
    case C_LIST:
      for (int i = -1; i <= 1; ++i)
        tft.fillRoundRect(x - S(10), y + S(i * 6) - S(1.5f), S(20), max(3, S(3)), 1, ink);
      break;
    case C_PREV:
      tft.fillRect(x - S(9), y - S(8), max(3, S(3)), S(16), ink);
      tft.fillTriangle(x + S(9), y - S(8), x + S(9), y + S(8), x - S(5), y, ink);
      break;
    case C_NEXT:
      tft.fillTriangle(x - S(9), y - S(8), x - S(9), y + S(8), x + S(5), y, ink);
      tft.fillRect(x + S(6), y - S(8), max(3, S(3)), S(16), ink);
      break;
    case C_PLAY:
      tft.fillCircle(x, y, S(20), ink);
      tft.fillTriangle(x - S(5), y - S(9), x - S(5), y + S(9), x + S(10), y, BG);
      break;
  }
}

void animateControls(int oldFocus) {
  float fromOld = controlScale[oldFocus];
  float fromNew = controlScale[focus];
  constexpr int FRAMES = 6;
  for (int frame = 1; frame <= FRAMES; ++frame) {
    float t = frame / (float)FRAMES;
    t = 1 - (1 - t) * (1 - t) * (1 - t);
    controlScale[oldFocus] = fromOld + (controlTarget(oldFocus) - fromOld) * t;
    controlScale[focus] = fromNew + (controlTarget(focus) - fromNew) * t;
    drawControlButton(oldFocus, controlScale[oldFocus]);
    drawControlButton(focus, controlScale[focus]);
    delay(12);
  }
}

void drawControls() {
  const Video &v = videos[current];
  // Title strip.
  tft.fillRect(0, 0, 320, 26, BG);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(WHITE);
  tft.drawString(fitText(v.title, 2, 250), 10, 13, 2);
  char idx[12];
  snprintf(idx, sizeof(idx), "%d/%d", current + 1, videoCount);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(MUTED);
  tft.drawString(idx, 310, 13, 2);
  // Bottom bar: progress and buttons.
  tft.fillRect(0, BAR_Y, 320, BAR_H, BG);
  uint32_t total = sampleRate ? audioBytes / 2 / sampleRate : 0;
  uint32_t now = sampleRate ? playedSamples() / sampleRate : 0;
  if (now > total) now = total;
  char a[12], b[12];
  fmtTime(a, now);
  fmtTime(b, total);
  tft.setTextColor(MUTED);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(a, 10, BAR_Y + 10, 1);
  tft.setTextDatum(MR_DATUM);
  tft.drawString(b, 310, BAR_Y + 10, 1);
  int px = 40, pw = 240, fill = total ? pw * now / total : 0;
  tft.fillRoundRect(px, BAR_Y + 8, pw, 4, 2, PROG_BG);
  if (fill > 0) tft.fillRoundRect(px, BAR_Y + 8, fill, 4, 2, ACCENT);
  for (int c = 0; c < C_COUNT; c++) {
    controlScale[c] = controlTarget(c);
    drawControlButton(c, controlScale[c]);
  }
  // Playback is upside-down, so turn the overlay text and progress 180 degrees too.
  tft.setRotation(PORT_ROT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(WHITE);
  tft.drawString(fitText(v.title, 2, 190), 120, 14, 2);
  char rotatedIdx[12];
  snprintf(rotatedIdx, sizeof(rotatedIdx), "%d/%d", current + 1, videoCount);
  tft.setTextColor(MUTED);
  tft.drawString(rotatedIdx, 120, 30, 2);
  tft.setRotation(LAND_ROT);
}

void setPaused(bool p) {
  paused = p;
  buttonsSetScanPeriod(p ? 1 : PLAY_SCAN_MS);
  if (p) {
    i2s_stop(I2S_NUM_0);
    focus = C_PLAY;
    drawControls();
  } else {
    i2s_start(I2S_NUM_0);
    // The next decoded frame repaints the whole screen over the overlay.
  }
}

// Returns false when the player should go back to the playlist.
bool playNext(int step) {
  int n = current;
  for (int tries = 0; tries < videoCount; tries++) {
    n = (n + step + videoCount) % videoCount;
    if (openVideo(n)) return true;
  }
  return false;
}

// Plays videos until the user asks for the playlist (current = last played).
void playLoop() {
  while (true) {
    int k = readKey();
    if (paused) {
      if (k == K_LEFT || k == K_RIGHT) {
        int old = focus;
        focus = (focus + (k == K_RIGHT ? 1 : C_COUNT - 1)) % C_COUNT;
        animateControls(old);
      } else if (k == K_SHOOT) {
        if (focus == C_PLAY) setPaused(false);
        else if (focus == C_LIST) { closeVideo(); return; }
        else if (!playNext(focus == C_NEXT ? 1 : -1)) { closeVideo(); return; }
      } else if (k == K_DOWN) {
        closeVideo();
        return;   // same shortcut as Music: Down returns to the playlist
      }
      delay(10);
      continue;
    }
    if (k == K_SHOOT) { setPaused(true); continue; }

    pumpAudio();
    float fps = videos[current].fps;
    uint32_t due = audioDone ? frameNo + 1
                             : (uint32_t)((uint64_t)playedSamples() * fps / sampleRate);
    if (frameNo > due) { delay(2); continue; }

    bool ended = false;
    while (frameNo + 1 < due) {           // behind: skip frames without decoding
      if (readFrame(nullptr) == 0) { ended = true; break; }
      frameNo++;
      stSkipped++;
      pumpAudio();
    }
    if (!ended) {
      uint32_t t0 = micros();
      size_t len = readFrame(frameBuf);
      uint32_t t1 = micros();
      if (len == 0) ended = true;
      else {
        frameNo++;
        if (len > 4 && jpeg.openRAM(frameBuf, len, drawBlock)) {
          jpeg.setPixelType(RGB565_BIG_ENDIAN);
          tft.startWrite();   // one SPI transaction per frame; the SD card is idle here
          jpeg.decode(0, 0, 0);
          tft.endWrite();
          jpeg.close();
        }
        stShown++;
        stReadUs += t1 - t0;
        stDecodeUs += micros() - t1;
        printStats();
      }
    }
    if (ended) {
      while (!audioDone) { pumpAudio(); delay(5); }
      delay(DMA_SAMPLES * 1000 / sampleRate);   // let the tail play out
      if (current + 1 >= videoCount || !playNext(1)) { closeVideo(); return; }
    }
  }
}

// Music-style loader. It is advanced only while useful startup work happens;
// there is deliberately no minimum display time or artificial startup delay.
constexpr int LD_X = 120, LD_Y = 140, LD_R = 26, LD_W = 5;
uint32_t ldStart = 0, ldLast = 0;
int ldMsg = -1;
const char *const LD_TEXT[] = {
  "Waking up the SD card", "Looking for your videos", "Reading video details",
  "Preparing the player", "Almost there"};
constexpr int LD_TEXT_N = sizeof(LD_TEXT) / sizeof(LD_TEXT[0]);

void loaderArc(int a, int b, uint16_t col) {
  a = ((a % 360) + 360) % 360;
  int len = ((b - a) % 360 + 360) % 360;
  if (len < 2) return;
  if (a + len <= 360) tft.drawArc(LD_X, LD_Y, LD_R, LD_R - LD_W, a, a + len, col, BG, true);
  else {
    tft.drawArc(LD_X, LD_Y, LD_R, LD_R - LD_W, a, 360, col, BG, true);
    tft.drawArc(LD_X, LD_Y, LD_R, LD_R - LD_W, 0, a + len - 360, col, BG, true);
  }
}

void loaderTick(bool force) {
  uint32_t now = millis();
  if (!force && now - ldLast < 20) return;
  ldLast = now;
  uint32_t t = now - ldStart;
  static const uint16_t cols[] = {0x4C3F, 0xE8E4, 0xFDA0, 0x2DAA};
  constexpr uint32_t PERIOD = 1333;
  uint32_t cycle = t / PERIOD;
  float phase = (t % PERIOD) / (float)PERIOD;
  float grow = phase < .5f ? phase * 2 : 1;
  float shrink = phase < .5f ? 0 : (phase - .5f) * 2;
  grow = grow * grow * (3 - 2 * grow);
  shrink = shrink * shrink * (3 - 2 * shrink);
  int base = (int)(t * .3f) + (int)cycle * 270;
  int tail = base + (int)(shrink * 270);
  int head = base + 20 + (int)(grow * 270);
  loaderArc(head, tail + 360, BG);
  loaderArc(tail, head, cols[cycle % 4]);

  int msg = min((int)(t / 500), LD_TEXT_N - 1);
  if (msg != ldMsg) {
    ldMsg = msg;
    tft.fillRect(0, LD_Y + 44, 240, 22, BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(MUTED, BG);
    tft.drawString(LD_TEXT[msg], 120, LD_Y + 55, 2);
  }
  int dots = (t / 400) % 4;
  int x = 123 + tft.textWidth(LD_TEXT[msg], 2) / 2;
  tft.fillRect(x, LD_Y + 57, 16, 4, BG);
  for (int i = 0; i < dots; ++i) tft.fillRect(x + i * 5, LD_Y + 58, 2, 2, MUTED);
}

void loaderBegin() {
  tft.setRotation(PORT_ROT);
  tft.fillScreen(BG);
  ldStart = millis();
  ldLast = 0;
  ldMsg = -1;
  loaderTick(true);
}

void message(const char *a, const char *b) {
  tft.setRotation(PORT_ROT);
  tft.fillScreen(BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(WHITE);
  tft.drawString(a, 120, 150, 2);
  tft.setTextColor(MUTED);
  tft.drawString(b, 120, 172, 2);
}

}  // namespace

// Entered from the home screen; returns when the user picks Menu.
void runVideoPlayer() {
  tft.setSwapBytes(false);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  loaderBegin();
  if (!mountSd()) {
    message("SD card error", "Shoot to go back");
    while (readKey() < 0) delay(10);
    return;
  }
  loaderTick(true);
  scanVideos();
  Serial.printf("[Video] before alloc: heap %u, largest block %u\n",
                (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
  frameBuf = (uint8_t *)malloc(FRAME_CAP);
  bufs = new (std::nothrow) Buffers();
  loaderTick(true);
  if (!frameBuf || !bufs) {
    delete bufs; bufs = nullptr;
    free(frameBuf); frameBuf = nullptr;
    message("Not enough memory", "Shoot to go back");
    while (readKey() < 0) delay(10);
    return;
  }
  installI2S(22050);
  Serial.printf("[Video] %d videos, heap %u, largest block %u\n", videoCount,
                (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());

  if (listSel >= videoCount) listSel = max(0, videoCount - 1);
  drawPlaylist();
  buttonsRequireRelease();
  while (true) {
    int k = readKey();
    if (k == K_UP) moveSelection(-1);
    else if (k == K_DOWN) moveSelection(+1);
    else if (k == K_LEFT) break;
    else if (k == K_SHOOT || k == K_RIGHT) {
      if (!videoCount) continue;
      if (openVideo(listSel)) {
        buttonsRequireRelease();
        playLoop();
        listSel = current;
        if (listSel < listTop || listSel >= listTop + LIST_ROWS)
          listTop = max(0, min(listSel - 2, videoCount - LIST_ROWS));
      }
      drawPlaylist();
      buttonsRequireRelease();
    }
    delay(10);
  }

  closeVideo();
  i2s_driver_uninstall(I2S_NUM_0);
  free(frameBuf); frameBuf = nullptr;
  delete bufs; bufs = nullptr;
}
