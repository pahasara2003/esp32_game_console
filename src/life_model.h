#pragma once

#include <stdint.h>
#include <string.h>

namespace lifeModel {
constexpr int WIDTH = 28;
constexpr int HEIGHT = 24;
constexpr int COUNT = WIDTH * HEIGHT;
constexpr int MAX_WIDTH = 112;
constexpr int MAX_HEIGHT = 96;
constexpr int CAPACITY = MAX_WIDTH * MAX_HEIGHT;

struct StepResult { uint16_t population; bool changed; };

// The simulation and the guide's small illustrations use the exact same rules.
inline StepResult evolve(const uint8_t *cells, uint8_t *next, int width, int height) {
  StepResult result = {0, false};
  for (int y = 0; y < height; ++y) {
    const uint8_t *above = cells + (y ? y - 1 : height - 1) * width;
    const uint8_t *row = cells + y * width;
    const uint8_t *below = cells + (y + 1 == height ? 0 : y + 1) * width;
    for (int x = 0; x < width; ++x) {
      int left = x ? x - 1 : width - 1;
      int right = x + 1 == width ? 0 : x + 1;
      int neighbors = above[left] + above[x] + above[right] + row[left] + row[right] +
                      below[left] + below[x] + below[right];
      int i = y * width + x;
      next[i] = neighbors == 3 || (cells[i] && neighbors == 2);
      result.population += next[i];
      result.changed |= next[i] != cells[i];
    }
  }
  return result;
}

// Conway B3/S23, with a toroidal board: cells across each edge are neighbors.
struct Board {
  uint8_t cells[CAPACITY] = {};
  uint8_t next[CAPACITY] = {};
  int width = WIDTH, height = HEIGHT;
  uint32_t generation = 0;
  uint16_t population = 0;

  void clear() {
    memset(cells, 0, sizeof(cells));
    memset(next, 0, sizeof(next));
    generation = 0;
    population = 0;
  }

  bool resize(int w, int h) {
    if (w < 3 || h < 3 || w > MAX_WIDTH || h > MAX_HEIGHT) return false;
    width = w;
    height = h;
    clear();
    return true;
  }

  int count() const { return width * height; }

  void set(int x, int y, bool alive) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    int index = y * width + x;
    if (cells[index] == alive) return;
    if (alive) ++population;
    else --population;
    cells[index] = alive;
  }

  bool step() {
    StepResult result = evolve(cells, next, width, height);
    memcpy(cells, next, count());
    population = result.population;
    ++generation;
    return result.changed;
  }
};
} // namespace lifeModel
