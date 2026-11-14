#include <Arduino.h>
#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3a.h>
#include <AudioOutputI2S.h>
#include <PNGdec.h>
#include <JPEGDEC.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <driver/i2s.h>
#include <new>
#include "buttons.h"

// Music player app, adapted from display_test/src/main.cpp.
#include "config.h"
constexpr int SD_CS = SD_CS_PIN, LRCLK = I2S_LRCLK_PIN, BCLK = I2S_BCLK_PIN, DIN = I2S_DOUT_PIN;
constexpr int SHOOT_PIN = 8, I2C_SDA = 10, I2C_SCL = 20;
constexpr int ART_X = 40, ART_Y = 29, ART_SIZE = 160;
constexpr int PROG_X = 18, PROG_Y = 243, PROG_W = 204, PROG_H = 3;
constexpr uint16_t COLOR_BG         = 0x0000; // Pitch Black OLED
constexpr uint16_t COLOR_CARD       = 0x10A2; // Dark Slate Card
constexpr uint16_t COLOR_SURFACE    = 0x18E3; // Elevated Surface
constexpr uint16_t COLOR_BORDER     = 0x2965; // Border Outline
constexpr uint16_t COLOR_GREEN      = 0x7ECA; // yellow-green (#7ED957)
constexpr uint16_t COLOR_TEXT_WHITE = 0xFFFF; // Crisp White
constexpr uint16_t COLOR_TEXT_MUTED = 0x9492; // Secondary Gray
constexpr uint16_t COLOR_TEXT_DIM   = 0x4A49; // Dark Accent Gray
constexpr uint16_t COLOR_PROG_BG    = 0x2124; // Progress Bar Track

constexpr uint16_t BG = COLOR_BG, CARD = COLOR_CARD;
constexpr uint16_t ACCENT = COLOR_GREEN, ACCENT_LIGHT = COLOR_TEXT_WHITE;
constexpr uint16_t MUTED = COLOR_TEXT_MUTED, WHITE = COLOR_TEXT_WHITE;
constexpr int MAX_SONGS = 64;
extern TFT_eSPI tft;
static TFT_eSPI &screen = tft;  // share the launcher display
constexpr int TEXT_MARGIN = 16;
constexpr int TEXT_X = TEXT_MARGIN;
constexpr int TEXT_W = 240 - 2 * TEXT_MARGIN;   // usable text width (208 px)

// Title/artist layout. Font 2 (16 px) keeps long names readable; the title is
// white and the artist muted so the hierarchy is still clear.
constexpr int TITLE_FONT = 2;
constexpr int ARTIST_FONT = 2;
constexpr int TITLE_BAND_Y = 194, TITLE_BAND_H = 22, TITLE_CY = 204;
constexpr int ARTIST_BAND_Y = 220, ARTIST_BAND_H = 16, ARTIST_CY = 228;
TFT_eSprite *coverSprite = nullptr;   // album cover currently on screen (big-endian RGB565)
TFT_eSprite *nextCoverSprite = nullptr; // incoming album cover, built before the slide
TFT_eSprite *coverTarget = nullptr;   // non-null while decoding into a sprite
uint16_t fadeRow[240];                 // scratch row for scaling / compositing
bool contentReady = false;             // a track's cover/text is currently on screen
bool playerVisible = false;            // true on the Now Playing screen, false on the playlist
static void loadLibrary();
static void loaderTick(bool force);
static void handleKeys();
static void drawPlaylist();
JPEGDEC jpeg;
// PNG decoder is ~51 KB, so it only exists while a PNG cover is drawn.
static PNG *pngDec = nullptr;
// Decoder (~6 KB) and read buffer (8 KB) exist only while the player is open.
static AudioGeneratorMP3a *mp3Dec = nullptr;
#define mp3 (*mp3Dec)
AudioFileSourceSD *musicFile = nullptr;
AudioFileSourceBuffer *musicBuffer = nullptr;
AudioOutputI2S *audioOut = nullptr;
static uint8_t *readBuffer = nullptr;
constexpr size_t READ_BUFFER_SIZE = 8192;
File artFile;
uint32_t artStart = 0, artSize = 0, lastProgress = 0, playStarted = 0, pausedAt = 0;
int previousBar = 0;
bool paused = false;
String songs[MAX_SONGS];
int songCount = 0, songIndex = 0;
bool adsPresent = false, controlsArmed = false, adcConverting = false, adcSettling = true;
uint8_t adcChannel = 0, adcPressCount[2] = {}, adcReleaseCount[2] = {};
bool adcDown[2] = {};
uint32_t adcStarted = 0, shootChanged = 0;
bool shootRaw = false, shootStable = false;
int pendingCommand = -1; // 0 previous, 1 next, 2 play/pause
int pngWidth = 0, pngHeight = 0, pngDrawWidth = 0, pngDrawHeight = 0;
int pngLastRow = -1;
uint16_t pngLine[1024], pngScaled[ART_SIZE];

struct Track {
  String path, title, artist;
  uint32_t artOffset = 0, artLength = 0;
  bool jpegArt = false, pngArt = false;
} track;

static uint32_t syncSafe(const uint8_t *p) {
  return (uint32_t(p[0] & 127) << 21) | (uint32_t(p[1] & 127) << 14) |
         (uint32_t(p[2] & 127) << 7) | (p[3] & 127);
}
static uint32_t bigEndian(const uint8_t *p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
         (uint32_t(p[2]) << 8) | p[3];
}
// Append one code point as plain ASCII. The TFT fonts only cover 32..126, so
// UTF-8 punctuation (curly quotes, en dashes, ...) is mapped to its ASCII
// equivalent instead of being drawn as garbage bytes.
static void appendAscii(String &s, uint32_t cp) {
  if (cp < 0x20) return;
  if (cp < 0x7F) { s += (char)cp; return; }

  switch (cp) {
    case 0xA0: s += ' '; return;
    case 0x2018: case 0x2019: case 0x201A: case 0x201B:
    case 0x02BC: case 0x2032: s += '\''; return;
    case 0x201C: case 0x201D: case 0x201E: case 0x2033: s += '"'; return;
    case 0x2013: case 0x2014: case 0x2015: case 0x2212: s += '-'; return;
    case 0x2026: s += "..."; return;
    case 0x00D7: s += 'x'; return;
    case 0x00F7: s += '/'; return;
    default: break;
  }

  // Latin-1 / Latin Extended-A letters -> nearest ASCII letter.
  static const char *const upper[] = {
    "A","A","A","A","A","A","AE","C","E","E","E","E","I","I","I","I",
    "D","N","O","O","O","O","O","x","O","U","U","U","U","Y","Th","ss",
    "a","a","a","a","a","a","ae","c","e","e","e","e","i","i","i","i",
    "d","n","o","o","o","o","o","/","o","u","u","u","u","y","th","y" };
  if (cp >= 0xC0 && cp <= 0xFF) { s += upper[cp - 0xC0]; return; }

  // Anything else: skip rather than print a placeholder.
}

