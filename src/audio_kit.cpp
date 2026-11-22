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
uint32_t musicGeneration = 0;
int lastGameState = -99;
uint32_t legacyPhase = 0;
// Original, sparse loops. A low music level leaves room for feedback sounds.
const Note arcade[] = {{262,150},{392,150},{523,150},{392,150},{330,150},{440,150},{587,150},{0,300}};
const Note blocks[] = {{330,200},{392,200},{440,200},{330,200},{294,200},{349,200},{392,300},{0,300}};
const Note ambient[] = {{131,650},{196,650},{262,650},{0,650},{165,650},{247,650},{330,650},{0,900}};

// Original 116 BPM modern happy synth-pop groove (D - A - Bm7 - Gmaj7):
// warm major chords, bouncy root-octave bass, crisp hi-hat/kick pulse, and
// a bright sunlit marimba/bell pluck melody.
constexpr uint32_t BO_STEP = RATE * 15 / 116; // 16th note at 116 BPM (~2851 samples)
const uint16_t boChords[4][4] = {
  {294, 370, 440, 554}, // Dmaj7  (D4, F#4, A4, C#5)
  {220, 277, 330, 494}, // Aadd9  (A3, C#4, E4, B4)
  {247, 294, 370, 440}, // Bm7    (B3, D4, F#4, A4)
  {196, 247, 294, 370}  // Gmaj7  (G3, B3, D4, F#4)
};
const uint16_t boBass[4] = {147, 110, 123, 98}; // D3, A2, B2, G2
const uint16_t boMelody[64] = {
  // Bar 1 (Dmaj7): sunny syncopated hook
  587, 0, 659, 740, 0, 880, 0, 740,   659, 0, 587, 0, 740, 0, 880, 0,
  // Bar 2 (Aadd9): uplifting bounce
  880, 0, 988, 880, 0, 740, 0, 659,   554, 0, 659, 0, 740, 0, 659, 0,
  // Bar 3 (Bm7): playful response
  587, 0, 659, 740, 0, 880, 0, 988,   880, 0, 740, 0, 659, 0, 587, 0,
  // Bar 4 (Gmaj7): bright turnaround
  784, 0, 740, 0, 659, 0, 740, 880,   0, 988, 0, 880, 740, 0, 659, 0
};
// A separate uplifting Galaga melody over the same warm pop instruments.
const uint16_t galaMelody[64] = {
  740, 0, 880, 0, 988, 0, 880, 740, 659, 0, 740, 0, 880, 0, 0, 0,
  659, 0, 740, 880, 0, 988, 0, 1109, 988, 0, 880, 0, 740, 0, 659, 0,
  740, 0, 880, 0, 988, 0, 1109, 988, 880, 0, 740, 0, 659, 0, 740, 0,
  784, 0, 880, 988, 0, 880, 0, 740, 659, 0, 740, 0, 587, 0, 0, 0
};
const int16_t boSine[64] = {0,3212,6393,9512,12539,15446,18204,20787,23170,25329,27245,28898,30273,31356,32137,32609,32767,32609,32137,31356,30273,28898,27245,25329,23170,20787,18204,15446,12539,9512,6393,3212,0,-3212,-6393,-9512,-12539,-15446,-18204,-20787,-23170,-25329,-27245,-28898,-30273,-31356,-32137,-32609,-32767,-32609,-32137,-31356,-30273,-28898,-27245,-25329,-23170,-20787,-18204,-15446,-12539,-9512,-6393,-3212};
uint32_t boClock = 0, boPhase[9] = {}, boMelodyAge = BO_STEP * 8;
uint16_t boMelodyHz = 587;
int boDuck = 256;
uint32_t pianoPhase[3] = {}, pianoAge = RATE;
uint16_t pianoHz = 0;

int softOscillator(uint32_t &phase, uint16_t hz) {
  phase += uint32_t(hz) * 65536 / RATE;
  unsigned position = phase & 65535;
  unsigned index = position >> 10;
  return boSine[index] + (int(boSine[(index + 1) & 63]) - boSine[index]) * int(position & 1023) / 1024;
}

