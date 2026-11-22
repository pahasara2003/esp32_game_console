#pragma once
#include "run_background_240x320.h"

// Decode only the requested row of each blending frame; no full-frame RAM buffer.
static uint16_t runBackgroundPixel(uint8_t frame, int x, int y, int slot) {
  struct Row {
    int key = -1;
    uint16_t pixels[240];
  };
  static Row cache[2];
  Row &row = cache[slot];
  int key = frame * 320 + y;
  if (row.key != key) {
    const uint8_t *cursor = run_background_data + pgm_read_dword(&run_background_offsets[key]);
    int position = 0;
    while (position < 240) {
      uint8_t token = pgm_read_byte(cursor++);
      if (token & 0x80) {
        int length = (token & 0x7f) + 3;
        int distance = pgm_read_byte(cursor++);
        for (int i = 0; i < length; ++i, ++position)
          row.pixels[position] = row.pixels[position - distance];
      } else {
        for (int i = 0; i <= token; ++i) {
          uint16_t value = pgm_read_byte(cursor++);
          value |= uint16_t(pgm_read_byte(cursor++)) << 8;
          row.pixels[position++] = value;
        }
      }
    }
    row.key = key;
  }
  return row.pixels[x];
}
