"""Compare Breakout's incremental composition with a clean full redraw on a pixel canvas.
The canvas tests clipping/layer damage; panel timing and TFT primitive shapes need hardware.
Run: python3 tests/breakout/test_render.py
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'src/breakout.ino').read_text()
source = source[:source.index('static void drawBreakoutHeader()')]
source = '\n'.join(line for line in source.splitlines() if not line.startswith('#include'))
with tempfile.TemporaryDirectory() as work:
    path = Path(work)
    (path / 'breakout_render.inc').write_text(source)
    subprocess.run(['g++', '-std=c++11', '-g',
                    '-I' + work, str(root / 'tests/breakout/test_render.cpp'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
