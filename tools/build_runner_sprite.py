#!/usr/bin/env python3
"""Generate display-sized, lossless palette/RLE sprites from CraftPix sheets."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets/run-character'
OUTPUT = ROOT / 'src/vampire_runner_sprites.h'
CHARACTERS = (
    ('Vampire girl', SOURCE, (34, 52, 83, 128)),
    ('Countess', SOURCE / 'Countess_Vampire', (34, 39, 94, 128)),
    ('Converted vampire', SOURCE / 'Converted_Vampire', (20, 30, 88, 128)),
)
ACTIONS = ('Run', 'Jump', 'Dead', 'Attack_1')
TRANSPARENT = 0xF81F


def rgb565(pixel):
    r, g, b, a = pixel
    return TRANSPARENT if a < 128 else ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3)


def encode(pixels):
    result = []
    for value in pixels:
        if result and result[-1] == value and result[-2] < 255:
            result[-2] += 1
        else:
            result.extend((1, value))
    return result


def decode(data):
    return [value for count, value in zip(data[::2], data[1::2]) for _ in range(count)]


def frames_for(folder, crop, action):
    sheet = Image.open(folder / (action + '.png')).convert('RGBA')
    assert sheet.height == 128 and sheet.width % 128 == 0
    width = 40 if action in ('Run', 'Jump') else 80 if action == 'Dead' else 100
    offset = 0 if width == 40 else -24 if action == 'Dead' else -20
    left, top, right, bottom = crop
    source_w = right - left
    extended = (left + offset * source_w / 40, top,
                left + (offset + width) * source_w / 40, bottom)
    frames = []
    for x in range(0, sheet.width, 128):
        original = sheet.crop((x, 0, x + 128, 128))
        # A common anchor per character prevents feet/body jitter between actions.
        bounds = tuple(round(v) for v in extended)
        visible = original.getbbox()
        assert visible is None or (bounds[0] <= visible[0] and bounds[1] <= visible[1]
                                   and bounds[2] >= visible[2] and bounds[3] >= visible[3])
        frame = original.crop(bounds).resize((width, 60), Image.Resampling.NEAREST)
        frames.append(frame)
    return width, offset, frames


def array(lines, kind, name, values):
    lines.append(f'const {kind} {name}[] PROGMEM = {{')
    for i in range(0, len(values), 24):
        lines.append('  ' + ', '.join(str(v) for v in values[i:i+24]) + ',')
    lines.append('};')


def main():
    lines = ['#pragma once', '#include <Arduino.h>',
             'constexpr int VAMPIRE_FRAME_W = 40;', 'constexpr int VAMPIRE_FRAME_H = 60;',
             'constexpr int VAMPIRE_DEAD_FRAME_W = 80;',
             'constexpr uint16_t VAMPIRE_TRANSPARENT = 0xF81F;',
             'struct RunnerSprite { const uint8_t *data; const uint32_t *offsets; uint8_t count, width; int8_t x; };',
             'struct RunnerCharacter { const char *name; const uint16_t *palette; RunnerSprite actions[4]; };']
    characters = []
    for index, (name, folder, crop) in enumerate(CHARACTERS):
        animations = [frames_for(folder, crop, action) for action in ACTIONS]
        palette = sorted({rgb565(p) for _, _, frames in animations for f in frames for p in f.getdata()})
        assert len(palette) <= 256
        lookup = {color: i for i, color in enumerate(palette)}
        array(lines, 'uint16_t', f'runner_palette_{index}', palette)
        entries = []
        preview = Image.new('RGBA', (100 * 10, 60 * 4), (18, 20, 34, 255))
        for action, (width, offset, frames) in enumerate(animations):
            data, offsets = [], [0]
            for f, frame in enumerate(frames):
                pixels = [lookup[rgb565(p)] for p in frame.getdata()]
                encoded = encode(pixels)
                assert decode(encoded) == pixels and len(pixels) == width * 60
                data.extend(encoded)
                offsets.append(len(data))
                preview.alpha_composite(frame, (f * 100 + 24 + offset, action * 60))
            symbol = f'runner_{index}_{action}'
            array(lines, 'uint8_t', symbol, data)
            array(lines, 'uint32_t', symbol + '_offsets', offsets)
            entries.append(f'{{{symbol}, {symbol}_offsets, {len(frames)}, {width}, {offset}}}')
        preview.resize((2000, 480), Image.Resampling.NEAREST).save(folder / 'vampire-runner-preview.png')
        characters.append(f'{{"{name}", runner_palette_{index}, {{{", ".join(entries)}}}}}')
    lines.append('const RunnerCharacter runner_characters[] = {' + ',\n'.join(characters) + '};')
    OUTPUT.write_text('\n'.join(lines) + '\n')


if __name__ == '__main__':
    main()