// Decode one ID3 text frame into ASCII-only String.
static String readText(File &f, uint32_t bytes) {
  if (!bytes) return "";
  int encoding = f.read();
  --bytes;
  String s;
  s.reserve(64);

  if (encoding == 1 || encoding == 2) {
    bool little = encoding == 1;
    if (bytes >= 2) {
      int a = f.read(), b = f.read();
      bytes -= 2;
      if (a == 0xff && b == 0xfe) little = true;
      else if (a == 0xfe && b == 0xff) little = false;
      else {
        int c = little ? a : b;
        appendAscii(s, (uint32_t)c);
      }
    }
    while (bytes >= 2 && s.length() < 64) {
      int a = f.read(), b = f.read();
      bytes -= 2;
      int c = little ? a : b;
      if (!c) break;
      appendAscii(s, (uint32_t)c);
    }
  } else if (encoding == 3) {
    // UTF-8
    uint32_t cp = 0;
    int extra = 0;
    while (bytes-- && s.length() < 64) {
      int c = f.read();
      if (c <= 0) break;
      if (extra) {
        if ((c & 0xC0) == 0x80) {
          cp = (cp << 6) | (uint32_t)(c & 0x3F);
          if (--extra == 0) appendAscii(s, cp);
        } else {
          extra = 0;  // malformed sequence, drop it
        }
        continue;
      }
      if (c < 0x80) {
        if (c >= 32) s += (char)c;
      } else if ((c & 0xE0) == 0xC0) {
        cp = c & 0x1F; extra = 1;
      } else if ((c & 0xF0) == 0xE0) {
        cp = c & 0x0F; extra = 2;
      } else if ((c & 0xF8) == 0xF0) {
        cp = c & 0x07; extra = 3;
      }
      // stray continuation byte: ignore
    }
  } else {
    // ISO-8859-1: every byte is one code point.
    while (bytes-- && s.length() < 64) {
      int c = f.read();
      if (c <= 0) break;
      appendAscii(s, (uint32_t)c);
    }
  }
  s.trim();
  return s;
}
static void parsePicture(File &f, uint32_t end) {
  if (f.position() >= end) return;
  int encoding = f.read();
  String mime;
  while (f.position() < end && mime.length() < 32) {
    int c = f.read();
    if (c <= 0) break;
    mime += char(c);
  }
  if (f.position() >= end) return;
  f.read(); // Picture type.
  if (encoding == 1 || encoding == 2) {
    while (f.position() + 1 < end) {
      int a = f.read(), b = f.read();
      if (!a && !b) break;
    }
  } else {
    while (f.position() < end && f.read() != 0) {}
  }
  uint32_t offset = f.position();
  if (offset >= end) return;
  mime.toLowerCase();
  bool jpg = mime.indexOf("jpeg") >= 0 || mime.indexOf("jpg") >= 0;
  bool png = mime.indexOf("png") >= 0;
  if ((jpg || png) && (!track.artLength || (jpg && !track.jpegArt))) {
    track.artOffset = offset;
    track.artLength = end - offset;
    track.jpegArt = jpg;
    track.pngArt = png;
  }
}
static void parseId3() {
  File f = SD.open(track.path);
  if (!f) return;
  uint8_t h[10];
  if (f.read(h, 10) != 10 || memcmp(h, "ID3", 3) ||
      (h[3] != 3 && h[3] != 4) || (h[5] & 0xc0)) {
    f.close();
    return;
  }
  uint8_t version = h[3];
  uint32_t tagEnd = min(uint32_t(f.size()), 10U + syncSafe(h + 6));
  while (f.position() + 10 <= tagEnd) {
    uint8_t frame[10];
    if (f.read(frame, 10) != 10 || !frame[0]) break;
    uint32_t size = version == 4 ? syncSafe(frame + 4) : bigEndian(frame + 4);
    uint32_t start = f.position(), end = start + size;
    if (!size || end < start || end > tagEnd) break;
    if (!frame[8] && !frame[9]) {
      if (!memcmp(frame, "TIT2", 4)) track.title = readText(f, size);
      if (!memcmp(frame, "TPE1", 4)) track.artist = readText(f, size);
      if (!memcmp(frame, "APIC", 4)) parsePicture(f, end);
    }
    f.seek(end);
  }
  f.close();
}
static void findSongs() {
  File folder = SD.open("/Music");
  if (!folder || !folder.isDirectory()) return;
  while (songCount < MAX_SONGS) {
    File entry = folder.openNextFile();
    if (!entry) break;
    String name = entry.name(), lower = name;
    lower.toLowerCase();
    bool found = !entry.isDirectory() && lower.endsWith(".mp3");
    entry.close();
    if (found) songs[songCount++] = String("/Music/") + name.substring(name.lastIndexOf('/') + 1);
    loaderTick(false);
  }
  folder.close();
}
static String findFirstMp3() {
  if (!songCount) findSongs();
  return songCount ? songs[0] : "";
}
static int32_t artRead(JPEGFILE *src, uint8_t *out, int32_t len) {
  if (len < 0) return 0;
  uint32_t remain = src->iPos < int32_t(artSize) ? artSize - src->iPos : 0;
  int32_t count = artFile.read(out, min(uint32_t(len), remain));
  src->iPos += count;
  return count;
}
static int32_t artSeek(JPEGFILE *src, int32_t pos) {
  if (pos >= 0 && uint32_t(pos) <= artSize && artFile.seek(artStart + pos))
    src->iPos = pos;
  return src->iPos;
}
static int artDraw(JPEGDRAW *block) {
  if (coverTarget) {
    if (block->x >= 0 && block->y >= 0 &&
        block->x + block->iWidth <= ART_SIZE &&
        block->y + block->iHeight <= ART_SIZE)
      coverTarget->pushImage(block->x, block->y, block->iWidth, block->iHeight, block->pPixels);
  } else if (block->x >= ART_X && block->y >= ART_Y &&
             block->x + block->iWidth <= ART_X + ART_SIZE &&
             block->y + block->iHeight <= ART_Y + ART_SIZE)
    screen.pushImage(block->x, block->y, block->iWidth, block->iHeight, block->pPixels);
  return 1;
}
static void *pngOpen(const char *, int32_t *size) {
  *size = artSize;
  artFile.seek(artStart);
  return &artFile;
}
static void pngClose(void *) {}
static int32_t pngRead(PNGFILE *src, uint8_t *out, int32_t len) {
  if (len < 0) return 0;
  uint32_t remain = src->iPos < int32_t(artSize) ? artSize - src->iPos : 0;
  int32_t count = artFile.read(out, min(uint32_t(len), remain));
  src->iPos += count;
  return count;
}
static int32_t pngSeek(PNGFILE *src, int32_t pos) {
  if (pos >= 0 && uint32_t(pos) <= artSize && artFile.seek(artStart + pos))
    src->iPos = pos;
  return src->iPos;
}
static int pngDraw(PNGDRAW *row) {
  if (row->iWidth > 1024 || !pngHeight || !pngDrawHeight) return 0;
  int targetY = row->y * pngDrawHeight / pngHeight;
  if (targetY < 0 || targetY >= pngDrawHeight || targetY == pngLastRow) return 1;
  pngLastRow = targetY;
  pngDec->getLineAsRGB565(row, pngLine, PNG_RGB565_BIG_ENDIAN, 0x18202c);
  for (int x = 0; x < pngDrawWidth; ++x)
    pngScaled[x] = pngLine[x * pngWidth / pngDrawWidth];
  int baseX = coverTarget ? 0 : ART_X;
  int baseY = coverTarget ? 0 : ART_Y;
  if (coverTarget)
    coverTarget->pushImage(baseX + (ART_SIZE - pngDrawWidth) / 2,
                           baseY + (ART_SIZE - pngDrawHeight) / 2 + targetY,
                           pngDrawWidth, 1, pngScaled);
  else
    screen.pushImage(baseX + (ART_SIZE - pngDrawWidth) / 2,
                     baseY + (ART_SIZE - pngDrawHeight) / 2 + targetY,
                     pngDrawWidth, 1, pngScaled);
  return 1;
}
static bool drawPngCover() {
  if (!track.pngArt || !track.artLength) return false;
  artFile = SD.open(track.path);
  if (!artFile) return false;
  artStart = track.artOffset;
  artSize = track.artLength;
  bool ok = false;
  pngDec = new (std::nothrow) PNG();
  if (!pngDec) { artFile.close(); return false; }
  PNG &png = *pngDec;
  if (png.open("embedded", pngOpen, pngClose, pngRead, pngSeek, pngDraw) == PNG_SUCCESS) {
    pngWidth = png.getWidth();
    pngHeight = png.getHeight();
    if (pngWidth > 0 && pngWidth <= 1024 && pngHeight > 0) {
      if (pngWidth <= ART_SIZE && pngHeight <= ART_SIZE) {
        pngDrawWidth = pngWidth;
        pngDrawHeight = pngHeight;
      } else if (pngWidth >= pngHeight) {
        pngDrawWidth = ART_SIZE;
        pngDrawHeight = max(1, pngHeight * ART_SIZE / pngWidth);
      } else {
        pngDrawHeight = ART_SIZE;
        pngDrawWidth = max(1, pngWidth * ART_SIZE / pngHeight);
      }
      pngLastRow = -1;
      ok = png.decode(nullptr, 0) == PNG_SUCCESS;
    }
    Serial.printf("Embedded PNG cover: %dx%d decoded=%d error=%d\n",
                  pngWidth, pngHeight, ok, png.getLastError());
    png.close();
  }
  delete pngDec;
  pngDec = nullptr;
  artFile.close();
  return ok;
}
static bool drawCover() {
  if (track.pngArt) return drawPngCover();
  if (!track.jpegArt || !track.artLength) return false;
  artFile = SD.open(track.path);
  if (!artFile) return false;
  artStart = track.artOffset;
  artSize = track.artLength;
  if (!artFile.seek(artStart) ||
      !jpeg.open((void *)&artFile, int(artSize), nullptr, artRead, artSeek, artDraw)) {
    artFile.close();
    return false;
  }
  int w = jpeg.getWidth(), h = jpeg.getHeight();
  int scale = 1, option = 0;
  if (w > ART_SIZE || h > ART_SIZE) { scale = 2; option = JPEG_SCALE_HALF; }
  if ((w + scale - 1) / scale > ART_SIZE || (h + scale - 1) / scale > ART_SIZE) {
    scale = 4; option = JPEG_SCALE_QUARTER;
  }
  if ((w + scale - 1) / scale > ART_SIZE || (h + scale - 1) / scale > ART_SIZE) {
    scale = 8; option = JPEG_SCALE_EIGHTH;
  }
  int dw = (w + scale - 1) / scale, dh = (h + scale - 1) / scale;
  bool ok = false;
  if (dw <= ART_SIZE && dh <= ART_SIZE) {
    // TFT_eSPI expects the 16-bit color words in display byte order.
    jpeg.setPixelType(RGB565_BIG_ENDIAN);
    int baseX = coverTarget ? 0 : ART_X;
    int baseY = coverTarget ? 0 : ART_Y;
    ok = jpeg.decode(baseX + (ART_SIZE - dw) / 2,
                     baseY + (ART_SIZE - dh) / 2, option);
  }
  Serial.printf("Embedded cover: %dx%d, decoded=%d, error=%d\n",
                w, h, ok, jpeg.getLastError());
  jpeg.close();
  artFile.close();
  return ok;
}
static uint32_t lastSec = 999999;
static bool exitRequested = false;

