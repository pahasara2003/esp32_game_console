#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "birthday.h"

// A short birthday story in six scenes:
//   1. Intro    - a beating heart and a typed greeting
//   2. Gift     - a wiggling present; Shoot opens it
//   3. Party    - cake, fireworks, confetti and the birthday song
//   4. Wish     - "make a wish"; Shoot blows out the candles
//   5. Notes    - a few wish cards (Left/Right or Shoot to turn)
//   6. Finale   - balloons, a heart and a closing message
// Up replays from the start, Down leaves at any time.

namespace birthday {
constexpr int WIDTH = 240, HEIGHT = 320, BAND = 80;
constexpr float PI_F = 3.14159265f;
constexpr uint16_t INK = 0xFFFF, GOLD = 0xFEAA, PINK = 0xFB98;
constexpr uint16_t LILAC = 0xB3FF, AQUA = 0x5FBA, CORAL = 0xFD0F;
constexpr uint16_t FOOT = 0xA4F6, CARDC = 0x2127;
constexpr const char *NAME = "Chamathka";
constexpr const char *RECIPIENT = "Chamathka!";
constexpr const char *WISH[] = {"A little console. A world of joy.", "Made for your next adventure."};

struct Page { const char *lines[3]; };
constexpr Page PAGES[] = {
  {{"Another year of you,", "and the world is", "brighter for it."}},
  {{"May this year bring", "laughter, adventures", "and dreams come true."}},
  {{"Keep smiling,", "keep shining,", "keep being you."}},
  {{"Every level of life", "is better with you", "in it. Play on!"}},
};
constexpr int PAGE_COUNT = sizeof(PAGES) / sizeof(PAGES[0]);

TFT_eSprite frame(&tft);
int origin = 0;
uint16_t background[HEIGHT];
constexpr int TILE = 16;
static_assert(WIDTH%TILE==0 && HEIGHT%TILE==0 && BAND%TILE==0,
              "Dirty tiles must cover each complete band");
uint32_t tileHash[(WIDTH/TILE)*(HEIGHT/TILE)];
uint16_t changedPixels[WIDTH*TILE];
bool cacheValid = false;

void pushChangedBand() {
  auto *pixels=static_cast<const uint16_t *>(frame.getPointer());
  bool oldSwap=tft.getSwapBytes();
  tft.setSwapBytes(false); // Sprite storage already has display byte order.
  tft.startWrite();
  for(int y=0;y<BAND;y+=TILE) {
    bool dirty[WIDTH/TILE];
    for(int x=0;x<WIDTH;x+=TILE) {
      uint32_t hash=2166136261u;
      for(int dy=0;dy<TILE;++dy)
        for(int dx=0;dx<TILE;++dx)
          hash=(hash^pixels[(y+dy)*WIDTH+x+dx])*16777619u;
      int index=((origin+y)/TILE)*(WIDTH/TILE)+x/TILE;
      dirty[x/TILE]=!cacheValid || hash!=tileHash[index];
      tileHash[index]=hash;
    }
    for(int tile=0;tile<WIDTH/TILE;) {
      if(!dirty[tile]) { ++tile;continue; }
      int first=tile;
      while(tile<WIDTH/TILE && dirty[tile]) ++tile;
      int x=first*TILE,w=(tile-first)*TILE;
      for(int dy=0;dy<TILE;++dy)
        memcpy(changedPixels+dy*w,pixels+(y+dy)*WIDTH+x,w*sizeof(uint16_t));
      tft.pushImage(x,origin+y,w,TILE,changedPixels);
    }
  }
  tft.endWrite();
  tft.setSwapBytes(oldSwap);
}

// ---- Drawing primitives (all in screen coordinates) -------------------------
uint16_t mix(uint16_t a, uint16_t b, float f) {
  f = f < 0 ? 0 : f > 1 ? 1 : f;
  int weight = int(f * 255);
  int r = (((a >> 11) & 31) * (255-weight) + ((b >> 11) & 31) * weight) / 255;
  int g = (((a >> 5) & 63) * (255-weight) + ((b >> 5) & 63) * weight) / 255;
  int blue = ((a & 31) * (255-weight) + (b & 31) * weight) / 255;
  return (r << 11) | (g << 5) | blue;
}
float progress(float t, float start, float duration) {
  float x = (t-start)/duration;
  return x < 0 ? 0 : x > 1 ? 1 : x;
}
float ease(float x) { return 1 - (1-x)*(1-x)*(1-x); }
uint32_t hash(uint32_t n) {
  n ^= n >> 16; n *= 0x7feb352d; n ^= n >> 15;
  n *= 0x846ca68b; return n ^ (n >> 16);
}
uint16_t bgAt(int y) { return background[constrain(y, 0, HEIGHT-1)]; }
void rect(int x, int y, int w, int h, uint16_t color, int radius=0) {
  if (radius) frame.fillRoundRect(x, y-origin, w, h, radius, color);
  else frame.fillRect(x, y-origin, w, h, color);
}
void line(int x, int y, int x2, int y2, uint16_t color) {
  frame.drawLine(x, y-origin, x2, y2-origin, color);
}
void circle(int x, int y, int r, uint16_t color) {
  if (r > 0) frame.fillCircle(x, y-origin, r, color);
}
void text(const char *label, int y, int font, uint16_t color, int center=120) {
  frame.setTextFont(font);
  frame.setTextDatum(MC_DATUM);
  frame.setTextColor(color);
  frame.drawString(label, center, y-origin);
}
// Text that fades in from the background colour (a = 0..1).
void fadeText(const char *label, int y, int font, uint16_t color, float a, int center=120) {
  if (a <= 0.01f) return;
  text(label, y, font, mix(bgAt(y), color, a), center);
}
// Typewriter: the first `shown` characters, positioned so the finished line
// is centred. A blinking caret follows while it is still typing.
void typeText(const char *label, int y, int font, uint16_t color, int shown, float t) {
  int len = strlen(label);
  shown = constrain(shown, 0, min(len, 47));
  if (shown == 0) return;
  char buf[48];
  memcpy(buf, label, shown); buf[shown] = 0;
  int x0 = 120 - frame.textWidth(label, font) / 2;
  frame.setTextFont(font);
  frame.setTextDatum(ML_DATUM);
  frame.setTextColor(color);
  frame.drawString(buf, x0, y-origin);
  if (shown < len && fmodf(t, 0.5f) < 0.3f) {
    int h = font == 4 ? 20 : 12;
    rect(x0 + frame.textWidth(buf, font) + 2, y - h/2, 2, h, color);
  }
}
void sparkle(int x, int y, int radius, uint16_t color) {
  line(x-radius, y, x+radius, y, color);
  line(x, y-radius, x, y+radius, color);
  circle(x, y, 1, INK);
}
// Heart centred on (cx, cy); r is the radius of each lobe.
void heart(int cx, int cy, float r, uint16_t color) {
  int top = cy - int(r * 0.5f);
  circle(cx - int(r*0.8f), top, int(r), color);
  circle(cx + int(r*0.8f), top, int(r), color);
  frame.fillTriangle(cx - int(r*1.75f), top + int(r*0.35f) - origin,
                     cx + int(r*1.75f), top + int(r*0.35f) - origin,
                     cx, top + int(r*2.0f) - origin, color);
  circle(cx - int(r*1.05f), top - int(r*0.25f), max(1, int(r*0.28f)), mix(color, INK, 0.55f));
}
void star(int cx, int cy, int r, uint16_t color) {
  for (int k = -1; k <= 1; ++k) {
    line(cx-r, cy+k, cx+r, cy+k, color);
    line(cx+k, cy-r, cx+k, cy+r, color);
  }
  int d = r * 6 / 10;
  line(cx-d, cy-d, cx+d, cy+d, color);
  line(cx-d, cy+d, cx+d, cy-d, color);
  circle(cx, cy, r/3, INK);
}
void flower(int cx, int cy, uint16_t petal) {
  for (int i = 0; i < 5; ++i) {
    float a = i * 2 * PI_F / 5 - PI_F / 2;
    circle(cx + int(cosf(a) * 9), cy + int(sinf(a) * 9), 7, petal);
  }
  circle(cx, cy, 6, GOLD);
}
void controller(int cx, int cy) {
  rect(cx-22, cy-12, 44, 24, LILAC, 10);
  rect(cx-15, cy-2, 12, 4, INK);
  rect(cx-11, cy-6, 4, 12, INK);
  circle(cx+10, cy-3, 3, GOLD);
  circle(cx+15, cy+3, 3, PINK);
}
void balloon(int x, int y, uint16_t color, float t, int i) {
  // String first so the balloon sits on top of it.
  int sx = x, sy = y + 15;
  for (int k = 1; k <= 4; ++k) {
    int nx = x + int(3 * sinf(t * 3 + i + k)), ny = y + 15 + k * 7;
    line(sx, sy, nx, ny, mix(bgAt(ny), INK, 0.45f));
    sx = nx; sy = ny;
  }
  frame.fillEllipse(x, y-origin, 11, 14, color);
  frame.fillEllipse(x-4, y-5-origin, 2, 4, mix(color, INK, 0.6f));
  frame.fillTriangle(x-3, y+16-origin, x+3, y+16-origin, x, y+12-origin, color);
}
void footer(const char *label, float a = 1) { fadeText(label, 305, 1, FOOT, a); }

// ---- Shared layers -----------------------------------------------------------
void atmosphere(float t) {
  for (int y = 0; y < BAND; ++y)
    frame.drawFastHLine(0, y, WIDTH, background[origin+y]);
  // Diffuse coloured light, composed beneath the text and illustration.
  for (int r = 71; r > 0; r -= 3) {
    float glow = (1-r/72.0f)*0.25f;
    circle(120, 212, r, mix(background[212], LILAC, glow));
  }
  for (int i = 0; i < 39; ++i) {
    uint32_t h = hash(i+71);
    int x = 8 + h%224, y = 9 + (h>>9)%285;
    float light = 0.28f + 0.45f*(0.5f+0.5f*sinf(t*1.5f+i*2.13f));
    uint16_t color = mix(background[y], i%3 ? LILAC : GOLD, light);
    frame.drawPixel(x, y-origin, color);
    if (i%9==0 && light>0.64f) sparkle(x, y, 2, color);
  }
  // A quiet, inset frame leaves the footer and all content inside safe margins.
  frame.drawRoundRect(6, 6-origin, 228, 308, 16, 0x2108);
  line(20, 27, 39, 27, mix(0x2108, GOLD, 0.55f));
  line(201, 27, 220, 27, mix(0x2108, GOLD, 0.55f));
}

void firework(float t, int index) {
  float age = fmodf(t + index*1.69f, 6.8f);
  if (t < 0 || age > 1.35f) return;
  int cx = index%2 ? 201:38;
  int cy = index < 2 ? 77:178;
  uint16_t base = index%3==0 ? GOLD:index%3==1 ? PINK:AQUA;
  float size = 2 + 33*(1-powf(1-age/1.35f, 2));
  float fade = 1-age/1.35f;
  for (int i=0; i<13; ++i) {
    float a=i*2*PI_F/13 + index*0.4f;
    int x=cx+int(cosf(a)*size), y=cy+int(sinf(a)*size+age*age*7);
    uint16_t color=mix(background[cy], base, fade);
    line(cx+int(cosf(a)*(size-4)), cy+int(sinf(a)*(size-4)+age*age*7), x, y, color);
    circle(x,y,1,mix(color,INK,fade*0.3f));
  }
}

void confetti(float t) {
  const uint16_t colors[] = {GOLD, PINK, AQUA, LILAC, CORAL};
  for (int i=0; i<36; ++i) {
    uint32_t h=hash(i+601);
    float speed=18+(h%19);
    float y=fmodf(t*speed + ((h>>6)%390), 390.0f)-35;
    int x=8+((h>>16)%224)+int(8*sinf(t*1.4f+i));
    // Confetti stays clear of the central birthday lettering and cake.
    if (x>37 && x<203 && y>38 && y<254) continue;
    if (y>260) continue;
    int w=1+int(2*fabsf(sinf(t*3+i))), yy=int(y);
    rect(x, yy, w, 4, colors[i%5], 1);
  }
}

// Cake coordinates scale around the centre of its plate, so its entrance
// cannot spill into the text above or the message below. `flame` 0..1
// shrinks the candle flames (0 = blown out).
void cake(float t, float reveal, float flame) {
  float scale=0.12f+0.88f*ease(reveal);
  auto X=[scale](int x){return 120+int((x-120)*scale);};
  auto Y=[scale](int y){return 245+int((y-245)*scale);};
  auto block=[&](int x,int y,int w,int h,uint16_t color,int radius=0) {
    rect(X(x), Y(y), max(1,int(w*scale)), max(1,int(h*scale)), color,
         radius ? max(1,int(radius*scale)):0);
  };
  frame.fillEllipse(120,Y(248)-origin,int(78*scale),max(1,int(7*scale)),0x0804);
  frame.fillEllipse(120,Y(244)-origin,int(76*scale),max(1,int(6*scale)),0xC55C);
  frame.fillEllipse(120,Y(242)-origin,int(70*scale),max(1,int(4*scale)),0xEF5F);
  block(61,209,118,32,0xD316,6);
  for(int x=64;x<176;++x)
    line(X(x),Y(216),X(x),Y(238),mix(0xFB34,0xB2DA,(x-64)/112.0f));
  block(61,204,118,14,0xFF5B,6);
  for(int i=0;i<8;++i) block(68+i*14,213,6,6+(i%3)*3,0xFF5B,3);
  block(77,180,86,29,0xC45F,5);
  for(int x=79;x<161;++x)
    line(X(x),Y(189),X(x),Y(204),mix(0xE51F,0xAB5D,(x-79)/82.0f));
  block(75,178,90,13,0xFFBC,5);
  for(int i=0;i<6;++i) block(82+i*13,187,5,5+(i%2)*5,0xFFBC,2);
  for(int i=0;i<11;++i) {
    int x=68+i*10,y=224+(i%3)*4;
    line(X(x),Y(y),X(x+2),Y(y+2),i%2 ? GOLD:0xF7FF);
  }
  for(int i=0;i<3;++i) {
    int x=96+i*24;
    block(x-2,160,5,20,i==1 ? AQUA:PINK,1);
    for(int y=163;y<178;y+=5) line(X(x-1),Y(y),X(x+1),Y(y+2),INK);
    if (reveal>0.85f && flame>0.02f) {
      int flicker=int(1.5f*sinf(t*8+i*2));
      circle(X(x),Y(155),max(1,int(7*scale*flame)),0x6A88);
      circle(X(x),Y(154+flicker),max(1,int(4*scale*flame)),0xFD68);
      int tip = 155 - int(9*flame);
      frame.fillTriangle(X(x-3),Y(155)-origin,X(x+3),Y(155)-origin,
                         X(x+flicker),Y(tip)-origin,GOLD);
      circle(X(x),Y(155),max(1,int(2*scale)),0xFFFC);
    } else if (reveal>0.85f) {
      circle(X(x), Y(158), 1, 0x4208);  // charred wick
    }
  }
}

// ---- Scene 1: intro ---------------------------------------------------------
char greeting[32];
constexpr float L1_AT = 0.9f, L1_CPS = 15, L2_AT = 2.2f, L3_AT = 3.5f, L23_CPS = 20;
constexpr const char *INTRO_L2 = "today is all about you.";
constexpr const char *INTRO_L3 = "I made something for you...";
constexpr float INTRO_END = 6.1f;

int introTyped(float st) {
  auto n = [](float st, float at, float cps, const char *s) {
    return constrain(int((st - at) * cps), 0, (int)strlen(s));
  };
  return n(st, L1_AT, L1_CPS, greeting) + n(st, L2_AT, L23_CPS, INTRO_L2) +
         n(st, L3_AT, L23_CPS, INTRO_L3);
}

void intro(float t, float st) {
  float out = 1 - progress(st, INTRO_END - 0.5f, 0.5f);
  float appear = ease(progress(st, 0, 0.8f)) * out;
  // Heartbeat: two quick pulses, then a rest.
  float beat = fmodf(st, 1.1f);
  float pulse = beat < 0.12f ? beat/0.12f : beat < 0.24f ? 1-(beat-0.12f)/0.12f :
                beat < 0.36f ? (beat-0.24f)/0.12f*0.6f : beat < 0.5f ? 0.6f*(1-(beat-0.36f)/0.14f) : 0;
  for (int r = 46; r > 14; r -= 4)
    circle(120, 104, r, mix(bgAt(104), PINK, (1 - r/48.0f) * 0.22f * appear * (0.7f + 0.3f*pulse)));
  if (appear > 0.02f) heart(120, 104, (10 + 2.2f*pulse) * (0.6f + 0.4f*appear), mix(bgAt(104), PINK, appear));
  // Little sparks circle the heart.
  for (int i = 0; i < 6; ++i) {
    float a = t * 0.9f + i * PI_F / 3;
    sparkle(120 + int(cosf(a) * 40), 104 + int(sinf(a) * 34), 1 + (i%2),
            mix(bgAt(104), i%2 ? GOLD : AQUA, appear));
  }
  auto shown = [&](float at, float cps, const char *s) { return int((st - at) * cps); };
  uint16_t white = mix(bgAt(180), INK, out), lilac = mix(bgAt(232), LILAC, out);
  typeText(greeting, 180, 4, white, shown(L1_AT, L1_CPS, greeting), t);
  typeText(INTRO_L2, 212, 2, mix(bgAt(212), INK, out * 0.85f), shown(L2_AT, L23_CPS, INTRO_L2), t);
  typeText(INTRO_L3, 236, 2, lilac, shown(L3_AT, L23_CPS, INTRO_L3), t);
  footer("SHOOT skip     DOWN exit", appear);
}

// ---- Scene 2: gift ----------------------------------------------------------
constexpr float GIFT_AUTO_OPEN = 7.0f;
void gift(float t, float st, float openAt) {
  float open = openAt < 0 ? 0 : ease(progress(st, openAt, 0.6f));
  float exit = openAt < 0 ? 0 : ease(progress(st, openAt + 0.6f, 0.3f));
  float enter = ease(progress(st, 0, 0.6f));
  // Bob gently, and wiggle every couple of seconds until opened.
  int bob = openAt < 0 ? int(3 * sinf(st * 2.4f)) : 0;
  float w = fmodf(st, 2.0f);
  int shake = (openAt < 0 && st > 1 && w < 0.4f) ? int(4 * sinf(w * 40) * (1 - w/0.4f)) : 0;
  int cx = 120 + shake;
  int cy = 174 + bob + int(exit*240) + int((1-enter)*30);
  float pulse = 0.5f + 0.5f*sinf(t*4);
  for (int r=47; r>26; r-=3)
    circle(cx, cy, r, mix(0x1007, LILAC, (48-r)/180.0f*enter));
  for (int i=0; i<10; ++i) {
    float a = t*1.1f + i*2*PI_F/10;
    int r = 49 + int(5*sinf(t*2+i));
    sparkle(cx+int(cosf(a)*r), cy+int(sinf(a)*r*0.82f),
            i%3==0 ? 2:1, mix(bgAt(cy), i%2 ? AQUA:GOLD, enter));
  }
  // Ribbon, shaded box and an opening lid.
  int lift = int(open*57);
  rect(cx-31, cy-11, 62, 43, mix(0x81B4, PINK, 0.6f), 5);
  for (int x=cx-29; x<cx+29; ++x)
    line(x, cy-8, x, cy+29, mix(PINK, LILAC, (x-cx+29)/75.0f));
  rect(cx-5, cy-11, 10, 43, GOLD);
  rect(cx-34-int(open*9), cy-20-lift, 68, 13, CORAL, 3);
  rect(cx-5-int(open*9), cy-20-lift, 10, 13, 0xFFF2);
  int bowX=cx-int(open*9), bowY=cy-23-lift;
  frame.drawEllipse(bowX-8, bowY-origin, 9, 5, GOLD);
  frame.drawEllipse(bowX+8, bowY-origin, 9, 5, GOLD);
  circle(bowX, bowY+2, 3, 0xFFF2);
  if (open>0) {
    // Light pours out, then a ring of sparkles bursts away.
    for (int r = 30; r > 4; r -= 5)
      circle(cx, cy-14, int(r*open), mix(bgAt(cy), GOLD, (1-r/32.0f)*0.6f*(1-exit)));
    for (int i=0; i<16; ++i) {
      float a = i*2*PI_F/16;
      float r = 15 + open*95;
      sparkle(cx+int(cosf(a)*r), cy-15+int(sinf(a)*r),
              1+(i%3==0), mix(0x1007, i%3==0 ? GOLD : i%3==1 ? PINK : INK, 1-open*0.7f));
    }
  }
  float fade = openAt < 0 ? enter : 1 - progress(st, openAt, 0.4f);
  fadeText("YOU FOUND A LITTLE SECRET", 72, 1, LILAC, fade);
  fadeText("A present, just for you", 92, 2, INK, fade);
  if (openAt < 0 && st > 0.8f)
    fadeText("Press SHOOT to open it", 262, 2, GOLD, fade * (0.55f + 0.45f*pulse));
  footer("SHOOT open     DOWN exit", fade);
}

// ---- Scene 3: party ---------------------------------------------------------
void party(float st) {
  float reveal=ease(progress(st,0,0.85f));
  float heading=ease(progress(st,0.1f,0.75f));
  float name=ease(progress(st,0.6f,0.8f));
  float wish=ease(progress(st,1.1f,0.9f));
  for(int i=0;i<4;++i) firework(st,i);
  confetti(st);
  fadeText("A SURPRISE JUST FOR YOU",23,1,GOLD,heading);
  if (heading>0) {
    int offset=int((1-heading)*12);
    fadeText("HAPPY",55+offset,4,INK,heading);
    fadeText("BIRTHDAY",86+offset,4,PINK,heading);
  }
  if (name>0) {
    int y=121+int((1-name)*7);
    text(RECIPIENT,y+1,4,mix(background[121],0x520C,name));
    text(RECIPIENT,y,4,mix(background[121],GOLD,name));
  }
  if (reveal>0) cake(st,reveal,1);
  fadeText(WISH[0],271,1,INK,wish);
  fadeText(WISH[1],285,1,LILAC,wish);
  footer("SHOOT make a wish     DOWN exit", wish);
}

// ---- Scene 4: wish ----------------------------------------------------------
constexpr float WISH_AUTO_BLOW = 14.0f, WISH_AFTER = 3.6f;
void wishScene(float t, float st, float blowAt) {
  bool blown = blowAt >= 0;
  float since = blown ? st - blowAt : 0;
  // Party heading melts away while the new one arrives.
  float old = 1 - progress(st, 0, 0.4f);
  if (old > 0) {
    fadeText("HAPPY",55,4,INK,old);
    fadeText("BIRTHDAY",86,4,PINK,old);
    fadeText(RECIPIENT,121,4,GOLD,old);
  }
  float flame = blown ? 1 - progress(since, 0, 0.25f) : 1;
  cake(t, 1, flame);

  float ask = ease(progress(st, 0.3f, 0.6f)) * (blown ? 1 - progress(since, 0, 0.4f) : 1);
  fadeText("Make a wish", 44, 4, GOLD, ask);
  fadeText("Close your eyes and", 80, 2, INK, ask * ease(progress(st, 0.7f, 0.6f)));
  fadeText("think of something lovely.", 100, 2, LILAC, ask * ease(progress(st, 1.1f, 0.6f)));
  if (!blown && st > 2.0f) {
    float pulse = 0.55f + 0.45f*sinf(t*4);
    fadeText("Press SHOOT to blow", 270, 2, INK, ease(progress(st, 2.0f, 0.5f)) * pulse);
    fadeText("out the candles", 288, 2, INK, ease(progress(st, 2.0f, 0.5f)) * pulse);
  }
  if (!blown) { footer("UP replay     DOWN exit", ask); return; }

  // Smoke curls up from each wick.
  for (int c = 0; c < 3; ++c) {
    int wx = 96 + c * 24;
    for (int k = 0; k < 6; ++k) {
      float age = since - 0.1f - k * 0.14f;
      if (age < 0 || age > 1.7f) continue;
      int y = 150 - int(age * 42), x = wx + int(5 * sinf(age * 5 + k + c));
      circle(x, y, 1 + int(age * 2.2f), mix(bgAt(y), 0x8C71, (1 - age/1.7f) * 0.7f));
    }
  }
  // Confetti bursts out of the cake.
  float b = since - 0.25f;
  if (b > 0 && b < 2.2f) {
    const uint16_t colors[] = {GOLD, PINK, AQUA, LILAC, CORAL, INK};
    for (int i = 0; i < 40; ++i) {
      uint32_t h = hash(i + 911);
      float a = -PI_F/2 + ((h % 1000) / 1000.0f - 0.5f) * 2.6f;
      float v = 90 + (h >> 10) % 110;
      int x = 120 + int(cosf(a) * v * b), y = 170 + int(sinf(a) * v * b + 70 * b * b);
      if (y < 10 || y > 300 || x < 8 || x > 232) continue;
      int sz = 2 + int(1.5f * fabsf(sinf(b * 6 + i)));
      rect(x, y, sz, 2 + (i%2) * 2, mix(colors[i%6], bgAt(y), progress(b, 1.4f, 0.8f)));
    }
  }
  for (int i = 0; i < 4; ++i) firework(since - 0.3f, i);
  float yay = ease(progress(since, 0.4f, 0.6f));
  fadeText("Wish granted!", 44 + int((1-yay)*8), 4, PINK, yay);
  fadeText("May every bit of it", 80, 2, INK, ease(progress(since, 0.8f, 0.6f)));
  fadeText("come true.", 100, 2, LILAC, ease(progress(since, 1.1f, 0.6f)));
  fadeText("Happy birthday!", 276, 2, GOLD, ease(progress(since, 1.4f, 0.6f)));
  footer("UP replay     DOWN exit", yay);
}

// ---- Scene 5: notes ---------------------------------------------------------
constexpr float PAGE_TIME = 4.6f;
void notes(float t, float st, int page, float pageSt, int dir) {
  float in = ease(progress(st, 0, 0.5f));
  fadeText("A few wishes for you", 40, 2, GOLD, in);
  // Card.
  int cardY = 62 + int((1-in)*20);
  rect(22, cardY, 196, 182, mix(bgAt(150), CARDC, in), 14);
  frame.drawRoundRect(22, cardY-origin, 196, 182, 14, mix(bgAt(150), LILAC, 0.35f*in));
  // Content slides in from the side it came from.
  float s = ease(progress(pageSt, 0, 0.4f)) * in;
  int dx = int((1 - s) * 28 * dir);
  uint16_t base = mix(bgAt(150), CARDC, in);
  float bob = 3 * sinf(t * 2.5f);
  if (s > 0.05f) {
    switch (page % 4) {
      case 0: heart(120 + dx, cardY + 44 + int(bob), 11, mix(base, PINK, s)); break;
      case 1: star(120 + dx, cardY + 44 + int(bob), 14, mix(base, GOLD, s)); break;
      case 2: flower(120 + dx, cardY + 44 + int(bob), mix(base, PINK, s)); break;
      default: controller(120 + dx, cardY + 44 + int(bob)); break;
    }
  }
  for (int i = 0; i < 3; ++i) {
    float li = ease(progress(pageSt, 0.1f + i * 0.12f, 0.4f)) * in;
    if (li <= 0.01f) continue;
    text(PAGES[page].lines[i], cardY + 104 + i * 22, 2,
         mix(base, i == 2 ? LILAC : INK, li), 120 + int((1-li) * 20 * dir));
  }
  // Page dots.
  for (int i = 0; i < PAGE_COUNT; ++i) {
    int x = 120 - (PAGE_COUNT - 1) * 7 + i * 14;
    if (i == page) rect(x - 6, 262, 12, 5, mix(bgAt(262), GOLD, in), 2);
    else circle(x, 264, 2, mix(bgAt(264), FOOT, in * 0.7f));
  }
  footer("LEFT/RIGHT turn   SHOOT next   DOWN exit", in);
}

// ---- Scene 6: finale --------------------------------------------------------
void finale(float t, float st) {
  const uint16_t colors[] = {PINK, GOLD, AQUA, LILAC, CORAL, 0x87F0};
  for (int i = 0; i < 4; ++i) firework(st + 0.6f, i);
  for (int i = 0; i < 9; ++i) {
    uint32_t h = hash(i + 313);
    float speed = 24 + h % 20;
    float y = 360 - fmodf(st * speed + (h >> 8) % 420, 420.0f);
    int lane = 24 + (i * 193 / 8) % 192;
    int x = lane + int(7 * sinf(st * 1.3f + i));
    balloon(x, int(y), colors[i % 6], t, i);
  }
  float a = ease(progress(st, 0.2f, 0.7f));
  float n = ease(progress(st, 0.6f, 0.7f));
  fadeText("Happy Birthday,", 92 + int((1-a)*10), 4, INK, a);
  if (n > 0) {
    int y = 126 + int((1-n)*8);
    text(RECIPIENT, y+1, 4, mix(bgAt(y), 0x520C, n));
    text(RECIPIENT, y, 4, mix(bgAt(y), GOLD, n));
  }
  float h = ease(progress(st, 1.0f, 0.6f));
  float beat = fmodf(st, 1.1f);
  float pulse = beat < 0.15f ? beat/0.15f : beat < 0.3f ? 1-(beat-0.15f)/0.15f : 0;
  for (int r = 30; r > 10; r -= 4)
    circle(120, 178, r, mix(bgAt(178), PINK, (1 - r/32.0f) * 0.25f * h));
  if (h > 0.02f) heart(120, 178, (8 + 1.6f*pulse) * h, mix(bgAt(178), PINK, h));
  fadeText("With love, always.", 226, 2, PINK, ease(progress(st, 1.5f, 0.6f)));
  fadeText("Have the best year yet!", 248, 2, LILAC, ease(progress(st, 1.9f, 0.6f)));
  footer("UP replay     SHOOT exit", ease(progress(st, 2.4f, 0.6f)));
}

// ---- Sound ------------------------------------------------------------------
struct Note { uint16_t frequency, beats; };
const Note SONG[] = {
  {262,1},{262,1},{294,2},{262,2},{349,2},{330,4},
  {262,1},{262,1},{294,2},{262,2},{392,2},{349,4},
  {262,1},{262,1},{523,2},{440,2},{349,2},{330,2},{294,4},
  {466,1},{466,1},{440,2},{349,2},{392,2},{349,4}
};
const Note TICK[]   = {{2093,1}};
const Note CHIME[]  = {{1047,1},{1319,1},{1568,1},{2093,3}};
const Note PAGE[]   = {{1568,1},{2093,1}};
const Note BLOW[]   = {{1200,1},{1000,1},{820,1},{660,1},{520,1},{0,2},{1568,1},{1976,1},{2349,1},{3136,4}};
const Note TADA[]   = {{1047,1},{1319,1},{1568,1},{2093,2},{1568,1},{2093,4}};

class Player {
 public:
  void play(const Note *s, int n, uint16_t beatMs, uint8_t mult, unsigned long now) {
    seq = s; count = n; beat = beatMs; mul = mult; index = 0; ringing = false; at = now; active = true;
    setBuzzerTone(0);
  }
  void stop() { active = false; setBuzzerTone(0); }
  bool busy() const { return active; }
  void update(unsigned long now) {
    if (!active) return;
    if (index >= count) { stop(); return; }
    if (!ringing) {
      setBuzzerTone(seq[index].frequency ? seq[index].frequency * mul : 0);
      at = now; ringing = true;
    }
    unsigned long d = seq[index].beats * beat;
    if (now - at >= d * 8 / 10) setBuzzerTone(0);
    if (now - at >= d) { ringing = false; ++index; }
  }
 private:
  const Note *seq = nullptr;
  int count = 0, index = 0;
  uint16_t beat = 100;
  uint8_t mul = 1;
  unsigned long at = 0;
  bool ringing = false, active = false;
};
#define PLAY(p, seq, beatMs, mult, now) (p).play(seq, sizeof(seq)/sizeof(seq[0]), beatMs, mult, now)

int command() {
  int c=buttonsNextPress();
  if (Serial.available()) {
    char key=Serial.read();
    if(key=='w'||key=='W') c=0;
    if(key=='d'||key=='D') c=1;
    if(key=='s'||key=='S') c=2;
    if(key=='a'||key=='A') c=3;
    if(key==' '||key=='e'||key=='E'||key=='q'||key=='Q'||key=='\r'||key=='\n') c=4;
  }
  return c;
}

enum Scene { INTRO, GIFT, PARTY, WISH_S, NOTES, FINALE };
const char *const SCENE_NAMES[] = {"intro", "gift", "party", "wish", "notes", "finale"};
}  // namespace birthday

