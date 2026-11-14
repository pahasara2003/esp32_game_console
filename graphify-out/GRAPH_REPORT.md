# Graph Report - src  (2026-11-13)

## Corpus Check
- Large corpus: 34 files · ~1,227,160 words. Semantic extraction will be expensive (many Claude tokens). Consider running on a subfolder.

## Summary
- 292 nodes · 519 edges · 14 communities (13 shown, 1 thin omitted)
- Extraction: 96% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 20 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- Button input and controls
- UI kit and cards
- Birthday scene
- Game registry
- Rocket demo
- Birthday code
- Conway life board
- Life patterns
- Camera viewer
- ADS1115 button driver
- Life field guide
- Mandelbrot renderer
- Smoke particles

## God Nodes (most connected - your core abstractions)
1. `Id` - 16 edges
2. `lifeLoop()` - 15 edges
3. `SpaceParticle` - 14 edges
4. `Board` - 12 edges
5. `Game` - 11 edges
6. `showRocketDemo()` - 11 edges
7. `gift()` - 10 edges
8. `showBirthdayScene()` - 10 edges
9. `Page` - 10 edges
10. `celebration()` - 9 edges

## Surprising Connections (you probably didn't know these)
- `command()` --calls--> `buttonsNextPress()`  [INFERRED]
  birthday.cpp → buttons.cpp
- `showBirthdayScene()` --calls--> `buttonsRequireRelease()`  [INFERRED]
  birthday.cpp → buttons.cpp
- `input()` --calls--> `buttonsHeld()`  [INFERRED]
  life.cpp → buttons.cpp
- `tetrisInputHeld()` --calls--> `buttonsHeld()`  [INFERRED]
  tetris_input.cpp → buttons.cpp
- `tetrisInputPoll()` --calls--> `buttonsNextPress()`  [INFERRED]
  tetris_input.cpp → buttons.cpp

## Import Cycles
- None detected.

## Communities (14 total, 1 thin omitted)

### Community 0 - "Button input and controls"
Cohesion: 0.12
Nodes (27): buttonsHeld(), buttonsNextPress(), buttonsRequireRelease(), lifeInputPoll(), beginRender(), changeZoom(), coarseNeighborhoodNeedsDetail(), crosshair() (+19 more)

### Community 1 - "UI kit and cards"
Cohesion: 0.14
Nodes (28): card(), chevron(), chip(), Control, action, key, gameOverSheet(), heading() (+20 more)

### Community 2 - "Birthday scene"
Cohesion: 0.16
Nodes (24): atmosphere(), cake(), celebration(), circle(), command(), confetti(), ease(), firework() (+16 more)

### Community 3 - "Game registry"
Cohesion: 0.08
Nodes (18): Game, gameLoop, gameSetup, highScores, icon, index, name, Board (+10 more)

### Community 4 - "Rocket demo"
Cohesion: 0.16
Nodes (26): advanceParticle(), clearRocketArea(), drawAsteroid(), drawRocketBody(), drawRocketFlame(), drawStar(), emitSmoke(), eraseParticle() (+18 more)

### Community 5 - "Birthday code"
Cohesion: 0.10
Nodes (6): BirthdayCode, lastPress, position, Vec2, x, y

### Community 6 - "Conway life board"
Cohesion: 0.20
Nodes (24): closeGuide(), input(), drawBoard(), drawFooter(), drawGuideDiagram(), drawGuidePage(), drawHeader(), drawOption() (+16 more)

### Community 7 - "Life patterns"
Cohesion: 0.08
Nodes (23): Id, ACORN, BEACON, BLINKERS, DIEHARD, GLIDER, GLIDER_FLEET, GOSPER_GUN (+15 more)

### Community 8 - "Camera viewer"
Cohesion: 0.12
Nodes (11): cameraViewerLoop(), cameraViewerSetup(), drawTile(), fetchFrame(), ImageData, bytes, length, offset (+3 more)

### Community 9 - "ADS1115 button driver"
Cohesion: 0.17
Nodes (5): configFor(), publish(), readRegister(), scan(), startConversion()

### Community 10 - "Life field guide"
Cohesion: 0.16
Nodes (12): Diagram, cells, next, phase, Page, category, frames, height (+4 more)

### Community 11 - "Mandelbrot renderer"
Cohesion: 0.22
Nodes (9): PixelJob, active, ci, cr, ii, iteration, rr, zi (+1 more)

### Community 12 - "Smoke particles"
Cohesion: 0.29
Nodes (7): SmokeParticle, life, radius, vx10, vy10, x10, y10

## Knowledge Gaps
- **88 isolated node(s):** `frequency`, `beats`, `at`, `index`, `ringing` (+83 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 118 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Game` connect `Game registry` to `Birthday code`?**
  _High betweenness centrality (0.062) - this node is a cross-community bridge._
- **What connects `frequency`, `beats`, `at` to the rest of the system?**
  _88 weakly-connected nodes found - possible missing edges._
- **Should `Button input and controls` be split into smaller, more focused modules?**
  _Cohesion score 0.12096774193548387 - nodes in this community are weakly interconnected._
- **Should `UI kit and cards` be split into smaller, more focused modules?**
  _Cohesion score 0.14408602150537633 - nodes in this community are weakly interconnected._
- **Should `Game registry` be split into smaller, more focused modules?**
  _Cohesion score 0.082010582010582 - nodes in this community are weakly interconnected._
- **Should `Birthday code` be split into smaller, more focused modules?**
  _Cohesion score 0.10461538461538461 - nodes in this community are weakly interconnected._
- **Should `Life patterns` be split into smaller, more focused modules?**
  _Cohesion score 0.08333333333333333 - nodes in this community are weakly interconnected._