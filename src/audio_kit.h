#pragma once
#include <stddef.h>
#include <stdint.h>

// Synthesized effects and flash recordings, mixed by the I2S task at 22050 Hz.
namespace audio {
enum class Cue : uint8_t {
  Move, Open, Back, Saved, Error, Rotate, Lock, Clear, FourLines,
  Level, Pickup, Milestone, Death, Win, Draw, Record, Paddle, Wall,
  Brick, Powerup, Jump, Laser, Hit, Dive, Wave, Life, Slide,
  PlayerMove, CpuMove, Reveal, Flag, Unflag, ZoomIn, ZoomOut,
  Pause, Resume, Sparkle, Pop, Firework, Countdown, GameOver, Lose
};
extern uint8_t effectsVolume, musicVolume;
extern bool gameMusic;
enum class MusicState { Stopped, Paused, Playing };
MusicState musicState();
int musicGame();
uint32_t musicSession();
void restartMusic();
void play(Cue cue, uint16_t pitch = 0);
void stopEnding();
void scene(int game);
void pauseMusic(bool paused);
void gameState(int state);
void external(bool active);
void birthdayNote(uint16_t frequency);
void birthdayStop();
void render(int16_t *samples, size_t count, uint32_t legacyTone, bool enabled,
            const int16_t *sdMusic = nullptr);
void load();
void save();
float mediaGain(float maximum = 0.8f);
}
