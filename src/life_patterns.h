#pragma once

#include <stdint.h>

namespace lifePatterns {
enum Id {
  GLIDER, BLINKERS, PULSAR, R_PENTOMINO, RANDOM,
  BEACON, TOAD, LWSS, PENTADECATHLON, GOSPER_GUN, ACORN, DIEHARD,
  GLIDER_FLEET, RANDOM_SPARSE, RANDOM_DENSE
};

struct Preset {
  const char *name;
  uint8_t cellSize;
  uint8_t width, height;
  uint8_t density;
  const char *rle;
};

// Standard seed orientations in RLE. Sources:
// https://conwaylife.com/patterns/ and the corresponding LifeWiki entries.
static const Preset PRESETS[] = {
  {"GLIDER",          8, 3, 3, 0, "bo$2bo$3o!"},
  {"BLINKERS",        8, 0, 0, 0, nullptr},
  {"PULSAR",          8, 0, 0, 0, nullptr},
  {"R-PENTOMINO",     4, 3, 3, 0, "b2o$2o$bo!"},
  {"RANDOM",          2, 0, 0, 30, nullptr},
  {"BEACON",          8, 4, 4, 0, "2o$2o$2b2o$2b2o!"},
  {"TOAD",            8, 4, 2, 0, "b3o$3o!"},
  {"LWSS",            8, 5, 4, 0, "bo2bo$o$o3bo$4o!"},
  {"PENTADECATHLON",  8, 10, 3, 0, "2bo4bo$2ob4ob2o$2bo4bo!"},
  {"GOSPER GUN",      2, 36, 9, 0,
    "24bo$22bobo$12b2o6b2o12b2o$11bo3bo4b2o12b2o$2o8bo5bo3b2o$"
    "2o8bo3bob2o4bobo$10bo5bo7bo$11bo3bo$12b2o!"},
  {"ACORN",           2, 7, 3, 0, "bo$3bo$2o2b3o!"},
  {"DIEHARD",         4, 8, 3, 0, "6bo$2o$bo3b3o!"},
  {"GLIDER FLEET",    8, 0, 0, 0, nullptr},
  {"RANDOM SPARSE",   2, 0, 0, 18, nullptr},
  {"RANDOM DENSE",    2, 0, 0, 45, nullptr}
};
constexpr int COUNT = sizeof(PRESETS) / sizeof(PRESETS[0]);
} // namespace lifePatterns