// --- Gradient Utility ---
static uint16_t lerpColor(uint16_t c1, uint16_t c2, uint8_t t) {
  int r1 = (c1 >> 11) & 0x1F, g1 = (c1 >> 5) & 0x3F, b1 = c1 & 0x1F;
  int r2 = (c2 >> 11) & 0x1F, g2 = (c2 >> 5) & 0x3F, b2 = c2 & 0x1F;
  int r = r1 + ((r2 - r1) * t) / 255;
  int g = g1 + ((g2 - g1) * t) / 255;
  int b = b1 + ((b2 - b1) * t) / 255;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

// Backdrop: a dark tint taken from the cover, fading to black by y=200.
static uint16_t bgTint = 0x18E3;
static uint16_t bgAt(int y) {
  if (y >= 200) return COLOR_BG;
  return lerpColor(bgTint, COLOR_BG, (uint8_t)(y * 255 / 200));
}

static void computeTint(TFT_eSprite *spr) {
  bgTint = 0x18E3;
  if (!spr) return;
  uint16_t *px = (uint16_t *)spr->getPointer();
  uint32_t r = 0, g = 0, b = 0, n = 0;
  for (int y = 4; y < ART_SIZE; y += 8)
    for (int x = 4; x < ART_SIZE; x += 8) {
      uint16_t v = px[y * ART_SIZE + x];
      v = (uint16_t)((v >> 8) | (v << 8));
      r += ((v >> 11) & 31) * 255 / 31;
      g += ((v >> 5) & 63) * 255 / 63;
      b += (v & 31) * 255 / 31;
      ++n;
    }
  int R = r / n, G = g / n, B = b / n, m = (R + G + B) / 3;
  // Push away from grey so the tint reads as a colour, then darken it so
  // white text on top stays readable.
  R = constrain(m + (R - m) * 8 / 5, 0, 255);
  G = constrain(m + (G - m) * 8 / 5, 0, 255);
  B = constrain(m + (B - m) * 8 / 5, 0, 255);
  int mx = max(R, max(G, B));
  if (mx < 12) return;
  R = R * 120 / mx; G = G * 120 / mx; B = B * 120 / mx;
  bgTint = ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
}

static void gradientCircleOn(TFT_eSPI &d, int cx, int cy, int r, uint16_t top, uint16_t bot) {
  for (int y = -r; y <= r; y++) {
    int dx = (int)sqrtf((float)(r * r - y * y));
    uint8_t t = ((y + r) * 255) / (2 * r);
    d.drawFastHLine(cx - dx, cy + y, 2 * dx + 1, lerpColor(top, bot, t));
  }
}

static void gradientCircle(int cx, int cy, int r, uint16_t top, uint16_t bot) {
  gradientCircleOn(screen, cx, cy, r, top, bot);
}

// --- Header: back chevron, "Now Playing", track position ---
static void drawTopBar() {
  for (int y = 0; y < 28; ++y) screen.drawFastHLine(0, y, 240, bgAt(y));
  for (int i = 0; i < 2; ++i) {  // down chevron = back to playlist
    screen.drawLine(14, 10 + i, 19, 15 + i, WHITE);
    screen.drawLine(19, 15 + i, 24, 10 + i, WHITE);
  }
  screen.setTextDatum(MC_DATUM);
  screen.setTextColor(WHITE);
  screen.drawString("Now Playing", 120, 14, 2);
  char idx[12];
  snprintf(idx, sizeof(idx), "%d/%d", songIndex + 1, songCount);
  screen.setTextDatum(MR_DATUM);
  screen.setTextColor(0xC618);
  screen.drawString(idx, 226, 14, 2);
}

// --- Transport controls: white play disc flanked by clean glyphs ---
constexpr int CTRL_Y = 290;
// Selectable controls: Left/Right move the green highlight, Shoot applies it.
enum { CTRL_LIST, CTRL_PREV, CTRL_PLAY, CTRL_NEXT, CTRL_COUNT };
static int ctrlSel = CTRL_PLAY;
static const int CTRL_CX[CTRL_COUNT] = {26, 71, 120, 169};
static const int CTRL_HW[CTRL_COUNT] = {22, 22, 26, 22};   // half-width of each redraw box
static float ctrlScale[CTRL_COUNT] = {1, 1, 1.15f, 1};
static float ctrlTarget(int id) { return ctrlSel == id ? (id == CTRL_PLAY ? 1.15f : 1.4f) : 1.0f; }

// One control drawn at scale s around its centre.
static TFT_eSprite *ctrlSpr = nullptr;  // only alive during an animation
static void drawCtrl(int id, float s, bool isPlaying) {
  const int bx = CTRL_CX[id] - CTRL_HW[id], by = 266, bw = CTRL_HW[id] * 2 + 1, bh = 49;
  // Compose off-screen when possible so the control never flashes black.
  TFT_eSPI &screen = ctrlSpr ? (TFT_eSPI &)*ctrlSpr : ::screen;
  const int ox = ctrlSpr ? bx : 0, oy = ctrlSpr ? by : 0;
  const int cx = CTRL_CX[id] - ox, cy = CTRL_Y - oy;
  uint16_t c = ctrlSel == id ? COLOR_GREEN : WHITE;
  auto S = [s](float v) { return (int)lroundf(v * s); };
  screen.fillRect(bx - ox, by - oy, bw, bh, COLOR_BG);
  switch (id) {
    case CTRL_LIST:
      for (int i = -1; i <= 1; ++i)
        screen.fillRoundRect(cx - S(10), cy + S(i * 6) - S(1.5f), S(20), max(3, S(3)), 1, c);
      break;
    case CTRL_PREV:
      screen.fillRect(cx - S(9), cy - S(8), max(3, S(3)), S(16), c);
      screen.fillTriangle(cx + S(9), cy - S(8), cx + S(9), cy + S(8), cx - S(5), cy, c);
      break;
    case CTRL_PLAY:
      screen.fillCircle(cx, cy, S(20), c);
      if (isPlaying) {
        screen.fillRoundRect(cx - S(8), cy - S(8), S(5), S(16), 1, COLOR_BG);
        screen.fillRoundRect(cx + S(3), cy - S(8), S(5), S(16), 1, COLOR_BG);
      } else {
        screen.fillTriangle(cx - S(5), cy - S(9), cx - S(5), cy + S(9), cx + S(10), cy, COLOR_BG);
      }
      break;
    case CTRL_NEXT:
      screen.fillTriangle(cx - S(9), cy - S(8), cx - S(9), cy + S(8), cx + S(5), cy, c);
      screen.fillRect(cx + S(6), cy - S(8), max(3, S(3)), S(16), c);
      break;
  }
  if (ctrlSpr) ctrlSpr->pushSprite(bx, by, 0, 0, bw, bh);
}

// Ease the old selection down and the new one up (~100 ms).
static void animateControls(int oldSel, bool isPlaying) {
  float from[CTRL_COUNT];
  for (int i = 0; i < CTRL_COUNT; ++i) from[i] = ctrlScale[i];
  ctrlSpr = new TFT_eSprite(&::screen);
  if (!ctrlSpr->createSprite(53, 49)) { delete ctrlSpr; ctrlSpr = nullptr; }
  const int FRAMES = 6;
  for (int f = 1; f <= FRAMES; ++f) {
    float t = f / (float)FRAMES;
    t = 1 - (1 - t) * (1 - t) * (1 - t);  // ease-out cubic
    for (int id : {oldSel, ctrlSel}) {
      ctrlScale[id] = from[id] + (ctrlTarget(id) - from[id]) * t;
      drawCtrl(id, ctrlScale[id], isPlaying);
    }
    delay(12);
  }
  if (ctrlSpr) { ctrlSpr->deleteSprite(); delete ctrlSpr; ctrlSpr = nullptr; }
}

static void drawControls(bool isPlaying) {
  const int cy = CTRL_Y;
  screen.fillRect(0, 266, 240, 49, COLOR_BG);
  for (int id = 0; id < CTRL_COUNT; ++id) {
    ctrlScale[id] = ctrlTarget(id);
    drawCtrl(id, ctrlScale[id], isPlaying);
  }

  // Repeat (muted)
  screen.drawRoundRect(206, cy - 5, 16, 11, 3, MUTED);
  screen.fillRect(211, cy - 6, 6, 3, COLOR_BG);
  screen.fillTriangle(213, cy - 9, 218, cy - 5, 213, cy - 1, MUTED);
}

// --- Messages: centred while the screen is empty, else a thin bottom line ---
static void drawBottomBar(const char *text = nullptr, uint16_t color = COLOR_TEXT_MUTED) {
  if (!text) return;
  screen.setTextDatum(MC_DATUM);
  screen.setTextColor(color, COLOR_BG);
  if (!contentReady) {
    screen.fillRect(0, 150, 240, 20, COLOR_BG);
    screen.drawString(text, 120, 160, 2);
  } else {
    screen.fillRect(0, 311, 240, 9, COLOR_BG);
    screen.drawString(text, 120, 315, 1);
  }
}

// --- Fallback Cover (vinyl disc with gradient) ---
static void fallbackCoverOn(TFT_eSPI &d, int ox, int oy, const char *label) {
  d.fillRoundRect(ox, oy, ART_SIZE, ART_SIZE, 8, COLOR_CARD);
  d.drawRoundRect(ox, oy, ART_SIZE, ART_SIZE, 8, COLOR_BORDER);

  int cx = ox + ART_SIZE / 2;
  int cy = oy + ART_SIZE / 2;

  // Concentric vinyl grooves
  d.fillCircle(cx, cy, 62, 0x0841);
  d.drawCircle(cx, cy, 62, 0x18C3);
  d.drawCircle(cx, cy, 54, 0x10A2);
  d.drawCircle(cx, cy, 46, 0x18C3);
  d.drawCircle(cx, cy, 38, 0x10A2);

  // Center label with gradient
  gradientCircleOn(d, cx, cy, 24, 0x2FEE, 0x0B86);
  d.fillCircle(cx, cy, 7, COLOR_CARD);
  d.drawCircle(cx, cy, 7, COLOR_BORDER);
  d.fillCircle(cx, cy, 2, COLOR_BG);

  d.setTextDatum(BC_DATUM);
  d.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD);
  d.drawString(label, cx, oy + ART_SIZE - 8, 1);
}

