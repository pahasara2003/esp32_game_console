#!/usr/bin/env python3
"""Compose a panning Crystal Caves showcase and encode it for the ESP32."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ASSET = Path('/home/pahasara/Downloads/craftpix-net-376431-free-crystal-caves-2d-platformer-tileset/PNG')
OUT = ROOT / 'src' / 'crystal_cave_world.h'
PREVIEW = ROOT / 'assets' / 'crystal-cave-preview.png'
W, H = 640, 360


def find(folder, text):
    return next((ASSET / folder).glob(f'*{text}*.png'))


def add(world, folder, text, x, y, size):
    sprite = Image.open(find(folder, text)).convert('RGBA')
    sprite.thumbnail(size, Image.Resampling.LANCZOS)
    world.alpha_composite(sprite, (x, y))


def rgb565(rgb):
    r, g, b = rgb
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def main():
    layer0 = Image.open(find('Background', 'Layer 00')).convert('RGBA').crop((300, 0, 300 + W, H))
    layer1 = Image.open(find('Background', 'Layer 01')).convert('RGBA').crop((300, 0, 300 + W, H))
    world = Image.alpha_composite(layer0, layer1)

    # Platforms, bridges, ladders and scenery are placed in separate depth bands.
    for x, y, tile in ((0, 190, 'Ground 08'), (180, 150, 'Ground 12'),
                       (395, 190, 'Ground 10'), (65, 260, 'Ground 02'),
                       (250, 285, 'Ground 11'), (500, 245, 'Ground 04')):
        add(world, 'Platfromer', tile, x, y, (128, 128))
    add(world, 'Platfromer', 'Ladder', 455, 100, (80, 130))
    add(world, 'Platfromer', 'Ladder', 290, 200, (70, 100))
    add(world, 'Environment', 'Tree', 16, 0, (180, 240))
    add(world, 'Environment', 'Crystal 01', 115, 210, (70, 70))
    add(world, 'Environment', 'Crystal 02', 220, 115, (70, 70))
    add(world, 'Environment', 'Fire', 90, 155, (64, 64))
    add(world, 'Environment', 'Crystal pile 02', 350, 295, (120, 48))
    add(world, 'Collectable Object', 'Star', 560, 180, (48, 48))
    add(world, 'Collectable Object', 'Crystal', 350, 115, (48, 48))
    add(world, 'Collectable Object', 'Coin 01', 310, 80, (40, 40))
    add(world, 'Environment', 'Signpost 02', 575, 235, (64, 64))
    add(world, 'Platfromer', 'Spike', 430, 320, (128, 64))

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
