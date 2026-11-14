# Task: more attractive welcome (boot) screen

## Goal
Redesign the power-on welcome screen so it looks modern and phone-like, matching the menu launcher.

## Where it lives
- `src/game.ino` -> `playBootAnimation()` (about line 459). Called from `setup()` after `initI2SAudio()`.
- Draws with `tft` (TFT_eSPI, 240x320 portrait, `setRotation(2)`), uses `TFT_eSprite logo` (144x128, 16-bit, may fail to allocate; a fallback path exists).
- Helpers: `bootRgb565(r,g,b)`, `fillBootGradient(...)`, `drawBootHand(...)`, `drawBootSpark(...)`. Palette is local: ink (12,14,22), muted (115,123,140), lime (200,255,56).
- Fonts available: built-in fonts 2, 4, 6, 7, 8 (no GFX fonts unless added). Avoid non-ASCII characters such as "·".
- Keep the menu's look in mind: `src/ui_kit.h` (CARD, GROUP, LABEL, SECONDARY colours; `ui::` helpers), `src/menu.ino`.

## Constraints
- Total animation under about 2.5 s; must not block SD or I2C init that follows.
- Must still play cleanly if the sprite cannot be allocated (no crash).
- Keep `setTextSize(1)` / rotation behaviour so the rest of boot is unaffected.
- Do not change pins (see CLAUDE.md).
- Flash only when the user asks. The serial monitor must be closed first.

## Deliverable
- Edit `playBootAnimation()` only, unless a helper is needed.
- Run `pio run` to confirm it compiles. Do not flash.
- Describe the new layout in 3 to 5 lines.
