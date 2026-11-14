# Agent Instructions

## Project

- ESP32-C3 retro game console using PlatformIO, Arduino, TFT_eSPI, and an ILI9341 240x320 portrait display.
- Default environment: `esp32-c3-devkitm-1`; baud rate: `115200`.
- Read `CLAUDE.md` for the detailed hardware pinout, UI conventions, and known issues.

## Development

- Keep changes focused and preserve existing user work.
- Use the shared helpers in `src/ui_kit.h/.cpp` for screens.
- Use built-in fonts 2 and 4 only; keep labels sentence-case and avoid unsupported special characters.
- Build after firmware changes:

  ```bash
  pio run
  ```

- Flash only when requested or when the connected console is available. Close the serial monitor before flashing.
- Do not kill processes, delete files, reset history, or force-push without explicit approval.

## Git collaboration

- Repository: `https://github.com/pahasara2003/esp32_game_console.git`
- Branches: `claude/work` is for Claude; `codex/work` is for Codex. Each agent commits only to its own branch.
- Do not commit directly to `main`. The user reviews and merges into `main`.
- Inspect status before editing and commit focused changes with clear messages.
- Push only your own branch. Codex/user will review and merge it.
- Never pull, merge, or overwrite remote work unless explicitly requested.
- In handoffs, report changed files, build/test results, commit hash, and remaining hardware checks.

## Scope

- Avoid changing generated files, backups, or unrelated projects unless required.
- Preserve the existing UI style and consistent game controls.