// --- Text Fit ---
static String fit(String s, int font, int maxW = TEXT_W) {
  if (screen.textWidth(s, font) <= maxW) return s;
  while (s.length() && screen.textWidth(s + "..", font) > maxW)
    s.remove(s.length() - 1);
  return s + "..";
}

// --- Status ---
static void status(const char *s, uint16_t color) {
  if (!playerVisible) { Serial.println(s); return; }
  if (strcmp(s, "PLAYING  >") == 0 || strcmp(s, "PLAYING") == 0) {
    drawControls(true);
    drawBottomBar();
  } else {
    drawBottomBar(s, color);
  }
}

// ==========================================================================
// Track-change transition: slide + cross-fade
//   next -> old content slides out left, new slides in from the right
//   prev -> old content slides out right, new slides in from the left
// ==========================================================================
// Precomputed per-frame brightness tables (avoid per-pixel divides).
struct ScaleTab {
  uint16_t r[32], g[64], b[32];
};

static void buildScaleTab(ScaleTab &s, uint8_t t) {
  s.r[0] = 0; s.g[0] = 0; s.b[0] = 0;
  for (int i = 1; i < 32; ++i) s.r[i] = (uint16_t)(((i * t + 127) / 255) << 11);
  for (int i = 1; i < 64; ++i) s.g[i] = (uint16_t)(((i * t + 127) / 255) << 5);
  for (int i = 1; i < 32; ++i) s.b[i] = (uint16_t)((i * t + 127) / 255);
}

static inline uint16_t applyTab(const ScaleTab &s, uint16_t c) {
  return s.r[(c >> 11) & 0x1F] | s.g[(c >> 5) & 0x3F] | s.b[c & 0x1F];
}

