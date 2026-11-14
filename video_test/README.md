# Standalone ESP32-C3 AVI player

This project automatically plays `/gameconsole-video-320x240-hq.avi` from the SD card.
The supplied native-resolution file in `assets/` is MJPEG video at 320 × 240 and 12 FPS,
encoded at JPEG quantizer 3 (higher quality than the earlier quantizer 6), with
16-bit mono PCM audio at 22,050 Hz in the same AVI container.

## Wiring

| Device signal | GPIO |
| --- | --- |
| Shared SPI SCK | 1 |
| Shared SPI MOSI | 2 |
| Shared SPI MISO | 21 |
| TFT CS | 4 |
| TFT DC | 3 |
| TFT backlight | 0 |
| SD CS | 9 |
| I2S BCLK | 6 |
| I2S LRCLK | 5 |
| I2S data to MAX98357A | 7 |

TFT reset is tied to 3.3 V. Display rotation is 1 (320 × 240 landscape).
The source was center-cropped and resized during conversion to fill the screen.
The ESP32 sends decoded pixels directly to the display at native resolution.
Playback has no title, timer,
or button controls. Diagnostics go to serial; file/setup errors appear onscreen.

## Build and flash

```bash
PLATFORMIO_CORE_DIR=/tmp/gameconsole-pio-core ~/.local/bin/pio run -d /home/pahasara/gameConsole/video_test
PLATFORMIO_CORE_DIR=/tmp/gameconsole-pio-core ~/.local/bin/pio run -d /home/pahasara/gameConsole/video_test -t upload --upload-port /dev/ttyACM0
```

Serial diagnostics use 115200 baud and show displayed/skipped frame counts.
Playback ends on the last frame; reset the board to replay.

## Supported files

The parser accepts ordinary RIFF AVI with an MJPG video stream and one 16-bit
PCM audio stream (mono or stereo, 8–48 kHz). Frame dimensions must fit the
320 × 240 display exactly, and individual JPEG frames must be no larger than 32 KB.
The AVI video stream's suggested buffer size determines the reusable frame-slot
capacity; if absent, the player reserves the full 32 KB per slot.
It does not decode MP4, H.264, compressed AVI audio, or OpenDML multi-RIFF files.

The SD reader uses an 8 KB read cache and batches sequential card reads.
Up to six preallocated JPEG buffers eliminate allocation/free operations per frame.
The number of slots adapts to available heap while reserving room for the I2S
driver and tasks. Up to four compressed frames and 32 KB of PCM audio can queue ahead. The C3 runs
at 160 MHz and the project uses speed optimization. An independent I2S task
feeds the amplifier. Video follows the audio sample clock and skips overdue
frames. The SD reader runs above the display task, with I2S output above both,
so the audio queue is replenished promptly after each frame transfer.
Rendering starts just ahead of the frame deadline using measured decode
and display time. A mutex serializes SD and TFT access to their shared 40 MHz
SPI bus. Each video frame uses one display transaction to avoid repeatedly
reconfiguring SPI between JPEG MCU blocks. Runtime diagnostics only write when USB has room, avoiding a blocked
serial connection stalling video playback.

The HQ version was flashed and checked on the ESP32-C3. A short startup capture
at nine seconds reported 97 displayed frames and 11 skipped frames. Audio stayed
buffered and free heap stayed at 41,008 bytes. Thus the source targets 12 FPS,
but busy scenes can drop video frames to preserve audio timing. This was a short
serial check, not a complete visual or end-to-end playback assessment.

## Higher-quality conversion

The current file was made directly from the downloaded 480p source:

```bash
ffmpeg -i /tmp/gameconsole-source.mp4 -map 0:v:0 -map 0:a:0 \
  -vf 'fps=12,scale=320:240:force_original_aspect_ratio=increase:flags=lanczos,crop=320:240,setsar=1' \
  -c:v mjpeg -pix_fmt yuvj420p -q:v 3 \
  -c:a pcm_s16le -ar 22050 -ac 1 \
  -f avi gameconsole-video-320x240-hq.avi
```

This version is approximately 30.6 MiB. The largest compressed frame is 25,279
bytes, so its reusable frame slots round up to 25,600 bytes. JPEG quantizer 3
reduces compression artifacts; detail is still limited by the 320 × 240 panel
and by artifacts already present in the downloaded source.
