"""Check Life's buffered statistics drawing: python3 tests/life/test_stats.py."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / "src/life.cpp").read_text()
draw_stats = source[source.index("static void drawStats() {"):source.index("\nstatic void drawHeader()")]
test = r'''
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
struct Draw { std::string text; int x, y; };
bool allocationWorks = true;
int pushes = 0;
struct TFT_eSPI {
  int clears = 0;
  std::vector<Draw> strings;
  virtual void fillRect(int, int, int, int, uint16_t) { ++clears; }
  void setTextDatum(int) {}
  void setTextColor(uint16_t, uint16_t) {}
  void drawString(const char *s, int x, int y, int) { strings.push_back({s, x, y}); }
};
TFT_eSPI tft;
struct TFT_eSprite : TFT_eSPI {
  TFT_eSprite(TFT_eSPI *) {}
  void *createSprite(int w, int h) { assert(w == 208 && h == 18); return allocationWorks ? this : nullptr; }
  void pushSprite(int x, int y) {
    assert(x == 16 && y == 31 && clears == 1 && strings.size() == 2);
    assert(strings[0].text == "Generation 123" && strings[0].x == 2 && strings[0].y == 1);
    assert(strings[1].text == "9 alive" && strings[1].x == 204 && strings[1].y == 1);
    ++pushes;
  }
};
struct { uint32_t generation = 123; unsigned population = 9; } board;
namespace ui { const uint16_t CARD = 1, SECONDARY = 2; }
const uint16_t LIVE = 3;
const int TL_DATUM = 0, TR_DATUM = 1;
unsigned long lastStats = 0;
unsigned long millis() { return 500; }
''' + draw_stats + r'''
int main() {
  drawStats();
  assert(pushes == 1 && tft.clears == 0 && tft.strings.empty() && lastStats == 500);
  allocationWorks = false;
  drawStats();
  assert(pushes == 1 && tft.clears == 1 && tft.strings.size() == 2);
  assert(tft.strings[0].text == "Generation 123" && tft.strings[0].x == 18 && tft.strings[0].y == 32);
  assert(tft.strings[1].text == "9 alive" && tft.strings[1].x == 220 && tft.strings[1].y == 32);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    path = Path(tmp)
    (path / "test.cpp").write_text(test)
    subprocess.run(["g++", "-std=c++11", str(path / "test.cpp"), "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
print("PASS: buffered statistics and allocation fallback")
