"""Run the actual Life model and controller with simulated input and display.

Run: python tools/check_life.py (requires g++).
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src" / "life.cpp").read_text()
source = "\n".join(line for line in source.splitlines() if not line.startswith("#include"))
prefix = r'''
#include "life_model.h"
#include "life_guide.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <iostream>
#include <random>
#include <string>
#include <vector>
struct String : std::string {
  using std::string::string;
  String(int n) : std::string(std::to_string(n)) {}
  String(const std::string &s) : std::string(s) {}
};
template<class T> T constrain(T v, T lo, T hi) { return std::min(hi, std::max(lo, v)); }
constexpr uint16_t UI_BG=0, UI_PANEL=0x1082, UI_SURFACE=0x2104,
  UI_CYAN=0x2EDC, UI_TEXT=0xEF7D, UI_MUTED=0x8C51, UI_ICON_BG=0x18C3,
  TFT_MAGENTA=0xF81F;
constexpr int TL_DATUM=0, TR_DATUM=1, MC_DATUM=2, SHOOT=5;
unsigned long clockMs=100;
unsigned long renderCostMs=0;
bool button=false;
int command=-1, switched=-1;
unsigned long millis() { return clockMs; }
int digitalRead(int) { return button; }
int findCommand() { int c=command; command=-1; return c; }
int lifeInputPoll(bool = true) { return findCommand(); }
void lifeInputRequireRelease() {}
void lifeInputBegin() {}
void lifeInputEnd() {}
void switchGame(int i) { switched=i; }
void playNavClick() {}
void playSelectTone() {}
std::mt19937 rng(1729);
long random(long bound) { return std::uniform_int_distribution<long>(0,bound-1)(rng); }
struct {
  std::deque<char> chars;
  int available() { return chars.size(); }
  char read() { char c=chars.front(); chars.pop_front(); return c; }
  template<class... T> void printf(T...) {}
} Serial;
struct {
  int clears=0, cells=0, pixels=0;
  int datum=TL_DATUM, font=1;
  void reset() { clears=cells=pixels=0; }
  void bounds(int x,int y,int w,int h) { assert(x>=0&&y>=0&&w>0&&h>0&&x+w<=240&&y+h<=320); }
  void fillScreen(uint16_t) { ++clears; }
  void fillRect(int x,int y,int w,int h,uint16_t) { bounds(x,y,w,h); pixels+=w*h; }
  void fillRoundRect(int x,int y,int w,int h,int r,uint16_t) {
    bounds(x,y,w,h); assert(r>=0&&r*2<=w&&r*2<=h);
    if(w==6&&h==6) ++cells;
  }
  void drawRoundRect(int x,int y,int w,int h,int r,uint16_t) { bounds(x,y,w,h); assert(r>=0); }
  void drawRect(int x,int y,int w,int h,uint16_t) { bounds(x,y,w,h); }
  void setTextDatum(int value) { datum=value; }
  void setTextFont(int value) { font=value; }
  template<class... T> void setTextColor(T...) {}
  void drawString(const std::string &text,int x,int y) {
    // Actual TFT_eSPI Font 2 advances for ASCII 32..95; Font 1 is 6x8.
    static const int widths[] = {
      6,3,4,9,8,9,9,3,7,7,8,6,3,6,5,7,
      8,8,8,8,8,8,8,8,8,8,3,3,6,6,6,8,
      9,8,8,8,8,8,8,8,8,4,8,8,7,10,8,8,
      8,8,8,8,8,8,8,10,8,8,8,4,7,4,7,9
    };
    int w=0,h=font==2?16:8;
    for(unsigned char c:text) {
      if(font==2) { assert(c>=32&&c<96); w+=widths[c-32]; }
      else w+=6;
    }
    if(datum==TR_DATUM) x-=w;
    if(datum==MC_DATUM) { x-=w/2; y-=h/2; }
    bounds(x,y,w,h);
  }
  void startWrite() {}
  void endWrite() { clockMs+=renderCostMs; }
  void setRotation(int) {}
} tft;
'''
suffix = r'''
static void key(char c) { Serial.chars.push_back(c); lifeLoop(); }
static void tick(unsigned long delta) { clockMs+=delta; lifeLoop(); }
static void fresh() {
  renderCostMs=0; button=false; Serial.chars.clear(); command=-1; clockMs=100; lifeSetup(); tft.reset();
}
int main() {
  using lifeModel::Board;
  Board b;
  b.set(2,2,true); b.set(3,2,true); b.set(2,3,true); b.set(3,3,true);
  assert(!b.step()&&b.population==4&&b.generation==1);
  b.clear(); for(int x=5;x<=7;++x) b.set(x,5,true);
  b.step(); assert(b.cells[4*b.width+6]&&b.cells[5*b.width+6]&&b.cells[6*b.width+6]&&b.population==3);
  b.step(); for(int x=5;x<=7;++x) assert(b.cells[5*b.width+x]);
  b.clear(); b.set(-1,0,true); b.set(b.width,0,true); b.set(0,b.height,true); assert(b.population==0);
  b.set(0,0,true); b.set(0,0,true); assert(b.population==1);
  b.set(0,0,false); b.set(0,0,false); assert(b.population==0);
  const int sizes[][2]={{28,24},{56,48},{112,96},{13,11}};
  for(const auto &size:sizes) {
    assert(b.resize(size[0],size[1])); int W=b.width,H=b.height,N=b.count();
    for(int sample=0;sample<80;++sample) {
      b.clear(); for(int i=0;i<N;++i) if(rng()%3==0) b.set(i%W,i/W,true);
      for(int generation=0;generation<12;++generation) {
        std::vector<int> counts(N); std::vector<uint8_t> expected(N); int population=0;
        for(int i=0;i<N;++i) if(b.cells[i])
          for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) {
            if(!dx&&!dy) continue;
            ++counts[((i/W+dy+H)%H)*W+(i%W+dx+W)%W];
          }
        for(int i=0;i<N;++i) { expected[i]=counts[i]==3||(b.cells[i]&&counts[i]==2); population+=expected[i]; }
        b.step(); assert(b.population==population);
        for(int i=0;i<N;++i) assert(b.cells[i]==expected[i]);
      }
    }
  }
  assert(!b.resize(113,96)&&b.width==13&&b.height==11);
  assert(!b.resize(0,0)&&!b.resize(2,2));
  assert(b.resize(112,96)); b.set(111,95,true); b.resize(28,24);
  for(int i=0;i<lifeModel::CAPACITY;++i) assert(b.cells[i]==0&&b.next[i]==0);
  std::cout<<"PASS: 3840 generation comparisons at four sizes, wrapping, population and resize invariants\n";
  fresh(); assert(lifeApp::mode==lifeApp::RUNNING);
  auto &board=lifeApp::board;
  assert(board.width==112&&board.height==96&&lifeApp::CELL_SIZE==2);
  assert(board.population>2500&&board.population<4000);
  key('e'); assert(lifeApp::mode==lifeApp::PAUSED);
  auto gen=board.generation;
  std::vector<uint8_t> saved(board.cells,board.cells+board.count());
  tick(1000); key('a'); key('d');
  assert(board.generation==gen&&memcmp(saved.data(),board.cells,board.count())==0);
  key('q'); key('s'); key('s'); key('s'); key('e');
  assert(lifeApp::mode==lifeApp::INFO);
  for(int p=0;p<lifeGuide::PAGE_COUNT;++p) { tick(450); key('d'); }
  assert(lifeApp::guidePage==lifeGuide::PAGE_COUNT-1);
  assert(board.generation==gen&&memcmp(saved.data(),board.cells,board.count())==0);
  key('e'); assert(lifeApp::mode==lifeApp::OPTIONS);
  key('q'); assert(lifeApp::mode==lifeApp::PAUSED);
  key('e'); assert(lifeApp::mode==lifeApp::RUNNING);
  key('q'); key('s'); key('e'); assert(board.generation==0&&lifeApp::mode==lifeApp::RUNNING);
  key('q'); for(int i=0;i<4;++i) key('s'); key('e'); assert(switched==0);
  fresh(); for(int i=0;i<15;++i) key('w'); assert(lifeApp::speed==lifeApp::SPEED_COUNT-1);
  for(int i=0;i<15;++i) key('s'); assert(lifeApp::speed==0);
  board.clear(); lifeApp::drawBoard(); board.set(0,0,true); lifeApp::drawBoard();
  board.set(0,0,false); tft.reset(); lifeApp::drawBoard(); assert(tft.pixels==4);
  tft.reset(); lifeApp::drawBoard(); assert(tft.pixels==0);
  std::cout<<"PASS: random-only startup, pause/resume, guide navigation, world preservation, speed and dirty drawing\n";
  fresh(); button=true; lifeSetup(); button=false; lifeLoop(); assert(lifeApp::mode==lifeApp::RUNNING);
  button=true; lifeLoop(); tick(40); button=false; lifeLoop(); assert(lifeApp::mode==lifeApp::PAUSED);
  button=true; lifeLoop(); tick(40); button=false; lifeLoop(); assert(lifeApp::mode==lifeApp::RUNNING);
  button=true; lifeLoop(); tick(900); assert(lifeApp::mode==lifeApp::OPTIONS);
  button=false; lifeLoop(); assert(lifeApp::mode==lifeApp::OPTIONS);
  key('q'); assert(lifeApp::mode==lifeApp::RUNNING);
  std::cout<<"PASS: launch-button suppression, physical tap/hold sequences, text/shape bounds and no cell-edit controls\n";
}
'''
with tempfile.TemporaryDirectory(prefix="life-check-") as tmp:
    path = Path(tmp)
    (path / "check.cpp").write_text(prefix + source + suffix)
    subprocess.run(["g++", "-std=c++11", "-O2", "-D_GLIBCXX_ASSERTIONS", "-I", str(ROOT / "src"),
                    str(path / "check.cpp"), "-o", str(path / "check")], check=True)
    subprocess.run([str(path / "check")], check=True)
