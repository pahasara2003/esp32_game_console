# ESP32-C3 Retro Game Console (PlatformIO & TFT_eSPI)

A retro game console project running on the **ESP32-C3** with an **ILI9341 SPI TFT Display (320x240)** and **ADS1115 I2C ADC** for analog gamepad/button controls.

---

## 🎮 Included Games & Menus

- **Main Menu:** Modern phone-style home screen over the blue-purple wallpaper: a frosted-glass "Games" header widget with an accent orb showing the playable count, floating 52×52 icons with soft drop shadows and white shadowed labels, a frosted focus card with an accent glow and underline capsule on the selected app, and a frosted page dock where the active page is an elongated pill. Nine apps fit each page; D-pad movement crosses between pages automatically.
- **Snake:** Nokia-style retro snake with particle effects and stars.
- **Breakout:** Brick-breaker game with bat and bouncing ball physics.
- **Tetris:** Classic block puzzle with rotation, line clearing, and pause menu. A theme-matched pastel gradient header sits above the centered 17×17 board with 12-pixel cells, solid square blocks over a seamless purple-black checkerboard, and separate score, centered next-piece, and time cards. Small off-screen sprites compose changing HUD values; moving pieces redraw only changed cells. Pause uses a bright selected row with dark text.
- **Tic-tac-toe:** Play X against the console's O opponent. Move with the D-pad, tap Shoot to place, and hold Shoot to pause.
- **Pong:** Move the paddle with Left and Right, keep the ball in play, and survive three misses. Shoot pauses.
- **Minesweeper:** Clear an 8×8 board with ten mines. White raised tiles, colored numbers, red flags, and a magenta selection outline follow the supplied icon's style. Tap Shoot to reveal, hold Shoot to flag, and press Up while holding Shoot to pause. The first reveal is safe. A small dot marks a revealed tile with no neighboring mines.
- **Conway's Life:** Opens directly into an automatic B3/S23 simulation with wrapping edges. The launcher calls it **Conway's**. Random mode is the only simulation mode; the Info field guide explains the rules and illustrates common patterns across 16 pages. Only changed cells redraw during simulation.
- **Launcher pages:** Two pages contain all games and utilities; move with the D-pad and press Shoot to open. Selection changes redraw two small composed tiles; a page change slides the app foreground sideways with a short eased transition, while the wallpaper and title stay fixed. The transition uses RGB565 strips and small masks rather than two full-screen buffers.
- **High Score System:** High scores persisted to ESP32 Flash via `Preferences` (NVS).

---

## Tetris controls

Tetris uses its own asynchronous ADS1115 scanner with settling, debounce, and separate held-button states. Up rotates once per press; Left/Right repeat after 180 ms, then every 65 ms; holding Down targets one fall step every 35 ms and can be combined with horizontal movement. Shoot pauses, and screen transitions require release before another action. Its checkerboard and HUD accents follow the selected theme; solid square pieces use a coordinated bright palette. Pieces spawn centrally. Input regression checks: `python3 tools/check_tetris_input.py` and `python3 tools/check_tetris_controls.py`.

## Conway's Life controls

| Screen | Controls |
| :--- | :--- |
| Running | Shoot pauses; Up/Down changes speed. |
| Paused | Shoot resumes; Up/Down changes speed. |
| Run or pause | Hold Shoot to open options. |
| Options | Up/Down chooses; Shoot selects. Resume, new random, speed, Info, or main menu. Hold Shoot to return. |

Speed limits: 1, 2, 4, 8, 12, 20, or 30 generations per second; actual speed depends on how many pixels change. Default target is 20. The app reads the D-pad asynchronously, batches same-color cell updates, and refreshes counters five times per second. USB serial `p` reports measured generation rate and the last frame's duration.

Random mode uses 2-pixel cells on a 112×96 board (10,752 cells). There is no cell editor or start prompt.

Hold Shoot → Info to read the field guide. Left/Right changes pages; Shoot returns to options. It includes the laws, population changes, blocks, beehives, loaves, boats, blinkers, toads, beacons, gliders, lightweight spaceships, and pulsars. Moving examples animate independently while your random world stays paused and unchanged.

A cell is born with three live neighbors; a live cell survives with two or three. Populations naturally shrink from isolation or overcrowding, and may become still, oscillate, or die out. Opposite edges are connected, so traveling patterns can eventually collide with existing cells. Extinction is labeled on screen; the app does not silently reseed or alter Conway's rules.

The supplied Tic-tac-toe, Pong, Minesweeper, Life, and Mandelbrot icons are in the launcher. Gallery artwork is converted and ready for a future app.

## Mandelbrot explorer

Open **MANDEL** in the launcher. Each fresh arrow press moves the view center once; the crosshair marks the zoom center. Release between presses. Shoot opens view options, with **Zoom in 2x**, **Zoom out 2x**, **Reset view**, **Back to view**, and **Main menu**. In options, Up/Down moves once per press; Left/Right immediately zooms out/in and returns to the view. Hold Shoot to close options. The explorer uses completed ADC scans, a 30ms stable-reading filter, and release gating between actions and screens.

The explorer starts at center -0.5 + 0i with a horizontal span of 3.5. Zoom ranges from 1x to 1,048,576x, with equal real/imaginary pixel scales and progressively higher iteration limits (96–496). Black denotes points that have not escaped within the current iteration limit. Deep views entirely inside the set can be solid black; pan to a boundary or reset the view to explore detail.

