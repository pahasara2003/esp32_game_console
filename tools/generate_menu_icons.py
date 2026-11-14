from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / "assets" / "menu-icons"
OUTPUT_DIR = SOURCE_DIR / "processed"
HEADER_PATH = ROOT / "src" / "modern_icons_40x40.h"
ICON_NAMES = ["tetris", "snake", "breakout", "run", "galaga", "sound", "settings", "about",
              "tic_tac", "pong", "mines", "life", "gallery", "mandelbrot"]
SIZE = 40
ART_SIZE = 36
ALPHA_CUTOFF = 96
TRANSPARENT_KEY = 0xF81F


def rgb565(r: int, g: int, b: int) -> int:
    value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    # Reserve magenta exclusively for transparency. Nudge an opaque collision.
    return 0xF81E if value == TRANSPARENT_KEY else value


def harden_alpha(image: Image.Image) -> Image.Image:
    output = Image.new("RGBA", image.size)
    output.putdata([
        (r, g, b, 255 if a >= ALPHA_CUTOFF else 0)
        for r, g, b, a in image.getdata()
    ])
    return output


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    arrays = []
    for name in ICON_NAMES:
        source = Image.open(SOURCE_DIR / f"{name}.png").convert("RGBA")
        bbox = source.getchannel("A").point(lambda a: 255 if a >= 16 else 0).getbbox()
        if bbox is None:
            raise ValueError(f"{name}.png has no visible pixels")
        source = source.crop(bbox)
        scale = min(ART_SIZE / source.width, ART_SIZE / source.height)
        size = (max(1, round(source.width * scale)), max(1, round(source.height * scale)))
        source = source.convert("RGBa").resize(size, Image.Resampling.LANCZOS).convert("RGBA")
        icon = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
        icon.alpha_composite(source, ((SIZE - size[0]) // 2, (SIZE - size[1]) // 2))
        icon = harden_alpha(icon)
        icon.save(OUTPUT_DIR / f"{name}-40.png")

        values = []
        for r, g, b, a in icon.getdata():
            values.append(TRANSPARENT_KEY if a == 0 else rgb565(r, g, b))
        rows = []
        for start in range(0, len(values), 12):
            rows.append("  " + ", ".join(f"0x{value:04X}" for value in values[start:start + 12]))
        arrays.append(
            f"const uint16_t {name}_icon[1600] PROGMEM = {{\n" + ",\n".join(rows) + "\n};"
        )

    header = """#pragma once

#include <stdint.h>
#if defined(ARDUINO)
#include <pgmspace.h>
#elif !defined(PROGMEM)
#define PROGMEM
#endif

// Generated from alpha-transparent artwork in assets/menu-icons.
// 0xF81F is reserved as the transparent key. Each icon is 40x40 RGB565.

""" + "\n\n".join(arrays) + "\n"
    HEADER_PATH.write_text(header)


if __name__ == "__main__":
    main()
