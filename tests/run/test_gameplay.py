"""Run the firmware's sprite compositor, pickups and combat on a pixel canvas."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
s = (root / 'src/run.ino').read_text()
def section(start, end):
    return s[s.index(start):s.index(end)]

harness = (root / 'tests/breakout/test_render.cpp').read_text().split('TFT_eSPI tft;')[0]
harness = harness.replace('Paddle, Wall, Brick, Lock, Sparkle, Milestone, Hit, Powerup',
                          'Paddle, Wall, Brick, Lock, Sparkle, Milestone, Hit, Powerup, Pickup')
harness = harness.replace('  void setViewport', '''  int transfers = 0, padding = 0;
  std::vector<int> writes = std::vector<int>(240*320,0);
  void startWrite() {} void endWrite() {}
  void setTextPadding(int n) { padding=n; }
  void setTextSize(int) {}
  void setSwapBytes(bool) {}
  void pushImage(int x,int y,int w,int h,const uint16_t *p) {
    ++transfers;
    for(int yy=0;yy<h;++yy) for(int xx=0;xx<w;++xx) {
      assert(x+xx>=0 && x+xx<240 && y+yy>=0 && y+yy<320);
      ++writes[(y+yy)*240+x+xx];
      drawPixel(x+xx,y+yy,p[yy*w+xx]);
    }
  }
  void setViewport''')
game_over = 'void gameOverSheet' + (root / 'src/ui_kit.cpp').read_text().split('void gameOverSheet', 1)[1].split('void startCard', 1)[0]
code = harness + r'''
#include <string>
struct String: std::string {using std::string::string; String(const std::string &s):std::string(s) {} String(int n):std::string(std::to_string(n)) {}};
#define pgm_read_word(p) (*(p))
#define pgm_read_dword(p) (*(p))
#define pgm_read_byte(p) (*(p))
#include "vampire_runner_sprites.h"
#include "run_rules.h"
#include "run_clouds.h"
#include "Fonts/Font16.c"
TFT_eSPI tft;
int gameState = 0;
bool menu_drawn = false;
int deaths = 0;
uint16_t backdropTint = 0;
void playRunnerDeathAnimation(int) { ++deaths; }
uint16_t runnerBackdropAt(int x, int y) { return ((x+y)&1 ? 0x1082 : 0x1083) ^ backdropTint; }
void restoreRunnerBackdropRect(int x,int y,int w,int h) {
  for(int yy=y;yy<y+h;++yy) for(int xx=x;xx<x+w;++xx) tft.drawPixel(xx,yy,runnerBackdropAt(xx,yy));
}
uint16_t runnerObstaclePixel(int,int,int) { return 0x4321; }
void cacheRunnerObstacleColors() {}
const uint16_t RUN_OBS_STONE[]={0x4321}, RUN_OBS_BRICK[]={0xA265}, RUN_OBS_HIGHLIGHT[]={0xFFE0};
uint16_t runnerTimeColor(const uint16_t *palette) {return *palette;}
static void dirtyRunnerRect(int,int,int,int);
static void renderRunnerDirty();
static void prepareRunMilestone();
uint8_t runDayFrame=0,runDayNextFrame=1,runDayBlend=0;
uint16_t blendRun565(uint16_t a,uint16_t,uint8_t) {return a;}
bool isJumping=false;
int legAnimFrame=0;
namespace ui {
constexpr uint16_t CARD=0x18E3, LABEL=0xFFFF, SECONDARY=0x9CF3, SEPARATOR=0x39C7;
struct List {int x,y,w,rowH,count;};
struct Row {int top,bottom; std::string label,value;};
std::vector<Row> rows;
int buttonTop,buttonBottom,cardTop,cardBottom,hintY,badgeBottom;
int textWidth(const char *s) {int w=0;while(*s) w+=widtbl_f16[(unsigned char)*s++-32];return w;}
void card(int,int y,int,int h) {cardTop=y;cardBottom=y+h;}
void heading(int,int,const char*,const char*) {}
void chip(int,int y,const char*,uint16_t) {badgeBottom=y+20;}
uint16_t accentTo(uint16_t c) {return c;}
void infoRow(const List &l,int i,const char *key,const char *value) {
  assert(textWidth(key)+textWidth(value)+8<=l.w-24);
  rows.push_back({l.y+i*l.rowH,l.y+(i+1)*l.rowH,key,value});
}
void gradientPill(int,int y,int,int h,const char*,uint16_t,uint16_t) {buttonTop=y;buttonBottom=y+h;}
void hint(int y,const char*,uint16_t bg,uint16_t=0) {
  hintY=y;
  tft.fillRect(120-tft.padding/2,y-8,tft.padding,16,bg);
}
}
constexpr int RUNNER_X=30, RUNNER_W=40, RUNNER_H=60, RUNNER_DEATH_W=80, RUNNER_DEATH_X=6;
constexpr int RUN_GROUND_Y=262, RUN_W=240, RUN_H=320;
constexpr uint16_t RUN_ACCENT=0x07FF, RUN_ACCENT_HI=0xBFFF, RUN_AMBER=0xFE04;
int score=0;
constexpr int ML_DATUM=1,TR_DATUM=2,MC_DATUM=3;
struct TFT_eSprite: TFT_eSPI {
  TFT_eSPI *panel; int datum=0; bool allocated=false;
  TFT_eSprite(TFT_eSPI *p):panel(p) {}
  void setColorDepth(int) {}
  bool createSprite(int w,int h) {width=vw=w;height=vh=h;pixels.assign(w*h,0);allocated=true;return true;}
  void fillSprite(uint16_t c) {std::fill(pixels.begin(),pixels.end(),c);}
  void setTextDatum(int d) {datum=d;}
  void setTextColor(uint16_t,uint16_t=0) {}
  void drawString(const std::string &s,int x,int y,int font) {
    assert(font==2); int w=ui::textWidth(s.c_str());
    int left=datum==TR_DATUM?x-w:x, top=datum==ML_DATUM?y-8:y;
    if(width==240) assert(left>=8 && left+w<=232 && top>=5 && top+16<=43);
    else {assert(w<=width); fillRect((width-w)/2,y-6,w,12,ui::LABEL);}
  }
  void pushSprite(int x,int y) {panel->pushImage(x,y,width,height,pixels.data());}
  bool created() {return allocated;}
  uint16_t readPixel(int x,int y) {return pixels[y*width+x];}
  void deleteSprite() {allocated=false;}
};
float runnerY=202, gameSpeed=4.2f;
''' + section('struct RunObstacle {', 'static unsigned long lastObsSpawn') + section('static int runCharacter', 'static int runPauseSelection') + section('struct RunCloud {', '// A 64-step') + section('static const uint16_t* runCloudImage', 'static void drawRunCloudClipped') + section('static void drawRunnerHeader()', 'static void updateRunnerScoreHUD') + section('static void drawRunnerBottomBar(', '// -----------------------------------------------------------------\n// RUNNER SPRITE') + section('static void drawRunner(', 'static void eraseRunner') + section('static void drawRunnerDeathFrame', 'static void playRunnerDeathAnimation') + section('static void addRunPickup', 'static uint16_t runObstacleColors') + section('// Merge old/new bounds', '// -----------------------------------------------------------------\n// GAME INITIALIZATION') + section('static void drawRunPickups', 'static void initRunnerGame') + section('static void drawRunnerChoice()', 'static void showRunnerStartScreen') + '\nnamespace ui {\n' + game_over + '\n}\n' + r'''
void fullScene() {dirtyRunnerRect(0,43,240,252); renderRunnerDirty();}
void matchesFullScene() {auto before=tft.pixels; fullScene(); assert(before==tft.pixels);}
void resetWrites() {tft.transfers=0; std::fill(tft.writes.begin(),tft.writes.end(),0);}
void singlePass() {assert(*std::max_element(tft.writes.begin(),tft.writes.end())<=1);}

int main() {
  for (auto &c : runClouds) {c.x=300;c.y=48;}
  // The top and side HUD margins follow the sky; nothing below the header is cleared.
  for(int tint : {0,0x2222,0x7777}) {
    backdropTint=tint; auto before=tft.pixels; resetWrites(); drawRunnerHeader();
    assert(tft.transfers==1);
    for(int y=0;y<320;++y) for(int x=0;x<240;++x) {
      if(y>=43) assert(tft.pixels[y*240+x]==before[y*240+x]);
      else if(y<5 || x<8 || x>=232) assert(tft.pixels[y*240+x]==runnerBackdropAt(x,y));
    }
    for(int x : {8,231}) for(int y : {5,42}) assert(tft.pixels[y*240+x]==runnerBackdropAt(x,y));
  }
  backdropTint=0;
  // Shield status changes replace the entire footer once, never clear it first.
  resetWrites(); drawRunnerBottomBar(true); assert(tft.transfers==1);
  resetWrites(); drawRunnerBottomBar(); assert(tft.transfers==0);
  runRewards.shield=8000; drawRunnerBottomBar(); assert(tft.transfers==1);
  resetWrites(); runRewards.shield=7900; drawRunnerBottomBar(); assert(tft.transfers==0);
  runRewards.shield=0; drawRunnerBottomBar(); assert(tft.transfers==1);
  // Every frame of all three characters decodes into its declared bounds.
  for (int c=0;c<3;++c) {
    for (const auto &sprite : runner_characters[c].actions) {
      for(int f=0;f<sprite.count;++f) {
        std::vector<uint8_t> frame(sprite.width*60+2,255);
        decodeRunnerFrame(sprite,f,frame.data()+1);
        assert(frame.front()==255 && frame.back()==255);
      }
    }
    runCharacter=c; runnerY=202; runRewards={};
    for (auto &o:obs) o.active=false;
    for (auto &p:runPickups) p.active=false;
    fullScene();
    runRewards.shield=8000;
    runRewards.startAttack(runner_characters[c].actions[3].count*70);
    drawRunner(30,202,202,0,false);
    matchesFullScene();
    runRewards.attack=0; drawRunner(30,202,202,0,false);
    // Attack's extended area must return to the exact background.
    for(int y=202;y<262;++y) for(int x=70;x<110;++x)
      assert(tft.pixels[y*240+x]==runnerBackdropAt(x,y));
    matchesFullScene();
    runRewards.shield=0; drawRunner(30,202,202,0,false);
    for(int y=190;y<202;++y) for(int x=30;x<70;++x)
      assert(tft.pixels[y*240+x]==runnerBackdropAt(x,y));
    for(int f=0;f<runner_characters[c].actions[2].count;++f) drawRunnerDeathFrame(202,f);
  }
  for(auto &p:runPickups) p.active=false;
  addRunPickup(180,150,false); addRunPickup(180,180,true); fullScene();
  assert(tft.pixels[150*240+180]==0xFFF0);
  assert(tft.pixels[180*240+180]==RUN_ACCENT);
  // Out-of-bounds dirty rectangles cannot touch the HUD/footer or send invalid transfers.
  auto untouched=tft.pixels;
  dirtyRunnerRect(-20,0,10,40); dirtyRunnerRect(240,295,30,40); renderRunnerDirty();
  assert(tft.pixels==untouched);
  // Moving overlapping actors must match a full redraw without painting any pixel twice.
  runCharacter=0; runRewards={};
  for(auto &o:obs) o.active=false;
  for(auto &p:runPickups) p.active=false;
  runClouds[0]={80,160,10,0,80};
  obs[0]={230,11,24,0,0,true};
  addRunPickup(210,175,false); addRunPickup(170,210,true);
  fullScene();
  for(int i=0;i<32;++i) {
    resetWrites();
    int previousY=(int)runnerY;
    runnerY=202-std::abs(16-i)*2; isJumping=runnerY<202; legAnimFrame=i/4;
    dirtyRunnerRect(runClouds[0].drawnX,160,run_cloud_width[0],run_cloud_height[0]);
    runClouds[0].x-=1; runClouds[0].drawnX=(int)runClouds[0].x;
    dirtyRunnerRect(runClouds[0].drawnX,160,run_cloud_width[0],run_cloud_height[0]);
    updateRunPickups(); updateAndDrawObstacles(); drawRunPickups();
    drawRunner(30,(int)runnerY,previousY,legAnimFrame,isJumping);
    singlePass(); matchesFullScene();
  }
  runnerY=202; isJumping=false;
  for(auto &o:obs) o.active=false;
  for(auto &p:runPickups) p.active=false;
  for(auto &c:runClouds) c.x=300;
  runRewards={}; deaths=0; gameState=0;
  addRunPickup(54,230,false); drawRunPickups(); updateRunPickups();
  assert(runRewards.coins==1 && !runPickups[0].active);
  addRunPickup(-5,200,false); updateRunPickups(); assert(runRewards.streak==0);
  addRunPickup(54,230,true); updateRunPickups(); assert(runRewards.shield==8000);
  obs[0]={88,11,24,0,0,true}; runRewards.startAttack(350); runRewards.update(140);
  updateAndDrawObstacles(); assert(!obs[0].active && runRewards.coins==6 && deaths==0);
  runRewards.attack=0; obs[0]={50,11,24,0,0,true}; updateAndDrawObstacles();
  assert(!obs[0].active && !runRewards.shield && deaths==0);
  runRewards.grace=0; obs[0]={50,11,24,0,0,true}; updateAndDrawObstacles();
  assert(deaths==1 && gameState==2);
  for(auto &o:obs) o.active=false;
  for(auto &p:runPickups) p.active=false;
  runPattern=runPatternStep=runSequence=0; spawnRunObstacle();
  assert(obs[0].style==0 && runSpawnDistance>=40*gameSpeed);
  int count=0; for(auto &p:runPickups) count+=p.active; assert(count==4);
  // Every six obstacles introduce a reachable shield; expired slots are reused.
  for (int step=1;step<6;++step) {
    for(auto &o:obs) o.active=false;
    for(auto &p:runPickups) p.active=false;
    if(runBonusNext) {
      spawnRunObstacle();
      for(auto &p:runPickups) p.active=false;
    }
    spawnRunObstacle();
  }
  int shields=0; for(auto &p:runPickups) shields+=p.active && p.shield;
  assert(shields==1 && runPatternStep==0 && runSequence==2);
  for(auto &p:runPickups) p.active=false;
  addRunPickup(247,220,false); addRunPickup(-7,220,true); drawRunPickups();
  for(auto &p:runPickups) p.active=false;
  // Every character keeps a block through wind-up, even in body contact, and hits at its middle frame.
  gameSpeed=0; deaths=0; gameState=0;
  for(int c=0;c<3;++c) for(int x : {50,90}) {
    runCharacter=c; runRewards={}; runRewards.shield=8000;
    for(auto &o:obs) o.active=false;
    for(auto &p:runDebris) p.life=0;
    int count=runner_characters[c].actions[3].count;
    runRewards.startAttack(count*runrules::attackFrameMs);
    obs[0]={(float)x,11,24,0,0,true};
    for(int age=0;age<count/2*70;age+=10) {
      updateAndDrawObstacles();
      assert(obs[0].active && runRewards.coins==0 && runRewards.shield>0 && deaths==0);
      runRewards.update(10);
    }
    assert(runrules::attackFrame(runRewards.attack,count)==count/2);
    updateAndDrawObstacles();
    assert(!obs[0].active && runRewards.coins==5 && runRewards.shield>0 && deaths==0);
    int pieces=0; for(auto &p:runDebris) pieces+=p.life>0; assert(pieces==12);
    int coins=runRewards.coins; updateAndDrawObstacles(); assert(runRewards.coins==coins);
    fullScene();
    for(int tick=0;tick<30;++tick) {
      resetWrites(); updateRunDebris(20); drawRunner(30,202,202,0,false);
      singlePass(); matchesFullScene();
    }
    for(auto &p:runDebris) assert(!p.life);
  }
  // At real scrolling speeds, approaching blocks survive wind-up and break on the first middle-frame tick.
  for(int c=0;c<3;++c) for(float speed : {runrules::startSpeed,runrules::maxSpeed}) {
    for(auto &o:obs) o.active=false;
    for(auto &p:runDebris) p.life=0;
    runCharacter=c; runRewards={}; runRewards.shield=8000; gameSpeed=speed;
    int count=runner_characters[c].actions[3].count, middleAt=count/2*70;
    int ticks=(middleAt+runrules::frameMs-1)/runrules::frameMs+1;
    obs[0]={50+speed*ticks,11,24,0,0,true};
    runRewards.startAttack(count*70);
    for(int age=0;age<middleAt;age+=runrules::frameMs) {
      updateAndDrawObstacles(); assert(obs[0].active && runRewards.coins==0);
      runRewards.update(runrules::frameMs);
    }
    updateAndDrawObstacles(); assert(!obs[0].active && runRewards.coins==5 && runRewards.shield>0);
  }
  gameSpeed=runrules::startSpeed;
  // Bonus arcs occupy an obstacle-free spawn event, high enough to require a jump.
  for(auto &o:obs) o.active=false;
  for(auto &p:runPickups) p.active=false;
  runBonusNext=true; spawnRunObstacle();
  int bonus=0;
  for(auto &o:obs) assert(!o.active);
  for(auto &p:runPickups) if(p.active) {
    ++bonus; assert(!p.shield && p.y<=186);
    assert(!runrules::overlaps(37,208,26,52,(int)p.x-6,p.y-6,12,12));
  }
  assert(bonus==5 && runSpawnDistance>=112+44*gameSpeed);
  for(float speed : {runrules::startSpeed,runrules::maxSpeed}) {
    int best=0;
    for(int jump=0;jump<70;++jump) {
      float y=202,vy=0; bool hit[5]={};
      for(int tick=0;tick<100;++tick) {
        if(tick==jump) vy=runrules::jumpVelocity;
        vy+=runrules::gravity; y=min(202.0f,y+vy); if(y==202) vy=0;
        for(int i=0;i<5;++i) {
          auto &p=runPickups[i];
          hit[i] |= runrules::overlaps(37,(int)y+6,26,52,(int)(p.x-speed*(tick+1))-6,p.y-6,12,12);
        }
      }
      int count=0;for(bool h:hit)count+=h;best=max(best,count);
    }
    assert(best==5);
  }
  // A collected coin rises, cannot award again, and clears every pixel on expiry.
  for(auto &p:runPickups) p.active=false;
  for(auto &p:runDebris) p.life=0;
  for(auto &coin:runCollectedCoins) coin.life=0;
  runRewards={}; runnerY=202;
  addRunPickup(54,230,false); updateRunPickups();
  assert(runRewards.coins==1 && runCollectedCoins[0].life==700);
  fullScene();
  assert(tft.pixels[230*240+(int)runCollectedCoins[0].x]==0xFFF0);
  float previous=runCollectedCoins[0].y;
  for(int tick=0;tick<35;++tick) {
    resetWrites(); updateRunCollectedCoins(20); updateRunPickups();
    drawRunner(30,202,202,0,false); singlePass(); matchesFullScene();
    assert(runCollectedCoins[0].y<previous && runRewards.coins==1);
    previous=runCollectedCoins[0].y;
  }
  assert(!runCollectedCoins[0].life && previous<145);
  // Background updates never send a blank notice before its text.
  runMilestone=100; runMilestoneTime=1800; runMilestoneDirty=true;
  fullScene(); auto notice=tft.pixels;
  for(int i=0;i<5;++i) {
    resetWrites(); dirtyRunnerRect(0,48,240,32); renderRunnerDirty(); singlePass();
    for(int y=52;y<76;++y) for(int x=32;x<208;++x)
      assert(tft.pixels[y*240+x]==notice[y*240+x]);
  }
  runMilestoneTime=0; dirtyRunnerRect(32,52,176,24); renderRunnerDirty();
  assert(!runNotice.created()); matchesFullScene();
  // Coin totals have their own row with room before the replay button, even at int limits.
  for(int coins : {0,123,2147483647}) {
    ui::rows.clear(); String value(coins);
    ui::gameOverSheet("Run",2147483647,2147483647,RUN_ACCENT,"Crashed"," m",value.c_str(),"Coins");
    assert(ui::rows.size()==3 && ui::rows[2].label=="Coins" && ui::rows[2].value==value);
    assert(ui::rows.back().bottom+12<=ui::buttonTop && ui::buttonBottom<274);
    assert(ui::cardTop>=43 && ui::badgeBottom<=ui::rows.front().top && ui::rows[0].bottom-ui::rows[0].top>=20);
    assert(ui::cardBottom<=277 && ui::buttonBottom<=ui::hintY-8 && ui::hintY+8<=ui::cardBottom);
  }
  // Existing two-row game-over screens and Rally's distance row retain their spacing.
  ui::rows.clear();
  ui::gameOverSheet("Tetris",123,456,RUN_ACCENT,"Game over",nullptr,nullptr,"Distance");
  assert(ui::rows.size()==2 && ui::rows.back().bottom<ui::buttonTop);
  ui::rows.clear();
  ui::gameOverSheet("Rally",123,456,RUN_ACCENT,"Time up",nullptr,"250 m","Distance");
  assert(ui::rows.size()==3 && ui::rows[2].label=="Distance" && ui::rows.back().bottom<ui::buttonTop);
  assert(ui::cardTop>=43);
  assert(ui::cardBottom<=277 && ui::buttonBottom<=ui::hintY-8 && ui::hintY+8<=ui::cardBottom);
  // Selection changes touch only the portrait, name and indicators.
  auto before=tft.pixels;
  for(int c=0;c<3;++c) {
    runCharacter=c; resetWrites(); drawRunnerChoice(); assert(tft.transfers==1);
    const auto &character=runner_characters[c];
    uint8_t frame[40*60]; decodeRunnerFrame(character.actions[0],0,frame);
    for(int y=0;y<60;++y) for(int x=0;x<40;++x) {
      uint16_t ink=character.palette[frame[y*40+x]];
      assert(tft.pixels[(104+y)*240+100+x]==(ink==VAMPIRE_TRANSPARENT?ui::CARD:ink));
    }
  }
  for(int y=0;y<320;++y) for(int x=0;x<240;++x) {
    bool portrait=x>=100&&x<140&&y>=104&&y<164;
    bool name=x>=32&&x<208&&y>=172&&y<188;
    bool dots=x>=105&&x<=135&&y>=195&&y<=201;
    if(!portrait&&!name&&!dots) assert(tft.pixels[y*240+x]==before[y*240+x]);
  }
  std::cout << "Run HUD/footer, stable notices, bonus arcs, rising coins, impacts and incremental rendering passed\n";
}
'''
code = code.replace('uint16_t blendRun565(uint16_t a,uint16_t,uint8_t) {return a;}', section('static uint16_t blendRun565', 'static const uint16_t RUN_OBS_BRICK'))
with tempfile.TemporaryDirectory() as tmp:
    path = Path(tmp)
    (path / 'Arduino.h').write_text('#pragma once\n#include <stdint.h>\n#define PROGMEM\n')
    (path / 'test.cpp').write_text(code)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-D_GLIBCXX_ASSERTIONS',
                    '-I', str(path), '-I', str(root / 'src'), '-I', str(root / 'lib/TFT_eSPI'), str(path / 'test.cpp'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
