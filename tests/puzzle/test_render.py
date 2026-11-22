"""Check puzzle slide frames restore the arcade backdrop and final tile state."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
src = (root / 'src/sliding_puzzle.ino').read_text()
state = src[src.index('constexpr int PUZZLE_MAX_SIZE'):src.index('static void drawPuzzleGoalPreview()')]
render = src[src.index('static void puzzleTilePosition'):src.index('static void drawPuzzleHeader()')]
harness = (root / 'tests/breakout/test_render.cpp').read_text().split('TFT_eSPI tft;')[0]
harness = 'extern int pushImages;\n' + harness
harness = harness.replace('  void setOrigin(int x,int y){', '''  bool swap=false;
  bool getSwapBytes(){return swap;} void setSwapBytes(bool s){swap=s;}
  void pushImage(int x,int y,int w,int h,uint16_t *data){++pushImages;for(int j=0;j<h;++j)for(int i=0;i<w;++i)drawPixel(x+i,y+j,data[j*w+i]);}
  void setTextDatum(int){} void setTextFont(int){} void setTextColor(uint16_t,uint16_t){}
  void drawString(const char*,int,int){}
  void setOrigin(int x,int y){''')
test = r'''
#define TFT_WHITE 0xFFFF
#define MC_DATUM 0
namespace ui { uint16_t inkOn(uint16_t) { return 0; } }
#include "arcade_style.h"
TFT_eSPI tft;
int pushImages = 0, spritePushes = 0;
static bool failAllocation=false;
std::vector<std::vector<uint16_t>> frames;
void delay(int) { frames.push_back(tft.pixels); }
struct TFT_eSprite: TFT_eSPI {
  TFT_eSPI *panel;
  TFT_eSprite(TFT_eSPI *p):panel(p){}
  void setColorDepth(int){}
  bool createSprite(int w,int h){if(failAllocation)return false;width=vw=w;height=vh=h;pixels.assign(w*h,0);return true;}
  void deleteSprite(){}
  void pushSprite(int x,int y){++spritePushes;for(int j=0;j<height;++j)for(int i=0;i<width;++i)panel->drawPixel(x+i,y+j,pixels[j*width+i]);}
};
''' + state + render + r'''
int main() {
  for (int size=4; size<=6; ++size) {
    puzzleSelectedSize=size; setPuzzleGeometry();
    for (int value=1; value<size*size; ++value)
      assert(puzzleColorBand(value,size)==(value-1)/size);
    for (int direction=0; direction<4; ++direction) {
      int from=size+1, to=from+(direction==0?-size:direction==1?1:direction==2?size:-1);
      int fx,fy,tx,ty; puzzleTilePosition(from,fx,fy); puzzleTilePosition(to,tx,ty);
      for (int fallback=0; fallback<2; ++fallback) {
        tft.fillRect(0,0,240,320,0x5555);
        restorePuzzleBackdrop(0,48,240,243);
        drawPuzzleTileAt(tft,fx,fy,1,false);
        drawPuzzleTileAt(tft,tx,ty,0,false);
        failAllocation=fallback; frames.clear(); animatePuzzleSlide(from,to,1);
        assert(frames.size() == 5);
        int sx = fx + puzzleTile / 2, sy = fy + puzzleTile / 2;
        if (tx > fx) sx = fx + 5;
        if (tx < fx) sx = fx + puzzleTile - 6;
        if (ty > fy) sy = fy + 5;
        if (ty < fy) sy = fy + puzzleTile - 6;
        assert(frames[0][sy*240+sx] == 0x10C9);
        assert(frames[0][(ty+puzzleTile/2)*240+tx+puzzleTile/2] == 0x10C9);
        auto actual=tft.pixels;
        restorePuzzleBackdrop(min(fx,tx),min(fy,ty),abs(tx-fx)+puzzleTile,abs(ty-fy)+puzzleTile);
        drawPuzzleTileAt(tft,fx,fy,0,false);
        drawPuzzleTileAt(tft,tx,ty,1,true);
        assert(actual==tft.pixels);
      }
    }
    failAllocation=false;
    puzzleBoard[0]=1; puzzleLastMoved=0;
    int px,py; puzzleTilePosition(0,px,py);
    drawPuzzleTileAt(tft,px,py,1,true);
    puzzleLastMoved=-1; pushImages=0; spritePushes=0;
    drawPuzzleTile(0);
    assert(pushImages==0 && spritePushes==1);
  }
  std::cout << "Puzzle palette, slide directions, background and allocation fallback passed\n";
}
'''
with tempfile.TemporaryDirectory() as work:
    path = Path(work)
    (path / 'Arduino.h').write_text('#include <cstdint>\n#include <cstdlib>\n#include <cstdio>\n')
    (path / 'test.cpp').write_text(harness + test)
    subprocess.run(['g++', '-std=c++11', '-I' + work, '-I' + str(root / 'src'),
                    str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
