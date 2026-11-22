"""Check embedded orchestral ADPCM decoding against FFmpeg and the audio mixer."""
from array import array
from pathlib import Path
import subprocess
import tempfile
import struct

root = Path(__file__).resolve().parents[2]
emitter = r'''
#include "audio_kit.h"
#include <cstdio>
#include <vector>
int main() {
  std::vector<int16_t> samples(441000);
  audio::musicVolume = 100; audio::scene(6);
  audio::render(samples.data(), samples.size(), 0, true);
  return fwrite(samples.data(), sizeof(int16_t), samples.size(), stdout) != samples.size();
}
'''
with tempfile.TemporaryDirectory() as work:
    tmp = Path(work)
    (tmp / 'emitter.cpp').write_text(emitter)
    obj = tmp / 'puzzle.o'
    assets = ['assets/puzzle_mozart.wav'] + ['assets/sounds/endings/' + name + '_embedded.wav' for name in ('game_over', 'lose', 'win')]
    subprocess.run(['ld', '-r', '-b', 'binary', *assets, '-o', str(obj)], cwd=root, check=True)
    def build(source, name):
        subprocess.run(['g++', '-std=c++11', '-Itests/audio', '-Isrc', 'src/audio_kit.cpp',
                        str(source), str(obj), '-o', str(tmp / name)], cwd=root, check=True)
    build(tmp / 'emitter.cpp', 'emit')
    actual = array('h', subprocess.check_output([str(tmp / 'emit')]))
    reference = array('h', subprocess.check_output(['ffmpeg', '-v', 'error', '-i',
                      str(root / 'assets/puzzle_mozart.wav'), '-f', 's16le', '-']))
    assert len(actual) == 441000 and len(reference) >= len(actual)
    assert all(a == int(b / 2) for a, b in zip(actual, reference))
    # Every recorded result must match FFmpeg sample-for-sample, across blocks and callbacks.
    for name, cue in (('game_over', 'GameOver'), ('lose', 'Lose'), ('win', 'Win')):
        (tmp / 'ending.cpp').write_text(r'''
#include "audio_kit.h"
#include <cstdio>
#include <vector>
int main() {
  std::vector<int16_t> samples(22050 * 12);
  audio::effectsVolume = 100;
  audio::play(audio::Cue::CUE);
  for (size_t i = 0; i < samples.size(); i += 257)
    audio::render(samples.data() + i, std::min(size_t(257), samples.size() - i), 0, true);
  return fwrite(samples.data(), sizeof(int16_t), samples.size(), stdout) != samples.size();
}
'''.replace('CUE', cue))
        build(tmp / 'ending.cpp', 'ending')
        actual = array('h', subprocess.check_output([str(tmp / 'ending')]))
        reference = array('h', subprocess.check_output(['ffmpeg', '-v', 'error', '-i',
                          str(root / ('assets/sounds/endings/' + name + '_embedded.wav')), '-f', 's16le', '-']))
        wav = (root / ('assets/sounds/endings/' + name + '_embedded.wav')).read_bytes()
        sample_count = struct.unpack_from('<I', wav, wav.index(b'fact') + 8)[0]
        assert 0 < sample_count <= len(reference)
        reference = reference[:sample_count]
        assert any(reference) and len(reference) < len(actual)
        assert all(a == int(b / 3) for a, b in zip(actual, reference)), (name, next((i,a,b) for i,(a,b) in enumerate(zip(actual, reference)) if a != int(b/3)))
        assert not any(actual[len(reference):]), name
    for name in ('test_puzzle', 'test_galaga', 'test_audio'):
        build('tests/audio/' + name + '.cpp', name)
        subprocess.run([str(tmp / name)], check=True)
    print('Orchestral decoding matches FFmpeg; game audio checks passed')
