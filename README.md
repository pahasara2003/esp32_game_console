<div align="center">

# 🎮 ESP32-C3 Game Console

**A pocket-sized retro handheld you can build yourself.**
Ten games, a music and video player, and a polished pixel UI, all on an ESP32-C3.

![MCU](https://img.shields.io/badge/MCU-ESP32--C3-E7352C?style=for-the-badge&logo=espressif&logoColor=white)
![Framework](https://img.shields.io/badge/Arduino-PlatformIO-00979D?style=for-the-badge&logo=platformio&logoColor=white)
![Display](https://img.shields.io/badge/Display-ILI9341%20240×320-8A2BE2?style=for-the-badge)
![Audio](https://img.shields.io/badge/Audio-I2S%20MAX98357A-F5A623?style=for-the-badge)

<br>

<img src="assets/home/tetris-52.png" width="52" alt="Tetris"> <img src="assets/home/snake-52.png" width="52" alt="Snake"> <img src="assets/home/breakout-52.png" width="52" alt="Breakout"> <img src="assets/home/run-52.png" width="52" alt="Run"> <img src="assets/home/galaga-52.png" width="52" alt="Galaga"> <img src="assets/home/tic_tac-52.png" width="52" alt="Tic-tac-toe"> <img src="assets/home/pong-52.png" width="52" alt="Pong"> <img src="assets/home/mines-52.png" width="52" alt="Minesweeper"> <img src="assets/home/life-52.png" width="52" alt="Life"> <img src="assets/home/mandelbrot-52.png" width="52" alt="Mandelbrot"> <img src="assets/home/sound-52.png" width="52" alt="Music"> <img src="assets/home/settings-52.png" width="52" alt="Settings">

</div>

---

## ✨ Highlights

- 🕹️ **10 games**, each with its own music, sound effects, pause menu and saved high scores
- 🧬 **Simulations**: Conway's Game of Life and an interactive Mandelbrot explorer
- 🎵 **Media**: music player and video player straight from the SD card
- 🎨 **Polished UI**: phone-style launcher, 8 colour themes, light and dark mode, one shared UI kit for every screen
- 🔊 **Game music built in**: tracks are embedded in flash, no SD card needed to play
- ⚡ **Fast display**: 80 MHz SPI writes (about 29 full-screen frames per second measured at 40 MHz)
- 🧠 **RAM-smart**: big buffers exist only while their app is running

## 🕹️ What's inside

| | Apps |
| :--- | :--- |
| **Arcade** | Snake · Tetris · Breakout · Run · Galaga · Rally |
| **Puzzle & classic** | Sliding Puzzle · Tic-tac-toe · Pong · Minesweeper |
| **Simulations** | Conway's Game of Life · Mandelbrot explorer |
| **Media** | Music player · Video player |
| **System** | Launcher · Settings · About |

## 👾 The arcade

> Insert coin. Or just press Shoot.

### 🏁 Rally
**Race the clock, not the road.** Hurtle down a drone-view track, steer a rotating car and chase checkpoints before the timer hits zero. Every checkpoint adds time, the grass slows you down, and nudging a rival earns bonus coins.
`Left / Right` steer · `Down` brake · `Shoot` pause · score: 1 point per metre, 20 per coin.

### 🧛 Run
**Sprint, jump, survive.** A caped vampire runner dashes through a moody scene, grabbing coins to build a combo multiplier. Snatch a shield for 8 seconds of invincibility and strike back with an attack animation.

### 🚀 Galaga
**A little cosmic joy.** Wave after wave of alien invaders. Steer your ship, fire away and protect your three lives.
`Left / Right` steer · `Up` fire · `Shoot` pause.

### 🧱 Tetris
**The classic, tuned for a tiny screen.** A centred board with solid square blocks, a next-piece preview, lines, level and time cards, and smooth held-button repeat so pieces glide exactly where you want them.

### 🐍 Snake
**Nokia nostalgia, now in colour.** Eat, grow, don't crash. Pick a wall layout (no walls, twin bars, four corners or a cross) and see how long you last.

### 🏓 Breakout
**Clear every brick.** Launch the ball, bounce it off your bat and smash through six rows of bricks.
`Left / Right` move · `Up` launch · `Shoot` pause.

### 🧩 Sliding Puzzle
**Slide, think, repeat.** Put the tiles back in order on a 4×4, 5×5 or 6×6 grid, and keep an eye on your move counter.

### ⭕ Tic-tac-toe
**Get three in a row.** You are X, the console plays O. `D-pad` moves, `Shoot` places your mark.

### 🏓 Pong
**Keep the ball in play.** Slide your paddle and survive before you rack up three misses.
`Left / Right` move · `Shoot` pause.

### 💣 Minesweeper
**Reveal every safe tile.** An 8×8 board hides ten mines, numbers count the danger nearby, and your first tap is always safe. Flag the mines you've found and clear the rest.

## 🔬 The lab

### 🧬 Conway's Game of Life
**Watch a tiny universe come alive.** Starts straight into a random 112×96 world that runs on the classic B3/S23 rules with wrap-around edges. Speed it up to 30 generations per second, pause, or open the built-in field guide with animated gliders, blinkers and pulsars.

### 🌀 Mandelbrot Explorer
**Dive into infinity.** Pan with the D-pad and zoom from 1x to over a million times into the famous fractal. It refines the picture progressively, 8 pixels down to 1, so you can keep moving while it paints.

## 🎧 Media & system

| | |
| :--- | :--- |
| 🎵 **Music** | A jukebox for the tracks on your SD card, with transport controls and a playlist. |
| 🎬 **Videos** | Plays video and audio from `/videos/<name>/` on the SD card. |
| ⚙️ **Settings** | Make it yours: 8 colour themes, brightness, sound, effects and music volume, game music, battery saver and score reset. |
| ℹ️ **About** | Console info screen. |

**Settings** cover theme, brightness, sound, effects volume, music volume, game music, battery saver and score reset.

## 🧰 Hardware

| Part | Details |
| :--- | :--- |
| 🧠 MCU | ESP32-C3 (`esp32-c3-devkitm-1`) |
| 🖥️ Display | ILI9341, 240×320 portrait, SPI at 80 MHz writes / 6 MHz reads |
| 🎛️ Controls | Up, Shoot, Down and Left through an ADS1115 ADC (I2C `0x48`); Right on GPIO 8 |
| 💾 Storage | SD card for music and video |
| 🔈 Audio | MAX98357A I2S amplifier and speaker |

### 🔌 Pinout

Taken from the firmware: the display pins from `lib/TFT_eSPI/User_Setups/Setup70c_ESP32_C3_ILI9341.h`, everything else from `src/config.h` and `src/buttons.cpp`.

| Signal | ESP32-C3 GPIO | Notes |
| :--- | :---: | :--- |
| Display CS | 4 | SPI display |
| Display DC | 3 | Data / command |
| Display MOSI | 2 | SPI display |
| Display SCK | 1 | SPI display |
| Display MISO | 21 | SPI display |
| Display RST | 3V3 | `TFT_RST -1`, tied to 3V3 |
| Backlight | 0 | Active HIGH |
| SD card CS | 9 | Shares the SPI bus with the display |
| I2S BCLK | 6 | MAX98357A |
| I2S LRCLK | 7 | MAX98357A |
| I2S DOUT | 5 | MAX98357A DIN |
| I2C SDA | 10 | ADS1115 bus |
| I2C SCL | 20 | ADS1115 bus |
| Right button | 8 | Input with pull-up, active LOW (10k pull-up) |

| Button | ADS1115 input |
| :--- | :---: |
| Up | A0 |
| Shoot | A1 |
| Down | A2 |
| Left | A3 |

> Backlight is HIGH = on. The Shoot button is read from ADS1115 channel A1, not from a GPIO.

### 📐 Schematic

Drawn in EasyEDA (`gameConsoleDIY`, V1.0, updated 2026-11-09): ESP32-C3 Super Mini, ILI9341 display with SD slot, ADS1115 button reader, MAX98357A amplifier, TP4056 battery charger and six tactile switches.

## 🚀 Quick start

You need [PlatformIO](https://platformio.org/).

```sh
pio run                # build
pio run -t upload      # flash the console
pio device monitor     # serial monitor at 115200 baud
```

<details>
<summary><b>Flashing tips</b></summary>

- USB CDC on boot is on, so serial output comes over the native USB port.
- The partition table is `console_partitions.csv`.
- Close the serial monitor before flashing, otherwise the port is busy.
- `tools/flasher_gui.py` is a small point-and-click flasher.

</details>

## 🎛️ Controls

The same buttons do the same thing everywhere.

| Button | Action |
| :--- | :--- |
| ⬅️ ➡️ ⬆️ ⬇️ D-pad | Move, choose, change a value |
| 🔴 Shoot (tap) | Select or act |
| 🔴 Shoot (hold) | Pause or open options |

Every game's start card lists its exact controls.

## 🗂️ Project layout

```text
src/
├── gameConsoleNew.ino     entry point
├── game.h / game.ino      engine, input, high scores, countdown, tone task
├── menu.ino               launcher, Settings, About
├── ui_kit.h / .cpp        shared ui:: helpers (cards, rows, pause sheets)
├── buttons.cpp            ADS1115 button scanning
├── audio_kit.*            sound effects and music helpers
├── sd_game_music.*        embedded game music
├── snake / tetris / breakout / run / galaga / sliding_puzzle / mini_games
├── rally.cpp  life.cpp  mandelbrot.cpp
└── music_player.cpp  video_player.cpp
lib/TFT_eSPI/              customised display driver
assets/                    graphics, audio, generated data
tools/                     asset converters, flasher, music selector
tests/                     host-side tests
docs/                      audio and game-music notes
menu_preview/              menu-only preview firmware (separate project)
```

## 🛠️ Tools

| Tool | What it does |
| :--- | :--- |
| `tools/make_icons.py`, `tools/build_home_assets.py` | Rebuild launcher icons and wallpaper headers |
| `tools/select_game_music.py`, `tools/prepare_game_music.py` | Pick and convert embedded game music (see `docs/game-music.md`) |
| `tools/yt_to_console.py` | Turn a video into the `/videos/<name>/` layout (`video.mjpeg`, `audio.wav`, `info.txt`) |
| `tools/flasher_gui.py` | GUI flasher |

## ✅ Tests

Host-side tests live in `tests/` and cover audio, puzzle, rally, run, breakout, galaga and life. Tetris input checks:

```sh
python3 tools/check_tetris_input.py
python3 tools/check_tetris_controls.py
```

## 📝 Developer notes

- Keep new large buffers off static RAM; allocate them while the app runs.
- UI text uses TFT_eSPI fonts 2 and 4 only, so avoid characters they can't draw.
- SPI clock is set in `lib/TFT_eSPI/User_Setups/Setup70c_ESP32_C3_ILI9341.h` (`SPI_FREQUENCY` 80 MHz, `SPI_READ_FREQUENCY` 6 MHz). Measured at 40 MHz, raising the clock from 4 to 40 MHz took full-screen transfers from 3.2 to about 28.8 per second.
