#include <Arduino.h>
#include "game.h"
#include "ui_kit.h"
#include "rally.h"
#include "rally_grass.h"
#include <math.h>
#include <string.h>

namespace {
// An 8-bit framebuffer keeps heap usage modest on the C3.
TFT_eSprite frame(&tft);
constexpr float length = 24000.f;
float position = 0, speed = 0, lateral = 0, heading = 0;
uint32_t lastFrame = 0;
int lap = 1;
bool paused = false, ready = false;
float center(float z) {
  return 650 * sinf(z * TWO_PI / length * 2) +
         280 * sinf(z * TWO_PI / length * 5);
}
// Fixed north-up drone camera follows the car. Positive heading turns right.
constexpr float mapScale = .24f;
constexpr int carY = 218;
void quad(float x1, float y1, float x2, float y2, float halfWidth,
          uint16_t color) {
  frame.fillTriangle(int(x1-halfWidth),int(y1),int(x1+halfWidth),int(y1),
                     int(x2-halfWidth),int(y2),color);
  frame.fillTriangle(int(x1+halfWidth),int(y1),int(x2+halfWidth),int(y2),
                     int(x2-halfWidth),int(y2),color);
}
void carPart(float left,float top,float right,float bottom,uint16_t color,
             float sn,float cs) {
  // Rotate every body panel and wheel around the same car origin.
  int ax=120+left*cs-top*sn, ay=carY+left*sn+top*cs;
  int bx=120+right*cs-top*sn, by=carY+right*sn+top*cs;
  int cx=120+right*cs-bottom*sn, cy=carY+right*sn+bottom*cs;
  int dx=120+left*cs-bottom*sn, dy=carY+left*sn+bottom*cs;
  frame.fillTriangle(ax,ay,bx,by,cx,cy,color);
  frame.fillTriangle(ax,ay,cx,cy,dx,dy,color);
}
void render() {
  float cameraX=center(position)+lateral;
  // Copy the CC0 grass tile into the 8-bit frame at world-aligned coordinates.
  uint8_t *pixels=static_cast<uint8_t *>(frame.getPointer());
  int textureX=int(floorf(cameraX*mapScale-120)) & (RALLY_GRASS_SIZE-1);
  int textureY=int(floorf(position*mapScale+carY));
  int firstSpan=min(240,RALLY_GRASS_SIZE-textureX);
  for(int y=0;y<288;++y) {
    const uint8_t *source=rallyGrass+(((textureY-y)&(RALLY_GRASS_SIZE-1))*RALLY_GRASS_SIZE);
    uint8_t *row=pixels+y*240;
    memcpy(row,source+textureX,firstSpan);
    if(firstSpan<240) memcpy(row+firstSpan,source,240-firstSpan);
  }
  // About 20 road strips instead of 180 perspective segments and scanlines.
  int first=int(floorf((position-320)/80));
  for(int i=first;i<first+20;++i) {
    float z1=i*80.f,z2=z1+80;
    float x1=120+(center(z1)-cameraX)*mapScale;
    float x2=120+(center(z2)-cameraX)*mapScale;
    float y1=carY-(z1-position)*mapScale;
    float y2=carY-(z2-position)*mapScale;
    quad(x1,y1,x2,y2,49,(i&1)?TFT_WHITE:0xB986);
    quad(x1,y1,x2,y2,45,0x4A49);
    if(i&1) quad(x1,y1,x2,y2,1,0xD69A);
  }
  // Sparse asphalt flecks are fixed to the road, not to screen pixels.
  int firstGrain=int(floorf((position-300.f)/24.f));
  for(int nz=firstGrain;nz<firstGrain+52;++nz) {
    float z=nz*24.f;
    int sy=carY-int((z-position)*mapScale);
    float roadX=120+(center(z)-cameraX)*mapScale;
    for(int nx=-9;nx<=9;++nx) {
      uint32_t grain=uint32_t(nz)*73856093u ^ uint32_t(nx)*19349663u;
      grain^=grain>>16;
      grain*=2246822519u;
      if((grain&7u)>1u) continue;
      int sx=int(roadX+nx*18*mapScale)+int((grain>>8)&3u)-1;
      int y=sy+int((grain>>12)&3u)-1;
      uint16_t color=(grain&0x18u)==0u?TFT_BLACK:0x6B4D;
      frame.fillRect(sx,y,(grain&0x20u)?2:1,1,color);
    }
  }
  float sn=sinf(heading),cs=cosf(heading);
  carPart(-12,-14,12,16,0x2945,sn,cs);
  carPart(-12,-12,-8,-4,TFT_BLACK,sn,cs);
  carPart(8,-12,12,-4,TFT_BLACK,sn,cs);
  carPart(-12,7,-8,15,TFT_BLACK,sn,cs);
  carPart(8,7,12,15,TFT_BLACK,sn,cs);
  carPart(-8,-19,8,18,0xEAC8,sn,cs);
  carPart(-6,-8,6,-2,0x2945,sn,cs);
  carPart(-6,7,6,12,0x2945,sn,cs);
  carPart(-7,-18,-3,-16,TFT_WHITE,sn,cs);
  carPart(3,-18,7,-16,TFT_WHITE,sn,cs);
  carPart(-7,15,-3,18,TFT_RED,sn,cs);
  carPart(3,15,7,18,TFT_RED,sn,cs);
  frame.setTextFont(2);
  frame.setTextColor(TFT_WHITE,0x18E3);
  frame.fillRect(0,0,240,24,0x18E3);
  frame.setCursor(6,4); frame.printf("%d km/h",int(speed*.16f));
  frame.setCursor(157,4); frame.printf("Lap %d",lap);
  frame.pushSprite(0,0);
}
}
void rallySetup() {
  frame.setColorDepth(8);
  ready=frame.createSprite(240,288)!=nullptr;
  tft.fillScreen(TFT_BLACK);
  if(!ready) { ui::heading(20,80,"Rally","Low memory"); return; }
  ui::hint(297,"Left/right steer  Down brake",TFT_BLACK);
  buttonsRequireRelease();
  lastFrame=millis();
}
void rallyLoop() {
  if(!ready) { delay(20); return; }
  int key;
  while((key=buttonsNextPress())>=0) if(key==4) {
    paused=!paused;
    if(paused) {
      const char *items[]={"Shoot: resume","Up: accelerate","Down: brake","Left/right: steer"};
      ui::pauseSheet("Rally",items,4,0,0xEAC8);
    }
    lastFrame=millis();
  }
  if(paused) { delay(5); return; }
  uint32_t now=millis();
  if(now-lastFrame<20) { delay(1); return; }
  float dt=min(.1f,(now-lastFrame)*.001f); lastFrame=now;
  uint8_t held=buttonsHeld();
  int steer=((held&2)?1:0)-((held&8)?1:0);
  bool onRoad=fabsf(lateral)<=175.f;
  float acceleration=(held&4)?-700.f:((held&1)?(onRoad?140.f:90.f)
                                                :(onRoad?75.f:-80.f));
  speed=constrain(speed+acceleration*dt,0.f,600.f);
  // Steering changes both the visible orientation and direction of travel.
  // Limit heading to forward travel for this endless northbound course.
  heading+=steer*2.7f*dt*(.55f+.45f*speed/600.f);
  heading=constrain(heading,-1.25f,1.25f);
  // Grass drag has no minimum speed; releasing Up lets the car stop completely.
  if(!onRoad) speed=max(0.f,speed-min(450.f,speed*4.f)*dt);
  float oldCenter=center(position);
  float newPosition=position+cosf(heading)*speed*dt;
  lateral+=sinf(heading)*speed*dt-(center(newPosition)-oldCenter);
  lateral=constrain(lateral,-600.f,600.f);
  position=newPosition;
  if(position>=length) { position-=length; ++lap; }
  uint32_t renderStart=micros();
  render();
  // Low-rate telemetry measures real frame throughput, including SPI transfer.
  static uint32_t reportAt=0, frames=0, renderUs=0;
  ++frames; renderUs+=micros()-renderStart;
  if(now-reportAt>=2000) {
    if(Serial.availableForWrite()>80)
      Serial.printf("[Rally] fps=%.1f render_ms=%.1f\n",
                    frames*1000.f/(now-reportAt),renderUs/(frames*1000.f));
    reportAt=now; frames=0; renderUs=0;
  }
}
