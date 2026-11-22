#include "ui_kit.h"

extern TFT_eSPI tft;

namespace ui {

uint16_t inkOn(uint16_t fill) {
  int r = ((fill >> 11) & 31) * 255 / 31;
  int g = ((fill >> 5) & 63) * 255 / 63;
  int b = (fill & 31) * 255 / 31;
  return (r * 299 + g * 587 + b * 114) / 1000 > 150 ? INK_DARK : LABEL;
}

void card(int x, int y, int w, int h) { tft.fillRoundRect(x, y, w, h, 16, CARD); }

void heading(int x, int y, const char *caption, const char *title, uint16_t bg) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(SECONDARY, bg);
  tft.drawString(caption, x, y, 2);
  tft.setTextColor(LABEL, bg);
  tft.drawString(title, x, y + 18, 4);
}

static void chevron(int x, int cy, uint16_t color) {
  for (int i = 0; i < 2; ++i) {
    tft.drawLine(x + i, cy - 4, x + 4 + i, cy, color);
    tft.drawLine(x + 4 + i, cy, x + i, cy + 4, color);
  }
}

// Paint one row's background, honouring the group's rounded ends.
static void rowBackground(const List &l, int i, uint16_t fill) {
  int y = l.y + i * l.rowH;
  bool first = i == 0, last = i == l.count - 1;
  const int r = 10;
  if (first && last) { tft.fillRoundRect(l.x, y, l.w, l.rowH, r, fill); return; }
  if (first) {
    tft.fillRoundRect(l.x, y, l.w, l.rowH, r, fill);
    tft.fillRect(l.x, y + l.rowH - r, l.w, r, fill);
  } else if (last) {
    tft.fillRoundRect(l.x, y, l.w, l.rowH, r, fill);
    tft.fillRect(l.x, y, l.w, r, fill);
  } else {
    tft.fillRect(l.x, y, l.w, l.rowH, fill);
  }
}

static void separator(const List &l, int i, bool hidden) {
  if (i == 0) return;
  int y = l.y + i * l.rowH;
  tft.drawFastHLine(l.x + 12, y, l.w - 12, hidden ? GROUP : SEPARATOR);
}

void listRow(const List &l, int i, const char *label, bool selected, uint16_t accent) {
  uint16_t fill = selected ? accent : GROUP;
  rowBackground(l, i, fill);
  // Separators vanish next to the highlighted row, like iOS.
  if (!selected) separator(l, i, false);
  int cy = l.y + i * l.rowH + l.rowH / 2;
  uint16_t ink = selected ? inkOn(accent) : LABEL;
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(ink, fill);
  tft.drawString(label, l.x + 12, cy + 1, 2);
  chevron(l.x + l.w - 16, cy, selected ? ink : SECONDARY);
}

void listAll(const List &l, const char *const *labels, int selected, uint16_t accent) {
  for (int i = 0; i < l.count; ++i) listRow(l, i, labels[i], i == selected, accent);
  // Redraw separators that a selected neighbour may have hidden.
  for (int i = 1; i < l.count; ++i)
    if (i != selected && i - 1 != selected) separator(l, i, false);
}

void valueRow(const List &l, int i, const char *label, const char *value, bool selected,
              uint16_t accent, uint16_t valueColor) {
  uint16_t fill = selected ? accent : GROUP;
  rowBackground(l, i, fill);
  if (!selected) separator(l, i, false);
  int cy = l.y + i * l.rowH + l.rowH / 2 + 1;
  uint16_t ink = selected ? inkOn(accent) : LABEL;
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(ink, fill);
  tft.drawString(label, l.x + 12, cy, 2);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(selected ? ink : valueColor, fill);
  tft.drawString(value, l.x + l.w - 12, cy, 2);
}

void infoRow(const List &l, int i, const char *key, const char *value) {
  rowBackground(l, i, GROUP);
  separator(l, i, false);
  int cy = l.y + i * l.rowH + l.rowH / 2 + 1;
  tft.setTextColor(SECONDARY, GROUP);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(key, l.x + 12, cy, 2);
  tft.setTextColor(LABEL, GROUP);
  tft.setTextDatum(MR_DATUM);
  tft.drawString(value, l.x + l.w - 12, cy, 2);
}

void stat(int x, int y, const char *label, const String &value, uint16_t valueColor,
          uint16_t bg) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(SECONDARY, bg);
  tft.drawString(label, x, y, 2);
  tft.setTextColor(valueColor, bg);
  tft.drawString(value, x, y + 17, 4);
}

void pill(int x, int y, int w, int h, const char *label, uint16_t fill) {
  tft.fillRoundRect(x, y, w, h, h / 2, fill);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(inkOn(fill), fill);
  tft.drawString(label, x + w / 2, y + h / 2 + 1, 2);
}