// Text colours are few, so a plain divide is fine here.
static uint16_t scaleColor(uint16_t c, uint8_t t) {
  int r = (c >> 11) & 0x1F, g = (c >> 5) & 0x3F, b = c & 0x1F;
  r = r * t / 255;
  g = g * t / 255;
  b = b * t / 255;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

// --- Title/artist bands ---------------------------------------------------
// Each line lives in its own TEXT_W x band-height sprite. A frame is composited
// in the sprite and pushed as one opaque rectangle, so the whole text box is
// repainted every time. This is what stops fragments of an old title lingering
// at the left/right ends of a new one.
struct TextBand {
  TFT_eSprite *sprite = nullptr;
  int y = 0, h = 0, font = 1;
  uint16_t color = 0xFFFF;
  int offset = 0;
  uint32_t next = 0;
  bool scrolling = false;
};

static TextBand titleBand, artistBand;
static const int SCROLL_GAP = 60;
static const int SCROLL_STEP_MS = 35;

static bool textBandCreate(TextBand &b, int y, int h, int font, uint16_t color) {
  b.y = y;
  b.h = h;
  b.font = font;
  b.color = color;
  b.sprite = new TFT_eSprite(&screen);
  if (!b.sprite->createSprite(TEXT_W, h)) {
    Serial.printf("ERROR: text sprite %dx%d alloc failed, free heap %u\n",
                  TEXT_W, h, (unsigned)ESP.getFreeHeap());
    delete b.sprite;
    b.sprite = nullptr;
    return false;
  }
  b.sprite->setTextDatum(ML_DATUM);
  b.sprite->setTextColor(color, BG);
  b.sprite->fillSprite(BG);
  return true;
}

static void textBandsInit() {
  bool ok = textBandCreate(titleBand, TITLE_BAND_Y, TITLE_BAND_H,
                           TITLE_FONT, COLOR_TEXT_WHITE);
  ok = textBandCreate(artistBand, ARTIST_BAND_Y, ARTIST_BAND_H,
                      ARTIST_FONT, COLOR_TEXT_MUTED) && ok;
  if (!ok) Serial.println("WARN: text band sprite unavailable");
}

static inline bool textBandsReady() { return titleBand.sprite && artistBand.sprite; }

static void bandClear(TextBand &b) {
  if (b.sprite) b.sprite->fillSprite(BG);
}

// `left` is the x of the first pixel of the line inside the band sprite.
static void bandDraw(TextBand &b, const String &text, int left, uint8_t bright) {
  if (!b.sprite || !bright || text.isEmpty()) return;
  uint16_t fg = scaleColor(b.color, bright);
  if (fg == BG) return;
  b.sprite->setTextDatum(ML_DATUM);
  b.sprite->setTextColor(fg, BG);
  b.sprite->drawString(text, left, b.h / 2, b.font);
}

static void bandPush(TextBand &b) {
  if (b.sprite) b.sprite->pushSprite(TEXT_X, b.y);
}

static int bandWidth(TextBand &b, const String &text) {
  return b.sprite ? b.sprite->textWidth(text, b.font) : 0;
}

static void bandShowCentered(TextBand &b, const String &text, uint8_t bright) {
  bandClear(b);
  bandDraw(b, text, (TEXT_W - bandWidth(b, text)) / 2, bright);
  bandPush(b);
}

static void bandShowScrolled(TextBand &b, const String &text) {
  bandClear(b);
  int period = bandWidth(b, text) + SCROLL_GAP;
  if (period > SCROLL_GAP) {
    bandDraw(b, text, -b.offset, 255);
    bandDraw(b, text, period - b.offset, 255);
  }
  bandPush(b);
}

static void textBandsStop() {
  titleBand.scrolling = false;
  artistBand.scrolling = false;
  titleBand.offset = artistBand.offset = 0;
}

// Static state after a track change: centre short lines, scroll long ones.
static void textBandsStart() {
  textBandsStop();
  if (!textBandsReady()) return;

  titleBand.scrolling = bandWidth(titleBand, track.title) > TEXT_W;
  artistBand.scrolling = bandWidth(artistBand, track.artist) > TEXT_W;

  if (titleBand.scrolling) bandShowScrolled(titleBand, track.title);
  else bandShowCentered(titleBand, track.title, 255);

  if (artistBand.scrolling) bandShowScrolled(artistBand, track.artist);
  else bandShowCentered(artistBand, track.artist, 255);

  uint32_t t = millis() + 700;  // pause before the marquee starts moving
  titleBand.next = t;
  artistBand.next = t;
}

static void textBandsUpdate() {
  TextBand *bands[2] = {&titleBand, &artistBand};
  const String *texts[2] = {&track.title, &track.artist};

  for (int i = 0; i < 2; ++i) {
    TextBand &b = *bands[i];
    if (!b.scrolling || !b.sprite) continue;
    if ((int32_t)(millis() - b.next) < 0) continue;
    b.next = millis() + SCROLL_STEP_MS;

    int period = bandWidth(b, *texts[i]) + SCROLL_GAP;
    if (period <= SCROLL_GAP) continue;
    b.offset += 2;
    if (b.offset >= period) b.offset -= period;
    bandShowScrolled(b, *texts[i]);
  }
}

static void ensureSprite(TFT_eSprite *&spr) {
  if (spr) return;
  spr = new TFT_eSprite(&screen);
  if (!spr->createSprite(ART_SIZE, ART_SIZE)) {
    Serial.printf("ERROR: sprite alloc failed (%d bytes), free heap %u\n",
                  ART_SIZE * ART_SIZE * 2, (unsigned)ESP.getFreeHeap());
    delete spr;
    spr = nullptr;
    return;
  }
  Serial.printf("Sprite allocated, free heap %u\n", (unsigned)ESP.getFreeHeap());
}

// Decode the current track's cover into `spr`. Falls back to the vinyl artwork
// so the sprite always holds the full thumbnail.
static void renderCoverInto(TFT_eSprite *spr) {
  if (!spr) return;
  spr->fillSprite(COLOR_BG);
  coverTarget = spr;
  bool ok = drawCover();
  coverTarget = nullptr;
  if (!ok) fallbackCoverOn(*spr, 0, 0, "NO COVER ART");
}

// Copy one sprite row into the compositing buffer at destX, dimmed by tab.
static void blitRowInto(TFT_eSprite *spr, int destX, int y, const ScaleTab &tab,
                        bool dim) {
  if (!spr) return;
  uint16_t *src = (uint16_t *)spr->getPointer() + (size_t)y * ART_SIZE;

  // Only walk the part of the row that lands on screen.
  int i0 = max(0, -destX);
  int i1 = min(ART_SIZE, 240 - destX);
  uint16_t *out = fadeRow + destX + i0;
  src += i0;
  for (int i = i0; i < i1; ++i, ++out, ++src) {
    uint16_t v = *src;                                   // big-endian in sprite
    uint16_t logical = (uint16_t)((v >> 8) | (v << 8));
    uint16_t s = dim ? applyTab(tab, logical) : logical;
    *out = (uint16_t)((s >> 8) | (s << 8));              // back to display order
  }
}

// Composite both covers and stream the whole 160-row band in one SPI window.
// This is the difference between ~30 ms and ~300 ms per frame.
static void composeCoverFrame(TFT_eSprite *a, int ax, const ScaleTab &ta, bool da,
                              TFT_eSprite *b, int bx, const ScaleTab &tb, bool db) {
  screen.startWrite();
  screen.setAddrWindow(0, ART_Y, 240, ART_SIZE);
  for (int y = 0; y < ART_SIZE; ++y) {
    uint16_t bg = bgAt(ART_Y + y);      // tinted backdrop behind the covers
    bg = (uint16_t)((bg >> 8) | (bg << 8));
    for (int i = 0; i < 240; ++i) fadeRow[i] = bg;
    blitRowInto(a, ax, y, ta, da);
    blitRowInto(b, bx, y, tb, db);
    screen.pushPixels(fadeRow, 240);
  }
  screen.endWrite();
}

// Push the current cover at a given brightness (first-track fade-in).
static void drawCoverBrightness(uint8_t t) {
  if (!coverSprite) return;
  ScaleTab tab;
  buildScaleTab(tab, t);
  uint16_t *src = (uint16_t *)coverSprite->getPointer();

  screen.startWrite();
  screen.setAddrWindow(ART_X, ART_Y, ART_SIZE, ART_SIZE);
  for (int y = 0; y < ART_SIZE; ++y) {
    uint16_t *row = src + y * ART_SIZE;
    for (int x = 0; x < ART_SIZE; ++x) {
      uint16_t v = row[x];
      uint16_t logical = (uint16_t)((v >> 8) | (v << 8));
      uint16_t s = applyTab(tab, logical);
      fadeRow[x] = (uint16_t)((s >> 8) | (s << 8));
    }
    screen.pushPixels(fadeRow, ART_SIZE);
  }
  screen.endWrite();
}

// Draw old and new title/artist for one frame of the slide. Both lines are
// composited inside their band sprites and pushed whole, so each frame is
// completely self-contained (no residue from the previous frame).
static void drawTextFrame(const String &ot, const String &oa,
                          const String &nt, const String &na,
                          int oldOff, int newOff, uint8_t tOld, uint8_t tNew) {
  bandClear(titleBand);
  bandDraw(titleBand, ot, (TEXT_W - bandWidth(titleBand, ot)) / 2 + oldOff, tOld);
  bandDraw(titleBand, nt, (TEXT_W - bandWidth(titleBand, nt)) / 2 + newOff, tNew);

  bandClear(artistBand);
  bandDraw(artistBand, oa, (TEXT_W - bandWidth(artistBand, oa)) / 2 + oldOff, tOld);
  bandDraw(artistBand, na, (TEXT_W - bandWidth(artistBand, na)) / 2 + newOff, tNew);

  bandPush(titleBand);
  bandPush(artistBand);
}

// Title + artist at full brightness (first track, no old content to slide).
static void drawTextContent(uint8_t t) {
  if (!textBandsReady()) {
    // Only reachable if the two small band sprites could not be allocated.
    screen.fillRect(TEXT_X, TITLE_BAND_Y, TEXT_W,
                    (ARTIST_BAND_Y + ARTIST_BAND_H) - TITLE_BAND_Y, COLOR_BG);
    screen.setTextDatum(MC_DATUM);
    screen.setTextColor(scaleColor(COLOR_TEXT_WHITE, t), COLOR_BG);
    screen.drawString(fit(track.title, TITLE_FONT, TEXT_W), 120, TITLE_CY, TITLE_FONT);
    screen.setTextColor(scaleColor(COLOR_TEXT_MUTED, t), COLOR_BG);
    screen.drawString(fit(track.artist, ARTIST_FONT, TEXT_W), 120, ARTIST_CY, ARTIST_FONT);
    return;
  }

  // Long lines are shown truncated while fading in; the marquee takes over.
  String title = fit(track.title, TITLE_FONT, TEXT_W);
  String artist = fit(track.artist, ARTIST_FONT, TEXT_W);
  bandShowCentered(titleBand, title, t);
  bandShowCentered(artistBand, artist, t);
}

static void drawContentAt(uint8_t t) {
  drawCoverBrightness(t);
  drawTextContent(t);
}

// Redraw just the progress area (no full-screen clear, so no flash).
// Progress bar with a position knob, composed in a small sprite (no flicker).
static TFT_eSprite *progSpr = nullptr;
static void drawProgressBar(int bar) {
  constexpr int H = 12, PAD = 6;
  if (!progSpr) {
    progSpr = new TFT_eSprite(&screen);
    if (!progSpr->createSprite(PROG_W + 2 * PAD, H)) { delete progSpr; progSpr = nullptr; }
  }
  if (!progSpr) {
    screen.fillRect(PROG_X, PROG_Y, PROG_W, 4, COLOR_PROG_BG);
    screen.fillRect(PROG_X, PROG_Y, bar, 4, COLOR_GREEN);
    return;
  }
  progSpr->fillSprite(COLOR_BG);
  progSpr->fillRoundRect(PAD, H / 2 - 2, PROG_W, 4, 2, 0x3186);
  if (bar > 0) progSpr->fillRoundRect(PAD, H / 2 - 2, bar, 4, 2, COLOR_GREEN);
  progSpr->fillCircle(PAD + bar, H / 2, 5, WHITE);
  progSpr->pushSprite(PROG_X - PAD, PROG_Y - 4);
}

static void drawTimes(const char *left, const char *right) {
  screen.setTextPadding(48);
  screen.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  screen.setTextDatum(TL_DATUM);
  screen.drawString(left, PROG_X, PROG_Y + 10, 2);
  screen.setTextDatum(TR_DATUM);
  screen.drawString(right, PROG_X + PROG_W, PROG_Y + 10, 2);
  screen.setTextPadding(0);
}

static void resetProgressArea() {
  // Keep clear of the artist band (ends at y=236) and the controls (y>=269).
  screen.fillRect(0, PROG_Y - 4, 240, 30, COLOR_BG);
  drawProgressBar(0);
  previousBar = 0;
  lastSec = 999999;
  drawTimes("0:00", "--:--");
}

// Static bits of the player screen (top bar, progress, controls).
static void drawPlayerBase() {
  for (int y = 0; y < 320; ++y) screen.drawFastHLine(0, y, 240, bgAt(y));
  textBandsStop();
  drawTopBar();
  resetProgressArea();
  drawControls(true);
  drawBottomBar();
}

static void fadeIn() {
  for (int t = 0; t <= 255; t += 26) drawContentAt((uint8_t)t);
  drawContentAt(255);
}

// Slide both covers across the screen while cross-fading them.
// dir = +1: content travels left (next)   -1: content travels right (prev)
static void slideFade(TFT_eSprite *oldSpr, const String &oldTitle, const String &oldArtist,
                      TFT_eSprite *newSpr, int dir) {
  const int steps = 10;
  const int span = 240;
  ScaleTab tabOld, tabNew;

  for (int i = 1; i <= steps; ++i) {
    uint8_t tNew = (uint8_t)(255 * i / steps);
    uint8_t tOld = (uint8_t)(255 - tNew);
    buildScaleTab(tabOld, tOld);
    buildScaleTab(tabNew, tNew);

    int sh = span * i / steps;
    int oldOff = -dir * sh;            // old slides away
    int newOff = dir * (span - sh);    // new arrives from the far side

    composeCoverFrame(oldSpr, ART_X + oldOff, tabOld, true,
                      newSpr, ART_X + newOff, tabNew, true);
    drawTextFrame(oldTitle, oldArtist, track.title, track.artist,
                  oldOff, newOff, tOld, tNew);
    if (i < steps) delay(2);
  }
}

static void startMarqueeIfNeeded() {
  textBandsStart();
}

// --- Time Formatting ---
static void formatTime(char *buf, uint32_t sec) {
  uint32_t m = sec / 60;
  uint32_t s = sec % 60;
  sprintf(buf, "%lu:%02lu", (unsigned long)m, (unsigned long)s);
}

// --- Progress Update ---
static void updateProgress() {
  if (!musicFile || !musicFile->isOpen()) return;
  uint32_t total = musicFile->getSize(), current = musicFile->getPos();
  if (!total) return;

  int bar = (uint64_t(current) * PROG_W) / total;
  if (bar > PROG_W) bar = PROG_W;

  if (bar != previousBar) {
    drawProgressBar(bar);
    previousBar = bar;
  }

  uint32_t now = paused ? pausedAt : millis();
  uint32_t elapsedSec = playStarted ? (now - playStarted) / 1000 : 0;
  if (elapsedSec != lastSec) {
    lastSec = elapsedSec;
    char elapsedStr[16];
    formatTime(elapsedStr, elapsedSec);

    char remainStr[16];
    if (current > 10000 && elapsedSec > 2) {
      float frac = (float)current / (float)total;
      uint32_t totalSec = (float)elapsedSec / frac;
      if (totalSec >= elapsedSec) {
        uint32_t rem = totalSec - elapsedSec;
        formatTime(remainStr, rem);
      } else {
        strcpy(remainStr, "0:00");
      }
    } else {
      strcpy(remainStr, "--:--");
    }

    char remainNeg[18];
    snprintf(remainNeg, sizeof(remainNeg), remainStr[0] == '-' ? "%s" : "-%s", remainStr);
    drawTimes(elapsedStr, remainNeg);
  }
}
// --- Track loading / switching ---
// dir: +1 when moving forward (slide left), -1 when moving backward (slide right).
static bool playTrack(int index, int dir = 1, bool show = true) {
  if (!audioOut) { Serial.println("Audio not initialized"); return false; }
  if (songCount == 0) findSongs();
  if (songCount == 0) { status("NO MP3 IN /Music", TFT_RED); return false; }
  if (index < 0 || index >= songCount) {
    Serial.printf("Invalid song number. Use 1..%d\n", songCount);
    return false;
  }

  // Keep the outgoing content so it can slide/fade away while the new cover loads.
  bool hadContent = contentReady;
  String oldTitle = track.title, oldArtist = track.artist;
  if (hadContent) textBandsStop();

  // Stop and release the current sources before opening the next track.
  mp3.stop();
  if (musicBuffer) { delete musicBuffer; musicBuffer = nullptr; }
  if (musicFile)   { delete musicFile;   musicFile = nullptr; }

  songIndex = index;
  track = Track();
  track.path = songs[index];
  track.title = track.path.substring(track.path.lastIndexOf('/') + 1);
  track.title.remove(track.title.lastIndexOf('.'));
  track.artist = "Music from SD card";
  parseId3();
  if (track.title.isEmpty()) track.title = "Untitled track";
  if (track.artist.isEmpty()) track.artist = "Unknown artist";

  // From the playlist, just switch the audio; the screen is drawn on open.
  if (!show) { contentReady = false; goto openAudio; }
  {
  // Decode the new cover into the spare sprite before touching the screen.
  ensureSprite(nextCoverSprite);
  if (nextCoverSprite) renderCoverInto(nextCoverSprite);

  bool canSlide = hadContent && coverSprite && nextCoverSprite;

  if (canSlide) {
    // Repaint only the thumbnail + text bands, so the rest of the UI never blinks.
    uint32_t t0 = millis();
    computeTint(nextCoverSprite);
    drawTopBar();
    slideFade(coverSprite, oldTitle, oldArtist, nextCoverSprite, dir);
    Serial.printf("Slide done in %lu ms (dir %d)\n", (unsigned long)(millis() - t0), dir);
    TFT_eSprite *tmp = coverSprite;
    coverSprite = nextCoverSprite;
    nextCoverSprite = tmp;
    resetProgressArea();
  } else if (nextCoverSprite) {
    // First track: nothing to slide from, just fade in.
    computeTint(nextCoverSprite);
    drawPlayerBase();
    TFT_eSprite *tmp = coverSprite;   // keep the spare, never drop it
    coverSprite = nextCoverSprite;
    nextCoverSprite = tmp;
    fadeIn();
  } else {
    // No spare sprite (out of memory): draw directly, no animation.
    Serial.println("WARN: cover sprite unavailable, drawing without animation");
    computeTint(nullptr);
    drawPlayerBase();
    if (!drawCover()) fallbackCoverOn(screen, ART_X, ART_Y, "NO COVER ART");
    drawTextContent(255);
  }
  startMarqueeIfNeeded();
  contentReady = true;
  }

openAudio:
  musicFile = new AudioFileSourceSD(track.path.c_str());
  if (!musicFile->isOpen()) { status("MP3 OPEN FAILED", TFT_RED); return false; }
  musicBuffer = new AudioFileSourceBuffer(musicFile, readBuffer, READ_BUFFER_SIZE);
  // ESP8266Audio's stop() marks I2S off without uninstalling the driver, so
  // begin() would fail to re-install it ("register I2S object ... failed")
  // and leave the amp silent. Uninstall first so every track starts clean.
  i2s_driver_uninstall(I2S_NUM_0);
  if (!mp3.begin(musicBuffer, audioOut)) { status("MP3 DECODE FAILED", TFT_RED); return false; }
  playStarted = millis();
  paused = false;
  status("PLAYING  >", ACCENT);
  Serial.printf("Now playing [%d/%d]: %s\n", songIndex + 1, songCount, track.title.c_str());
  return true;
}

static void listSongs() {
  if (songCount == 0) findSongs();
  Serial.printf("Songs on SD card (%d):\n", songCount);
  for (int i = 0; i < songCount; ++i)
    Serial.printf("  %2d) %s%s\n", i + 1, songs[i].c_str(),
                  i == songIndex ? "  <-- playing" : "");
}

static void printStatus() {
  Serial.printf("Current [%d/%d]: %s\n", songIndex + 1, songCount, track.title.c_str());
  Serial.printf("Artist: %s\n", track.artist.c_str());
  Serial.printf("Heap free: %u | sprites: cur=%s next=%s | scroll: t=%d a=%d\n",
                (unsigned)ESP.getFreeHeap(),
                coverSprite ? "ok" : "null", nextCoverSprite ? "ok" : "null",
                titleBand.scrolling, artistBand.scrolling);
  Serial.printf("Widths: title=%d artist=%d (box %d)\n",
                screen.textWidth(track.title, TITLE_FONT),
                screen.textWidth(track.artist, ARTIST_FONT), TEXT_W);
}

static void printHelp() {
  Serial.println("Commands:");
  Serial.println("  list        - list songs on the SD card");
  Serial.println("  play <n>    - play song number n (1-based)");
  Serial.println("  next        - play the next song");
  Serial.println("  prev        - play the previous song");
  Serial.println("  status      - show the current song");
  Serial.println("  help        - show this help");
  Serial.println("  q / menu    - return to the home screen");
}

static void processCommand(String cmd) {
  cmd.trim();
  String c = cmd;
  c.toLowerCase();
  if (c == "list" || c == "ls") {
    listSongs();
  } else if (c == "next" || c == "n") {
    if (songCount) playTrack((songIndex + 1) % songCount, +1);
  } else if (c == "prev" || c == "p") {
    if (songCount) playTrack((songIndex - 1 + songCount) % songCount, -1);
  } else if (c == "play") {
    if (songCount) playTrack(songIndex, +1);
  } else if (c.startsWith("play ")) {
    int n = c.substring(5).toInt() - 1;
    if (n != songIndex) playTrack(n, n > songIndex ? +1 : -1);
  } else if (c == "status") {
    printStatus();
  } else if (c == "q" || c == "menu" || c == "exit") {
    exitRequested = true;
  } else if (c == "help" || c == "?") {
    printHelp();
  } else {
    Serial.printf("Unknown command: %s\n", cmd.c_str());
    printHelp();
  }
}


static String serialLine;
static void handleSerial() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      if (serialLine.length()) processCommand(serialLine);
      serialLine = "";
    } else if (serialLine.length() < 80) {
      serialLine += ch;
    }
  }
}

