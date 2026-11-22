#include "run_rules.h"
#include <cassert>
#include <iostream>
using namespace runrules;
int main() {
  for(int count : {5,6}) {
    assert(attackFrame(0,count)==-1);
    for(int elapsed=0;elapsed<count*70;++elapsed) {
      int frame=attackFrame(count*70-elapsed,count);
      assert(frame==elapsed/70);
      assert((frame==count/2)==(elapsed>=count/2*70 && elapsed<(count/2+1)*70));
    }
  }
  Rewards r;
  assert(!r.startAttack(350) && !r.hit());
  for (int i = 0; i < 4; ++i) assert(r.collect() == 1);
  assert(r.collect() == 2 && r.multiplier() == 2);
  for (int i = 0; i < 4; ++i) assert(r.collect() == 2);
  assert(r.collect() == 3 && r.coins == 17);
  r.streak = 0;
  assert(r.collect() == 1);
  r.shield = shieldDuration;
  assert(r.startAttack(350) && !r.startAttack(350));
  r.update(70); assert(r.attack == 280 && r.shield == 7930);
  r.update(1000); assert(r.attack == 0 && r.shield == 6930);
  assert(r.hit() && r.shield == 0 && r.grace == 650);
  assert(r.hit());
  r.update(650); assert(!r.hit());
  r.shield = 10; r.startAttack(350); r.update(11);
  assert(!r.attack && !r.shield && !r.hit());
  assert(tick(5, 0) == 5 && tick(5, 5) == 0 && tick(5, UINT32_MAX) == 0);
  assert(overlaps(30, 200, 40, 60, 69, 230, 10, 20));
  assert(!overlaps(30, 200, 40, 60, 70, 230, 10, 20));
  // Enumerate input timings against the real jump/collision dimensions.
  for (float speed = startSpeed; speed <= maxSpeed + .01f; speed += .05f) {
    for (const auto &pattern : patterns) for (const auto &step : pattern) {
      int width = step.style == 2 ? 18 : step.style == 1 ? 12 : 11;
      int height = step.style == 2 ? 20 : step.style == 1 ? 36 : 24;
      bool possible = false;
      for (int jumpAt = 0; jumpAt < 50; ++jumpAt) {
        float y = 202, vy = 0, x = 250;
        bool airborne = false, collision = false;
        int landing = -1;
        for (int frame = 0; frame < 100; ++frame) {
          if (frame == jumpAt) { vy = jumpVelocity; airborne = true; }
          vy += gravity; y += vy;
          if (y >= 202) { y = 202; vy = 0; if (airborne) landing = frame; airborne = false; }
          x -= speed;
          collision |= overlaps(37, (int)y + 6, 26, 52, (int)x, 262-height, width, height);
        }
        if (!collision && landing >= 0 && landing < jumpAt + step.gapFrames - 5) possible = true;
      }
      assert(possible);
    }
  }
  std::cout << "Run rewards, shield expiry, attacks, collision and jump clearance passed\n";
}
