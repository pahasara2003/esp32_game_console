"""Check the firmware decoder against every original RGB565 pixel."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from PIL import Image

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / 'tools'))
from run_background_codec import write_backgrounds

harness = r'''
#include <cassert>
#include <fstream>
#include "run_background.h"
int main(int argc, char **argv) {
  assert(argc == 2);
  std::ifstream reference(argv[1], std::ios::binary);
  for (int frame = 0; frame < RUN_BACKGROUND_FRAME_COUNT; ++frame) {
    for (int y = 0; y < 320; ++y) {
      uint16_t row[240];
      assert(reference.read(reinterpret_cast<char *>(row), sizeof(row)));
      for (int x = 0; x < 240; ++x) {
        assert(runBackgroundPixel(frame, x, y, 0) == row[x]);
        assert(runBackgroundPixel(frame, 239-x, y, 1) == row[239-x]);
      }
    }
  }
}
'''

with tempfile.TemporaryDirectory() as directory:
    work = Path(directory)
    (work / 'pgmspace.h').write_text('''#pragma once
#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_dword(p) (*(const uint32_t *)(p))
''')
    (work / 'check.cpp').write_text(harness)
    frames = []
    for frame in range(9):
        image = Image.open(root / f'assets/run-scene/day-night-{frame}.png').convert('RGB')
        frames.append([((r & 248) << 8) | ((g & 252) << 3) | (b >> 3)
                       for r, g, b in image.getdata()])
    # Regeneration must be deterministic, so future artwork uses the same codec.
    write_backgrounds(work / 'generated.h', frames)
    assert (work / 'generated.h').read_bytes() == (root / 'src/run_background_240x320.h').read_bytes()
    reference = work / 'reference.raw'
    reference.write_bytes(b''.join(struct.pack('<76800H', *frame) for frame in frames))
    subprocess.run(['g++', '-std=c++11', '-O2', '-I' + str(work), '-I' + str(root / 'src'),
                    str(work / 'check.cpp'), '-o', str(work / 'check')], check=True)
    subprocess.run([str(work / 'check'), str(reference)], check=True)
    print('PASS: all 691200 original pixels, both row caches, reverse reads and deterministic generation')