static uint16_t mix(uint16_t a, uint16_t b, int amount) {
  int r = (((a >> 11) & 31) * (255 - amount) + ((b >> 11) & 31) * amount) / 255;
  int g = (((a >> 5) & 63) * (255 - amount) + ((b >> 5) & 63) * amount) / 255;
  int bl = ((a & 31) * (255 - amount) + (b & 31) * amount) / 255;
  return (r << 11) | (g << 5) | bl;
}

void gradientPill(int x, int y, int w, int h, const char *label, uint16_t from, uint16_t to) {
  int r = h / 2;
  for (int i = 0; i < w; ++i) {
    int dx = i < r ? r - i : i > w - 1 - r ? i - (w - 1 - r) : 0;
    int half = dx ? (int)sqrtf((float)(r * r - dx * dx)) : r;
    if (half > 0) tft.drawFastVLine(x + i, y + r - half, 2 * half, mix(from, to, i * 255 / (w - 1)));
  }
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(inkOn(mix(from, to, 128)));  // transparent text over the gradient
  tft.drawString(label, x + w / 2, y + h / 2 + 1, 2);
}

// Second gradient stop for an accent: shifted toward magenta for a lively sweep.
static uint16_t accentTo(uint16_t accent) { return mix(accent, 0xF81F, 110); }

void chip(int x, int y, const char *label, uint16_t fill) {
  int w = tft.textWidth(label, 2) + 16;
  pill(x, y, w, 20, label, fill);
}

void hint(int y, const char *text, uint16_t bg, uint16_t color) {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(color, bg);
  tft.drawString(text, 120, y, 2);
}

// ---- Standard screens ---------------------------------------------------
// The pause sheet stays inside (18,58)-(222,258): the resume countdown only
// repaints that area of the game scene.
static List pauseList(int count) {
  int rowH = count > 3 ? 26 : 30;
  return {36, 132, 168, rowH, count};
}

void pauseSheet(const char *game, const char *const *items, int count, int selected,
                uint16_t accent) {
  List l = pauseList(count);
  int bottom = l.y + l.rowH * count + 14;
  card(24, 72, 192, bottom - 72);
  heading(40, 84, game, "Paused");
  listAll(l, items, selected, accent);
}

void pauseItem(const char *const *items, int count, int item, int selected, uint16_t accent) {
  List l = pauseList(count);
  listRow(l, item, items[item], item == selected, accent);
  // Neighbour separators can change when the highlight moves.
  for (int i = max(1, item); i <= min(count - 1, item + 1); ++i)
    if (i != selected && i - 1 != selected)
      tft.drawFastHLine(l.x + 12, l.y + i * l.rowH, l.w - 12, SEPARATOR);
}

void gameOverSheet(const char *game, int score, int best, uint16_t accent, const char *title,
                   const char *unit, const char *distance, const char *distanceLabel) {
  tft.setTextSize(1);
  card(24, 62, 192, distance ? 214 : 210);
  heading(40, 76, game, title);
  // Keep the badge below the title, independent of either text's width.
  if (score > 0 && score >= best) chip(40, 126, "New best", accent);
  String s = String(score), b = String(best);
  if (unit) { s += unit; b += unit; }
  const List scores = {32, distance ? 148 : 154, 176, distance ? 20 : 24, distance ? 3 : 2};
  infoRow(scores, 0, "Score", s.c_str());
  infoRow(scores, 1, "Best", b.c_str());
  if (distance) infoRow(scores, 2, distanceLabel, distance);
  gradientPill(40, distance ? 220 : 210, 160, 30, "Play again", accent, accentTo(accent));
  hint(distance ? 262 : 254, "Up for home screen", CARD);
}

void startCard(int y, const char *name, const char *tagline, const Control *controls,
               int controlCount, int best, uint16_t accent, const char *unit,
               const char *button) {
  List l = {32, y + 72, 176, 26, controlCount};
  int statY = l.y + l.rowH * controlCount + 12;
  int bottom = statY + 92;
  card(18, y, 204, bottom - y);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(LABEL, CARD);
  tft.drawString(name, 32, y + 16, 4);
  tft.setTextColor(SECONDARY, CARD);
  tft.drawString(tagline, 32, y + 44, 2);
  for (int i = 0; i < controlCount; ++i) infoRow(l, i, controls[i].key, controls[i].action);
  String b = best > 0 ? String(best) : String("-");
  if (unit && best > 0) b += unit;
  stat(32, statY, "Best", b, best > 0 ? accent : SECONDARY);
  gradientPill(32, statY + 50, 176, 30, button, accent, accentTo(accent));
}

}  // namespace ui
