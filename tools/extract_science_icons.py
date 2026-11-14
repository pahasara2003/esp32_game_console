"""Extract the six supplied app icons, keeping their rounded outside edges transparent."""
from collections import deque
from pathlib import Path
import sys

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets" / "menu-icons"
SHEET = ASSETS / "science-icon-sheet.png"
BOXES = {
    "tic_tac": (53, 111, 395, 469),
    "pong": (409, 111, 761, 469),
    "mines": (774, 111, 1126, 469),
    "gallery": (1140, 111, 1485, 469),
    "life": (53, 521, 395, 875),
    "mandelbrot": (409, 521, 761, 875),
}


def main():
    original = Image.open(sys.argv[1] if len(sys.argv) > 1 else SHEET).convert("RGB")
    if original.size != (1536, 1024):
        raise ValueError("The supplied icon sheet must be 1536 x 1024")
    ASSETS.mkdir(parents=True, exist_ok=True)
    original.save(SHEET)
    for name, box in BOXES.items():
        crop = original.crop(box).convert("RGBA")
        width, height = crop.size
        pixels = crop.load()
        # Flood only the dark sheet background connected to the outside. Interior
        # dark ink, especially the Mandelbrot set and Pong field, stays opaque.
        seen = set()
        queue = deque([(x, 0) for x in range(width)] +
                      [(x, height - 1) for x in range(width)] +
                      [(0, y) for y in range(height)] +
                      [(width - 1, y) for y in range(height)])
        while queue:
            x, y = queue.popleft()
            if not (0 <= x < width and 0 <= y < height) or (x, y) in seen:
                continue
            seen.add((x, y))
            r, g, b, _ = pixels[x, y]
            if max(r, g, b) > 38 or max(r, g, b) - min(r, g, b) > 20:
                continue
            pixels[x, y] = (r, g, b, 0)
            queue.extend(((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)))
        crop = crop.crop(crop.getchannel("A").getbbox())
        crop.save(ASSETS / f"{name}.png")
        print(name, crop.size)


if __name__ == "__main__":
    main()
