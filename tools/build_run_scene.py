#!/usr/bin/env python3
"""Build the Run background and clean transparent parallax cloud sprites."""
from pathlib import Path
from PIL import Image
from run_background_codec import write_backgrounds

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets" / "run-scene"
BG_SOURCE = ASSETS / "base-source.png"
DAY_NIGHT_SOURCE = ASSETS / "day-night-sheet.png"
CLOUD_SOURCE = ASSETS / "day-night-cloud-sheet.png"
BG_PREVIEW = ROOT / "assets" / "run-background-240x320.png"
BG_HEADER = ROOT / "src" / "run_background_240x320.h"
CLOUD_HEADER = ROOT / "src" / "run_clouds.h"
TRANSPARENT = 0xF81F

# Four matching cloud silhouettes for each of the nine day/night phases.
# Boxes tightly follow opaque components and exclude labels and separator lines.
CLOUD_SIZES = [(72, 24), (62, 19), (56, 18), (42, 13)]
TIME_CLOUD_BOXES = [
    [(120,9,472,121), (491,25,666,77), (755,17,920,62), (663,83,789,121)],
    [(128,182,476,290), (490,194,671,245), (754,187,917,229), (663,249,789,287)],
    [(132,348,487,460), (497,363,678,415), (759,354,912,398), (668,418,790,452)],
    [(132,519,484,624), (503,530,680,578), (759,522,924,563), (669,583,793,620)],
    [(133,682,482,781), (503,693,679,738), (758,685,914,724), (674,741,791,774)],
    [(133,834,476,930), (502,844,678,889), (761,836,921,874), (670,889,791,922)],
    [(136,983,485,1080), (504,996,675,1041), (761,988,918,1029), (668,1045,793,1078)],
    [(130,1134,480,1230), (491,1146,669,1190), (760,1140,916,1179), (673,1192,792,1223)],
    [(130,1288,488,1395), (497,1303,677,1348), (754,1294,916,1335), (669,1350,794,1386)],
]

def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def emit_values(lines, pixels, columns=16):
    for start in range(0, len(pixels), columns):
        lines.append("  " + ", ".join(f"0x{x:04X}" for x in pixels[start:start+columns]) + ",")

def main():
    # The supplied 3x3 sheet is ordered from bright day through sunset and
    # moonlit night. Crop inside its black separators, then retain each panel
    # as a native 240x320 RGB565 frame for smooth on-device crossfading.
    sheet_bg = Image.open(DAY_NIGHT_SOURCE).convert("RGB")
    x_ranges = [(2, 360), (364, 722), (726, 1084)]
    y_ranges = [(1, 482), (484, 964), (967, 1446)]
    backgrounds = []
    for row, (y0, y1) in enumerate(y_ranges):
        for col, (x0, x1) in enumerate(x_ranges):
            frame = sheet_bg.crop((x0, y0, x1, y1)).resize(
                (240, 320), Image.Resampling.NEAREST)
            frame.save(ASSETS / f"day-night-{row * 3 + col}.png")
            backgrounds.append(frame)
    backgrounds[0].save(BG_PREVIEW)

    write_backgrounds(BG_HEADER, [[rgb565(*pixel) for pixel in frame.getdata()]
                                 for frame in backgrounds])

    cloud_sheet = Image.open(CLOUD_SOURCE).convert("RGBA")
    lines = ["#pragma once", "#include <Arduino.h>", "",
             f"constexpr uint16_t RUN_CLOUD_TRANSPARENT = 0x{TRANSPARENT:04X};",
             f"constexpr uint8_t RUN_CLOUD_COUNT = {len(CLOUD_SIZES)};",
             f"constexpr uint8_t RUN_CLOUD_TIME_COUNT = {len(TIME_CLOUD_BOXES)};", ""]
    for variant, size in enumerate(CLOUD_SIZES):
        w, h = size
        lines.append(f"const uint16_t run_cloud_{variant}[RUN_CLOUD_TIME_COUNT][{w*h}] PROGMEM = {{")
        for phase, boxes in enumerate(TIME_CLOUD_BOXES):
            cloud = cloud_sheet.crop(boxes[variant])
            cleaned = Image.new("RGBA", cloud.size, (0, 0, 0, 0))
            src = cloud.load(); dst = cleaned.load()
            for y in range(cloud.height):
                for x in range(cloud.width):
                    r, g, b, a = src[x, y]
                    if a >= 128:
                        dst[x, y] = (r, g, b, 255)
            cleaned = cleaned.resize(size, Image.Resampling.NEAREST)
            cleaned.save(ASSETS / f"cloud-{phase}-{variant}.png")
            pixels = [rgb565(r, g, b) if a >= 128 else TRANSPARENT
                      for r, g, b, a in cleaned.getdata()]
            lines.append("{")
            emit_values(lines, pixels)
            lines.append("},")
        lines += ["};", ""]
    lines.append("const uint8_t run_cloud_width[RUN_CLOUD_COUNT] = {" +
                 ", ".join(str(w) for w, _ in CLOUD_SIZES) + "};")
    lines.append("const uint8_t run_cloud_height[RUN_CLOUD_COUNT] = {" +
                 ", ".join(str(h) for _, h in CLOUD_SIZES) + "};")
    lines.append("")
    CLOUD_HEADER.write_text("\n".join(lines))

if __name__ == "__main__":
    main()
