"""Run Rally's actual movement and rival updates on the host, without a display."""
from pathlib import Path
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[2] / "src/rally.cpp").read_text()


def section(start, end):
    return source[source.index(start):source.index(end)]


test = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
using std::min;
using std::max;
constexpr float TWO_PI = 6.28318530718f;
template<class T> T constrain(T n, T a, T b) { return min(b, max(a, n)); }
long random(long a, long b) {
  static uint32_t seed = 42;
  seed = seed * 1664525u + 1013904223u;
  return a + seed % (b - a);
}
constexpr float length = 24000.f;
''' + section("float position =", "void flash(") + section("float center(", "// Fixed north-up") + r'''
''' + section("int metersToNextLap()", "void placeCoin(") + r'''
void placeCoin(Coin &, float) {}
bool inRiver(float, float) { return false; }
''' + section("void resetRace()", "void startRace()") + r'''
void drive(uint8_t held, float dt) {
''' + section("  int steer =", "  if (position >= length)") + r'''
}
void updateRivals(float dt) {
''' + section("  // Skills are rolled", "  // Rivals keep their own distance") + r'''
}
int main() {
  position = 0; assert(metersToNextLap() == 2400);
  position = length - 10; assert(metersToNextLap() == 1);
  position = length - .5f; assert(metersToNextLap() == 1);
  position = length; assert(metersToNextLap() == 0);
  for (int checkpoint = 0; checkpoint <= 16; ++checkpoint)
    assert(isLapLine(checkpoint * 3000.f) == (checkpoint % 8 == 0));
  resetRace();
  for (int i = 0; i < 200; ++i) {
    position = 0; lateral = 0;
    drive(0, .02f);
  }
  assert(fabsf(speed - PLAYER_TOP_SPEED) < .1f);
  // The player can catch the fastest leader; trailing rivals can catch up.
  assert(PLAYER_TOP_SPEED > 690.f);
  assert(fabsf(PLAYER_TOP_SPEED * .3f - 235.f) < .01f);
  for (float angle : {0.f, -.4f, .4f}) {
    resetRace(); position = distance = 1800.f; heading = angle; speed = 190.f;
    lateral = 500.f;
    float x0 = center(position) + lateral, z0 = distance;
    for (int i = 0; i < 300; ++i) drive(0, .02f);
    assert(heading == angle);
    float dx = center(position) + lateral - x0;
    assert(fabsf(dx - tanf(angle) * (distance - z0)) < .1f);
  }
  resetRace(); drive(2, .1f); assert(heading > 0);
  resetRace(); drive(8, .1f); assert(heading < 0);
  bool won[RIVALS] = {};
  for (int race = 0; race < 100; ++race) {
    resetRace();
    for (const Rival &r : rivals) {
      assert(r.pace >= 540 && r.pace <= 690);
      assert(r.acceleration >= 240 && r.acceleration <= 380);
      assert(r.cornerPenalty >= 80 && r.cornerPenalty <= 220);
    }
    for (int tick = 0; tick < 1200; ++tick) {
      distance = tick * 75.f;
      updateRivals(.1f);
      for (const Rival &r : rivals) assert(r.speed >= 0 && r.speed <= 830.01f);
    }
    int leader = 0;
    for (int i = 1; i < RIVALS; ++i)
      if (rivals[i].z > rivals[leader].z) leader = i;
    won[leader] = true;
  }
  for (bool colorWon : won) assert(colorWon);
  std::cout << "PASS: passing speed, straight coasting, steering, and varied rival leaders\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    path = Path(tmp)
    (path / "test.cpp").write_text(test)
    subprocess.run(["g++", "-std=c++11", str(path / "test.cpp"), "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
