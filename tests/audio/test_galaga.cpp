#include "audio_kit.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

static bool silent(const std::vector<int16_t> &s) {
  return std::all_of(s.begin(), s.end(), [](int16_t v){ return v==0; });
}
static void reset() { audio::external(true); audio::external(false); audio::scene(0); }
int main() {
 std::vector<int16_t> out(44100);
  audio::musicVolume = 100;
  std::vector<int16_t> breakout(44100);
  reset(); audio::scene(3); audio::render(breakout.data(), breakout.size(), 0, true);
  reset(); audio::scene(5);
  audio::render(out.data(),out.size(),0,true); assert(!silent(out));
  assert(out != breakout);
  std::vector<int16_t> track(22050 * 16);
  reset(); audio::scene(5); audio::render(track.data(), track.size(), 0, true);
  int peak = 0, jump = 0;
  for (size_t i = 1; i < track.size(); ++i) {
    peak = std::max(peak, std::abs(int(track[i])));
    jump = std::max(jump, std::abs(int(track[i]) - int(track[i-1])));
  }
  assert(peak > 2200 && peak < 11000 && jump < 1500);
  audio::pauseMusic(true);
  audio::render(out.data(),out.size(),0,true); assert(silent(out));
  audio::pauseMusic(false);
  audio::render(out.data(),out.size(),0,true); assert(!silent(out));
  audio::scene(0);
  audio::render(out.data(),out.size(),0,true); assert(silent(out));
}
