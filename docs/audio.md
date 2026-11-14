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
background loops. No downloaded tracks are required for these loops.

The Nokia welcome MP3 is embedded in firmware from `assets/nokia_welcome.mp3`
using PlatformIO's binary embedding. Playback reads directly from flash and
does not mount the SD card or allocate an SD read buffer. The older SD copy
`/System/Nokia Tune.mp3` is no longer used. Regular user music stays in `/Music`.
Reserve SD `/System/Music/` and `/System/Sounds/` for future long system assets.

The media apps retain exclusive I2S ownership during playback. Game sound
requests are discarded while media owns the output, so clicks cannot interrupt
a song or a video. Synthesized effects do not run in a second competing writer.

Host synthesis checks (NVS and hardware are stubbed):

```
g++ -std=c++11 -Itests/audio -Isrc src/audio_kit.cpp tests/audio/test_audio.cpp -o /tmp/console-audio-test
/tmp/console-audio-test
```

Hardware checks: compare effect/music levels; verify no missed controls during
rapid effects; pause/resume each game; enter and leave Music and Videos; restart
to verify saved settings and clean Nokia welcome playback.