void showBirthdayScene() {
  using namespace birthday;
  // The 38 KB band buffer avoids a 154 KB framebuffer allocation on the C3.
  // Each band is fully composed before transfer: no erase/redraw flicker.
  frame.setColorDepth(16);
  if (!frame.createSprite(WIDTH,BAND)) {
    tft.fillScreen(0x0804);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4);tft.setTextColor(INK);
    tft.drawString("Happy Birthday",120,123);
    tft.setTextColor(GOLD);tft.drawString(RECIPIENT,120,158);
    tft.setTextFont(1);tft.drawString("SHOOT TO RETURN",120,277);
    buttonsRequireRelease();
    while(command()!=4) delay(5);
    return;
  }
  snprintf(greeting, sizeof(greeting), "Hey %s,", NAME);
  for(int y=0;y<HEIGHT;++y) background[y]=mix(0x0803,0x180B,y/319.0f);
  buttonsRequireRelease();

  Player player;
  Scene scene = INTRO;
  unsigned long started = millis(), sceneAt = started, lastFrame = 0;
  float openAt = -1, blowAt = -1, pageAt = 0;
  int page = 0, dir = 1, typed = 0;
  bool songStarted = false;
  unsigned long frames = 0, rateWindow = started;

  auto go = [&](Scene s, unsigned long now) {
    scene = s; sceneAt = now; openAt = -1; blowAt = -1;
    page = 0; dir = 1; pageAt = 0; typed = 0; songStarted = false;
    if (s != PARTY) player.stop();
    if (s == FINALE) PLAY(player, TADA, 110, 1, now);
    Serial.printf("[Birthday] Scene: %s\n", SCENE_NAMES[s]);
  };
  go(INTRO, started);
  Serial.println("[Birthday] Happy Birthday Chamathka!");

  for (;;) {
    unsigned long now = millis();
    float st = (now - sceneAt) / 1000.0f;
    int c = command();
    if (c == 2) break;
    if (c == 0 && now - started > 500) {
      started = now; go(INTRO, now); buttonsRequireRelease();
      Serial.println("[Birthday] Replay");
      continue;
    }

    // Input and automatic progress for each scene.
    switch (scene) {
      case INTRO: {
        int n = introTyped(st);
        if (n > typed) { typed = n; if (!player.busy()) PLAY(player, TICK, 18, 1, now); }
        if (c == 4 || st >= INTRO_END) go(GIFT, now);
        break;
      }
      case GIFT:
        if (openAt < 0 && (c == 4 || st >= GIFT_AUTO_OPEN)) {
          openAt = st; audio::play(audio::Cue::Sparkle);
          PLAY(player, CHIME, 90, 1, now);
        }
        if (openAt >= 0 && st >= openAt + 0.95f) go(PARTY, now);
        break;
      case PARTY:
        if (!songStarted && st >= 0.55f) { audio::play(audio::Cue::Firework); PLAY(player, SONG, 155, 2, now); songStarted = true; }
        if (c == 4 || (songStarted && !player.busy() && st > 9.0f)) {
          if (c == 4) audio::play(audio::Cue::Pop);
          go(WISH_S, now);
        }
        break;
      case WISH_S:
        if (blowAt < 0 && st > 1.0f && (c == 4 || st >= WISH_AUTO_BLOW)) { blowAt = st; PLAY(player, BLOW, 70, 1, now); }
        if (blowAt >= 0 && (st >= blowAt + WISH_AFTER || (c == 4 && st > blowAt + 1.0f))) go(NOTES, now);
        break;
      case NOTES: {
        float pt = st - pageAt;
        bool next = c == 1 || c == 4 || pt >= PAGE_TIME;
        if (c == 3 && page > 0) { --page; dir = -1; pageAt = st; PLAY(player, PAGE, 45, 1, now); }
        else if (next) {
          if (page + 1 >= PAGE_COUNT) go(FINALE, now);
          else { ++page; dir = 1; pageAt = st; PLAY(player, PAGE, 45, 1, now); }
        }
        break;
      }
      case FINALE:
        if (c == 4 && st > 0.8f) goto done;
        break;
    }
    player.update(now);
    st = (millis() - sceneAt) / 1000.0f;

    if (now - lastFrame >= 33) {
      float t = (now - started) / 1000.0f;
      float pageSt = st - pageAt;
      for (origin = 0; origin < HEIGHT; origin += BAND) {
        atmosphere(t);
        switch (scene) {
          case INTRO:  intro(t, st); break;
          case GIFT:   gift(t, st, openAt); break;
          case PARTY:  party(st); break;
          case WISH_S: wishScene(t, st, blowAt); break;
          case NOTES:  notes(t, st, page, pageSt, dir); break;
          case FINALE: finale(t, st); break;
        }
        pushChangedBand();
        player.update(millis());
        yield();
      }
      cacheValid = true;
      lastFrame = now;
      ++frames;
      if (millis()-rateWindow>=2000 && Serial.availableForWrite()>=96) {
        unsigned long elapsed=millis()-rateWindow;
        Serial.printf("[Birthday] %.1f fps, frame=%lums\n", frames*1000.0f/elapsed,
                      (unsigned long)(millis()-now));
        frames=0; rateWindow=millis();
      }
    }
    delay(1);
  }
done:
  setBuzzerTone(0);
  frame.deleteSprite();
  buttonsRequireRelease();
  Serial.println("[Birthday] Returned to Info");
}
