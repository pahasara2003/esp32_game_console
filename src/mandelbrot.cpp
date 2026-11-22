#include "ui_kit.h"
#include <Arduino.h>
#include <math.h>
#include "buttons.h"
#include "game.h"


namespace mandelApp {
// The plot is square-pixel: both complex axes use the same scale.
constexpr int X = 8, Y = 64, W = 224, H = 208;
constexpr int MAX_ZOOM = 20;
constexpr int OPTIONS = 5;
static_assert(X + W <= 240 && Y + H <= 280, "Plot must clear the footer");
static double centerRe = -0.5, centerIm = 0.0;
static int zoom = 0, option = 0;
static bool menu = false, rendering = false;
static bool shootHeld = false, holdSent = false;
static unsigned long pressedAt = 0, lastStatus = 0;
static bool inputArmed = false;
static bool navigationPending = false;
static int block = 8, column = 0, row = 0, limit = 96;
static uint16_t strip[W * 8];
static uint16_t palette[256];
// Coarse preview classification. A solid interior block is already known to
// be black, so it does not need the expensive 4x/2x/1x refinement passes.
constexpr int COARSE_W = (W + 7) / 8;
constexpr int COARSE_H = (H + 7) / 8;
// At high magnification, a fractal edge can be thinner than one coarse cell.
// Keep every pixel in the refinement passes once the view reaches this zoom.
constexpr int ADAPTIVE_MAX_ZOOM = 7;
static uint16_t coarseColor[COARSE_W * COARSE_H];

static bool isStableBackground(uint16_t color) {
  if (color == 0 || color == palette[0]) return true;
  // Treat only very dark blue as background. Brighter blues remain detail so
  // the colored edge around the set is never discarded.
  int r = ((color >> 11) & 31) * 255 / 31;
  int g = ((color >> 5) & 63) * 255 / 63;
  int b = (color & 31) * 255 / 31;
  return b > 45 && b > r + 18 && b > g + 8 && r < 55 && g < 100;
}

static bool coarseNeighborhoodNeedsDetail(int coarseX, int coarseY) {
  uint16_t center = coarseColor[coarseY * COARSE_W + coarseX];
  if (!isStableBackground(center)) return true;
  for (int ny = max(0, coarseY - 1); ny <= min(COARSE_H - 1, coarseY + 1); ++ny) {
    for (int nx = max(0, coarseX - 1); nx <= min(COARSE_W - 1, coarseX + 1); ++nx) {
      // A neighboring dark-blue shade is still a visible gradient. Refine it
      // instead of painting the whole area as a single coarse block.
      if (coarseColor[ny * COARSE_W + nx] != center) return true;
    }
  }
  return false;
}
struct PixelJob {
  bool active = false;
  double cr, ci, zr, zi, rr, ii;
  int iteration;
};
static PixelJob pixel;
constexpr unsigned long WORK_US = 2000;

static double pixelScale() { return ldexp(3.5 / W, -zoom); }

static uint16_t rgb(int r, int g, int b) {
  return ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3);
}

static void makePalette() {
  // Vivid indigo, violet, magenta, orange and gold around a black set.
  const uint8_t stops[][3] = {{8, 8, 48}, {35, 35, 205}, {116, 28, 240},
    {235, 35, 143}, {255, 113, 32}, {255, 226, 95}, {40, 166, 245}, {8, 8, 48}};
  for (int i = 0; i < 256; ++i) {
    int pos = i * 7, a = pos / 256, f = pos % 256;
    int r = (stops[a][0] * (256-f) + stops[a+1][0] * f) / 256;
    int g = (stops[a][1] * (256-f) + stops[a+1][1] * f) / 256;
    int b = (stops[a][2] * (256-f) + stops[a+1][2] * f) / 256;
    palette[i] = rgb(r,g,b);
  }
}

static bool sample(double cr, double ci, uint16_t &color, unsigned long started) {
  // Analytic cardioid / period-2 bulb checks avoid iterating the solid interior.
  if (!pixel.active) {
    double shifted = cr - 0.25, q = shifted * shifted + ci * ci;
    if (q * (q + shifted) <= 0.25 * ci * ci ||
        (cr + 1.0) * (cr + 1.0) + ci * ci <= 0.0625) {
      color = 0; return true;
    }
    pixel.active = true;
    pixel.cr = cr; pixel.ci = ci;
    pixel.zr = pixel.zi = pixel.rr = pixel.ii = 0;
    pixel.iteration = 0;
  }
  // Even one expensive boundary pixel can yield to the controls. The preview
  // uses fewer iterations; later passes compute the full detail limit.
  int iterations = block == 8 ? min(limit, 48) : limit;
  while (pixel.rr + pixel.ii <= 4.0 && pixel.iteration < iterations) {
    if ((pixel.iteration & 7) == 0 && (unsigned long)(micros() - started) >= WORK_US)
      return false;
    pixel.zi = 2.0 * pixel.zr * pixel.zi + pixel.ci;
    pixel.zr = pixel.rr - pixel.ii + pixel.cr;
    pixel.rr = pixel.zr * pixel.zr;
    pixel.ii = pixel.zi * pixel.zi;
    ++pixel.iteration;
  }
  pixel.active = false;
  if (pixel.iteration == iterations) { color = 0; return true; }
  // Smooth escape coloring reduces hard iteration bands without using a
  // frame buffer. Only escaped samples need the logarithms.
  double smooth = pixel.iteration + 1.0 - log2(log2(pixel.rr + pixel.ii) * 0.5);
  color = palette[((int)(smooth * 7.0)) & 255];
  return true;
}

