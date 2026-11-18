#!/usr/bin/env python3
"""Turn the preview PGM captures into 2x PNGs (nearest neighbour).

    python tools/pgm2png.py build/preview/out docs/preview
"""
import glob
import os
import sys

from PIL import Image


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "build/preview/out"
    dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(src, "png")
    scale = int(sys.argv[3]) if len(sys.argv) > 3 else 2
    os.makedirs(dst, exist_ok=True)
    files = sorted(glob.glob(os.path.join(src, "*.pgm")))
    if not files:
        raise SystemExit("no .pgm in " + src)
    for f in files:
        img = Image.open(f).convert("L")
        img = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
        name = os.path.splitext(os.path.basename(f))[0] + ".png"
        img.save(os.path.join(dst, name))
    print("%d frames -> %s" % (len(files), dst))


if __name__ == "__main__":
    main()