static bool sdReady = false;
static TFT_eSprite *rowSpr = nullptr;

static void freeSprite(TFT_eSprite *&s) {
  if (s) { s->deleteSprite(); delete s; s = nullptr; }
}

static void freePlayer() {
  if (mp3Dec) mp3.stop();
  if (musicBuffer) { delete musicBuffer; musicBuffer = nullptr; }
  if (musicFile) { delete musicFile; musicFile = nullptr; }
  freeSprite(coverSprite); freeSprite(nextCoverSprite);
  freeSprite(titleBand.sprite); freeSprite(artistBand.sprite);
  freeSprite(progSpr); freeSprite(rowSpr);
  delete mp3Dec; mp3Dec = nullptr;
  free(readBuffer); readBuffer = nullptr;
  contentReady = false;
  paused = false;
}

// ==========================================================================
// Library / playlist screen
// ==========================================================================
static String songTitles[MAX_SONGS], songArtists[MAX_SONGS];
static bool libraryLoaded = false;
static int listSel = 0, listTop = 0;
constexpr int LIST_Y = 66, ROW_H = 44, LIST_ROWS = 5;
constexpr int FOOTER_Y = LIST_Y + ROW_H * LIST_ROWS;  // 286
constexpr uint16_t COLOR_SELECTED = 0x2124;

