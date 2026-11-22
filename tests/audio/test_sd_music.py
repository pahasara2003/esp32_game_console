"""Check embedded game tracks with the real Helix decoder (legacy test filename)."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
library = root / '.pio/libdeps/esp32-c3-devkitm-1/ESP8266Audio/src'

import hashlib
import json
import re
import math
import sys
sys.path.insert(0, str(root / 'tools'))
from prepare_game_music import MUSIC_BUDGET

manifest = json.loads((root / 'assets/game_music.json').read_text())
bundle = (root / 'assets/game_music.bin').read_bytes()
assert len(bundle) == manifest['bytes'] <= MUSIC_BUDGET
assert len(manifest['games']) == 12
rows = [tuple(map(int, row)) for row in re.findall(
    r'\{(\d+), (\d+)\}', (root / 'src/game_music_tracks.h').read_text())]
assert len(rows) == 13 and rows[0] == (0, 0)
for index, row in zip(manifest['games'].values(), rows[1:]):
    track = manifest['tracks'][index]
    assert row == (track['offset'], track['size'])
for track in manifest['tracks']:
    data = bundle[track['offset']:track['offset'] + track['size']]
    assert len(data) == track['size']
    assert hashlib.sha256(data).hexdigest() == track['sha256']
    selected_length = track.get('end', track.get('start', 0) + 30) - track.get('start', 0)
    assert selected_length <= track['duration'] < selected_length + .2
for a, b in [('tetris', 'pong'), ('snake', 'tic_tac_toe'),
             ('galaga', 'minesweeper'), ('life', 'mandelbrot')]:
    assert manifest['games'][a] == manifest['games'][b]

test = r"""
#include "audio_kit.h"
#include <cassert>
#include <vector>
#include <algorithm>
namespace audio {
MusicState mode = MusicState::Playing;
int game = 1;
uint32_t generation = 0;
MusicState musicState() { return mode; }
int musicGame() { return game; }
uint32_t musicSession() { return generation; }
}
#include "sd_game_music.cpp"
static bool audible(const std::vector<int16_t> &v) {
  return std::any_of(v.begin(), v.end(), [](int16_t x) { return x != 0; });
}
int main() {
  sd_music::begin();
  std::vector<int16_t> baseline(8000), first(4000), second(4000), out(256);
  assert(sd_music::fill(baseline.data(), baseline.size(), true));
  ++audio::generation;
  assert(sd_music::fill(first.data(), first.size(), true));
  auto position = sd_music::player->position;
  audio::mode = audio::MusicState::Paused;
  assert(!sd_music::fill(out.data(), out.size(), true));
  assert(sd_music::player->position == position);
  audio::mode = audio::MusicState::Playing;
  assert(!sd_music::fill(out.data(), out.size(), false));
  assert(sd_music::player->position == position);
  assert(sd_music::fill(second.data(), second.size(), true));
  assert(std::equal(first.begin(), first.end(), baseline.begin()));
  assert(std::equal(second.begin(), second.end(), baseline.begin() + first.size()));
  ++audio::generation;
  assert(sd_music::fill(first.data(), first.size(), true));
  assert(std::equal(first.begin(), first.end(), baseline.begin()));
  for (int game = 1; game <= 12; ++game) {
    audio::game = game; ++audio::generation;
    bool heard = false;
    // More than two loops per game exercises the original AFK failure case.
    for (int block = 0; block < 22050 * loopSeconds[game] / 256; ++block) {
      assert(sd_music::fill(out.data(), out.size(), true));
      assert(sd_music::player && !sd_music::failed);
      assert(sd_music::player->size == gameMusicTracks[game].size);
      heard |= audible(out);
    }
    assert(heard);
  }
  audio::mode = audio::MusicState::Stopped;
  assert(!sd_music::fill(out.data(), out.size(), true));
  assert(sd_music::player == nullptr);
  audio::mode = audio::MusicState::Playing;
  for (int game : {-1, 0, 13}) {
    audio::game = game;
    assert(!sd_music::fill(out.data(), out.size(), true));
    assert(sd_music::player == nullptr);
  }
  puts("PASS: 12 embedded tracks, multiple loops per game, pause/resume, restart, mute and invalid games");
}
"""
seconds = [0] + [math.ceil(2 * manifest['tracks'][i]['duration']) + 3 for i in manifest['games'].values()]
test = test.replace('int main() {', 'const int loopSeconds[] = {' + ','.join(map(str, seconds)) + '};\nint main() {')
with tempfile.TemporaryDirectory() as work:
    tmp = Path(work)
    (tmp / 'pgmspace.h').write_text('''#pragma once
#define PROGMEM
#define pgm_read_byte(p) (*(const unsigned char *)(p))
#define pgm_read_word(p) (*(const unsigned short *)(p))
''')
    (tmp / 'Arduino.h').write_text(r"""
#pragma once
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <cstring>
using std::min;
struct SerialStub { template<class... T> void printf(const char *, T...) {} };
static SerialStub Serial;
""")
    objects = []
    for source in sorted((library / 'libhelix-mp3').glob('*.c')):
        obj = tmp / (source.stem + '.o')
        subprocess.run(['gcc', '-O2', '-DARDUINO', '-I' + str(tmp), '-c', str(source), '-o', str(obj)], check=True)
        objects.append(str(obj))
    subprocess.run(['ld', '-r', '-b', 'binary', 'assets/game_music.bin', '-o', str(tmp / 'music.o')], cwd=root, check=True)
    (tmp / 'test.cpp').write_text(test)
    subprocess.run(['g++', '-std=c++11', '-O2', '-DARDUINO', '-I' + str(tmp), '-I' + str(root / 'src'),
                    '-I' + str(library), str(tmp / 'test.cpp'), *objects, str(tmp / 'music.o'),
                    '-o', str(tmp / 'test')], check=True)
    subprocess.run([str(tmp / 'test')], check=True)
print('PASS: bundle hashes, selected durations and shared music mappings')
