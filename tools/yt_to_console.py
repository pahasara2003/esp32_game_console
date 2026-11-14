#!/usr/bin/env python3
"""Download a YouTube video and convert it for the game console.

Output (copy the folder to the SD card):
  <name>/video.mjpeg  320x240 MJPEG (baseline JPEG frames), letterboxed
  <name>/audio.wav    mono 16-bit PCM for the MAX98357A over I2S
  <name>/info.txt     title and fps for the Videos playlist

Usage:
  python3 tools/yt_to_console.py URL [-n name] [--fps 12] [--quality 3]
         [--rate 22050] [--crop] [--out videos]

Quality is favoured over frame rate: low JPEG q (sharper), Lanczos scaling,
a light unsharp pass, and a modest default frame rate.
"""
import argparse
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

W, H = 320, 240


def run(cmd):
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)


def slug(text):
    s = re.sub(r"[^A-Za-z0-9]+", "_", text).strip("_").lower()
    return s[:24] or "video"


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("url")
    ap.add_argument("-n", "--name", help="output folder name (default: from title)")
    ap.add_argument("--fps", type=float, default=12, help="frame rate (default 12)")
    ap.add_argument("--quality", type=int, default=3,
                    help="JPEG q, 2=best/biggest .. 10=smallest (default 3)")
    ap.add_argument("--rate", type=int, default=22050, help="audio sample rate (default 22050)")
    ap.add_argument("--crop", action="store_true",
                    help="fill the screen by cropping instead of black bars")
    ap.add_argument("--out", default="videos", help="parent output dir (default ./videos)")
    a = ap.parse_args()

    for tool in ("yt-dlp", "ffmpeg"):
        if not shutil.which(tool):
            sys.exit(f"{tool} not found in PATH")

    with tempfile.TemporaryDirectory() as tmp:
        src_tpl = str(Path(tmp) / "src.%(ext)s")
        # Best source up to 1080p: more source detail gives a cleaner downscale.
        run(["yt-dlp", "-f", "bv*[height<=1080]+ba/b[height<=1080]/b",
             "--merge-output-format", "mkv", "-o", src_tpl, a.url])
        src = next(Path(tmp).glob("src.*"))

        title = subprocess.run(["yt-dlp", "--get-title", a.url],
                               capture_output=True, text=True).stdout.strip()
        name = a.name or slug(title)
        out = Path(a.out) / name
        out.mkdir(parents=True, exist_ok=True)
        # Playlist title and frame rate for the console's Videos app.
        (out / "info.txt").write_text(f"{title or name}\n{a.fps:g}\n", encoding="utf-8")

        if a.crop:
            fit = (f"scale={W}:{H}:force_original_aspect_ratio=increase:flags=lanczos,"
                   f"crop={W}:{H}")
        else:
            fit = (f"scale={W}:{H}:force_original_aspect_ratio=decrease:flags=lanczos,"
                   f"pad={W}:{H}:(ow-iw)/2:(oh-ih)/2:black")
        # Keep 4:2:0: ffmpeg's 4:4:4 MJPEG uses 1x2 sampling that JPEGDEC
        # renders as rainbow lines, and 4:2:0 decodes fastest (~11 fps).
        vf = f"fps={a.fps},{fit},unsharp=5:5:0.6:5:5:0.0,format=yuvj420p"
        run(["ffmpeg", "-y", "-i", str(src), "-vf", vf, "-an",
             "-c:v", "mjpeg", "-q:v", str(a.quality), "-huffman", "default",
             "-f", "mjpeg", str(out / "video.mjpeg")])

        run(["ffmpeg", "-y", "-i", str(src), "-vn", "-ac", "1", "-ar", str(a.rate),
             "-af", "loudnorm=I=-16:TP=-1.5", "-c:a", "pcm_s16le",
             str(out / "audio.wav")])

    v = (out / "video.mjpeg").stat().st_size / 1e6
    s = (out / "audio.wav").stat().st_size / 1e6
    print(f"\nDone: {out}/  video {v:.1f} MB, audio {s:.1f} MB, {a.fps:g} fps")
    print("Copy this folder to the SD card (e.g. /videos/%s/)." % name)


if __name__ == "__main__":
    main()
