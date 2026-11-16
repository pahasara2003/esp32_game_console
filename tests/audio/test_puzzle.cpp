#include "audio_kit.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

static bool silent(const std::vector<int16_t> &samples) {
  return std::all_of(samples.begin(), samples.end(), [](int16_t value) { return value == 0; });
}
static void reset() { audio::external(true); audio::external(false); audio::scene(0); }
int main() {
  audio::musicVolume = 100;
  audio::effectsVolume = 100;
  audio::gameMusic = true;
  std::vector<int16_t> track(22050 * 32), silence(4096), first(4096), resumed(4096);
  reset(); audio::scene(6);
  audio::render(track.data(), track.size(), 0, true);
  int peak = 0, jump = 0, clipped = 0;
  double energy = 0;
  for (size_t i = 1; i < track.size(); ++i) {
    peak = std::max(peak, std::abs(int(track[i])));
    jump = std::max(jump, std::abs(int(track[i]) - int(track[i-1])));
    clipped += std::abs(int(track[i])) >= 12000;
    energy += double(track[i]) * track[i];
  }
  double rms = std::sqrt(energy / track.size());
  std::cout << "Puzzle music peak " << peak << ", RMS " << rms << ", max step " << jump << '\n';
  assert(rms > 1000 && peak <= 12000 && jump < 10000 && clipped == 0);
  reset(); audio::scene(6); audio::render(first.data(), first.size(), 0, true);
  reset(); audio::scene(6);
  audio::render(resumed.data(), 1000, 0, true);
  audio::pauseMusic(true); audio::render(silence.data(), silence.size(), 0, true); assert(silent(silence));
  audio::pauseMusic(false); audio::render(resumed.data()+1000, resumed.size()-1000, 0, true);
  assert(first == resumed);
  audio::musicVolume = 0; audio::render(silence.data(), silence.size(), 0, true); assert(silent(silence));
  audio::musicVolume = 100; audio::play(audio::Cue::Slide);
  audio::render(resumed.data(), resumed.size(), 0, true); assert(!silent(resumed));
  std::cout << "Puzzle loop, volume, pause and sound effect mixing passed\n";
}
