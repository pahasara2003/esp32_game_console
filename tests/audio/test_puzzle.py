"""Check embedded orchestral ADPCM decoding against FFmpeg and the audio mixer."""
from array import array
from pathlib import Path
import subprocess
import tempfile

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
    subprocess.run(['ld', '-r', '-b', 'binary', 'assets/puzzle_mozart.wav', '-o', str(obj)], cwd=root, check=True)
    def build(source, name):
        subprocess.run(['g++', '-std=c++11', '-Itests/audio', '-Isrc', 'src/audio_kit.cpp',
                        str(source), str(obj), '-o', str(tmp / name)], cwd=root, check=True)
    build(tmp / 'emitter.cpp', 'emit')
    actual = array('h', subprocess.check_output([str(tmp / 'emit')]))
    reference = array('h', subprocess.check_output(['ffmpeg', '-v', 'error', '-i',
                      str(root / 'assets/puzzle_mozart.wav'), '-f', 's16le', '-']))
    assert len(actual) == 441000 and len(reference) >= len(actual)
    assert all(a == int(b / 2) for a, b in zip(actual, reference))
    for name in ('test_puzzle', 'test_galaga', 'test_audio'):
        build('tests/audio/' + name + '.cpp', name)
        subprocess.run([str(tmp / name)], check=True)
    print('Orchestral decoding matches FFmpeg; game audio checks passed')
