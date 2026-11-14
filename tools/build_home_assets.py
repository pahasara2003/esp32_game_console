"""Convert the generated wallpaper and existing transparent icons to RGB565.

Run from any directory: python tools/build_home_assets.py
"""
from pathlib import Path
from PIL import Image, ImageOps, ImageEnhance

ROOT = Path(__file__).resolve().parents[1]
HOME = ROOT / "assets/home"
ICON_DIR = ROOT / "assets/menu-icons"
NAMES = ["tetris", "snake", "breakout", "run", "galaga", "sound", "settings",
         "about", "tic_tac", "pong", "mines", "life", "mandelbrot"]
WALLPAPER_SOURCE = "abstract-blue-purple-source.png"
WALLPAPER_PREVIEW = "abstract-blue-purple-240x320.png"
PREFIX = "#pragma once\n#include <stdint.h>\n#include <pgmspace.h>\n\n"


def rgb565(r, g, b):
    return ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3)


def array(name, values):
    rows = ["  " + ", ".join(f"0x{v:04X}" for v in values[i:i+16])
            for i in range(0, len(values), 16)]
    return f"const uint16_t {name}[{len(values)}] PROGMEM = {{\n" + ",\n".join(rows) + "\n};\n"


def main():
    HOME.mkdir(exist_ok=True)
    wallpaper = ImageOps.fit(Image.open(HOME / WALLPAPER_SOURCE).convert("RGB"),
                             (240, 320), method=Image.Resampling.LANCZOS)
    # Bake a subtle 18% black tint into the wallpaper, keeping all redraw paths
    # and page transitions consistent without additional per-frame work.
    wallpaper = ImageEnhance.Brightness(wallpaper).enhance(0.82)
    wallpaper.save(HOME / WALLPAPER_PREVIEW)
    (ROOT / "src/home_wallpaper.h").write_text(
        PREFIX + "// User-selected abstract wallpaper, native RGB565; 240x320.\n" +
        array("home_wallpaper", [rgb565(*p) for p in wallpaper.getdata()]))
    arrays = []
    for name in NAMES:
        source = Image.open(ICON_DIR / f"{name}.png").convert("RGBA")
        bbox = source.getchannel("A").point(lambda a: 255 if a >= 16 else 0).getbbox()
        source = source.crop(bbox)
        ratio = min(52 / source.width, 52 / source.height)
        size = tuple(max(1, round(v * ratio)) for v in source.size)
        # Premultiplied resampling preserves clean colors at transparent edges.
        source = source.convert("RGBa").resize(size, Image.Resampling.LANCZOS).convert("RGBA")
        icon = Image.new("RGBA", (52, 52))
        icon.alpha_composite(source, ((52 - size[0]) // 2, (52 - size[1]) // 2))
        values, preview = [], []
        for r, g, b, a in icon.getdata():
            opaque = a >= 128
            value = rgb565(r, g, b)
            values.append((0xF81E if value == 0xF81F else value) if opaque else 0xF81F)
            preview.append((r, g, b, 255 if opaque else 0))
        icon.putdata(preview)
        icon.save(HOME / f"{name}-52.png")
        arrays.append(array(f"{name}_home_icon", values))
    (ROOT / "src/home_icons.h").write_text(
        PREFIX + "// 52x52 icons. Native RGB565; magenta 0xF81F is transparent.\n" + "\n".join(arrays))
    print("Built selected 240x320 wallpaper and 13 transparent 52x52 icons")


if __name__ == "__main__":
    main()
