#!/usr/bin/env python3
"""Preview the supplied cave composition; --encode prepares console assets."""
import argparse
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets' / 'crystal-cave-reference.png'
OUT = ROOT / 'src' / 'crystal_cave_world.h'
PREVIEW = ROOT / 'assets' / 'crystal-cave-preview.png'


def rgb565(rgb):
    r, g, b = rgb
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--encode', action='store_true')
    args = parser.parse_args()
    # This static scene uses the user's exact reference, not reconstructed tiles.
    # Preserve its geometry at 1:1 scale; the display will pan over it.
    world = Image.open(SOURCE).convert('RGB')
    W, H = world.size
    world.save(PREVIEW)
    if not args.encode:
        print(f'Preview only: {W}x{H}; console assets unchanged.')
        return

    # A 128-color indexed image keeps the native composition while fitting flash.
    indexed = world.convert('RGB').quantize(colors=128, method=Image.Quantize.MEDIANCUT)
    palette = indexed.getpalette()[:128 * 3]
    rgb_palette = [rgb565(tuple(palette[i:i + 3])) for i in range(0, len(palette), 3)]
    pixels = list(indexed.getdata())
    encoded = bytearray()
    rows = [0]
    for y in range(H):
        row = pixels[y * W:(y + 1) * W]
        start = 0
        while start < W:
            color = row[start]
            length = 1
            while start + length < W and row[start + length] == color and length < 255:
                length += 1
            encoded.extend((length, color))
            start += length
        rows.append(len(encoded))

    lines = [
        '#pragma once', '#include <Arduino.h>', '',
        f'constexpr uint16_t CRYSTAL_WORLD_W = {W};',
        f'constexpr uint16_t CRYSTAL_WORLD_H = {H};',
        'constexpr uint16_t CRYSTAL_WORLD_PALETTE = 128;',
        f'constexpr uint32_t CRYSTAL_WORLD_DATA_SIZE = {len(encoded)};', '',
        'const uint16_t crystal_world_palette[CRYSTAL_WORLD_PALETTE] PROGMEM = {',
    ]
    for i in range(0, len(rgb_palette), 8):
        lines.append('  ' + ', '.join(f'0x{x:04X}' for x in rgb_palette[i:i + 8]) + ',')
    lines += ['};', '', 'const uint32_t crystal_world_rows[CRYSTAL_WORLD_H + 1] PROGMEM = {']
    for i in range(0, len(rows), 8):
        lines.append('  ' + ', '.join(str(x) for x in rows[i:i + 8]) + ',')
    lines += ['};', '', 'const uint8_t crystal_world_data[CRYSTAL_WORLD_DATA_SIZE] PROGMEM = {']
    for i in range(0, len(encoded), 24):
        lines.append('  ' + ', '.join(str(x) for x in encoded[i:i + 24]) + ',')
    lines += ['};', '']
    OUT.write_text('\n'.join(lines))
    world.save(PREVIEW)


if __name__ == '__main__':
    main()
