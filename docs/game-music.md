# Embedded music during games

All 12 games play selected mono MP3 loops from firmware flash. The SD card is
not needed for game music. Eight unique tracks are stored once; shared games
reference the same audio bytes:

| Shared music | Games |
| --- | --- |
| Tetris | Tetris, Pong |
| Snake | Snake, Tic-tac-toe |
| Galaga | Galaga, Minesweeper |
| Life | Life, Mandelbrot |

Breakout, Run, Puzzle and Rally each have their own track. The selected start/end region
of each supplied source is converted to 22050 Hz mono at 32 kbps. New selections default to the first 30 seconds (or the whole song if shorter). Original SD files are unchanged.

## Regenerate

Choose a segment by listening first:

```sh
python3 tools/select_game_music.py /path/to/card/GameMusic
```

The local browser tool lists unique songs with their shared games. Use the two timeline handles to choose independent start/end points, then
**Preview selected loop**. Number fields allow precise timing, and **Set start
here** / **Set end here** use the current playback position.
**Save all selections** writes `assets/game_music_selection.json`. Choices are
keyed by source audio hash so shared games keep the same segment. The converter
uses both saved boundaries. Existing start-only selections remain compatible.
Each loop can have its own duration (at least one second); the storage meter
estimates the combined size. Saving rejects selections above the 1344 KiB
audio budget, leaving space for the rest of the firmware.
Saving does not modify the current firmware or flash the console.

```sh
python3 tools/prepare_game_music.py /path/to/card/GameMusic
python3 tests/audio/test_sd_music.py
pio run
```

Source filenames are tetris.mp3, snake.mp3, breakout.mp3, run.mp3, galaga.mp3,
puzzle.mp3, tic_tac_toe.mp3, pong.mp3, minesweeper.mp3, life.mp3,
mandelbrot.mp3 and rally.mp3. The script validates conversions, deduplicates
identical sources/audio and limits the bundle to 1344 KiB. It generates
`assets/game_music.bin`, `assets/game_music.json` and `src/game_music_tracks.h`.
PlatformIO embeds the bundle; the manifest records duration, offsets and hashes.
The custom app partition reclaims the previously unused filesystem partition,
while preserving saved settings and coredump offsets.

A new run restarts its assigned track. Tracks loop during active gameplay;
pause menus and countdowns preserve playback position. Ending sounds take
priority. Leaving a game releases the decoder. Music and Videos retain exclusive
speaker ownership and still use their own SD libraries. Sound, Game music and
Music volume settings continue to apply.

The existing I2S task decodes from flash outside the mixer critical section.
The legacy `sd_music` API and source/test filenames remain to avoid changing
unrelated callers; they no longer mount or read an SD card.

The host test uses the real installed Helix decoder and the actual embedded
bundle. It checks every game for more than two loops, exact
pause/resume and restart continuity, mute, invalid games, shared mappings and
bundle hashes. Speaker quality and gameplay performance need a hardware check.
