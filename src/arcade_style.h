#pragma once
#include <Arduino.h>

// Breakout's twilight gradient, glass grid and fixed star field.
namespace arcadeStyle {
inline uint16_t mix(uint16_t a, uint16_t b, int amount) {
  int r = (((a >> 11) & 31) * (255 - amount) + ((b >> 11) & 31) * amount) / 255;
  int g = (((a >> 5) & 63) * (255 - amount) + ((b >> 5) & 63) * amount) / 255;
  int bl = ((a & 31) * (255 - amount) + (b & 31) * amount) / 255;
  return (r << 11) | (g << 5) | bl;
}
inline uint16_t fade(uint16_t color, int amount) {
  return (((color >> 11) * amount / 255) << 11) |
         ((((color >> 5) & 63) * amount / 255) << 5) |
         ((color & 31) * amount / 255);
}
inline uint16_t backdrop(int x, int y) {
  static uint16_t rows[243], gridRows[243];
  static bool starRows[243], ready = false;
  if (!ready) {
    for (int row = 0; row < 243; ++row) {
      rows[row] = row < 121 ? mix(0x1045, 0x0887, row * 255 / 121)
                            : mix(0x0887, 0x0928, (row - 121) * 255 / 121);
      gridRows[row] = mix(rows[row], 0x3A52, 46);
    }
    for (int i = 0; i < 22; ++i) {
      int row = 6 + (i * 53 + i * i * 7) % 223;
      starRows[row] = true;
      if (i % 3 == 2) starRows[row - 1] = starRows[row + 1] = true;
    }
    ready = true;
  }
  int row = y - 48;
  if (row < 0 || row > 242) return UI_BG;
  uint16_t bg = row % 12 == 0 || (x > 0 && x % 12 == 0) ? gridRows[row] : rows[row];
  if (!starRows[row]) return bg;
  for (int i = 0; i < 22; ++i) {
    int sx = 8 + (i * 37 + (i & 3) * 19) % 224;
    int sy = 54 + (i * 53 + i * i * 7) % 223;
    if (x == sx && y == sy) return i % 3 == 2 ? 0x9E7F : (i % 3 == 1 ? 0x5B94 : 0x39E9);
    if (i % 3 == 2 && ((x == sx && abs(y - sy) == 1) || (y == sy && abs(x - sx) == 1))) return 0x39E9;
  }
  return bg;
}
// Draw the same background by rows and grid points instead of evaluating every pixel.
template<class Surface>
inline void drawBackdrop(Surface &canvas, int top, int bottom) {
  top = max(48, top); bottom = min(290, bottom);
  for (int y = top; y <= bottom; ++y) {
    canvas.drawFastHLine(0, y, 240, backdrop(1, y));
    if ((y - 48) % 12 != 0)
      for (int x = 12; x < 240; x += 12) canvas.drawPixel(x, y, backdrop(x, y));
  }
  for (int i = 0; i < 22; ++i) {
    int x = 8 + (i * 37 + (i & 3) * 19) % 224;
    int y = 54 + (i * 53 + i * i * 7) % 223;
    if (y >= top && y <= bottom) canvas.drawPixel(x, y, backdrop(x, y));
    if (i % 3 == 2) {
      if (y >= top && y <= bottom) {
        canvas.drawPixel(x - 1, y, backdrop(x - 1, y));
        canvas.drawPixel(x + 1, y, backdrop(x + 1, y));
      }
      if (y - 1 >= top && y - 1 <= bottom) canvas.drawPixel(x, y - 1, backdrop(x, y - 1));
      if (y + 1 >= top && y + 1 <= bottom) canvas.drawPixel(x, y + 1, backdrop(x, y + 1));
    }
  }
}

}
