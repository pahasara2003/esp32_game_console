#include <Arduino.h>
#include "game.h"
#include "ui_kit.h"
#include "rally.h"
#include <math.h>

namespace {
// An 8-bit framebuffer keeps heap usage modest on the C3.
TFT_eSprite frame(&tft);
constexpr float length = 24000.f;
float position = 0, speed = 0, lateral = 0;
uint32_t lastFrame = 0;
int lap = 1;
bool paused = false, ready = false;
float elevation(float z) {
  return 170 * sinf(z * TWO_PI / length * 3) +
         85 * sinf(z * TWO_PI / length * 7);
}
float center(float z) {
  return 650 * sinf(z * TWO_PI / length * 2) +
         280 * sinf(z * TWO_PI / length * 5);
}
void span(int y, float left, float right, uint16_t color) {
  int a = max(0, (int)left), b = min(239, (int)right);
  if (b >= a) frame.drawFastHLine(a, y, b-a+1, color);
}
void render(int steer) {
  frame.fillSprite(0x8DFA);
  frame.fillTriangle(0,130,65,63,145,130,0x7C51);
  frame.fillTriangle(80,130,180,52,239,130,0x6BCF);
  frame.fillRect(0,130,240,158,0x4B67);
  float cameraY = elevation(position) + 120;
  float origin = center(position);
  float tangent = (center(position+20)-origin)/20;
  int clip = 287;
  // Near-to-far horizon clipping hides road behind crests. Interpolate each
  // projected segment into scanlines; all geometry is periodic at the lap seam.
  for (int i=0; i<180 && clip>60; ++i) {
    float z1=55+i*22, z2=z1+22;
    float s1=150/z1, s2=150/z2;
    float y1=112-(elevation(position+z1)-cameraY)*s1;
    float y2=112-(elevation(position+z2)-cameraY)*s2;
    if (y2>=y1) continue;
    float x1=120+(center(position+z1)-origin-tangent*z1-lateral)*s1;
    float x2=120+(center(position+z2)-origin-tangent*z2-lateral)*s2;
    int bottom=min(clip,min(287,(int)y1));
    int top=max(0,(int)ceilf(y2));
    bool stripe=((int)((position+z1)/110)&1);
    for(int y=bottom; y>=top; --y) {
      float f=(y1-y)/(y1-y2);
      float x=x1+(x2-x1)*f, w=190*(s1+(s2-s1)*f);
      frame.drawFastHLine(0,y,240,stripe?0x4B67:0x5368);
      span(y,x-w*1.12f,x+w*1.12f,stripe?TFT_WHITE:0xB986);
      span(y,x-w,x+w,0x528A);
      if(stripe) span(y,x-w*.018f,x+w*.018f,0xD69A);
      if (((int)((position+z1)/110)%6)==0) {
        span(y,x-w*1.3f-2,x-w*1.3f+2,TFT_WHITE);
        span(y,x+w*1.3f-2,x+w*1.3f+2,TFT_WHITE);
      }
    }
    if(top<=bottom) clip=top-1;
  }
  int x=120+steer*3;
  // Placeholder rear-view car: wheels, body, glass, lights and spoiler.
  frame.fillEllipse(x,266,30,8,0x2945);
  frame.fillRoundRect(x-27,240,10,29,3,TFT_BLACK);
  frame.fillRoundRect(x+17,240,10,29,3,TFT_BLACK);
  frame.fillTriangle(x-23,259,x-15,224,x+23,259,0xC986);
  frame.fillTriangle(x-15,224,x+15,224,x+23,259,0xEAC8);
  frame.fillTriangle(x-12,228,x+12,228,x+16,242,0x2945);
  frame.fillTriangle(x-12,228,x+16,242,x-16,242,0x2945);
  frame.fillRoundRect(x-24,246,48,20,3,0xD986);
  frame.fillRect(x-19,252,9,5,TFT_RED);
  frame.fillRect(x+10,252,9,5,TFT_RED);
  frame.fillRect(x-7,257,14,4,TFT_WHITE);
  frame.fillRect(x-26,244,52,4,0x18E3);
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
  if(now-lastFrame<40) { delay(1); return; }
  float dt=min(.1f,(now-lastFrame)*.001f); lastFrame=now;
  uint8_t held=buttonsHeld();
  int steer=((held&2)?1:0)-((held&8)?1:0);
  speed += ((held&4)?-700.f:((held&1)?260.f:120.f))*dt;
  speed=constrain(speed,0.f,1050.f);
  float curvature=(center(position+35)-2*center(position)+center(position-35))/(35*35);
  lateral+=steer*(65+speed*.20f)*dt-curvature*speed*speed*dt*.30f;
  lateral=constrain(lateral,-330.f,330.f);
  // Grass limits speed but must not cancel the throttle at low speed.
  // Braking can still stop the car anywhere on the course.
  if(fabsf(lateral)>175 && speed>180.f)
    speed=max(180.f,speed-450*dt);
  position+=speed*dt;
  if(position>=length) { position-=length; ++lap; }
  render(steer);
}
