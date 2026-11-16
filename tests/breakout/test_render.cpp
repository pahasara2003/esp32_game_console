#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
using std::min;
using std::max;
template<class T> T constrain(T n,T a,T b) { return min(b,max(a,n)); }
long random(long a,long b) { static uint32_t n=9; n=n*1664525+1013904223; return a+n%(b-a); }
#define UI_BG 0x0000
#define UI_PANEL 0x1082
#define UI_SURFACE 0x2945
#define UI_TEXT 0xF7DE
#define UI_MUTED 0xAD55
#define UI_CYAN 0x07FF
namespace ui {
constexpr uint16_t INK_DARK = 0x0841;
}
namespace audio {
enum class Cue { Paddle, Wall, Brick, Lock, Sparkle, Milestone, Hit, Powerup };
void play(Cue, int=0) {}
}
// Pixel canvas with the same clipping/origin contract as TFT_eSPI.
struct TFT_eSPI {
  int width=240,height=320,ox=0,oy=0,vx=0,vy=0,vw=240,vh=320;
  std::vector<uint16_t> pixels = std::vector<uint16_t>(240*320,0x5555);
  void setViewport(int x,int y,int w,int h,bool=false) {vx=x;vy=y;vw=w;vh=h;ox=oy=0;}
  int getViewportX(){return vx;} int getViewportY(){return vy;}
  int getViewportWidth(){return vw;} int getViewportHeight(){return vh;}
  void setOrigin(int x,int y){ox=x;oy=y;}
  void drawPixel(int x,int y,uint16_t c) {
    x+=ox;y+=oy;
    if(x>=max(0,vx)&&x<min(width,vx+vw)&&y>=max(0,vy)&&y<min(height,vy+vh)) pixels[y*width+x]=c;
  }
  void fillRect(int x,int y,int w,int h,uint16_t c){for(int j=0;j<h;++j)for(int i=0;i<w;++i)drawPixel(x+i,y+j,c);}
  void drawFastHLine(int x,int y,int w,uint16_t c){fillRect(x,y,w,1,c);}
  void drawFastVLine(int x,int y,int h,uint16_t c){fillRect(x,y,1,h,c);}
  void fillRoundRect(int x,int y,int w,int h,int,uint16_t c){fillRect(x,y,w,h,c);}
  void drawRoundRect(int x,int y,int w,int h,int,uint16_t c){
    drawFastHLine(x,y,w,c);drawFastHLine(x,y+h-1,w,c);drawFastVLine(x,y,h,c);drawFastVLine(x+w-1,y,h,c);
  }
  void fillCircle(int x,int y,int r,uint16_t c){for(int j=-r;j<=r;++j)for(int i=-r;i<=r;++i)if(i*i+j*j<=r*r)drawPixel(x+i,y+j,c);}
  void drawCircle(int x,int y,int r,uint16_t c){for(int j=-r;j<=r;++j)for(int i=-r;i<=r;++i)if(std::abs(i*i+j*j-r*r)<=r)drawPixel(x+i,y+j,c);}
  void drawLine(int x,int y,int xx,int yy,uint16_t c){
    int n=max(std::abs(xx-x),std::abs(yy-y));
    for(int i=0;i<=n;++i)drawPixel(x+(xx-x)*i/n,y+(yy-y)*i/n,c);
  }
};
TFT_eSPI tft;
static bool failAllocation=false;
struct TFT_eSprite: TFT_eSPI {
  TFT_eSPI *panel;
  TFT_eSprite(TFT_eSPI *p):panel(p){}
  void setColorDepth(int){}
  bool createSprite(int w,int h){if(failAllocation)return false;width=vw=w;height=vh=h;pixels.assign(w*h,0);return true;}
  void deleteSprite(){}
  void pushSprite(int x,int y,int sx,int sy,int w,int h){for(int j=0;j<h;++j)for(int i=0;i<w;++i)panel->drawPixel(x+i,y+j,pixels[(sy+j)*width+sx+i]);}
};
#include "breakout_render.inc"
static void matchesFullRedraw() {
  auto incremental=tft.pixels;
  renderBreakoutFrame(true);
  assert(incremental==tft.pixels);
  for(int y=0;y<320;++y)if(y<BO_PLAYFIELD_TOP||y>BO_PLAYFIELD_BOTTOM)
    for(int x=0;x<240;++x)assert(tft.pixels[y*240+x]==0x5555);
}
int main(){
  dirtyBreakout(-20,10);dirtyBreakout(291,9);assert(boDirtyBands==0);
  dirtyBreakout(47,2);assert(boDirtyBands==1);boDirtyBands=0;
  dirtyBreakout(290,10);assert(boDirtyBands==(1<<15));boDirtyBands=0;
  initBreakoutBackdropGrid();
  for(auto &row:bricks)for(auto &b:row)b=1;
  ballLaunched=true;renderBreakoutFrame(true);matchesFullRedraw();
  // Each row has a distinct jewel color; shades brighten smoothly toward white.
  for(int r=0;r<BRICK_ROWS;++r) {
    assert(breakoutBrickColor(r, 0)==BO_ROW_PALETTE[r]);
    if (r > 0) assert(breakoutBrickColor(r, 0)!=breakoutBrickColor(r-1, 0));
    for(int s=1;s<3;++s) {
      uint16_t previous=breakoutBrickColor(r, s-1), next=breakoutBrickColor(r, s);
      assert((next>>11)>=(previous>>11));
      assert(((next>>5)&63)>=((previous>>5)&63));
      assert((next&31)>=(previous&31));
    }
  }
  int bx=BRICK_START_X,by=BRICK_START_Y;
  for(int r=0;r<BRICK_ROWS;++r)for(int c=0;c<BRICK_COLS;++c)boBrickShade[r][c]=(r+c)%3;
  renderBreakoutFrame(true);
  assert(tft.pixels[(by+5)*240+bx+12]==tft.pixels[(by+5)*240+bx+BRICK_WIDTH-4]);
  assert(tft.pixels[(by+5)*240+bx+12]!=tft.pixels[(by+BRICK_HEIGHT+BRICK_GAP_Y+5)*240+bx+12]);
  for(int frame=0;frame<300;++frame){
    dirtyBreakoutObjects();
    updateBreakoutEffects();
    ballX=4+(frame*7)%232;ballY=51+(frame*5)%239;
    paddleX=4+(frame*3)%170;
    if(frame%13==0){int r=(frame/13)%6,c=(frame/7)%6;bricks[r][c]=!bricks[r][c];dirtyBreakout(BRICK_START_Y+r*14,BRICK_HEIGHT);spawnParticles(20+c*38,60+r*14,BO_HEADER_START);spawnPopup(20+c*38,60+r*14,70,BO_HEADER_END);}
    if(frame%29==0){boCeilingFlash=4;boLeftFlash=frame%2?3:0;boRightFlash=frame%2?0:3;}
    powerup={ballX+3,ballY+2,(frame%3)+1,frame%20<10};
    paddleWidth=frame%30<15?46:64;boPaddlePulse=frame%11;
    dirtyBreakoutObjects();renderBreakoutFrame();matchesFullRedraw();
  }
  dirtyBreakoutObjects();
  boTrailCount=boParticleCount=0;powerup.active=false;boPopup.life=0;
  boCeilingFlash=boLeftFlash=boRightFlash=0;
  for(auto &rg:boRings) rg.life=0;
  dirtyBreakoutObjects();renderBreakoutFrame();matchesFullRedraw();
  failAllocation=true;renderBreakoutFrame(true);matchesFullRedraw();failAllocation=false;
  // Modal restoration may repaint only inside the countdown's viewport.
  auto before=tft.pixels;tft.setViewport(88,128,64,64,false);renderBreakoutFrame(true);
  assert(before==tft.pixels);
  failAllocation=true;renderBreakoutFrame(true);assert(before==tft.pixels);
  assert(tft.vx==88&&tft.vy==128&&tft.vw==64&&tft.vh==64);
  std::cout<<"Breakout overlap, effect cleanup, dirty bands, boundaries and allocation fallback passed\n";
}
