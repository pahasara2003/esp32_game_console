# Console audio

All system effects and game loops use the MAX98357A over I2S, at 22050 Hz.
`src/audio_kit.cpp` synthesizes original triangle-wave effects and sparse music
loops without SD reads or heap allocations in the playback task. Notes have
short attack/release envelopes. Effects duck the music and higher-priority
events interrupt lower-priority ones. Legacy birthday/jukebox tones still work.

Settings has Effects volume, Music volume, and Game music controls. Shoot saves
them to NVS. Music volume also scales the Nokia welcome tune, MP3 player and
video audio. Sound off mutes synthesized system/game audio and the welcome tune;
the media apps have their separate music volume. Game music is quieter than
effects and pauses on game menus. Snake, Pong, Tic-tac-toe and Mines stay quiet
between effects. Tetris, Breakout, Run, Galaga, Puzzle, Life and Mandelbrot have
background loops. Breakout uses an original 116 BPM modern happy synth-pop groove (warm major
chords, bouncy pop bass, soft kick pulse and sunlit marimba pluck melody) rather
than a retro sequence. Its music has more presence, with smooth effect ducking;
the existing Music volume still applies.
No downloaded tracks are required for these loops. Optional per-game SD MP3s
replace the built-in loop during active gameplay; see [SD game music](game-music.md)
for filenames and playback rules.

The Nokia welcome MP3 is embedded in firmware from `assets/nokia_welcome.mp3`
using PlatformIO's binary embedding. Playback reads directly from flash and
does not mount the SD card or allocate an SD read buffer. The older SD copy
`/System/Nokia Tune.mp3` is no longer used. Regular user music stays in `/Music`.
Reserve SD `/System/Music/` and `/System/Sounds/` for future long system assets.

The media apps retain exclusive I2S ownership during playback. Game sound
requests are discarded while media owns the output, so clicks cannot interrupt
a song or a video. Synthesized effects do not run in a second competing writer.

The selected game result recordings are embedded in flash as mono 22050 Hz
IMA ADPCM, decoded incrementally by the same I2S task. No SD card is required.
Effects volume and Sound off apply; Music volume and Game music do not mute
these recordings. Each plays once, takes priority over clicks/high-score beeps,
and temporarily replaces the background music. Controls stay responsive.
Restarting, leaving a game, or entering media stops a recording.

Selected clips (all CC0; details in `assets/sounds/endings/SOURCES.md`):

- Game over: Aszura's original "Game Over (8-bit style)", 9.24 seconds.
- Lose: 0new4y's "Game Over Trumpet SFX", 1.11 seconds.
- Win: celestialghost8's "Victory", 4.88 seconds.

| Game | Game over recording | Lose recording | Win recording |
| --- | --- | --- | --- |
| Tetris | A new piece cannot spawn (stack reaches the top) | — | — |
| Snake | Hit a wall or own body | — | Fill every available cell |
| Breakout | Lose the final ball/life | Miss a ball with lives remaining | Clear every brick in a level |
| Run | Hit an obstacle | — | — |
| Galaga | Lose the final ship/life | Ship hit by a bullet or alien, with lives remaining | Clear a wave |
| Sliding puzzle | — | — | Solve the puzzle, at any board size |
| Tic-tac-toe | — | Console wins the round | Player gets three in a row |
| Pong | Miss the final ball/life | Miss a ball with lives remaining | — |
| Minesweeper | — | Reveal a mine | Reveal all safe cells |
| Rally | Timer reaches zero in 4th-6th place | — | Finish in 1st-3rd place |
| Life / Mandelbrot | — | — | — |

Rally finishes in 1st, 2nd or 3rd place show a full-screen gold, silver or bronze
medal poster with the place label, score, best score, total distance and replay controls.
The other result screen also shows total distance. Yellow lines mark lap boundaries;
ordinary checkpoints stay checkered. Lap completion plays the short four-tone
Milestone chime (335 ms); the full victory recording plays only for a podium finish. Cyan text with a transparent background overlays the track with the current lap
and meters to the next lap, using the existing scale (10 internal distance units per displayed
meter, 2.4 km per lap). Score is floor(internal distance / 10) + 20 * coins,
equivalent to one point per displayed meter plus 20 per coin. Rival bumps grant
three coins (60 points); finishing place changes the medal, not the score.

Life and Mandelbrot have no win/loss outcome. Tic-tac-toe draws keep the existing
short draw cue; pickups, line clears, checkpoints and other routine actions keep
their existing short effects. Endless games do not get an artificial victory.

`console_partitions.csv` enlarges the app from 3 MiB to 3.25 MiB using part of
the unused internal SPIFFS area. NVS (settings/high scores) and coredump offsets
stay unchanged. Flash the partition table together with the firmware using the
normal PlatformIO upload; copying only firmware.bin onto an old partition table
will not fit. SD card contents are independent of these partitions.

Regenerate the embedded clips (FFmpeg required):

```
python3 tools/prepare_ending_sounds.py
```

Host checks compare all three decoded recordings against FFmpeg, check callback
continuity, termination, volume/mute, priority and restart cancellation, and run
the existing synthesis/music checks (NVS and hardware are stubbed):

```
python3 tests/audio/test_puzzle.py
pio run
```

Hardware checks: compare effect/music levels; verify no missed controls during
rapid effects; pause/resume each game; enter and leave Music and Videos; restart
to verify saved settings and clean Nokia welcome playback.
