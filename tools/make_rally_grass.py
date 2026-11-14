"""Convert surfacecurve's CC0 grass texture into Rally's RGB332 flash tile.

Source: https://opengameart.org/content/grass-textureseamless-2d
Download tileable_grass_00.png, then pass its path to this script.
"""

import sys
from pathlib import Path

from PIL import Image


source = Image.open(sys.argv[1]).convert("RGB")
source = source.resize((256, 256), Image.Resampling.LANCZOS)
pixels = []
for red, green, blue in source.get_flattened_data():
    # Warm the source toward green while retaining its natural dark variation.
    red = min(255, int(red * 0.95))
    green = min(255, int(green * 1.4))
    blue = min(255, blue)
    pixels.append((red & 0xE0) | ((green >> 3) & 0x1C) | (blue >> 6))

output = Path(__file__).resolve().parents[1] / "src" / "rally_grass.h"
with output.open("w") as header:
    header.write("// Generated from tileable_grass_00.png by surfacecurve (CC0).\n")
    header.write("// https://opengameart.org/content/grass-textureseamless-2d\n")
    header.write("#pragma once\n#include <stdint.h>\n#include <pgmspace.h>\n")
    header.write("constexpr int RALLY_GRASS_SIZE = 256;\n")
    header.write("const uint8_t rallyGrass[RALLY_GRASS_SIZE * RALLY_GRASS_SIZE] PROGMEM = {\n")
    for offset in range(0, len(pixels), 24):
        header.write("  " + ", ".join(f"0x{value:02X}" for value in pixels[offset:offset + 24]) + ",\n")
    header.write("};\n")
