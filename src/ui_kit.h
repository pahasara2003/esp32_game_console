#pragma once
// Shared phone-style UI pieces for every game: one look for start cards,
// pause sheets, game-over sheets and footer hints. Sentence-case labels,
// one card colour, no decorative stripes or stacked borders.
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace ui {

constexpr uint16_t CARD      = 0x18E3;  // elevated dark surface
constexpr uint16_t GROUP     = 0x2945;  // inset list group
constexpr uint16_t SEPARATOR = 0x39C7;
constexpr uint16_t LABEL     = 0xFFFF;
constexpr uint16_t SECONDARY = 0x9CF3;  // muted grey text
constexpr uint16_t INK_DARK  = 0x0841;  // text on bright accent fills

// Text on an accent fill: dark on light accents, white on dark ones.
uint16_t inkOn(uint16_t fill);

void card(int x, int y, int w, int h);

// Small caption above a large title ("Snake" / "Game over").
void heading(int x, int y, const char *caption, const char *title, uint16_t bg = CARD);

// Rows of an inset grouped list. Selected row is filled with the accent.
struct List {
  int x, y, w, rowH;
  int count;
};
void listRow(const List &l, int i, const char *label, bool selected, uint16_t accent);
void listAll(const List &l, const char *const *labels, int selected, uint16_t accent);

// Settings-style row: label left, value right; accent fill when selected.
void valueRow(const List &l, int i, const char *label, const char *value, bool selected,
              uint16_t accent, uint16_t valueColor = SECONDARY);

// Key/value row for a controls list ("D-pad" ... "Steer").
void infoRow(const List &l, int i, const char *key, const char *value);

// Label above a big number.
void stat(int x, int y, const char *label, const String &value, uint16_t valueColor = LABEL,
          uint16_t bg = CARD);

// Rounded pill button with centred label.
void pill(int x, int y, int w, int h, const char *label, uint16_t fill);

// Primary button: capsule with a left-to-right gradient from `from` to `to`.
void gradientPill(int x, int y, int w, int h, const char *label, uint16_t from, uint16_t to);

// Small accent chip, e.g. "New best".
void chip(int x, int y, const char *label, uint16_t fill);

// One quiet line of help text, centred.
void hint(int y, const char *text, uint16_t bg, uint16_t color = SECONDARY);

// ---- Standard screens ---------------------------------------------------
// Pause sheet: card at (24,72) with "Paused" and a list of actions.
void pauseSheet(const char *game, const char *const *items, int count, int selected,
                uint16_t accent);
void pauseItem(const char *const *items, int count, int item, int selected, uint16_t accent);

// Game-over sheet with score/best and a "Play again" button.
void gameOverSheet(const char *game, int score, int best, uint16_t accent,
                   const char *title = "Game over", const char *unit = nullptr);

// Start card: name, tagline, controls list, best score, start button.
struct Control { const char *key, *action; };
void startCard(int y, const char *name, const char *tagline, const Control *controls,
               int controlCount, int best, uint16_t accent, const char *unit = nullptr,
               const char *button = "Press any button");

}  // namespace ui