static void drawHeader() {
  tft.fillRoundRect(8, 8, 224, 46, 12, ui::CARD);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ui::LABEL, ui::CARD);
  tft.drawString("Mandelbrot", 18, 12, 2);
  char text[48];
  snprintf(text, sizeof(text), "%lux zoom", (unsigned long)(1UL << zoom));
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(UI_CYAN, ui::CARD);
  tft.drawString(text, 220, 12, 2);
  // Coordinates stay in the compact font: they need every digit.
  snprintf(text, sizeof(text), "%.8f %+.8fi", centerRe, centerIm);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(1);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString(text, 18, 37);
}

static void drawStatus() {
  tft.fillRect(16, 281, 208, 15, ui::CARD);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(rendering ? UI_CYAN : ui::SECONDARY, ui::CARD);
  const char *state = rendering ? (block == 8 ? "Preview" : block > 1 ? "Refining" : "Detail") : "Ready";
  tft.drawString(state, 20, 281, 2);
  char text[20];
  snprintf(text, sizeof(text), "%d iterations", limit);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(ui::SECONDARY, ui::CARD);
  tft.drawString(text, 220, 281, 2);
  tft.fillRect(8, 58, 224, 2, ui::GROUP);
  int pass = block == 8 ? 0 : block == 4 ? 1 : block == 2 ? 2 : 3;
  int progress = rendering ? (pass * H + row) * W / (H * 4) : W;
  if (progress) tft.fillRect(8, 58, progress, 2, UI_CYAN);
  lastStatus = millis();
}

static void drawFooter() {
  tft.fillRoundRect(8, 280, 224, 32, 12, ui::CARD);
  ui::hint(304, "D-pad pans, Shoot for options", ui::CARD, ui::LABEL);
  drawStatus();
}

static void beginRender() {
  // Cancel all old refinement, including a pixel interrupted mid-iteration.
  pixel.active = false;
  navigationPending = false;
  rendering = true; block = 8; column = row = 0;
  for (int i = 0; i < COARSE_W * COARSE_H; ++i) coarseColor[i] = 0;
  limit = 96 + zoom * 20;
  drawHeader();
  drawFooter();
}

static void crosshair() {
  int cx = X + W / 2, cy = Y + H / 2;
  tft.drawFastHLine(cx - 7, cy, 5, TFT_WHITE);
  tft.drawFastHLine(cx + 3, cy, 5, TFT_WHITE);
  tft.drawFastVLine(cx, cy - 7, 5, TFT_WHITE);
  tft.drawFastVLine(cx, cy + 3, 5, TFT_WHITE);
}

static void renderSlice() {
  if (!rendering || menu || navigationPending) return;
  unsigned long started = micros();
  double scale = pixelScale();
  // Keep partial scanline state across loops. A difficult boundary pixel
  // cannot force the rest of the image to finish before controls are polled.
  do {
    int bw = min(block, W - column), bh = min(block, H - row);
    uint16_t color;
    bool detailNeeded = true;
    if (block < 8 && zoom <= ADAPTIVE_MAX_ZOOM) {
      int coarseX = column / 8;
      int coarseY = row / 8;
      // Never skip an isolated background block. Its 3x3 coarse neighborhood
      // must also be stable, which preserves thin fractal margins.
      detailNeeded = coarseNeighborhoodNeedsDetail(coarseX, coarseY);
    }
    if (detailNeeded) {
      if (!sample(centerRe + (column + bw * 0.5 - W * 0.5) * scale,
                  centerIm + (H * 0.5 - row - bh * 0.5) * scale, color, started)) return;
      if (block == 8)
        coarseColor[(row / 8) * COARSE_W + (column / 8)] = color;
    } else {
      // Keep the coarse background color visible; skipping refinement must
      // never turn a dark-blue exterior region black.
      color = coarseColor[(row / 8) * COARSE_W + (column / 8)];
    }
    for (int dy = 0; dy < bh; ++dy)
      for (int dx = 0; dx < bw; ++dx) strip[dy * W + column + dx] = color;
    column += bw;
    if (column == W) {
      tft.setSwapBytes(true);
      tft.pushImage(X, Y + row, W, bh, strip);
      row += bh; column = 0;
      if (row == H) {
        if (block == 1) {
          rendering = false;
          crosshair(); drawStatus();
          return;
        }
        block /= 2; row = 0;
        drawStatus();
      }
      // Limit each loop to a single display transfer as well as the CPU budget.
      return;
    }
  } while ((unsigned long)(micros() - started) < WORK_US);
  if (millis() - lastStatus >= 200) drawStatus();
}

