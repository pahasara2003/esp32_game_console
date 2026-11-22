"""Exercise Rally's actual end-of-race routing for all six finishing places."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'src/rally.cpp').read_text()
result = source[source.index('void gameOver() {'):source.index('\n}\nvoid rallySetup()')]
ranking = source[source.index('int place() {'):source.index('int metersToNextLap()')]
test = r'''
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
struct String : std::string {
  String(float value, int decimals) {
    char text[32]; snprintf(text, sizeof(text), "%.*f", decimals, double(value)); assign(text);
  }
  String(const std::string &text) : std::string(text) {}
};
constexpr int RIVALS = 5, ACCENT = 0;
struct Rival { float z; } rivals[RIVALS];
float distance = 100;
int state, best, points, medal, renders, sheets, released, saved;
namespace audio {
enum class Cue { Win, GameOver };
Cue played;
void play(Cue cue) { played = cue; }
}
struct Preferences {
  void begin(const char *, bool) {}
  void putInt(const char *, int value) { saved = value; }
  void end() {}
};
int score() { return points; }
void render() { ++renders; }
void drawPodium(int rank, int value) { medal = rank; assert(value == points); }
void buttonsRequireRelease() { ++released; }
namespace ui {
void gameOverSheet(const char *title, int value, int record, int, const char *subtitle, const char *unit, const char *travelled) {
  ++sheets;
  assert(unit == nullptr && strcmp(travelled, "0.01 km") == 0);
  assert(strncmp(title, "Place ", 6) == 0 && strcmp(subtitle, "Time up") == 0);
  assert(value == points && record == best);
}
}
''' + ranking + result + r'''
int main() {
  for (int rank = 1; rank <= 6; ++rank) {
    for (int previousBest : {50, 200}) {
      for (int i = 0; i < RIVALS; ++i) rivals[i].z = i < rank - 1 ? 110 : 90;
      state = 1; best = previousBest; points = 100;
      medal = renders = sheets = released = saved = 0;
      gameOver();
      assert(state == 3 && released == 1);
      assert(best == (previousBest < points ? points : previousBest));
      assert(saved == (previousBest < points ? points : 0));
      assert(medal == (rank <= 3 ? rank : 0));
      assert(sheets == (rank <= 3 ? 0 : 1) && renders == sheets);
      assert(audio::played == (rank <= 3 ? audio::Cue::Win : audio::Cue::GameOver));
    }
  }
  puts("PASS: six finishing places, medal/audio routing, best scores and result controls");
}
'''
with tempfile.TemporaryDirectory() as work:
    tmp = Path(work)
    (tmp / 'test.cpp').write_text('#include <initializer_list>\n' + test)
    subprocess.run(['g++', '-std=c++11', str(tmp / 'test.cpp'), '-o', str(tmp / 'test')], check=True)
    subprocess.run([str(tmp / 'test')], check=True)
