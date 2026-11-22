#include "sd_game_music.h"
#include "audio_kit.h"
#include <Arduino.h>
#include "game_music_tracks.h"
#include <libhelix-mp3/mp3dec.h>
#include <new>
#include <string.h>

namespace sd_music {
namespace {
// Owned only by the existing I2S task; game tracks read flash, never SD/SPI.
struct Player {
  const uint8_t *data = nullptr;
  uint32_t size = 0, position = 0;
  HMP3Decoder decoder = nullptr;
  uint8_t input[4096];
  int16_t pcm[2304];
  int bytes = 0, frames = 0, channels = 2, rate = 22050;
  uint32_t phase = 0;
  ~Player() { if (decoder) MP3FreeDecoder(decoder); }

  bool open(int game) {
    if (decoder) MP3FreeDecoder(decoder);
    decoder = nullptr;
    bytes = frames = 0; phase = 0;
    data = gameMusicData + gameMusicTracks[game].offset;
    size = gameMusicTracks[game].size;
    position = 0;
    if (!size) return false;
    decoder = MP3InitDecoder();
    if (!decoder) return false;
    return true;
  }

  bool decode() {
    // Bound work on corrupt/non-MP3 files and let the normal mixer keep running.
    for (int attempt = 0; attempt < 8; ++attempt) {
      int read = min(uint32_t(sizeof(input) - bytes), size - position);
      memcpy(input + bytes, data + position, read);
      position += read;
      if (read > 0) bytes += read;
      if (bytes < 4 && read <= 0) return false;
      int sync = MP3FindSyncWord(input, bytes);
      if (sync < 0) {
        if (!read) return false;
        input[0] = input[bytes - 1]; bytes = 1;
        continue;
      }
      if (sync) { bytes -= sync; memmove(input, input + sync, bytes); }
      MP3FrameInfo info;
      if (MP3GetNextFrameInfo(decoder, &info, input) != 0 || info.layer != 3 ||
          info.samprate < 8000 || info.samprate > 48000 ||
          (info.nChans != 1 && info.nChans != 2)) {
        --bytes; memmove(input, input + 1, bytes); continue;
      }
      unsigned char *cursor = input;
      int remaining = bytes;
      int status = MP3Decode(decoder, &cursor, &remaining, pcm, 0);
      if (status == ERR_MP3_INDATA_UNDERFLOW) {
        if (!read || bytes == sizeof(input)) return false;
        continue;
      }
      if (remaining < 0 || remaining > bytes) return false;
      int consumed = bytes - remaining;
      if (!consumed) consumed = 1;
      bytes -= consumed; memmove(input, input + consumed, bytes);
      if (status != ERR_MP3_NONE) continue;
      MP3GetLastFrameInfo(decoder, &info);
      if (info.outputSamps <= 0 || info.outputSamps > 2304) return false;
      channels = info.nChans; rate = info.samprate;
      frames = info.outputSamps / channels;
      return true;
    }
    return false;
  }

  bool sample(int16_t &out) {
    if (!frames && !decode()) return false;
    unsigned index = phase / 22050;
    if (index >= unsigned(frames)) {
      phase -= frames * 22050;
      frames = 0;
      if (!decode()) return false;
      index = phase / 22050;
    }
    int value = pcm[index * channels];
    if (channels == 2) value = (value + pcm[index * 2 + 1]) / 2;
    out = int16_t(value);
    phase += rate;
    return true;
  }
};
Player *player = nullptr;
bool failed = false;
int selectedGame = 0;
uint32_t selectedSession = 0;
}

void begin() {
  Serial.printf("[Game music] Embedded 30-second loops ready\n");
}
void stop() {
  delete player; player = nullptr; failed = false;
}
bool fill(int16_t *samples, size_t count, bool enabled) {
  audio::MusicState state = audio::musicState();
  int game = audio::musicGame();
  uint32_t session = audio::musicSession();
  if (game != selectedGame || session != selectedSession) {
    stop(); selectedGame = game; selectedSession = session;
  }
  if (state == audio::MusicState::Stopped) { stop(); return false; }
  if (state != audio::MusicState::Playing || !enabled || failed || game <= 0 || game >= int(sizeof(gameMusicTracks) / sizeof(*gameMusicTracks))) return false;
  if (!player) {
    player = new (std::nothrow) Player;
    if (!player || !player->open(game)) { stop(); failed = true; return false; }
  }
  memset(samples, 0, count * sizeof(*samples));
  for (size_t i = 0; i < count; ++i) {
    if (!player->sample(samples[i])) {
      // Loop this game's embedded track; decoder failure keeps the mixer alive.
      if (!player->open(game) || !player->sample(samples[i])) {
        stop(); failed = true; return false;
      }
    }
  }
  return audio::musicGame() == game && audio::musicSession() == session &&
         audio::musicState() == audio::MusicState::Playing;
}
}