int pianoSample() {
  if (!pianoHz || pianoAge >= RATE) return 0;
  int attack = min(pianoAge, uint32_t(100));
  int decay = 256 - int(pianoAge * 256 / RATE);
  int wave = softOscillator(pianoPhase[0], pianoHz) / 2 +
             softOscillator(pianoPhase[1], pianoHz * 2) / 4 +
             softOscillator(pianoPhase[2], pianoHz * 3) / 8;
  ++pianoAge;
  return wave * attack / 100 * decay / 256 * decay / 256 * 8 / 7;
}

int popMusicSample() {
  const uint16_t *melody = activeScene == 5 ? galaMelody : boMelody;
  uint32_t step = boClock / BO_STEP;
  uint32_t withinStep = boClock % BO_STEP;
  uint32_t chord = (step / 16) & 3;
  uint32_t withinBar = boClock % (BO_STEP * 16);
  uint32_t beatPos = boClock % (BO_STEP * 4);

  if (withinStep == 0 && melody[step]) {
    boMelodyHz = melody[step];
    boMelodyAge = 0;
  }

  // Warm lush synth pad with subtle sidechain pump on every beat
  int pad = 0;
  for (int i = 0; i < 4; ++i) pad += softOscillator(boPhase[i], boChords[chord][i]);
  int barEdge = min(uint32_t(RATE / 20), min(withinBar, BO_STEP * 16 - withinBar));
  uint32_t pumpRamp = beatPos < 256 ? (256 - beatPos) : min(uint32_t(256), (beatPos - 256) * 256 / (BO_STEP * 3));
  int pump = 175 + pumpRamp * 81 / 256;
  pad = (pad / 6) * barEdge / int(RATE / 20) * pump / 256;

  // Bouncy modern pop bassline (root on beat, octave on off-beat, fifth pickup)
  uint32_t eig = (step >> 1) & 7;
  uint16_t bassHz = boBass[chord];
  if (eig & 1) bassHz *= 2;
  else if (eig == 6) bassHz = bassHz * 3 / 2;
  uint32_t withinEig = boClock % (BO_STEP * 2);
  int bassEdge = min(uint32_t(220), min(withinEig, BO_STEP * 2 - 1 - withinEig));
  int bassDec = max(0, 256 - int(withinEig * 180 / (BO_STEP * 2)));
  int bass = (softOscillator(boPhase[4], bassHz) * 3 / 4 +
              softOscillator(boPhase[5], bassHz * 2) / 4);
  bass = (bass / 4) * bassEdge / 220 * bassDec / 256;

  // Bright happy marimba/bell pluck (fundamental + octave harmonic sparkle)
  int lead = 0;
  if (boMelodyAge < BO_STEP * 5) {
    int atk = min(boMelodyAge, uint32_t(180));
    int dec = max(0, 256 - int(boMelodyAge * 256 / (BO_STEP * 5)));
    int env = (dec * dec) >> 8;
    if (melody[step + 1 <= 63 ? step + 1 : 0] && withinStep > BO_STEP - 180)
      env = env * int(BO_STEP - withinStep) / 180;
    int osc = softOscillator(boPhase[6], boMelodyHz) * 5 / 6 +
              softOscillator(boPhase[7], boMelodyHz * 2) / 7;
    lead = (osc / 5) * atk / 180 * env / 256;
  }
  ++boMelodyAge;

  // Soft modern four-on-the-floor kick thump for happy danceable groove
  int kick = 0;
  if (beatPos < 1100) {
    if (beatPos == 0) boPhase[8] = 0;
    uint16_t kHz = 48 + (1100 - beatPos) * 72 / 1100;
    int kAtk = min(beatPos, uint32_t(80));
    int kEnv = (1100 - beatPos) * 256 / 1100;
    kick = (softOscillator(boPhase[8], kHz) / 5) * kAtk / 80 * kEnv / 256;
  }

  if (++boClock >= BO_STEP * 64) boClock = 0;
  int target = effect.count ? 120 : 256;
  if (boDuck < target) ++boDuck;
  if (boDuck > target) boDuck = max(target, boDuck - 2);
  return (pad + bass + lead + kick) * boDuck / 256;
}

