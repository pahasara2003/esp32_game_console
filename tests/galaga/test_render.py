"""Exercise Galaga's transparent sprite and Breakout-style score popup."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
galaga = (root / 'src/galaga.ino').read_text()
breakout = (root / 'src/breakout.ino').read_text()
harness = (root / 'tests/breakout/test_render.cpp').read_text().split('TFT_eSPI tft;')[0]
enemy = galaga[galaga.index('static bool prepareAlienSprite'):galaga.index('static void drawGalagaLaser')]
laser = galaga[galaga.index('static void drawGalagaLaser'):galaga.index('static void drawAlienBullet')]
pop = galaga[galaga.index('struct GalaPop'):galaga.index('// -----------------------------------------------------------------\n// BULLETS')]
glyph = breakout[breakout.index('template<class Surface>\nstatic void drawTinyDigit'):breakout.index('// Compose every layer')]
test = r'''
TFT_eSPI tft;
static unsigned long now;
unsigned long millis() { return now; }
#define GALA_W 240
#define GALA_PLAYFIELD_TOP 48
#define GALA_PLAYFIELD_BOTTOM 290
#define GALA_PURPLE 0xA33F
#define GALA_GREEN 0x268C
#define GALA_RED 0xF98B
#define GALA_AMBER 0xFE64
#define GALA_DARK_TEXT 0x2087
#define GALA_CYAN 0x36FF
#define SHIP_Y 268
#define GALA_LASER_WIDTH 4
#define GALA_LASER_HEIGHT 12
#define GALA_LASER_COLOR 0x47F5
struct Bullet { float x, y; bool active; };
Bullet pBullets[1];
#include "arcade_style.h"
int pushes = 0;
struct Sprite: TFT_eSPI {
  bool ready = false;
  bool created() { return ready; }
  bool createSprite(int w, int h) { width=vw=w; height=vh=h; pixels.assign(w*h,0); return ready=true; }
  void fillSprite(uint16_t c) { std::fill(pixels.begin(), pixels.end(), c); }
  void pushSprite(int x, int y, uint16_t transparent) {
    ++pushes;
    for (int j=0;j<height;++j) for (int i=0;i<width;++i)
      if (pixels[j*width+i]!=transparent) tft.drawPixel(x+i,y+j,pixels[j*width+i]);
  }
  bool pushToSprite(Sprite *dest, int x, int y, uint16_t transparent) {
    for (int j=0;j<height;++j) for (int i=0;i<width;++i)
      if (pixels[j*width+i]!=transparent) dest->drawPixel(x+i,y+j,pixels[j*width+i]);
    return true;
  }
  void pushSprite(int x, int y, int sx, int sy, int w, int h) {
    ++pushes;
    for (int j=0;j<h;++j) for (int i=0;i<w;++i)
      tft.drawPixel(x+i,y+j,pixels[(sy+j)*width+sx+i]);
  }
} galaEnemyArt, galaEnemyMove;
uint16_t galagaBackdropColor(int x, int y) { return arcadeStyle::backdrop(x,y); }
void restoreGalagaBackdropRect(int x,int y,int w,int h) {
  for (int yy=max(48,y);yy<min(291,y+h);++yy)
    for (int xx=max(0,x);xx<min(240,x+w);++xx) tft.drawPixel(xx,yy,arcadeStyle::backdrop(xx,yy));
}
''' + glyph + enemy + laser + pop + r'''
int main() {
  tft.fillRect(0,0,240,320,0x1234);
  drawAlienSprite(30,80,1);
  assert(tft.pixels[80*240+30] == 0x1234);
  assert(tft.pixels[85*240+33] != 0x1234);
  tft.fillRect(0,0,240,320,0x1234);
  drawAlienSprite(30,286,1);
  for (int y=291;y<320;++y) for(int x=0;x<240;++x) assert(tft.pixels[y*240+x]==0x1234);
  assert(tft.getViewportY()==0 && tft.getViewportHeight()==320);
  tft.fillRect(0,0,240,320,0x1234);
  restoreGalagaBackdropRect(20, 70, 40, 40);
  drawAlienSprite(30,80,1);
  pushes = 0; moveAlienSprite(30,80,31,80,1);
  assert(pushes == 1);
  assert(tft.pixels[85*240+33] != arcadeStyle::backdrop(33,85));
  assert(tft.pixels[80*240+30] == arcadeStyle::backdrop(30,80));
  tft.fillRect(0,0,240,320,0x1234);
  pBullets[0] = {100, 200, true}; drawGalagaLaser(0);
  assert(tft.pixels[200*240+100] != 0x1234);
  assert(tft.pixels[215*240+100] != 0x1234);
  restoreGalagaBackdropRect(96, 200, 9, 33);
  for (int y=200;y<233;++y) for (int x=96;x<105;++x)
    assert(tft.pixels[y*240+x] == arcadeStyle::backdrop(x,y));
  tft.fillRect(0,0,240,320,0x1234);
  pBullets[0].y = 254; drawGalagaLaser(0);
  for (int y=268;y<320;++y) for(int x=0;x<240;++x) assert(tft.pixels[y*240+x]==0x1234);
  tft.fillRect(0,0,240,320,0x1234);
  galaPops[0] = {100, 100.0f, 150, GALA_RED, 0, true}; now = 1;
  drawGalagaPops();
  int px = 100 - (3*8+6)/2;
  assert(tft.pixels[(99+4)*240+px] != 0x1234);
  assert(tft.pixels[(99+0)*240+(px+8+2)] != 0x1234);
  assert(tft.pixels[100*240+100] == 0x1234); // no ring around the text
  now = 600; drawGalagaPops(); assert(!galaPops[0].active);
  std::cout << "Galaga sprites, score popup and missile trail passed\n";
}
'''
with tempfile.TemporaryDirectory() as work:
    path = Path(work)
    (path / 'Arduino.h').write_text('#include <cstdint>\n#include <cstdlib>\n')
    (path / 'test.cpp').write_text(harness + test)
    subprocess.run(['g++', '-std=c++11', '-I' + work, '-I' + str(root / 'src'),
                    str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
