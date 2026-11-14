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
  std::vector<int16_t> out(44100), reference(2205), actual(2205);
  audio::effectsVolume=100; audio::musicVolume=100;
  for (int cue=0; cue<=int(audio::Cue::Firework); ++cue) {
    reset(); audio::play(static_cast<audio::Cue>(cue));
    audio::render(out.data(),out.size(),0,true);
    assert(!silent(out));
    assert(std::all_of(out.end()-2205,out.end(),[](int16_t v){return v==0;}));
    for (int16_t v:out) assert(v>=-12000 && v<=12000);
  }
  reset(); audio::play(audio::Cue::Death);
  audio::render(reference.data(),reference.size(),0,true);
  reset(); audio::play(audio::Cue::Death); audio::play(audio::Cue::Move);
  audio::render(actual.data(),actual.size(),0,true);
  assert(actual==reference); // A button click must not interrupt a death cue.
  reset(); audio::play(audio::Cue::Pickup); audio::effectsVolume=0;
  audio::render(out.data(),out.size(),0,true); assert(silent(out));
  audio::effectsVolume=100;
  reset(); audio::scene(1); audio::render(out.data(),out.size(),0,true); assert(!silent(out));
  audio::pauseMusic(true); audio::render(out.data(),out.size(),0,true); assert(silent(out));
  audio::pauseMusic(false); audio::gameMusic=false;
  audio::render(out.data(),out.size(),0,true); assert(silent(out));
  audio::gameMusic=true; audio::external(true); audio::play(audio::Cue::Win);
  audio::render(out.data(),out.size(),0,true); assert(silent(out));
  audio::external(false); audio::render(out.data(),out.size(),440,false); assert(silent(out));
  reset(); audio::render(out.data(),out.size(),440,true); assert(!silent(out));
  std::cout << "All cues, termination, priority, mute, music pause and I2S ownership passed\n";
}
