"""Check the shared backdrop against Breakout and Pong's dirty-band redraws."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
breakout = (root / 'src/breakout.ino').read_text().split('static void drawBreakoutHeader()')[0]
breakout = '\n'.join(line for line in breakout.splitlines() if not line.startswith('#include'))
pong = (root / 'src/mini_games.ino').read_text().split('namespace pong {', 1)[1].split('static unsigned long scoreFlashAt', 1)[0]
harness = (root / 'tests/breakout/test_render.cpp').read_text().split('#include "breakout_render.inc"')[0]
test = r'''
#include "breakout_render.inc"
#include "arcade_style.h"
#define TFT_WHITE 0xFFFF
namespace pong {
''' + pong + r'''
}
int main() {
  initBreakoutBackdropGrid();
  ballX = ballY = -100; paddleX = -100;
  renderBreakoutFrame(true);
  for (int y = 48; y <= 290; ++y)
    for (int x = 0; x < 240; ++x)
      assert(tft.pixels[y * 240 + x] == arcadeStyle::backdrop(x, y));
  TFT_eSPI fast;
  arcadeStyle::drawBackdrop(fast, 48, 290);
  for (int y = 48; y <= 290; ++y)
    for (int x = 0; x < 240; ++x)
      assert(fast.pixels[y * 240 + x] == arcadeStyle::backdrop(x, y));
  pong::paddleX = 94; pong::ballX = 120; pong::ballY = 170;
  pong::dirtyBands = 0; pong::dirtyObjects();
  assert(!(pong::dirtyBands & (1 << ((249 - 48) / 16))));
  pong::render(true);
  for (int frame = 0; frame < 100; ++frame) {
    pong::dirtyObjects();
    pong::ballX = 20 + frame * 7 % 200;
    pong::ballY = 112 + frame * 5 % 145;
    pong::dirty(245, 15); // movePaddle marks the paddle only when it moves
    pong::paddleX = 19 + frame * 3 % 150;
    if (frame % 12 == 0) pong::impact((int)pong::ballX, (int)pong::ballY);
    else if (pong::impactLife) --pong::impactLife;
    for (int i = min(pong::trailCount, 7); i > 0; --i) {
      pong::trailX[i] = pong::trailX[i-1]; pong::trailY[i] = pong::trailY[i-1];
    }
    pong::trailX[0] = pong::ballX; pong::trailY[0] = pong::ballY;
    pong::trailCount = min(8, pong::trailCount + 1);
    pong::dirtyObjects(); pong::render();
    auto incremental = tft.pixels;
    pong::render(true);
    assert(incremental == tft.pixels);
  }
  failAllocation = true;
  auto before = tft.pixels; pong::render(true); assert(before == tft.pixels);
  std::cout << "Shared Breakout backdrop and Pong incremental rendering passed\n";
}
'''
with tempfile.TemporaryDirectory() as work:
    path = Path(work)
    (path / 'Arduino.h').write_text('#include <cstdint>\n#include <cstdlib>\n')
    (path / 'breakout_render.inc').write_text(breakout)
    (path / 'test.cpp').write_text(harness + test)
    subprocess.run(['g++', '-std=c++11', '-I' + work, '-I' + str(root / 'src'),
                    str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
