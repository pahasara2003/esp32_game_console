#!/usr/bin/env python3
"""Convert CraftPix Vampire Girl sheets to compact RGB565 console sprites."""

from pathlib import Path
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "run-character"
OUTPUT = ROOT / "src" / "vampire_runner_sprites.h"
PREVIEW = SOURCE / "vampire-runner-preview.png"

FRAME_W = 128
FRAME_H = 128
OUT_W = 32
OUT_H = 48
FRAME_COUNT = 6
DEAD_FRAME_COUNT = 10
DEAD_OUT_W = 52
TRANSPARENT = 0xF81F

# One common crop keeps the character anchored consistently between frames.
CROP = (34, 52, 83, 128)


def rgb565(pixel):
    r, g, b, a = pixel
    if a < 128:
        return TRANSPARENT
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def convert_sheet(name, frame_count=FRAME_COUNT, crop=CROP, out_w=OUT_W):
    sheet = Image.open(SOURCE / f"{name}.png").convert("RGBA")
    frames = []
    previews = []
    for index in range(frame_count):
        frame = sheet.crop((index * FRAME_W, 0, (index + 1) * FRAME_W, FRAME_H))
        frame = frame.crop(crop).resize((out_w, OUT_H), Image.Resampling.NEAREST)
        canvas = Image.new("RGBA", (out_w, OUT_H), (0, 0, 0, 0))
        canvas.alpha_composite(frame, (0, 0))
        frames.append([rgb565(pixel) for pixel in canvas.getdata()])
        previews.append(canvas)
    return frames, previews


def emit_array(lines, symbol, frames, count_symbol="VAMPIRE_FRAME_COUNT", pixels_symbol="VAMPIRE_FRAME_PIXELS"):
    lines.append(f"const uint16_t {symbol}[{count_symbol}][{pixels_symbol}] PROGMEM = {{")
    for frame in frames:
        lines.append("  {")
        for start in range(0, len(frame), 16):
            values = ", ".join(f"0x{value:04X}" for value in frame[start:start + 16])
            lines.append(f"    {values},")
        lines.append("  },")
    lines.append("};")
    lines.append("")


def main():
    run_frames, run_preview = convert_sheet("Run")
    jump_frames, jump_preview = convert_sheet("Jump")
    dead_frames, dead_preview = convert_sheet("Dead", DEAD_FRAME_COUNT, (5, 52, 87, 128), DEAD_OUT_W)

    lines = [
        "#pragma once",
        "#include <Arduino.h>",
        "",
        "constexpr uint8_t VAMPIRE_FRAME_W = 32;",
        "constexpr uint8_t VAMPIRE_FRAME_H = 48;",
        "constexpr uint8_t VAMPIRE_FRAME_COUNT = 6;",
        "constexpr uint16_t VAMPIRE_TRANSPARENT = 0xF81F;",
        "constexpr uint16_t VAMPIRE_FRAME_PIXELS = VAMPIRE_FRAME_W * VAMPIRE_FRAME_H;",
        "constexpr uint8_t VAMPIRE_DEAD_FRAME_W = 52;",
        "constexpr uint8_t VAMPIRE_DEAD_FRAME_COUNT = 10;",
        "constexpr uint16_t VAMPIRE_DEAD_FRAME_PIXELS = VAMPIRE_DEAD_FRAME_W * VAMPIRE_FRAME_H;",
        "",
    ]
    emit_array(lines, "vampire_run_frames", run_frames)
    emit_array(lines, "vampire_jump_frames", jump_frames)
    emit_array(lines, "vampire_dead_frames", dead_frames,
               "VAMPIRE_DEAD_FRAME_COUNT", "VAMPIRE_DEAD_FRAME_PIXELS")
    OUTPUT.write_text("\n".join(lines))

    scale = 4
    gap = 8
    cell_w = DEAD_OUT_W * scale
    preview = Image.new("RGBA", ((cell_w + gap) * DEAD_FRAME_COUNT - gap,
                                 OUT_H * scale * 3 + gap * 2), (18, 20, 34, 255))
    for row, frames in enumerate((run_preview, jump_preview, dead_preview)):
        for column, frame in enumerate(frames):
            enlarged = frame.resize((frame.width * scale, OUT_H * scale), Image.Resampling.NEAREST)
            x = column * (cell_w + gap) + (cell_w - enlarged.width) // 2
            preview.alpha_composite(enlarged, (x, row * (OUT_H * scale + gap)))
    preview.save(PREVIEW)


if __name__ == "__main__":
    main()
