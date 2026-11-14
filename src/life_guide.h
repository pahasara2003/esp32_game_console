#pragma once

#include "life_model.h"

namespace lifeGuide {
struct Page {
  const char *title;
  const char *category;
  const char *lines[7];
  uint8_t width, height;
  const char *rle;
  uint8_t frames;
};

// Pattern names and canonical seeds follow the LifeWiki / Golly Life lexicon.
// These illustrations have their own state and never seed the live simulation.
static const Page PAGES[] = {
  {"WHAT IS LIFE?", "OVERVIEW", {
    "Conway's Game of Life is a", "cellular automaton from 1970.",
    "No player moves the cells.", "Simple local rules create",
    "complex, changing structures.", "Each tick is one generation.",
    "All cells update together."
  }, 0, 0, nullptr, 0},
  {"THE FOUR RULES", "B3 / S23", {
    "Each cell has eight neighbors.", "1. BIRTH: dead + exactly 3.",
    "2. SURVIVE: alive + 2 or 3.", "3. ISOLATION: alive + 0 or 1",
    "   means the cell dies.", "4. CROWDING: alive + 4 to 8",
    "   means the cell dies."
  }, 0, 0, nullptr, 0},
  {"YOUR RANDOM WORLD", "READING THE SCREEN", {
    "Lime = alive. Dark = dead.", "GEN counts completed updates.",
    "ALIVE is the current population.", "This world has 112 x 96 cells.",
    "Opposite edges are connected:", "going off one side reappears",
    "on the other. No cells age."
  }, 0, 0, nullptr, 0},
  {"BLOCK", "STILL LIFE / 4 CELLS", {
    "Four cells in a solid square.", "Every cell has 3 neighbors.",
    "The shape stays unchanged.", "Look for tiny 2 x 2 islands."
  }, 2, 2, "2o$2o!", 1},
  {"BEEHIVE", "STILL LIFE / 6 CELLS", {
    "Six cells surround a gap.", "The middle row is widest.",
    "It stays in this same shape.", "Look for small hollow rings."
  }, 4, 3, "b2o$o2bo$b2o!", 1},
  {"LOAF", "STILL LIFE / 7 CELLS", {
    "A stable, uneven hollow ring.", "It resembles a tilted beehive.",
    "Its seven cells remain alive.", "Rotations are the same pattern."
  }, 4, 4, "b2o$o2bo$bobo$2bo!", 1},
  {"BOAT", "STILL LIFE / 5 CELLS", {
    "A compact five-cell cluster.", "A solid corner forms its top.",
    "The pointed end gives its name.", "It stays still once isolated."
  }, 3, 3, "2o$obo$bo!", 1},
  {"BLINKER", "OSCILLATOR / PERIOD 2", {
    "A row becomes a column.", "It repeats every 2 generations.",
    "A repeating shape is called", "an oscillator."
  }, 3, 1, "3o!", 2},
  {"TOAD", "OSCILLATOR / PERIOD 2", {
    "Two offset rows of 3 cells.", "The ends fold out and back.",
    "Repeats every 2 generations.", "It oscillates in one place."
  }, 4, 2, "b3o$3o!", 2},
  {"BEACON", "OSCILLATOR / PERIOD 2", {
    "Two blocks touch diagonally.", "The inside corners blink.",
    "It alternates 6 and 8 cells.", "Returns after 2 generations."
  }, 4, 4, "2o$2o$2b2o$2b2o!", 2},
  {"GLIDER", "SPACESHIP / PERIOD 4", {
    "Five cells travel diagonally.", "The shape repeats in 4 ticks,",
    "shifted one cell right/down.", "It can cross the wrapped edges."
  }, 3, 3, "bo$2bo$3o!", 16},
  {"LIGHTWEIGHT SHIP", "SPACESHIP / PERIOD 4", {
    "A lightweight spaceship.", "It travels sideways across",
    "two cells every 4 generations.", "Its direction follows its shape."
  }, 5, 4, "bo2bo$o$o3bo$4o!", 8},
  {"PULSAR", "OSCILLATOR / PERIOD 3", {
    "A large, symmetric oscillator.", "Its four arms pulse together.",
    "Returns after 3 generations.", "This diagram shows the cycle."
  }, 13, 13, "2b3o3b3o2$o4bobo4bo$o4bobo4bo$o4bobo4bo$2b3o3b3o2$"
             "2b3o3b3o$o4bobo4bo$o4bobo4bo$o4bobo4bo2$2b3o3b3o!", 3},
  {"POPULATION CHANGES", "SURVIVAL AND EXTINCTION", {
    "Random worlds can shrink fast.", "Isolation and crowds kill cells.",
    "Survivors may settle or repeat.", "Moving groups can collide.",
    "A still world can stay alive.", "EXTINCT means no cells remain.",
    "New random starts a fresh world."
  }, 0, 0, nullptr, 0},
  {"SPOTTING PATTERNS", "OBSERVATION", {
    "Pause to inspect the world.", "Look for 2 x 2 solid blocks.",
    "Look for lines that blink.", "Gliders move diagonally.",
    "Clusters can hide or collide.", "Named examples are only some",
    "of the shapes that can form."
  }, 0, 0, nullptr, 0},
  {"ABOUT THIS GUIDE", "OFFLINE REFERENCE", {
    "Pattern names follow the", "Life community's conventions.",
    "Golly and LifeWiki document", "these rules and life forms.",
    "Diagrams are enlarged examples.", "They never change your world.",
    "Find them in random play."
  }, 0, 0, nullptr, 0}
};
constexpr int PAGE_COUNT = sizeof(PAGES) / sizeof(PAGES[0]);
constexpr int WIDTH = 20, HEIGHT = 18, COUNT = WIDTH * HEIGHT;

struct Diagram {
  uint8_t cells[COUNT] = {}, next[COUNT] = {};
  uint8_t phase = 0;

  void reset(const Page &page) {
    memset(cells, 0, sizeof(cells));
    phase = 0;
    if (!page.rle) return;
    int left = (WIDTH - page.width) / 2, top = (HEIGHT - page.height) / 2;
    int x = 0, y = 0, number = 0;
    for (const char *p = page.rle; *p && *p != '!'; ++p) {
      if (*p >= '0' && *p <= '9') { number = number * 10 + *p - '0'; continue; }
      int run = number ? number : 1;
      number = 0;
      if (*p == '$') { x = 0; y += run; }
      else if (*p == 'b') x += run;
      else if (*p == 'o') {
        for (int i = 0; i < run; ++i, ++x)
          if (left + x >= 0 && left + x < WIDTH && top + y >= 0 && top + y < HEIGHT)
            cells[(top + y) * WIDTH + left + x] = 1;
      }
    }
  }

  void advance(const Page &page) {
    if (page.frames <= 1) return;
    if (++phase >= page.frames) { reset(page); return; }
    lifeModel::evolve(cells, next, WIDTH, HEIGHT);
    memcpy(cells, next, sizeof(cells));
  }
};
} // namespace lifeGuide
