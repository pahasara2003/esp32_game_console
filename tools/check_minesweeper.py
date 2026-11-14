"""Check actual firmware logic on the host with mocked display and inputs.

Run: python tools/check_minesweeper.py (requires g++).
Hardware ADC timing and display appearance still require the console.
"""
from pathlib import Path
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[1] / 'src' / 'mini_games.ino').read_text()
source = '\n'.join(line for line in source.splitlines() if not line.startswith('#include'))
prefix = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <random>
#include <string>
#include <vector>
using std::min; using std::max; using std::abs;
struct String : std::string {
  using std::string::string;
  String(int value) : std::string(std::to_string(value)) {}
};
template<class T> T constrain(T value, T low, T high) { return min(high, max(low, value)); }
constexpr uint16_t UI_BG=0, UI_PANEL=1, UI_SURFACE=2, UI_CYAN=3,
  UI_GREEN=4, UI_PURPLE=5, UI_YELLOW=6, UI_RED=7, UI_TEXT=8,
  UI_MUTED=9, UI_ICON_BG=10, TFT_WHITE=65535, TFT_BLACK=0, TFT_MAGENTA=0xF81F;
constexpr int TL_DATUM=0, MC_DATUM=1, TR_DATUM=2, SHOOT=5;
static unsigned long nowMs=100;
static bool shootDown=false;
static int command=-1, switched=-1;
unsigned long millis() { return nowMs; }
int digitalRead(int) { return shootDown; }
int findCommand() { int next=command; command=-1; return next; }
int getDirectHorizontalInput() { return 0; }
void playNavClick() {}
void playSelectTone() {}
void switchGame(int index) { switched=index; }
std::mt19937 rng;
long random(long bound) { return std::uniform_int_distribution<long>(0,bound-1)(rng); }
struct {
  std::deque<char> data;
  int available() { return data.size(); }
  char peek() { return data.front(); }
  char read() { char c=data.front(); data.pop_front(); return c; }
} Serial;
struct {
  int clears=0, tiles=0, dots=0;
  void reset() { clears=tiles=dots=0; }
  template<class... T> void fillScreen(T...) { ++clears; }
  template<class... T> void fillRect(T...) { ++tiles; }
  template<class... T> void fillCircle(T...) { ++dots; }
  template<class... T> void fillRoundRect(T...) {}
  template<class... T> void drawRoundRect(T...) {}
  template<class... T> void drawFastVLine(T...) {}
  template<class... T> void drawFastHLine(T...) {}
  template<class... T> void drawLine(T...) {}
  template<class... T> void drawCircle(T...) {}
  template<class... T> void fillTriangle(T...) {}
  template<class... T> void setTextDatum(T...) {}
  template<class... T> void setTextFont(T...) {}
  template<class... T> void setTextColor(T...) {}
  template<class... T> void drawString(T...) {}
  template<class... T> void setRotation(T...) {}
} tft;
struct { int scores=0; void recordScore(int) { ++scores; } } currentGame;
'''
suffix = r'''
static void fresh() {
  shootDown=false; command=-1; Serial.data.clear(); nowMs=100;
  mini::resetInput(); currentGame.scores=0; switched=-1; mines::start(); tft.reset();
}
static void key(char c) { Serial.data.push_back(c); minesweeperLoop(); }
static void manualBoard() {
  fresh();
  for (int i=0;i<10;++i) mines::bomb[i]=true;
  for (int i=0;i<64;++i) {
    int row=i/8,col=i%8,count=0;
    for(int dr=-1;dr<=1;++dr) for(int dc=-1;dc<=1;++dc) {
      if(!dr&&!dc) continue;
      int r=row+dr,c=col+dc;
      if(r>=0&&r<8&&c>=0&&c<8&&mines::bomb[r*8+c]) ++count;
    }
    mines::counts[i]=count;
  }
  mines::planted=true;
}
int main() {
  int failures=0;
  for(int seed=0;seed<64;++seed) for(int safe=0;safe<64;++safe) {
    fresh(); rng.seed(seed*64+safe); mines::plant(safe);
    int total=0;
    for(int i=0;i<64;++i) {
      total += mines::bomb[i];
      int expected=0;
      for(int j=0;j<64;++j)
        if(i!=j && abs(i/8-j/8)<=1 && abs(i%8-j%8)<=1 && mines::bomb[j]) ++expected;
      assert(mines::counts[i]==expected);
      if(i!=safe && (rng()%7)==0) mines::flagged[i]=true;
    }
    assert(total==10 && !mines::bomb[safe]);
    bool reference[64]={}; std::deque<int> queue{safe};
    while(!queue.empty()) {
      int i=queue.front(); queue.pop_front();
      if(reference[i] || mines::flagged[i] || mines::bomb[i]) continue;
      reference[i]=true;
      if(mines::counts[i]) continue;
      for(int j=0;j<64;++j)
        if(i!=j && abs(i/8-j/8)<=1 && abs(i%8-j%8)<=1) queue.push_back(j);
    }
    mines::uncover(safe);
    int expectedExposed=0;
    for(int i=0;i<64;++i) {
      assert(mines::revealed[i]==reference[i]);
      assert(mines::dirty[i]==reference[i]);
      expectedExposed += reference[i];
    }
    assert(mines::exposed==expectedExposed);
    mines::uncover(safe); assert(mines::exposed==expectedExposed);
    tft.reset(); mines::drawDirty();
    assert(tft.clears==0 && tft.tiles==expectedExposed);
    tft.reset(); mines::drawDirty(); assert(tft.tiles==0);
  }
  std::cout << "PASS: 4096 randomized boards: mine count, first-cell safety, independent neighbor and flood-fill oracles, flags, incremental redraws\n";
  for(int cell=0;cell<64;++cell) {
    fresh(); rng.seed(cell); mines::cursor=cell; key('e');
    assert(mines::planted && !mines::bomb[cell]);
    assert(mines::revealed[cell] && mines::result!=-1);
  }
  fresh(); mines::uncover(27); assert(mines::exposed==64);
  std::cout << "PASS: safe first reveal at every position; flood-fill across an entirely empty board\n";
  manualBoard(); mines::cursor=0; key('f'); assert(mines::flagged[0] && mines::flags==1);
  key('e'); assert(mini::phase==mini::PLAY && mines::exposed==0);
  key('f'); assert(!mines::flagged[0] && mines::flags==0);
  key('e'); assert(mini::phase==mini::FINISH && mines::result==-1 && currentGame.scores==0);
  key('e'); assert(mini::phase==mini::PLAY && !mines::planted && mines::exposed==0);
  manualBoard(); mines::cursor=63;
  for(int i=10;i<63;++i) mines::revealed[i]=true;
  mines::exposed=53; key('e');
  assert(mines::exposed==54 && mini::phase==mini::FINISH && mines::result==1);
  assert(currentGame.scores==1); minesweeperLoop(); assert(currentGame.scores==1);
  std::cout << "PASS: flag protection, unflag, mine loss, restart, 54-safe-cell win and single score recording\n";
  for(int cell=0;cell<64;++cell) for(int dir=0;dir<4;++dir) {
    fresh(); mines::cursor=cell; command=dir; minesweeperLoop();
    int row=cell/8,col=cell%8;
    if(dir==0) row=max(0,row-1);
    if(dir==1) col=min(7,col+1);
    if(dir==2) row=min(7,row+1);
    if(dir==3) col=max(0,col-1);
    assert(mines::cursor==row*8+col);
    assert(tft.clears==0 && tft.tiles==(mines::cursor==cell ? 0 : 2));
  }
  fresh(); mines::cursor=0;
  for(int dir : {0,3}) { command=dir; minesweeperLoop(); assert(mines::cursor==0); }
  command=1; minesweeperLoop(); assert(mines::cursor==1 && tft.clears==0 && tft.tiles==2);
  tft.reset(); minesweeperLoop(); assert(tft.clears==0 && tft.tiles==0);
  fresh(); shootDown=true; minesweeperLoop(); nowMs+=40; shootDown=false; minesweeperLoop();
  assert(mines::planted && mines::revealed[27]);
  fresh(); shootDown=true; minesweeperLoop(); nowMs+=900; minesweeperLoop();
  assert(mines::flagged[27] && mines::flags==1 && !mines::planted);
  nowMs+=900; minesweeperLoop(); assert(mines::flags==1);
  shootDown=false; minesweeperLoop(); assert(!mines::planted);
  fresh(); shootDown=true; command=0; minesweeperLoop(); assert(mini::phase==mini::PAUSE);
  shootDown=false; minesweeperLoop(); assert(mini::phase==mini::PAUSE);
  key('e'); assert(mini::phase==mini::PLAY && mines::exposed==0);
  std::cout << "PASS: cursor boundaries, idle rendering, short reveal, one flag per hold, pause/resume without accidental reveal\n";
  fresh(); shootDown=true; minesweeperLoop(); nowMs+=900; minesweeperLoop();
  command=0; minesweeperLoop();
  if(mini::phase!=mini::PAUSE) {
    std::cout << "FAIL: Up must still pause after the current Shoot hold has placed a flag\n"; ++failures;
  }
  if(mini::phase==mini::PAUSE) {
    shootDown=false; minesweeperLoop(); assert(mini::phase==mini::PAUSE);
    key('e'); assert(mini::phase==mini::PLAY && mines::flagged[27] && mines::exposed==0);
    shootDown=true; command=0; minesweeperLoop(); shootDown=false; minesweeperLoop();
    key('s'); key('e');
    assert(mini::phase==mini::PLAY && !mines::planted && mines::flags==0 && mines::exposed==0);
    for(int i=0;i<64;++i) assert(!mines::revealed[i] && !mines::flagged[i] && !mines::dirty[i]);
  }
  fresh(); mines::revealed[mines::cursor]=true; tft.reset(); mines::drawTile(mines::cursor);
  if(tft.dots==0) {
    std::cout << "FAIL: selected revealed zero must have a visible empty-cell marker\n"; ++failures;
  }
  if(failures) return 1;
  std::cout << "PASS: regression cases for long-hold pause and visible revealed zeros\n";
}
'''
with tempfile.TemporaryDirectory(prefix='minesweeper-check-') as tmp:
    path = Path(tmp)
    (path/'check.cpp').write_text(prefix + source + suffix)
    subprocess.run(['g++','-std=c++17','-O1','-g','-D_GLIBCXX_ASSERTIONS',str(path/'check.cpp'),'-o',str(path/'check')],check=True)
    subprocess.run([str(path/'check')],check=True)
