#include <Arduino.h>
#include "game.h"
#include "rocket_demo.h"

namespace {
constexpr uint8_t STAR_COUNT = 28;
constexpr uint8_t ASTEROID_COUNT = 5;
constexpr uint8_t FLAME_FRAME_COUNT = 4;
constexpr int16_t ROCKET_MASK_Y = 98;
constexpr int16_t ROCKET_MASK_HALF_W = 40;
constexpr int16_t ROCKET_MASK_H = 138;
constexpr int16_t ROCKET_MIN_X = 42;
constexpr int16_t ROCKET_MAX_X = 198;
constexpr uint8_t SMOKE_COUNT = 12;
constexpr uint8_t FLAME_OUTER_LENGTH[FLAME_FRAME_COUNT] = {20, 27, 33, 24};
constexpr uint8_t FLAME_MIDDLE_LENGTH[FLAME_FRAME_COUNT] = {16, 22, 27, 19};
constexpr uint8_t FLAME_CORE_LENGTH[FLAME_FRAME_COUNT] = {10, 15, 19, 13};
constexpr int ROCKET_SCALE = 90;
constexpr int ROCKET_DRAW_ORIGIN_Y = 100;

int16_t rocketX = 120;
int16_t rocketTargetX = 120;
int8_t flameBend = 0;

int scaled(int value) {
  return value * ROCKET_SCALE / 100;
}

int scaledY(int y) {
  return ROCKET_DRAW_ORIGIN_Y + scaled(y - ROCKET_DRAW_ORIGIN_Y);
}

struct SpaceParticle {
  int16_t x10;
  int16_t y10;
  int8_t drift10;
  uint8_t speed10;
  uint8_t layer;
  uint8_t radius;
};

struct SmokeParticle {
  int16_t x10;
  int16_t y10;
  int8_t vx10;
  uint8_t vy10;
  uint8_t life;
  uint8_t radius;
};

SpaceParticle stars[STAR_COUNT];
SpaceParticle asteroids[ASTEROID_COUNT];
SmokeParticle smoke[SMOKE_COUNT];

bool overlapsRocketFrame(const SpaceParticle &particle, bool asteroid) {
  int x = particle.x10 / 10;
  int y = particle.y10 / 10;
  int r = asteroid ? particle.radius + 1 : 2;
  return x + r >= rocketX - ROCKET_MASK_HALF_W &&
         x - r < rocketX + ROCKET_MASK_HALF_W &&
         y + r >= ROCKET_MASK_Y && y - r < ROCKET_MASK_Y + ROCKET_MASK_H;
}

void resetStar(SpaceParticle &star, bool randomY) {
  star.layer = random(0, 3);
  star.x10 = random(3, 237) * 10;
  star.y10 = (randomY ? random(2, 318) : random(-35, -3)) * 10;
  star.speed10 = 2 + star.layer * 3;
  star.drift10 = 0;
  star.radius = star.layer == 2 ? 2 : 1;
}

void resetAsteroid(SpaceParticle &rock, bool randomY) {
  rock.layer = random(1, 3);
  rock.radius = random(2, 4);
  rock.x10 = random(10, 230) * 10;
  rock.y10 = (randomY ? random(0, 320) : random(-100, -12)) * 10;
  rock.speed10 = random(7, 13) + rock.layer * 2;
  rock.drift10 = 0;
}

void initializeSpace() {
  for (auto &star : stars) resetStar(star, true);
  for (auto &rock : asteroids) resetAsteroid(rock, true);
}

void eraseParticle(const SpaceParticle &particle, bool asteroid) {
  int x = particle.x10 / 10;
  int y = particle.y10 / 10;
  int r = asteroid ? particle.radius + 2 : 3;
  tft.fillRect(x - r, y - r, r * 2 + 1, r * 2 + 1, TFT_BLACK);
}

void advanceParticle(SpaceParticle &particle, bool asteroid) {
  particle.x10 += particle.drift10;
  particle.y10 += particle.speed10;
  int margin = asteroid ? 12 : 4;
  if (particle.y10 > (320 + margin) * 10 ||
      particle.x10 < -margin * 10 || particle.x10 > (240 + margin) * 10) {
    if (asteroid) resetAsteroid(particle, false);
    else resetStar(particle, false);
  }
}

void drawStar(const SpaceParticle &star) {
  int x = star.x10 / 10;
  int y = star.y10 / 10;
  const uint16_t colors[] = {0x2948, 0x7DFF, TFT_WHITE};
  uint16_t color = colors[star.layer];
  tft.drawPixel(x, y, color);
  if (star.layer >= 1) tft.drawPixel(x + 1, y, color);
  if (star.layer == 2) {
    tft.drawPixel(x - 1, y, color);
    tft.drawPixel(x, y - 1, color);
    tft.drawPixel(x, y + 1, color);
  }
}

void drawAsteroid(const SpaceParticle &rock) {
  int x = rock.x10 / 10;
  int y = rock.y10 / 10;
  int r = rock.radius;
  uint16_t body = rock.layer == 2 ? 0x8C51 : 0x52AA;
  tft.fillCircle(x, y, r, body);
  tft.drawPixel(x - r / 2, y - r / 2, 0xC618);
  tft.drawPixel(x + 1, y + 1, 0x3186);
  if (r >= 3) tft.drawPixel(x + 1, y - 1, 0x39E7);
}

void updateSpace() {
  for (auto &star : stars) {
    if (!overlapsRocketFrame(star, false)) eraseParticle(star, false);
  }
  for (auto &rock : asteroids) {
    if (!overlapsRocketFrame(rock, true)) eraseParticle(rock, true);
  }
  for (auto &star : stars) {
    advanceParticle(star, false);
    if (!overlapsRocketFrame(star, false)) drawStar(star);
  }
  for (auto &rock : asteroids) {
    advanceParticle(rock, true);
    if (!overlapsRocketFrame(rock, true)) drawAsteroid(rock);
  }
}

void clearRocketArea(int x) {
  tft.fillRect(x - ROCKET_MASK_HALF_W, ROCKET_MASK_Y,
               ROCKET_MASK_HALF_W * 2 + 1, ROCKET_MASK_H, TFT_BLACK);
}

bool updateRocketMotion(bool &bendChanged) {
  int oldX = rocketX;
  int direction = 0;
  if (rocketX < rocketTargetX) direction = 1;
  else if (rocketX > rocketTargetX) direction = -1;
  if (direction != 0) {
    int distance = abs(rocketTargetX - rocketX);
    rocketX += direction * min(distance, 2);
  }

  int desiredBend = direction > 0 ? -7 : (direction < 0 ? 7 : 0);
  int oldBend = flameBend;
  if (flameBend < desiredBend) flameBend += min(desiredBend - flameBend, 2);
  else if (flameBend > desiredBend) flameBend -= min(flameBend - desiredBend, 2);
  bendChanged = flameBend != oldBend;
  return rocketX != oldX;
}

void eraseSmoke() {
  for (auto &puff : smoke) {
    if (puff.life == 0) continue;
    int x = puff.x10 / 10;
    int y = puff.y10 / 10;
    int r = puff.radius + 1;
    tft.fillRect(x - r, y - r, r * 2 + 1, r * 2 + 1, TFT_BLACK);
  }
}

void updateAndDrawSmoke() {
  for (auto &puff : smoke) {
    if (puff.life == 0) continue;
    puff.x10 += puff.vx10;
    puff.y10 += puff.vy10;
    puff.life--;
    if (puff.life == 0 || puff.y10 >= 3190) {
      puff.life = 0;
      continue;
    }
    int x = puff.x10 / 10;
    int y = puff.y10 / 10;
    uint16_t color = puff.life > 22 ? TFT_WHITE :
                     (puff.life > 11 ? 0xCE79 : 0x7BEF);
    tft.fillCircle(x, y, puff.radius, color);
  }
}

void emitSmoke(uint8_t flameFrame) {
  for (auto &puff : smoke) {
    if (puff.life != 0) continue;
    puff.x10 = (rocketX + scaled(flameBend) + random(-2, 3)) * 10;
    puff.y10 = (scaledY(192) + scaled(FLAME_OUTER_LENGTH[flameFrame])) * 10;
    puff.vx10 = flameBend / 2 + random(-2, 3);
    puff.vy10 = random(4, 8);
    puff.life = random(25, 34);
    puff.radius = random(0, 3) == 0 ? 2 : 1;
    break;
  }
}

void drawRocketBody(int x) {
  auto X = [x](int offset) { return x + scaled(offset); };
  auto Y = [](int y) { return scaledY(y); };
  auto S = [](int value) { return max(1, scaled(value)); };

  // Longer white fins with cool grey edge shading and bright inner highlights.
  tft.fillTriangle(X(-15), Y(151), X(-15), Y(187), X(-36), Y(195), 0x7BEF);
  tft.fillTriangle(X(-15), Y(153), X(-33), Y(192), X(-18), Y(178), 0xD69A);
  tft.fillTriangle(X(-17), Y(158), X(-29), Y(187), X(-19), Y(176), TFT_WHITE);
  tft.fillTriangle(X(15), Y(151), X(15), Y(187), X(36), Y(195), 0x7BEF);
  tft.fillTriangle(X(15), Y(153), X(33), Y(192), X(18), Y(178), 0xD69A);
  tft.fillTriangle(X(17), Y(158), X(29), Y(187), X(19), Y(176), TFT_WHITE);

  // Layered nose cone gives a bright enamel appearance.
  tft.fillTriangle(X(0), Y(101), X(-19), Y(134), X(19), Y(134), 0x7800);
  tft.fillTriangle(X(0), Y(104), X(-16), Y(133), X(16), Y(133), 0xF800);
  tft.fillTriangle(X(-2), Y(108), X(-11), Y(130), X(2), Y(130), 0xFD40);
  tft.drawFastHLine(X(-15), Y(132), S(31), 0xFFFF);

  // Metallic body with a bright center and cool shaded edges.
  tft.fillRoundRect(X(-18), Y(126), S(36), S(57), S(8), 0x7BEF);
  tft.fillRoundRect(X(-15), Y(127), S(29), S(54), S(7), 0xD69A);
  tft.fillRoundRect(X(-10), Y(128), S(18), S(52), S(6), 0xF7BE);
  tft.fillRoundRect(X(-7), Y(129), S(8), S(49), S(4), TFT_WHITE);
  tft.drawFastVLine(X(14), Y(135), S(37), 0x8410);
  tft.drawFastVLine(X(-15), Y(137), S(34), 0xE71C);

  // Deep glass porthole, cyan inner glow, and reflected highlight.
  tft.fillCircle(X(0), Y(146), S(10), 0x2104);
  tft.fillCircle(X(0), Y(146), S(8), 0x02B8);
  tft.fillCircle(X(0), Y(146), S(6), 0x05FF);
  tft.fillCircle(X(-2), Y(143), S(2), TFT_WHITE);
  tft.drawPixel(X(4), Y(149), 0x03EF);

  // Lower accent stripe and layered engine nozzle.
  tft.fillRoundRect(X(-4), Y(161), S(8), S(18), S(3), 0xF800);
  tft.drawFastVLine(X(-2), Y(163), S(13), 0xFD20);
  tft.fillRoundRect(X(-12), Y(178), S(24), S(9), S(3), 0x2104);
  tft.fillRect(X(-9), Y(184), S(18), S(6), 0x4A49);
  tft.drawFastHLine(X(-7), Y(185), S(14), 0x9CD3);
}

void drawRocketFlame(int x, uint8_t frame, int8_t bend) {
  // Only this small region changes; the ship body never moves.
  int baseY = scaledY(190);
  int scaledBend = scaled(bend);
  tft.fillRect(x - scaled(19), baseY, scaled(39), scaled(40), TFT_BLACK);
  int outerEnd = baseY + scaled(FLAME_OUTER_LENGTH[frame]);
  int middleEnd = baseY + scaled(FLAME_MIDDLE_LENGTH[frame]);
  int coreEnd = baseY + scaled(FLAME_CORE_LENGTH[frame]);
  tft.fillTriangle(x - scaled(11), baseY, x + scaled(11), baseY,
                   x + scaledBend, outerEnd, 0xA800);
  tft.fillTriangle(x - scaled(8), baseY, x + scaled(8), baseY,
                   x + scaledBend * 3 / 4, middleEnd, 0xFC00);
  tft.fillTriangle(x - scaled(5), baseY, x + scaled(5), baseY,
                   x + scaledBend / 2, coreEnd, 0xFFE0);
  tft.fillTriangle(x - scaled(2), baseY, x + scaled(2), baseY,
                   x + scaledBend / 3, coreEnd - scaled(4), TFT_WHITE);
  tft.drawPixel(x + scaledBend - 2, outerEnd - 2, 0xF800);
  tft.drawPixel(x + scaledBend + 2, middleEnd - 1, 0xFD20);
}
}  // namespace

