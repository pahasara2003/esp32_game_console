#!/usr/bin/env python3
"""Convert the supplied four-frame rocket sheet to compact indexed sprites."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets' / 'rocket-sprite-sheet.png'
OUT = ROOT / 'src' / 'rocket_demo_frames.h'
PREVIEW = ROOT / 'assets' / 'rocket-demo-preview.gif'
FRAME_W, FRAME_H = 80, 110
SPRITE_MAX_W, SPRITE_MAX_H = 60, 90
SPRITE_TOP = 10


def main():
    sheet = Image.open(SOURCE).convert('RGB')
    cells = []
    bounds = []
    for index in range(4):
        cell = sheet.crop((index * sheet.width // 4, 0,
                           (index + 1) * sheet.width // 4, sheet.height))
        cells.append(cell)
        bounds.append(cell.point(lambda p: 255 if p > 18 else 0).getbbox())

    # Use one crop for every frame. A per-frame crop recenters frames with longer
    # flames and makes the whole rocket appear to jump vertically.
    shared_box = (
        min(box[0] for box in bounds),
        min(box[1] for box in bounds),
        max(box[2] for box in bounds),
        max(box[3] for box in bounds),
    )
    frames = []
    for cell in cells:
        sprite = cell.crop(shared_box)
        # About one-third smaller than the previous 120 x 170 presentation.
        sprite.thumbnail((SPRITE_MAX_W, SPRITE_MAX_H), Image.Resampling.NEAREST)
        canvas = Image.new('RGB', (FRAME_W, FRAME_H), (0, 0, 0))
        canvas.paste(sprite, ((FRAME_W - sprite.width) // 2, SPRITE_TOP))
        frames.append(canvas)

    # Shared indexed palette keeps the four pixel-art frames small in flash.
    sheet4 = Image.new('RGB', (FRAME_W * 4, FRAME_H), (0, 0, 0))
    for i, frame in enumerate(frames): sheet4.paste(frame, (i * FRAME_W, 0))
    indexed = sheet4.quantize(colors=64, method=Image.Quantize.MEDIANCUT)
    palette = indexed.getpalette()[:64 * 3]
    rgb565 = [((palette[i] & 0xF8) << 8) | ((palette[i + 1] & 0xFC) << 3) |
              (palette[i + 2] >> 3) for i in range(0, len(palette), 3)]
    lines = ['#pragma once', '#include <Arduino.h>', '',
             f'constexpr uint16_t ROCKET_FRAME_W = {FRAME_W};',
             f'constexpr uint16_t ROCKET_FRAME_H = {FRAME_H};',
             'constexpr uint8_t ROCKET_FRAME_COUNT = 4;',
             'constexpr uint8_t ROCKET_PALETTE_SIZE = 64;', '',
             'const uint16_t rocket_palette[ROCKET_PALETTE_SIZE] PROGMEM = {']
    for i in range(0, len(rgb565), 8):
        lines.append('  ' + ', '.join(f'0x{x:04X}' for x in rgb565[i:i + 8]) + ',')
    lines += ['};', '',
              'const uint8_t rocket_frames[ROCKET_FRAME_COUNT][ROCKET_FRAME_W * ROCKET_FRAME_H] PROGMEM = {']
    for frame in range(4):
        lines.append('  {')
        # The atlas is row-major, so crop each frame before flattening it.
        # Slicing the atlas buffer directly mixes scanlines from all 4 frames.
        frame_image = indexed.crop((frame * FRAME_W, 0,
                                    (frame + 1) * FRAME_W, FRAME_H))
        data = list(frame_image.getdata())
        for i in range(0, len(data), 24):
            lines.append('    ' + ', '.join(str(x) for x in data[i:i + 24]) + ',')
        lines.append('  },')
    lines += ['};', '']
    OUT.write_text('\n'.join(lines))
    frames[0].save(PREVIEW, save_all=True, append_images=frames[1:],
                   duration=120, loop=0, disposal=2)


if __name__ == '__main__': main()
