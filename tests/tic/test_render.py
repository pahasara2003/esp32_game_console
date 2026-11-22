"""Check tic-tac-toe tiles, cursor updates and game logic."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
src = (root / 'src/mini_games.ino').read_text()
tic = src.split('namespace tic {', 1)[1].split('static void draw() {', 1)[0]
harness = (root / 'tests/breakout/test_render.cpp').read_text().split('TFT_eSPI tft;')[0]
test = r'''
#define TFT_WHITE 0xFFFF
namespace ui { uint16_t inkOn(uint16_t) { return 0; } }
#include "arcade_style.h"
TFT_eSPI tft;
static bool failAllocation=false;
struct TFT_eSprite: TFT_eSPI {
  TFT_eSPI *panel;
  TFT_eSprite(TFT_eSPI *p):panel(p){}
  void setColorDepth(int){}
  bool createSprite(int w,int h){if(failAllocation)return false;width=vw=w;height=vh=h;pixels.assign(w*h,0);return true;}
  void deleteSprite(){}
  void pushSprite(int x,int y){for(int j=0;j<height;++j)for(int i=0;i<width;++i)panel->drawPixel(x+i,y+j,pixels[j*width+i]);}
};
namespace tic {
''' + tic + r'''
}
int main() {
  arcadeStyle::drawBackdrop(tft,48,290);
  for (int i=0;i<9;++i) tic::drawTile(i);
  assert(tft.pixels[105*240+40] != tft.pixels[168*240+40]);
  assert(tft.pixels[168*240+40] != tft.pixels[231*240+40]);
  tic::cells[4]=1; tic::drawTile(4);
  tic::cursor=5; tic::drawTile(4); tic::drawTile(5);
  auto incremental=tft.pixels;
  arcadeStyle::drawBackdrop(tft,48,290);
  for (int i=0;i<9;++i) tic::drawTile(i);
  assert(incremental==tft.pixels);
  tic::cells[0]=tic::cells[1]=tic::cells[2]=2;
  assert(tic::winner()==2);
  failAllocation=true; tic::drawTile(5);
  assert(incremental==tft.pixels);
  std::cout << "Tic tile colors, cursor redraw, winner and allocation fallback passed\n";
}
'''
with tempfile.TemporaryDirectory() as work:
    path = Path(work)
    (path / 'Arduino.h').write_text('#include <cstdint>\n#include <cstdlib>\n#include <cstring>\n')
    (path / 'test.cpp').write_text(harness + test)
    subprocess.run(['g++', '-std=c++11', '-I' + work, '-I' + str(root / 'src'),
                    str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