// IMA ADPCM keeps the orchestral recording in flash and decodes it incrementally.
extern const uint8_t puzzleRecordingStart[] asm("_binary_assets_puzzle_mozart_wav_start");
extern const uint8_t puzzleRecordingEnd[] asm("_binary_assets_puzzle_mozart_wav_end");
const int16_t imaSteps[89] = {
  7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,
  88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,
  544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,
  2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,
  10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767
};
const int8_t imaIndex[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};
const uint8_t *puzzleData = nullptr;
uint32_t puzzleDataBytes = 0, puzzleSamples = 0, puzzlePosition = 0;
int puzzlePredictor = 0, puzzleStepIndex = 0, puzzleDuck = 256;

uint32_t readPuzzleLE32(const uint8_t *p) {
  return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

// Mono 22050 Hz, 1024-byte IMA blocks, decoded directly from embedded flash.
extern const uint8_t gameOverStart[] asm("_binary_assets_sounds_endings_game_over_embedded_wav_start");
extern const uint8_t gameOverEnd[] asm("_binary_assets_sounds_endings_game_over_embedded_wav_end");
extern const uint8_t loseStart[] asm("_binary_assets_sounds_endings_lose_embedded_wav_start");
extern const uint8_t loseEnd[] asm("_binary_assets_sounds_endings_lose_embedded_wav_end");
extern const uint8_t winStart[] asm("_binary_assets_sounds_endings_win_embedded_wav_start");
extern const uint8_t winEnd[] asm("_binary_assets_sounds_endings_win_embedded_wav_end");
struct Ending {
  const uint8_t *data = nullptr;
  uint32_t samples = 0, position = 0;
  int predictor = 0, stepIndex = 0;
};
Ending ending;

void beginEnding(const uint8_t *start, const uint8_t *end) {
  ending = Ending{};
  size_t size = end - start;
  uint32_t bytes = 0;
  for (size_t pos = 12; pos + 8 <= size;) {
    const uint8_t *chunk = start + pos;
    uint32_t length = readPuzzleLE32(chunk + 4);
    if (length > size - pos - 8) break;
    if (!memcmp(chunk, "fact", 4) && length >= 4) ending.samples = readPuzzleLE32(chunk + 8);
    if (!memcmp(chunk, "data", 4)) { ending.data = chunk + 8; bytes = length; }
    pos += 8 + length + (length & 1);
  }
  if (!ending.data || bytes % 1024 || ending.samples > bytes / 1024 * 2041)
    ending = Ending{};
}

int endingSample() {
  uint32_t block = ending.position / 2041;
  uint32_t sample = ending.position % 2041;
  const uint8_t *data = ending.data + block * 1024;
  if (sample == 0) {
    ending.predictor = int16_t(uint16_t(data[0]) | uint16_t(data[1]) << 8);
    ending.stepIndex = min(88, int(data[2]));
  } else {
    uint32_t nibble = sample - 1;
    int code = (data[4 + nibble / 2] >> ((nibble & 1) * 4)) & 15;
    int step = imaSteps[ending.stepIndex];
    int diff = step >> 3;
    if (code & 1) diff += step >> 2;
    if (code & 2) diff += step >> 1;
    if (code & 4) diff += step;
    ending.predictor = constrain(ending.predictor + ((code & 8) ? -diff : diff), -32768, 32767);
    ending.stepIndex = constrain(ending.stepIndex + imaIndex[code], 0, 88);
  }
  ++ending.position;
  return ending.predictor;
}

void resetPuzzleMusic() {
  puzzlePosition = 0; puzzleDuck = 256;
  puzzleData = nullptr; puzzleDataBytes = puzzleSamples = 0;
  size_t size = puzzleRecordingEnd - puzzleRecordingStart;
  for (size_t pos = 12; pos + 8 <= size;) {
    const uint8_t *chunk = puzzleRecordingStart + pos;
    uint32_t length = readPuzzleLE32(chunk + 4);
    if (length > size - pos - 8) break;
    if (!memcmp(chunk, "fact", 4) && length >= 4) puzzleSamples = readPuzzleLE32(chunk + 8);
    if (!memcmp(chunk, "data", 4)) { puzzleData = chunk + 8; puzzleDataBytes = length; }
    pos += 8 + length + (length & 1);
  }
  if (!puzzleData || puzzleDataBytes % 1024 || puzzleSamples > puzzleDataBytes / 1024 * 2041)
    puzzleSamples = 0;
}

int puzzleMusicSample() {
  if (!puzzleSamples) return 0;
  if (puzzlePosition >= puzzleSamples) puzzlePosition = 0;
  uint32_t block = puzzlePosition / 2041;
  uint32_t sample = puzzlePosition % 2041;
  const uint8_t *data = puzzleData + block * 1024;
  if (sample == 0) {
    puzzlePredictor = int16_t(uint16_t(data[0]) | uint16_t(data[1]) << 8);
    puzzleStepIndex = min(88, int(data[2]));
  } else {
    uint32_t nibble = sample - 1;
    int code = (data[4 + nibble / 2] >> ((nibble & 1) * 4)) & 15;
    int step = imaSteps[puzzleStepIndex];
    int diff = step >> 3;
    if (code & 1) diff += step >> 2;
    if (code & 2) diff += step >> 1;
    if (code & 4) diff += step;
    puzzlePredictor = constrain(puzzlePredictor + ((code & 8) ? -diff : diff), -32768, 32767);
    puzzleStepIndex = constrain(puzzleStepIndex + imaIndex[code], 0, 88);
  }
  ++puzzlePosition;
  int target = effect.count ? 160 : 256;
  if (puzzleDuck < target) ++puzzleDuck;
  if (puzzleDuck > target) puzzleDuck = max(target, puzzleDuck - 2);
  return puzzlePredictor * puzzleDuck / 256;
}

void resetBreakoutMusic() {
  boClock = 0;
  memset(boPhase, 0, sizeof(boPhase));
  boMelodyAge = BO_STEP * 8;
  boMelodyHz = 587;
  boDuck = 256;
}

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
  else if (activeScene == 4) notes = arcade;
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
  if (cue == Cue::GameOver || cue == Cue::Lose || cue == Cue::Win) {
    portENTER_CRITICAL(&guard);
    if (!held) {
      effect = Voice{};
      if (cue == Cue::GameOver) beginEnding(gameOverStart, gameOverEnd);
      else if (cue == Cue::Lose) beginEnding(loseStart, loseEnd);
      else beginEnding(winStart, winEnd);
    }
    portEXIT_CRITICAL(&guard);
    return;
  }
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
    case Cue::GameOver: case Cue::Lose: case Cue::Win: break;
    case Cue::Draw: add(440,80); add(440,80); add(330,130); next.priority=5; break;
    case Cue::Record: add(784,65); add(988,65); add(1175,65); add(1568,140); add(0,50); add(1568,180); next.priority=7; break;
    case Cue::Paddle: add(659,16); add(988,22); break;
    case Cue::Wall: add(523,14); add(784,18); break;
    case Cue::Brick: {
      uint16_t f = pitch ? pitch : 1320;
      add(f, 18); add(min(uint16_t(3136), uint16_t(f * 5 / 4)), 24);
      next.priority=2; break;
    }
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
    case Cue::Countdown: add(880,45); break;
    case Cue::Pop: add(1800,8); add(240,24); next.priority=2; break;
    case Cue::Firework: add(330,35); add(660,35); add(1320,35); add(1760,65); next.priority=3; break;
  }
  portENTER_CRITICAL(&guard);
  if (!held && ending.position >= ending.samples && (!effect.count || next.priority >= effect.priority)) {
    effect = next; beginNote(effect);
  }
  portEXIT_CRITICAL(&guard);
}
void stopEnding() {
  portENTER_CRITICAL(&guard);
  ending = Ending{};
  portEXIT_CRITICAL(&guard);
}
void scene(int game) {
  portENTER_CRITICAL(&guard);
  if (activeScene != game) {
    ending = Ending{};
    activeScene=game; effect=Voice{}; background=Voice{}; paused=false; lastGameState=-99;
    resetBreakoutMusic();
    resetPuzzleMusic();
  }
  portEXIT_CRITICAL(&guard);
}
void pauseMusic(bool value) { paused=value; }
void gameState(int state) {
  pauseMusic(state != 0);
  if (state != lastGameState) {
    if (state == 0) stopEnding();
    if (state == 1) play(Cue::Pause);
    else if (state == 0 && lastGameState == 1) play(Cue::Resume);
    lastGameState=state;
  }
}
void external(bool value) {
  portENTER_CRITICAL(&guard);
  ending = Ending{};
  held=value; effect=Voice{}; background=Voice{};
  resetBreakoutMusic();
  resetPuzzleMusic();
  portEXIT_CRITICAL(&guard);
}
void birthdayNote(uint16_t frequency) {
  portENTER_CRITICAL(&guard);
  pianoHz = frequency;
  pianoAge = 0;
  memset(pianoPhase, 0, sizeof(pianoPhase));
  portEXIT_CRITICAL(&guard);
}
void birthdayStop() {
  portENTER_CRITICAL(&guard);
  pianoHz = 0;
  portEXIT_CRITICAL(&guard);
}
int musicGame() {
  portENTER_CRITICAL(&guard);
  int game = activeScene;
  portEXIT_CRITICAL(&guard);
  return game;
}
uint32_t musicSession() {
  portENTER_CRITICAL(&guard);
  uint32_t session = musicGeneration;
  portEXIT_CRITICAL(&guard);
  return session;
}
void restartMusic() {
  portENTER_CRITICAL(&guard);
  ++musicGeneration; paused = true;
  portEXIT_CRITICAL(&guard);
}
MusicState musicState() {
  portENTER_CRITICAL(&guard);
  MusicState state = !activeScene || held ? MusicState::Stopped :
    (paused || !gameMusic || ending.position < ending.samples ? MusicState::Paused : MusicState::Playing);
  portEXIT_CRITICAL(&guard);
  return state;
}
void render(int16_t *out, size_t count, uint32_t legacy, bool enabled, const int16_t *sdMusic) {
  portENTER_CRITICAL(&guard);
  for (size_t i=0; i<count; ++i) {
    if (held || !enabled) { ending=Ending{}; effect=Voice{}; out[i]=0; continue; }
    if (ending.position < ending.samples) {
      // Recorded results own the mix, so music, clicks and record jingles cannot mask them.
      out[i] = int16_t(endingSample() / 3 * effectsVolume / 100);
      continue;
    }
    int fx = sample(effect);
    int music = 0;
    if (gameMusic && !paused && !legacy) {
      if (sdMusic) music = sdMusic[i];
      else if (activeScene == 6) music = puzzleMusicSample();
      else if (activeScene == 3 || activeScene == 5) music = popMusicSample();
      else { loopMusic(); music=sample(background); }
    }
    legacyPhase += legacy * 65536 / RATE;
    int tone = legacy ? ((legacyPhase & 32768) ? 3000 : -3000) : 0;
    int value = (fx / 8 + tone) * effectsVolume / 100;
    // Duck the background under effects and legacy birthday/jukebox notes.
    value += (music / (sdMusic ? (effect.count ? 4 : 2) : (activeScene == 6 ? 2 : ((activeScene == 3 || activeScene == 5) ? 4 : (effect.count ? 32 : 16))))) * musicVolume / 100;
    if (pianoHz) {
      value += pianoSample();
      out[i] = int16_t(constrain(value, -32768, 32767));
    } else {
      out[i] = int16_t(constrain(value, -12000, 12000));
    }
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
