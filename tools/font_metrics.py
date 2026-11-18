#!/usr/bin/env python3
"""Print u8g2 font metrics + string widths, to pick font roles for a panel.

    python tools/font_metrics.py "Brightness" "100%" "12:59"
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from extract_fonts import decode_c_string  # noqa: E402

SRC = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__))))), "ESP-IDF", "u8g2-dl", "csrc", "u8g2_fonts.c")

CANDIDATES = [
    "u8g2_font_haxrcorp4089_tr", "u8g2_font_4x6_tr", "u8g2_font_5x7_tr",
    "u8g2_font_6x12_tr", "u8g2_font_6x13_tr", "u8g2_font_7x13_tr",
    "u8g2_font_8x13_tr", "u8g2_font_9x18_tr", "u8g2_font_10x20_tr",
    "u8g2_font_logisoso16_tr", "u8g2_font_logisoso22_tr", "u8g2_font_logisoso28_tr",
    "u8g2_font_logisoso28_tn", "u8g2_font_logisoso42_tn", "u8g2_font_logisoso42_tr",
]


def load(src, name):
    pat = re.compile(r"const\s+uint8_t\s+" + name + r"\s*\[[^\]]*\]\s*U8G2_FONT_SECTION\([^)]*\)\s*=\s*")
    m = pat.search(src)
    if not m:
        return None
    i, chunks = m.end(), []
    while True:
        while src[i] in " \t\r\n":
            i += 1
        if src[i] == ";":
            break
        i += 1
        start = i
        while src[i] != '"':
            i += 2 if src[i] == "\\" else 1
        chunks.append(src[start:i])
        i += 1
    return decode_c_string("".join(chunks))


class Reader:
    def __init__(self, data, pos=0):
        self.d, self.p, self.bit = data, pos, 0

    def bits(self, n):
        v = 0
        for k in range(n):
            byte = self.d[self.p]
            v |= ((byte >> self.bit) & 1) << k
            self.bit += 1
            if self.bit == 8:
                self.bit = 0
                self.p += 1
        return v

    def signed(self, n):
        v = self.bits(n)
        return v - (1 << (n - 1))


def glyph_table(data):
    bp = [data[4], data[5], data[6], data[7], data[8]]
    start_a = (data[17] << 8) | data[18]
    start_a_l = (data[19] << 8) | data[20]
    out = {}
    for lo, start in (("A", start_a), ("a", start_a_l)):
        p = 23 + start
        while data[p + 1] != 0:
            code = data[p]
            size = data[p + 1]
            r = Reader(data, p + 2)
            w = r.bits(bp[0])
            h = r.bits(bp[1])
            x = r.signed(bp[2])
            y = r.signed(bp[3])
            dx = r.signed(bp[4])
            out[chr(code)] = (w, h, x, y, dx)
            p += size
    return out


def main():
    with open(SRC, "r", encoding="utf-8", errors="replace") as f:
        src = f.read()
    samples = sys.argv[1:] or ["Brightness", "100%", "12:59", "MOTION UI FRAMEWORK"]
    print("%-28s %3s %3s %4s  %s" % ("font", "asc", "desc", "line", "  ".join("%s=%d" % (s, 0) for s in [])))
    for name in CANDIDATES:
        data = load(src, name)
        if not data:
            continue
        asc, desc = data[13] - 256 if data[13] > 127 else data[13], data[14] - 256 if data[14] > 127 else data[14]
        tbl = glyph_table(data)
        widths = []
        for s in samples:
            widths.append(sum(tbl.get(c, (0, 0, 0, 0, 0))[4] for c in s))
        print("%-28s %3d %4d %4d  %s" % (name.replace("u8g2_font_", ""), asc, desc, asc - desc,
                                         "  ".join("%s:%d" % (s[:12], w) for s, w in zip(samples, widths))))


if __name__ == "__main__":
    main()
