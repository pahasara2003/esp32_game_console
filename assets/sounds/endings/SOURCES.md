# Game ending sounds

Downloaded from OpenGameArt.org on 2026-11-16. Each source lists CC0 (public domain dedication); attribution is not required.

| Local file | Original title | Author | Source |
| --- | --- | --- | --- |
| game_over.wav | Game Over (8-bit style) | Aszura | https://opengameart.org/content/game-over-8-bit-style |
| game_over_kosmo.wav | Game over short jingle | Kosmo The Cat | https://opengameart.org/content/game-over-short-jingle |
| game_over_greyfrog.wav | Game Over | GreyFrogGames | https://opengameart.org/content/game-over-10 |
| game_over_old_school.wav | Game Over Sound(Old School) | den_yes | https://opengameart.org/content/game-over-soundold-school |
| game_over_previous.wav | Game Over (8-bit style) | Aszura | https://opengameart.org/content/game-over-8-bit-style |
| lose.wav | Game Over Trumpet SFX | 0new4y | https://opengameart.org/content/game-over-trumpet-sfx |
| win.wav | Victory | celestialghost8 | https://opengameart.org/content/victory |

All files are original WAV files, renamed only, except game_over_kosmo.wav which was converted from OGG to 16-bit PCM WAV without trimming. The three selected clips are integrated into the console firmware through their `_embedded.wav` versions. Alternative auditions are not embedded.

Original download URLs:

- https://opengameart.org/sites/default/files/GameOver_0.wav
- https://opengameart.org/sites/default/files/GameOver.wav
- https://opengameart.org/sites/default/files/losetrumpet.wav
- https://opengameart.org/sites/default/files/Victory.wav

- https://opengameart.org/sites/default/files/gameover.wav

- https://opengameart.org/sites/default/files/Game%20over%20jingle_0.ogg

## Embedded versions

`game_over_embedded.wav`, `lose_embedded.wav`, and `win_embedded.wav` derive from
the corresponding selected WAV files. `tools/prepare_ending_sounds.py` normalizes
their loudness, downmixes to mono at 22050 Hz and encodes IMA ADPCM with 1024-byte
blocks. The full selected clips are retained. Playback uses Effects volume.
See `docs/audio.md` for the game/event mapping.