void showRocketDemo() {
  tft.setRotation(2);
  buttonsRequireRelease();
  tft.fillScreen(TFT_BLACK);
  rocketX = 120;
  rocketTargetX = 120;
  flameBend = 0;
  for (auto &puff : smoke) puff.life = 0;
  uint8_t frame = 0;
  unsigned long nextFrame = millis() + 110;
  unsigned long nextSpaceUpdate = millis() + 35;
  unsigned long nextSmokeEmit = millis() + 80;
  initializeSpace();
  updateSpace();
  drawRocketBody(rocketX);
  drawRocketFlame(rocketX, frame, flameBend);
  for (;;) {
    int command = findCommand(false);
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'e' || c == 'E' || c == ' ' || c == '\n') command = 4;
      else if (c == 'a' || c == 'A') command = 3;
      else if (c == 'd' || c == 'D') command = 1;
    }
    if (command == 4) break;
    if (command == 3) {
      rocketTargetX = max((int)ROCKET_MIN_X, (int)rocketTargetX - 28);
    } else if (command == 1) {
      rocketTargetX = min((int)ROCKET_MAX_X, (int)rocketTargetX + 28);
    }
    unsigned long now = millis();
    if (now >= nextSpaceUpdate) {
      eraseSmoke();
      int oldRocketX = rocketX;
      bool bendChanged = false;
      bool rocketMoved = updateRocketMotion(bendChanged);
      if (rocketMoved) clearRocketArea(oldRocketX);
      updateSpace();
      updateAndDrawSmoke();
      if (now >= nextSmokeEmit) {
        emitSmoke(frame);
        nextSmokeEmit = now + 85;
      }
      if (rocketMoved) drawRocketBody(rocketX);
      if (rocketMoved || bendChanged) drawRocketFlame(rocketX, frame, flameBend);
      nextSpaceUpdate = now + 35;
    }
    if (now >= nextFrame) {
      frame = (frame + 1) % FLAME_FRAME_COUNT;
      drawRocketFlame(rocketX, frame, flameBend);
      nextFrame = now + 120;
    }
    delay(2);
  }
  buttonsRequireRelease();
}