static bool trackLoaded() { return musicFile != nullptr; }

// Read every song's ID3 title/artist once, with a small progress bar.
static void loadLibrary() {
  if (libraryLoaded) return;
  for (int i = 0; i < songCount; ++i) {
    track = Track();
    track.path = songs[i];
    track.title = track.path.substring(track.path.lastIndexOf('/') + 1);
    track.title.remove(track.title.lastIndexOf('.'));
    parseId3();
    songTitles[i] = track.title.length() ? track.title : String("Untitled track");
    songArtists[i] = track.artist.length() ? track.artist : String("Unknown artist");
    loaderTick(false);
  }
  track = Track();
  libraryLoaded = true;
}

static uint16_t avatarColor(const String &s) {
  static const uint16_t palette[] = {0xF9F0, 0xFCA0, 0x1DCA, 0x2BDF, 0x903F, 0x0657};
  uint32_t h = 0;
  for (unsigned i = 0; i < s.length(); ++i) h = h * 31 + (uint8_t)s[i];
  return palette[h % 6];
}

static String fitW(const String &s, int font, int w) { return fit(s, font, w); }

static void drawRow(int slot) {
  int i = listTop + slot, y = LIST_Y + slot * ROW_H;
  if (!rowSpr) {
    rowSpr = new TFT_eSprite(&screen);
    if (!rowSpr->createSprite(240, ROW_H)) { delete rowSpr; rowSpr = nullptr; }
  }
  // Compose in the row sprite when memory allows; otherwise draw directly.
  TFT_eSPI &r = rowSpr ? (TFT_eSPI &)*rowSpr : screen;
  const int oy = rowSpr ? 0 : y;
  r.fillRect(0, oy, 240, ROW_H, BG);
  if (i < songCount) {
    bool sel = i == listSel;
    bool cur = trackLoaded() && i == songIndex;
    if (sel) r.fillRoundRect(6, oy + 2, 222, ROW_H - 4, 10, COLOR_SELECTED);

    // Letter avatar
    uint16_t c = avatarColor(songTitles[i]);
    r.fillRoundRect(14, oy + 6, 32, 32, 8, c);
    r.fillRoundRect(14, oy + 22, 32, 16, 8, lerpColor(c, 0x0000, 60));
    r.fillRect(14, oy + 22, 32, 8, lerpColor(c, 0x0000, 60));
    char initial = '#';
    for (unsigned k = 0; k < songTitles[i].length(); ++k)
      if (isalnum((uint8_t)songTitles[i][k])) { initial = toupper(songTitles[i][k]); break; }
    char ini[2] = {initial, 0};
    r.setTextDatum(MC_DATUM);
    r.setTextColor(WHITE);
    r.drawString(ini, 30, oy + 23, 4);

    r.setTextDatum(ML_DATUM);
    r.setTextColor(cur ? ACCENT : WHITE);
    r.drawString(fitW(songTitles[i], 2, 146), 56, oy + 14, 2);
    r.setTextColor(MUTED);
    r.drawString(fitW(songArtists[i], 2, 146), 56, oy + 31, 2);

    if (cur) {  // "now playing" equaliser bars
      uint16_t eq = paused ? MUTED : ACCENT;
      const int hts[] = {8, 14, 10};
      for (int b = 0; b < 3; ++b) r.fillRect(208 + b * 5, oy + 29 - hts[b], 3, hts[b], eq);
    } else if (sel) {
      r.drawLine(210, oy + 16, 215, oy + 22, MUTED); r.drawLine(215, oy + 22, 210, oy + 28, MUTED);
      r.drawLine(211, oy + 16, 216, oy + 22, MUTED); r.drawLine(216, oy + 22, 211, oy + 28, MUTED);
    }
  }
  if (rowSpr) rowSpr->pushSprite(0, y);
}

static void drawScrollbar() {
  if (songCount <= LIST_ROWS) return;
  int trackH = ROW_H * LIST_ROWS - 8;
  int thumbH = max(16, trackH * LIST_ROWS / songCount);
  int thumbY = (trackH - thumbH) * listTop / (songCount - LIST_ROWS);
  screen.fillRect(235, LIST_Y + 4, 3, trackH, BG);
  screen.fillRoundRect(235, LIST_Y + 4 + thumbY, 3, thumbH, 1, 0x4A69);
}

static void drawPlaylistHeader() {
  for (int y = 0; y < LIST_Y; ++y)
    screen.drawFastHLine(0, y, 240, lerpColor(0x2AC2, BG, (uint8_t)(y * 255 / (LIST_Y - 1))));
  screen.setTextDatum(TL_DATUM);
  screen.setTextColor(WHITE);
  screen.drawString("Library", 16, 12, 4);
  char sub[24];
  snprintf(sub, sizeof(sub), "%d songs", songCount);
  screen.setTextColor(0xC618);
  screen.drawString(sub, 17, 40, 2);
  screen.fillCircle(210, 30, 17, ACCENT);
  screen.fillTriangle(204, 21, 204, 39, 220, 30, BG);
}

static void drawFooter() {
  screen.fillRect(0, FOOTER_Y, 240, 320 - FOOTER_Y, BG);
  if (trackLoaded()) {
    // Mini player: tap D / E on the playing song to open Now Playing.
    screen.fillRoundRect(8, FOOTER_Y + 3, 224, 28, 8, 0x18E3);
    int cy = FOOTER_Y + 17;
    if (paused) screen.fillTriangle(20, cy - 6, 20, cy + 6, 29, cy, WHITE);
    else { screen.fillRect(19, cy - 6, 3, 12, WHITE); screen.fillRect(26, cy - 6, 3, 12, WHITE); }
    screen.setTextDatum(ML_DATUM);
    screen.setTextColor(WHITE);
    screen.drawString(fitW(songTitles[songIndex], 2, 150), 38, cy, 2);
    screen.setTextDatum(MR_DATUM);
    screen.setTextColor(MUTED);
    screen.drawString("Right >", 224, cy, 2);
  } else {
    screen.setTextDatum(MC_DATUM);
    screen.setTextColor(MUTED);
    screen.drawString("< Home   Shoot play   Open >", 120, FOOTER_Y + 17, 2);
  }
}

static void drawPlaylist() {
  drawPlaylistHeader();
  for (int k = 0; k < LIST_ROWS; ++k) drawRow(k);
  screen.fillRect(236, LIST_Y, 4, ROW_H * LIST_ROWS, BG);
  drawScrollbar();
  drawFooter();
}

