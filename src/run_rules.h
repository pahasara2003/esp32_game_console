#pragma once
#include <stdint.h>

namespace runrules {
constexpr uint32_t frameMs = 15;
constexpr float startSpeed = 6.0f, maxSpeed = 10.0f;
constexpr float gravity = 1.10f, jumpVelocity = -11.0f;
constexpr uint32_t shieldDuration = 8000;
constexpr uint32_t attackFrameMs = 70;
inline int attackFrame(uint32_t remaining, int count) {
  return remaining ? count - int((remaining + attackFrameMs - 1) / attackFrameMs) : -1;
}
inline uint32_t tick(uint32_t remaining, uint32_t elapsed) {
  return elapsed >= remaining ? 0 : remaining - elapsed;
}
struct Rewards {
  int coins = 0;
  int streak = 0;
  uint32_t shield = 0, attack = 0, grace = 0;
  int collect() {
    ++streak;
    int reward = multiplier();
    coins += reward;
    return reward;
  }
  int multiplier() const { return streak >= 10 ? 3 : streak >= 5 ? 2 : 1; }
  void update(uint32_t elapsed) {
    shield = tick(shield, elapsed);
    attack = tick(attack, elapsed);
    grace = tick(grace, elapsed);
    if (!shield) attack = 0;
  }
  bool startAttack(uint32_t duration) {
    if (!shield || attack) return false;
    attack = duration;
    return true;
  }
  bool hit() {
    if (grace) return true;
    if (!shield) return false;
    shield = attack = 0;
    grace = 650;
    return true;
  }
};
struct ObstacleStep { uint8_t style, gapFrames; };
// Each obstacle gets a complete jump plus landing/reaction time, at every speed.
constexpr ObstacleStep patterns[3][3] = {
  {{0, 40}, {2, 44}, {0, 48}},
  {{2, 44}, {0, 40}, {1, 52}},
  {{1, 52}, {0, 44}, {2, 48}}
};
inline bool overlaps(int ax, int ay, int aw, int ah,
                     int bx, int by, int bw, int bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}
}
