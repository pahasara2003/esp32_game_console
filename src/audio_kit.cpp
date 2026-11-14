#include "audio_kit.h"
#include <Arduino.h>
#include <Preferences.h>

namespace audio {
uint8_t effectsVolume = 70, musicVolume = 80;
bool gameMusic = true;
namespace {
constexpr uint32_t RATE = 22050;
struct Note { uint16_t hz, ms; };
struct Voice {
  Note notes[8] = {};
  uint8_t count = 0, index = 0, priority = 0;
  uint32_t left = 0, length = 0, phase = 0;
};
Voice effect, background;
portMUX_TYPE guard = portMUX_INITIALIZER_UNLOCKED;
bool held = false, paused = false;
int activeScene = 0;
int lastGameState = -99;
uint32_t legacyPhase = 0;
// Original, sparse loops. A low music level leaves room for feedback sounds.
const Note calm[] = {{262,280},{330,280},{392,420},{0,420},{330,280},{294,280},{262,560},{0,560}};
const Note arcade[] = {{262,150},{392,150},{523,150},{392,150},{330,150},{440,150},{587,150},{0,300}};
const Note blocks[] = {{330,200},{392,200},{440,200},{330,200},{294,200},{349,200},{392,300},{0,300}};
const Note bounce[] = {{262,180},{0,180},{392,180},{330,180},{0,180},{440,180},{392,180},{0,360}};
const Note space[] = {{220,180},{330,180},{440,180},{659,180},{587,180},{440,180},{330,180},{0,360}};
const Note ambient[] = {{131,650},{196,650},{262,650},{0,650},{165,650},{247,650},{330,650},{0,900}};

void beginNote(Voice &v) {
  v.length = uint32_t(v.notes[v.index].ms) * RATE / 1000;
  v.left = v.length;
  v.phase = 0;
}
int sample(Voice &v) {
  if (!v.count) return 0;
  if (!v.left) {
    if (++v.index >= v.count) { v.count = 0; return 0; }
    beginNote(v);
  }
  uint16_t hz = v.notes[v.index].hz;
  v.phase += uint32_t(hz) * 65536 / RATE;
  int phase = v.phase & 65535;
  int triangle = phase < 32768 ? phase * 2 - 32768 : 98303 - phase * 2;
  // 4 ms attack/release avoids the clicks of abrupt square waves.
  uint32_t edge = min(uint32_t(88), min(v.left, v.length - v.left));
  --v.left;
  return hz ? triangle * int(edge) / 88 : 0;
}
void loopMusic() {
  if (background.count || !activeScene) return;
  const Note *notes = nullptr;
  if (activeScene == 1) notes = blocks;
  else if (activeScene == 3) notes = bounce;
  else if (activeScene == 4) notes = arcade;
  else if (activeScene == 5) notes = space;
  else if (activeScene == 6) notes = calm;
  else if (activeScene == 10 || activeScene == 11) notes = ambient;
  if (!notes) return;
  memcpy(background.notes, notes, sizeof(background.notes));
  if (activeScene == 11) {
    for (auto &note:background.notes) { note.hz/=2; note.ms*=2; }
  }
  background.count = 8; background.index = 0; beginNote(background);
}
}

void play(Cue cue, uint16_t pitch) {
  Voice next;
  next.priority = 1;
  auto add = [&](uint16_t hz, uint16_t ms) { next.notes[next.count++] = {hz, ms}; };
  switch (cue) {
    case Cue::Move: add(1200,12); break;
    case Cue::Open: add(660,35); add(990,55); break;
    case Cue::Back: add(880,30); add(587,40); break;
    case Cue::Saved: add(784,50); add(1047,50); add(1319,90); next.priority=3; break;
    case Cue::Error: add(180,70); add(0,30); add(180,70); next.priority=4; break;
    case Cue::Rotate: add(740,14); break;
    case Cue::Lock: add(330,16); add(165,24); break;
    case Cue::Clear: add(660,45); add(880,45); add(1320,80); next.priority=3; break;
    case Cue::FourLines: add(523,55); add(659,55); add(784,55); add(1047,160); next.priority=4; break;
    case Cue::Level: case Cue::Milestone: case Cue::Life:
      add(784,65); add(988,65); add(1175,65); add(1568,140); next.priority=5; break;
    case Cue::Pickup: add(1175,25); add(1760,45); next.priority=2; break;
    case Cue::Death: add(440,80); add(330,90); add(220,120); add(110,140); next.priority=6; break;
    case Cue::Win: add(523,90); add(659,90); add(784,90); add(1047,220); next.priority=6; break;
    case Cue::Draw: add(440,80); add(440,80); add(330,130); next.priority=5; break;
    case Cue::Record: add(784,65); add(988,65); add(1175,65); add(1568,140); add(0,50); add(1568,180); next.priority=7; break;
    case Cue::Paddle: add(880,20); break;
    case Cue::Wall: add(440,15); break;
    case Cue::Brick: add(pitch ? pitch : 1320,25); next.priority=2; break;
    case Cue::Powerup: case Cue::Sparkle: add(1047,35); add(1568,35); add(2093,60); next.priority=3; break;
    case Cue::Jump: add(659,20); add(880,30); next.priority=2; break;
    case Cue::Laser: add(2093,12); add(1568,18); break;
    case Cue::Hit: add(988,20); add(659,35); next.priority=3; break;
    case Cue::Dive: add(1047,30); add(784,30); add(523,45); next.priority=2; break;
    case Cue::Wave: add(330,70); add(0,40); add(440,70); add(0,40); add(660,120); next.priority=4; break;
    case Cue::Slide: add(660,12); add(550,18); break;
    case Cue::PlayerMove: add(784,35); break;
    case Cue::CpuMove: add(523,35); break;
    case Cue::Reveal: add(990,16); break;
    case Cue::Flag: add(660,25); add(990,30); break;
    case Cue::Unflag: add(990,25); add(660,30); break;
    case Cue::ZoomIn: add(330,25); add(440,25); add(660,40); break;
    case Cue::ZoomOut: add(660,25); add(440,25); add(330,40); break;
    case Cue::Pause: add(660,35); add(440,50); next.priority=3; break;
    case Cue::Resume: add(440,35); add(660,50); next.priority=3; break;
    case Cue::Pop: add(1800,8); add(240,24); next.priority=2; break;
    case Cue::Firework: add(330,35); add(660,35); add(1320,35); add(1760,65); next.priority=3; break;
  }
  portENTER_CRITICAL(&guard);
  if (!held && (!effect.count || next.priority >= effect.priority)) {
    effect = next; beginNote(effect);
  }
  portEXIT_CRITICAL(&guard);
}
void scene(int game) {
  portENTER_CRITICAL(&guard);
  if (activeScene != game) {
    activeScene=game; effect=Voice{}; background=Voice{}; paused=false; lastGameState=-99;
  }
  portEXIT_CRITICAL(&guard);
}
void pauseMusic(bool value) { paused=value; }
void gameState(int state) {
  pauseMusic(state != 0);
  if (state != lastGameState) {
    if (state == 1) play(Cue::Pause);
    else if (state == 0 && lastGameState == 1) play(Cue::Resume);
    lastGameState=state;
  }
}
void external(bool value) {
  portENTER_CRITICAL(&guard);
  held=value; effect=Voice{}; background=Voice{};
  portEXIT_CRITICAL(&guard);
}
void render(int16_t *out, size_t count, uint32_t legacy, bool enabled) {
  portENTER_CRITICAL(&guard);
  for (size_t i=0; i<count; ++i) {
    if (held || !enabled) { effect=Voice{}; out[i]=0; continue; }
    int fx = sample(effect);
    int music = 0;
    if (gameMusic && !paused && !legacy) { loopMusic(); music=sample(background); }
    legacyPhase += legacy * 65536 / RATE;
    int tone = legacy ? ((legacyPhase & 32768) ? 3000 : -3000) : 0;
    int value = (fx / 8 + tone) * effectsVolume / 100;
    // Duck the background under effects and legacy birthday/jukebox notes.
    value += (music / (effect.count ? 64 : 32)) * musicVolume / 100;
    out[i] = int16_t(constrain(value, -12000, 12000));
  }
  portEXIT_CRITICAL(&guard);
}
void load() {
  Preferences p; p.begin("console", true);
  effectsVolume=min(uint8_t(100),p.getUChar("fxvol",70));
  musicVolume=min(uint8_t(100),p.getUChar("musicvol",80));
  gameMusic=p.getBool("gamemusic",true); p.end();
}
void save() {
  Preferences p; p.begin("console", false);
  p.putUChar("fxvol",effectsVolume); p.putUChar("musicvol",musicVolume);
  p.putBool("gamemusic",gameMusic); p.end();
}
float mediaGain(float maximum) { return maximum * musicVolume / 100.0f; }
}