Rendering proceeds through 8-, 4-, 2-, and 1-pixel previews. CPU slices are limited to about 2ms, including checks inside expensive pixel calculations, and only one ADC channel is read between slices. A possible button press pauses refinement during debounce. Confirmed navigation cancels the old render, including any partially computed pixel, and immediately starts a low-iteration preview at the new center. Later passes add detail. Only the plot and changing status fields are updated; a small strip buffer avoids a full-screen frame buffer.

## Home-screen assets

The launcher uses a subtle 18% black tint baked into the wallpaper. Its title starts 12 pixels below the top edge and uses an opaque pastel gradient with dark lettering. Three app rows start at y=68, 143, and 218, with eight pixels of top padding inside each tile and extra space below the app name. Settings offers Aurora, Ocean, Orchid, Rose, Citrus, Sunset, Mint, and Silver: each changes the title gradient and selection accents, with a live gradient preview in Settings. Utility surfaces stay charcoal for readable contrast.

The final user-selected wallpaper source and converted display image are `assets/home/abstract-blue-purple-source.png` and `assets/home/abstract-blue-purple-240x320.png`. Existing user-supplied app artwork is converted with clean transparency at 52×52 by `tools/build_home_assets.py`, which also builds `src/home_icons.h` and `src/home_wallpaper.h`. The launcher preview is `assets/home/launcher-preview.png`. A small temporary tile sprite composes the wallpaper, selection styling, icon, and label before transfer, and is freed when launching an app.

## Display speed

The ILI9341 write clock is now 40 MHz, previously 4 MHz. On the connected ESP32-C3, eight 240×320 RGB565 image transfers measured:

| SPI write clock | Time per full image | Image transfers per second |
| :--- | :--- | :--- |
| 4 MHz | 312.504 ms | 3.20 |
| 40 MHz | 34.697 ms | 28.82 |

These numbers measure image transfers, not game or Mandelbrot frame rates. A generated full-screen gradient/checkerboard/ball demo measured approximately 16.1 FPS including image generation and text after composing the text panels off-screen. Each transferred frame includes the finished panels, avoiding an intermediate background-only repaint. The panel is connected for writes only, so clean reception must be checked visually; upload success and throughput do not verify pixel accuracy.

The temporary visual demo was confirmed clean on this console and then removed from the normal firmware. Its source is archived in `backups/display-demo-40mhz.zip`. Normal games and apps retain the 40 MHz write clock.

---

## 🔌 Hardware Circuit & Pinout

### 1. ILI9341 Display (SPI)
Configured via [`lib/TFT_eSPI/User_Setups/Setup70c_ESP32_C3_ILI9341.h`](file:///home/pahasara/Documents/antigravity/embedded/esp32c3-ili9341/lib/TFT_eSPI/User_Setups/Setup70c_ESP32_C3_ILI9341.h):

| ILI9341 Pin | ESP32-C3 Pin | Notes |
| :--- | :--- | :--- |
| **CS** | **GPIO 3** | Chip Select |
| **DC / RS** | **GPIO 1** | Data / Command Select |
| **RST / RESET** | **GPIO 2** | Display Hardware Reset |
| **MOSI / SDI** | **GPIO 0** | SPI Data Out |
| **SCLK / CLK** | **GPIO 4** | SPI Clock |
| **MISO / SDO** | **GPIO 10** | SPI Data In |
| **VCC** | **3.3V** / **5V** | Power |
| **GND** | **GND** | Ground |
| **LED / BL** | **3.3V** *(or GPIO)* | Backlight |

### 2. Controls & Peripherals

| Peripheral | ESP32-C3 Pin | Notes |
| :--- | :--- | :--- |
| **I2C SDA** | **GPIO 8** | Connected to ADS1115 ADC (`0x48`) |
| **I2C SCL** | **GPIO 9** | Connected to ADS1115 ADC (`0x48`) |
| **Shoot Button** | **GPIO 5** | Digital Input (`INPUT_PULLDOWN`) |

---

## 📁 Project Structure

```text
esp32c3-ili9341/
├── lib/
│   └── TFT_eSPI/           # Customized TFT_eSPI with ESP32-C3 register & timing fixes
├── src/
│   ├── gameConsoleNew.ino  # Entry point (setup and loop)
│   ├── game.h / game.ino   # Core Game engine, ADC input handler, high score manager
│   ├── menu.ino            # Main menu launcher & icon renderer
│   ├── snake.ino           # Snake game
│   ├── breakout.ino        # Breakout game
│   ├── tetris.ino          # Tetris game
│   ├── mini_games.ino      # Tic-tac-toe, Pong, and Minesweeper
│   ├── life.cpp            # Life interface, random world, and controls
│   ├── life_model.h        # Conway B3/S23 simulation
│   ├── life_guide.h        # Offline rules and animated pattern diagrams
│   ├── life_input.cpp      # Nonblocking ADS1115 controls for Life
│   ├── config.h            # Colors, screen dimensions, button pins
│   ├── icons.h             # 60x60 game icon bitmaps
│   ├── images.h            # UI bitmaps (pause, undo, play)
│   └── Petme8x8.h          # 8x8 bitmap font
├── platformio.ini          # PlatformIO config & library dependencies
└── README.md
```

---

## 🛠️ Build and Upload in VS Code

1. Open folder in VS Code:
   `/home/pahasara/Documents/antigravity/embedded/esp32c3-ili9341`
2. Click **Build (`✓`)** or press `Ctrl+Alt+B`.
3. Connect ESP32-C3 and click **Upload (`→`)**.
4. Open the Serial Monitor at `115200` baud.