static void requireRelease() {
  buttonsRequireRelease();
  inputArmed = false;
  shootHeld = holdSent = false;
  navigationPending = false;
}

static int input() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'w' || c == 'W') return 0;
    if (c == 'd' || c == 'D') return 1;
    if (c == 's' || c == 'S') return 2;
    if (c == 'a' || c == 'A') return 3;
    if (c == 'e' || c == 'E' || c == ' ') return 4;
    if (c == 'q' || c == 'Q') return 5;
  }
  uint8_t held = buttonsHeld();
  if (!inputArmed) {
    if (!held) inputArmed = true;
    return -1;
  }
  unsigned long now = millis();
  int command = buttonsNextPress();
  navigationPending = command >= 0 || (held & 15);
  if (command == 4 && !shootHeld) {
    shootHeld = true;
    holdSent = false;
    pressedAt = now;
  }
  if (shootHeld) {
    if ((held & 16) && !holdSent && now - pressedAt >= 850) {
      holdSent = true;
      return 5;
    }
    if (!(held & 16)) {
      bool shortPress = !holdSent;
      shootHeld = false;
      navigationPending = false;
      if (shortPress) return 4;
    }
  } else if (command >= 0 && command < 4) {
    navigationPending = false;
    return command;
  }
  navigationPending = false;
  return -1;
}

static const ui::List MANDEL_OPTIONS = {32, 130, 176, 24, OPTIONS};
static void drawOption(int i) {
  const char *labels[] = {"Zoom in 2x", "Zoom out 2x", "Reset view", "Back to view", "Main menu"};
  const char *label = i == 0 && zoom == MAX_ZOOM ? "Zoom in (at max)" :
                      i == 1 && zoom == 0 ? "Zoom out (at min)" : labels[i];
  ui::listRow(MANDEL_OPTIONS, i, label, i == option, UI_CYAN);
}

static void openMenu() {
  menu = true; option = 0;
  requireRelease();
  ui::card(18, 76, 204, 186);
  ui::heading(32, 86, "Mandelbrot", "View options");
  for (int i = 0; i < OPTIONS; ++i) drawOption(i);
  tft.fillRoundRect(8, 280, 224, 32, 12, ui::CARD);
  ui::hint(289, "Up/Down choose, Shoot selects", ui::CARD, ui::LABEL);
  ui::hint(304, "Left / Right zoom quickly", ui::CARD);
}

static void changeZoom(int direction) {
  audio::play(direction > 0 ? audio::Cue::ZoomIn : audio::Cue::ZoomOut);
  zoom = constrain(zoom + direction, 0, MAX_ZOOM);
  menu = false;
  requireRelease();
  beginRender();
}
} // namespace mandelApp

void mandelbrotSetup() {
  using namespace mandelApp;
  tft.setRotation(2);
  centerRe = -0.5; centerIm = 0; zoom = 0;
  menu = false;
  requireRelease();
  makePalette();
  tft.fillScreen(UI_BG);
  beginRender();
}

void mandelbrotLoop() {
  using namespace mandelApp;
  audio::pauseMusic(menu);
  int cmd = input();
  if (menu) {
    if (cmd == 0 || cmd == 2) {
      int previous = option;
      option = (option + (cmd == 2 ? 1 : OPTIONS - 1)) % OPTIONS;
      playNavClick();
      drawOption(previous); drawOption(option);
    } else if (cmd == 1 || cmd == 3) changeZoom(cmd == 1 ? 1 : -1);
    else if (cmd == 5) { menu = false; requireRelease(); beginRender(); }
    else if (cmd == 4) {
      if (option == 4) { switchGame(0); return; }
      if (option <= 1) changeZoom(option == 0 ? 1 : -1);
      else {
        if (option == 2) { centerRe = -0.5; centerIm = 0; zoom = 0; audio::play(audio::Cue::Saved); }
        menu = false; requireRelease(); beginRender();
      }
    }
    return;
  }
  if (cmd == 4 || cmd == 5) { audio::play(audio::Cue::Open); openMenu(); return; }
  if (cmd >= 0 && cmd <= 3) {
    playNavClick();
    double shift = pixelScale() * 32;
    // Up moves the center toward positive imaginary values.
    if (cmd == 0) centerIm += shift;
    if (cmd == 2) centerIm -= shift;
    if (cmd == 1) centerRe += shift;
    if (cmd == 3) centerRe -= shift;
    // Keep navigation in a useful neighborhood and labels inside the header.
    centerRe = fmax(-4.0, fmin(4.0, centerRe));
    centerIm = fmax(-4.0, fmin(4.0, centerIm));
    beginRender();
  }
  renderSlice();
}
