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
  std::vector<int16_t> out(22050 * 12), reference(2205), actual(2205);
  audio::effectsVolume=100; audio::musicVolume=100;
  for (int cue=0; cue<=int(audio::Cue::Lose); ++cue) {
    reset(); audio::play(static_cast<audio::Cue>(cue));
    audio::render(out.data(),out.size(),0,true);
    assert(!silent(out));
    assert(std::all_of(out.end()-2205,out.end(),[](int16_t v){return v==0;}));
    for (int16_t v:out) assert(v>=-12000 && v<=12000);
  }
  // SD music uses the same volume/mute path and cannot play through pauses.
  std::vector<int16_t> sdTrack(out.size(), 8000);
  reset(); assert(audio::musicState() == audio::MusicState::Stopped);
  audio::scene(2); assert(audio::musicGame() == 2);
  assert(audio::musicState() == audio::MusicState::Playing);
  audio::render(out.data(),out.size(),0,true,sdTrack.data());
  assert(std::all_of(out.begin(),out.end(),[](int16_t v){return v==4000;}));
  audio::pauseMusic(true); assert(audio::musicState() == audio::MusicState::Paused);
  audio::render(out.data(),out.size(),0,true,sdTrack.data()); assert(silent(out));
  uint32_t session = audio::musicSession(); audio::restartMusic();
  assert(audio::musicSession() == session + 1 && audio::musicState() == audio::MusicState::Paused);
  audio::gameState(0); audio::musicVolume=0;
  audio::render(out.data(),out.size(),0,true,sdTrack.data()); assert(silent(out));
  audio::musicVolume=100;
  audio::render(out.data(),out.size(),0,false,sdTrack.data()); assert(silent(out));
  audio::play(audio::Cue::Win); assert(audio::musicState() == audio::MusicState::Paused);
  audio::external(true); assert(audio::musicState() == audio::MusicState::Stopped);
  // Recorded results resist high-score/menu cues and ignore music volume/pause.
  for (auto cue : {audio::Cue::GameOver, audio::Cue::Lose, audio::Cue::Win}) {
    reset(); audio::play(cue);
    audio::render(reference.data(),reference.size(),0,true);
    reset(); audio::play(cue); audio::play(audio::Cue::Record); audio::play(audio::Cue::Move);
    audio::musicVolume=0; audio::pauseMusic(true);
    audio::render(actual.data(),actual.size(),0,true); assert(actual==reference);
    audio::musicVolume=100;
    audio::stopEnding(); audio::render(out.data(),out.size(),0,true); assert(silent(out));
    reset(); audio::play(cue); audio::effectsVolume=0;
    audio::render(out.data(),out.size(),0,true); assert(silent(out));
    audio::effectsVolume=100;
    reset(); audio::gameState(2); audio::play(cue); audio::gameState(0);
    audio::render(out.data(),out.size(),0,true); assert(silent(out));
    reset(); audio::play(cue); audio::scene(1); audio::pauseMusic(true);
    audio::render(out.data(),out.size(),0,true); assert(silent(out));
    reset(); audio::play(cue); audio::render(out.data(),out.size(),0,false);
    audio::render(out.data(),out.size(),0,true); assert(silent(out));
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
  reset(); audio::birthdayNote(440);
  audio::render(out.data(),out.size(),0,true);
  assert(!silent(out));
  assert(std::all_of(out.begin()+22050,out.end(),[](int16_t v){return v==0;}));
  int pianoPeak = 0;
  for (int16_t v:out) pianoPeak = std::max(pianoPeak, std::abs(int(v)));
  assert(pianoPeak > 25000 && pianoPeak < 32767);
  audio::musicVolume=0; audio::birthdayNote(440);
  audio::render(out.data(),out.size(),0,true); assert(!silent(out));
  audio::musicVolume=100;
  audio::birthdayNote(523); audio::birthdayStop();
  audio::render(out.data(),out.size(),0,true); assert(silent(out));
  audio::birthdayNote(523); audio::render(out.data(),out.size(),0,false); assert(silent(out));
  audio::birthdayStop();
  // Exercise two complete Breakout loops, including all chord/loop boundaries.
  reset(); audio::scene(3);
  std::vector<int16_t> calmTrack(22050 * 32);
  audio::render(calmTrack.data(),calmTrack.size(),0,true);
  int peak = 0, step = 0;
  for (size_t i=1; i<calmTrack.size(); ++i) {
    peak = std::max(peak, std::abs(int(calmTrack[i])));
    step = std::max(step, std::abs(int(calmTrack[i])-calmTrack[i-1]));
  }
  assert(peak > 2200 && peak < 11000); // More presence than the old 1024-peak loop.
  assert(step < 1500); // Soft waveform and envelopes at note/chord boundaries.
  audio::pauseMusic(true); audio::render(out.data(),out.size(),0,true); assert(silent(out));
  audio::pauseMusic(false); audio::musicVolume=0;
  audio::render(out.data(),out.size(),0,true); assert(silent(out));
  audio::musicVolume=100;
  audio::external(true); audio::render(out.data(),out.size(),0,true); assert(silent(out));
  audio::external(false);
  // Callback buffer sizes must not change the track or pause position.
  reset(); audio::scene(3); audio::render(reference.data(),reference.size(),0,true);
  reset(); audio::scene(3);
  audio::render(actual.data(),1000,0,true);
  audio::pauseMusic(true); audio::render(out.data(),out.size(),0,true); assert(silent(out));
  audio::pauseMusic(false);
  audio::render(actual.data()+1000,actual.size()-1000,0,true);
  assert(actual==reference);
  std::cout << "Breakout levels, envelopes, looping, pause and callback continuity passed\n";
  std::cout << "All cues, termination, priority, mute, music pause and I2S ownership passed\n";
}