static void moveSelection(int delta) {
  int old = listSel, oldTop = listTop;
  listSel = (listSel + delta + songCount) % songCount;
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

// ==========================================================================
// Screen switching and controls
// ==========================================================================
static void showPlaylist() {
  textBandsStop();
  playerVisible = false;
  contentReady = false;
  listSel = songIndex;
  if (listSel < listTop || listSel >= listTop + LIST_ROWS)
    listTop = max(0, min(listSel - 2, songCount - LIST_ROWS));
  drawPlaylist();
}

// Re-open Now Playing for the song that is already playing (no restart).
static void showNowPlaying() {
  freeSprite(rowSpr);  // 21 KB back for the cover sprites and audio
  playerVisible = true;
  ensureSprite(coverSprite);
  renderCoverInto(coverSprite);
  computeTint(coverSprite);
  drawPlayerBase();
  if (coverSprite) drawContentAt(255);
  else { if (!drawCover()) fallbackCoverOn(screen, ART_X, ART_Y, "NO COVER ART"); drawTextContent(255); }
  textBandsStart();
  contentReady = true;
  drawControls(!paused);
  updateProgress();
}

static void togglePause() {
  if (!trackLoaded()) return;
  paused = !paused;
  if (paused) {
    pausedAt = millis();
    i2s_zero_dma_buffer(I2S_NUM_0);  // silence instead of looping the last buffer
  } else {
    playStarted += millis() - pausedAt;
  }
  if (playerVisible) drawControls(!paused);
  else { drawRow(songIndex - listTop); drawFooter(); }
}

// Console buttons map onto the original serial keys.
static int readKey() {
  static const char keys[] = {'w', 'd', 's', 'a', 'e'};
  int c = buttonsNextPress();
  if (c >= 0 && c < 5) {
    // Left on the playlist leaves the player (launcher convention).
    if (c == 3 && !playerVisible) return 'q';
    return keys[c];
  }
  return Serial.available() ? tolower(Serial.read()) : -1;
}

static void handleKeys() {
  int k = readKey();
  if (k < 0) return;
  if (playerVisible) {
    if (k == 'a' || k == 'd') {
      int old = ctrlSel;
      ctrlSel = (ctrlSel + (k == 'd' ? 1 : CTRL_COUNT - 1)) % CTRL_COUNT;
      animateControls(old, !paused);
    } else if (k == 'e' || k == ' ') {
      switch (ctrlSel) {
        case CTRL_LIST: showPlaylist(); break;
        case CTRL_PREV: playTrack((songIndex - 1 + songCount) % songCount, -1); break;
        case CTRL_NEXT: playTrack((songIndex + 1) % songCount, +1); break;
        default: togglePause(); break;
      }
    } else if (k == 's' || k == 'q') showPlaylist();
    return;
  }
  if (k == 'w') moveSelection(-1);
  else if (k == 's') moveSelection(+1);
  else if (k == 'q' || k == 'a') exitRequested = true;
  else if (k == 'd') {  // open Now Playing with the cover
    ctrlSel = CTRL_PLAY;
    if (trackLoaded()) showNowPlaying();
    else {
      freeSprite(rowSpr);
      playerVisible = true;
      contentReady = false;
      playTrack(listSel, +1, true);
    }
  } else if (k == 'e' || k == ' ') {  // play here, mini banner at the bottom
    if (trackLoaded() && listSel == songIndex) { togglePause(); return; }
    int old = trackLoaded() ? songIndex : -1;
    playTrack(listSel, +1, false);
    if (old >= listTop && old < listTop + LIST_ROWS) drawRow(old - listTop);
    drawRow(listSel - listTop);
    drawFooter();
  }
}

// ==========================================================================
// Loading screen: a spinning arc that grows and shrinks while it cycles
// colours, with a rotating line of text below. The SD card shares the SPI
// bus with the display, so it is advanced by loaderTick() between steps
// instead of from a separate task.
// ==========================================================================
constexpr int LD_X = 120, LD_Y = 140, LD_R = 26, LD_W = 5;
static uint32_t ldStart = 0, ldLast = 0;
static int ldMsg = -1;
static const char *const LD_TEXT[] = {
  "Waking up the SD card", "Looking for your songs", "Reading album tags",
  "Dusting off the covers", "Tuning the speaker", "Almost there"};
constexpr int LD_TEXT_N = sizeof(LD_TEXT) / sizeof(LD_TEXT[0]);

// Clockwise arc from a to b (degrees, may wrap past 360).
static void ldArc(int a, int b, uint16_t col) {
  a = ((a % 360) + 360) % 360;
  int len = ((b - a) % 360 + 360) % 360;
  if (len < 2) return;
  if (a + len <= 360) screen.drawArc(LD_X, LD_Y, LD_R, LD_R - LD_W, a, a + len, col, BG, true);
  else {
    screen.drawArc(LD_X, LD_Y, LD_R, LD_R - LD_W, a, 360, col, BG, true);
    screen.drawArc(LD_X, LD_Y, LD_R, LD_R - LD_W, 0, a + len - 360, col, BG, true);
  }
}

static void loaderTick(bool force) {
  uint32_t now = millis();
  if (!force && now - ldLast < 20) return;
  ldLast = now;
  uint32_t t = now - ldStart;
  static const uint16_t cols[] = {0x4C3F, 0xE8E4, 0xFDA0, 0x2DAA};  // blue red yellow green
  const uint32_t P = 1333;
  uint32_t cycle = t / P;
  float ph = (t % P) / (float)P;                       // 0..1 within one grow/shrink
  float grow = ph < 0.5f ? ph * 2 : 1.0f;              // head runs ahead first half
  float shrink = ph < 0.5f ? 0.0f : (ph - 0.5f) * 2;   // tail catches up second half
  grow = grow * grow * (3 - 2 * grow);
  shrink = shrink * shrink * (3 - 2 * shrink);
  int base = (int)(t * 0.3f) + (int)cycle * 270;       // steady spin + per-cycle jump
  int tail = base + (int)(shrink * 270);
  int head = base + 20 + (int)(grow * 270);
  ldArc(head, tail + 360, BG);
  ldArc(tail, head, cols[cycle % 4]);

  int m = min((int)(t / 1500), LD_TEXT_N - 1);
  if (m != ldMsg) {
    ldMsg = m;
    screen.fillRect(0, LD_Y + 44, 240, 22, BG);
    screen.setTextDatum(MC_DATUM);
    screen.setTextColor(MUTED, BG);
    screen.drawString(LD_TEXT[m], 120, LD_Y + 55, 2);
  }
  // Animated dots so the line feels alive between messages.
  int dots = (t / 400) % 4;
  int w = screen.textWidth(LD_TEXT[m], 2);
  int x = 120 + w / 2 + 3;
  screen.fillRect(x, LD_Y + 57, 16, 4, BG);
  for (int i = 0; i < dots; ++i) screen.fillRect(x + i * 5, LD_Y + 58, 2, 2, MUTED);
}

static void loaderBegin() {
  screen.fillScreen(BG);
  ldStart = millis();
  ldLast = 0;
  ldMsg = -1;
  loaderTick(true);
}

// Keep the animation up for a moment so it never just flashes.
static void loaderFinish() {
  while (millis() - ldStart < 1600) { loaderTick(false); delay(5); }
}

static void loaderFail(const char *msg) {
  screen.fillScreen(BG);
  contentReady = false;
  status(msg, TFT_RED);
}

static void musicSetup() {
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  loaderBegin();
  while (millis() - ldStart < 300) { loaderTick(false); delay(4); }
  if (!sdReady) {
    SPI.end();
    SPI.begin(1, 21, 2, SD_CS);
    digitalWrite(TFT_CS, HIGH);
    sdReady = SD.begin(SD_CS, SPI, 40000000);
  }
  if (!sdReady) {
    loaderFail("SD CARD ERROR");
    Serial.println("SD card mount failed");
    return;
  }
  loaderTick(true);
  if (!songCount) findSongs();
  loaderTick(true);
  if (!songCount) { loaderFail("NO MP3 IN /Music"); return; }
  if (!mp3Dec) mp3Dec = new (std::nothrow) AudioGeneratorMP3a();
  if (!readBuffer) readBuffer = (uint8_t *)malloc(READ_BUFFER_SIZE);
  if (!mp3Dec || !readBuffer) { loaderFail("NOT ENOUGH MEMORY"); return; }
  if (!audioOut) {
    audioOut = new AudioOutputI2S(0, AudioOutputI2S::EXTERNAL_I2S, 16);
    audioOut->SetPinout(BCLK, LRCLK, DIN);
    audioOut->SetGain(0.80);
  }

  // Allocate the small text band sprites before the big covers so the heap
  // stays tidy, then both thumbnails up front so the slide transition never
  // has to find ~51KB in a fragmented heap mid-playback.
  textBandsInit();
  ensureSprite(coverSprite);
  ensureSprite(nextCoverSprite);
  Serial.printf("Heap after sprites: %u bytes\n", (unsigned)ESP.getFreeHeap());

  loaderTick(true);
  loadLibrary();
  loaderFinish();
  Serial.println("Music player ready: w/s browse, e play, q home");
}

static void musicLoop() {
  handleKeys();
  if (exitRequested) return;
  if (playerVisible) textBandsUpdate();
  if (paused || !mp3.isRunning()) { delay(5); return; }
  if (!mp3.loop()) {
    // End of track: advance to the next song and wrap at the end of the list.
    mp3.stop();
    Serial.println("Playback finished; advancing to next track");
    int next = (songIndex + 1) % songCount;
    playTrack(next, +1, playerVisible);
    if (!playerVisible) { listSel = next; showPlaylist(); }
    return;
  }
  if (playerVisible && millis() - lastProgress >= 250) {
    lastProgress = millis();
    updateProgress();
  }
}

// Opens on the playlist; q there returns to the home screen.
void runMusicPlayer() {
  exitRequested = false;
  screen.setSwapBytes(false);  // the launcher leaves byte-swapping on; covers need it off
  playerVisible = true;        // so setup/error messages are drawn
  contentReady = false;
  musicSetup();
  bool ready = libraryLoaded && songCount && audioOut && mp3Dec && readBuffer;
  if (ready) showPlaylist();
  while (!exitRequested) {
    if (ready) musicLoop();
    else {
      int k = readKey();
      if (k == 'q' || k == 'a' || k == 'e' || k == ' ') exitRequested = true;
      delay(10);
    }
    yield();
  }
  freePlayer();
  playerVisible = false;
  Serial.println("Back to home screen");
}
