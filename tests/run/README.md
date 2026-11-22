Run checks from the repository root:

```sh
c++ -std=c++11 -Wall -Wextra -I src tests/run/test_rules.cpp -o /tmp/run-rules-test
/tmp/run-rules-test
python3 tests/run/test_gameplay.py
python3 tests/run/test_background.py
python3 tools/build_runner_sprite.py
pio run
```

The generator checks lossless palette/RLE round trips for every frame. The host
checks cover rewards, shields, attack timing, collision, jump clearance, native
sprite decoding, extended attack cleanup, marker cleanup, pickups and spawning.
Incremental frames must match a full redraw, including clouds overlapping pickups
and the runner, while sending each affected pixel only once per frame. Selector
checks enforce limited redraw bounds. Game-over rows are checked using the real
font widths, including the largest coin count and the existing Rally layout.
HUD checks change the sky tint and verify every top/side margin and rounded
corner. Attack checks cover all three characters, body contact during wind-up,
exact middle-frame hits at both running speeds, one bonus per block, and debris
movement/expiry without trails.

Background checks compare all nine compressed frames against the source PNGs,
pixel for pixel, using the firmware decoder and both row caches. The lossless
row format uses literal pixels and backward matches; two decoded rows add about
1 KiB of RAM and preserve random pixel reads for day/night blending. Regenerate
the compressed header with `python3 tools/build_run_scene.py`.

On hardware, check the selector, frame rate, jump timing, sound, pause/resume
while shielded, shield expiry during an attack, and milestone text at
100/250/500 metres. The renderer targets a 15 ms frame interval; actual frame
rate still needs measurement on the console with game music playing.

Smoothing regressions cover composed notices and footer transfers, separate high
coin arcs reachable at starting/maximum speed, and collected coin ascent, expiry,
and cleanup without repeat rewards.
