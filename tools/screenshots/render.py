#!/usr/bin/env python3
"""Paints the screens written by `scenes` as PNG files for the manual.

    render.py pictures DIR         make the made-up avatar and landscape
    render.py cells CELLS_DIR OUT  paint every CELLS_DIR/*.cells to OUT/*.png

Needs Pillow and DejaVu Sans Mono. Emoji are drawn from Twemoji, fetched
once from cdn.jsdelivr.net into CELLS_DIR/emoji.
"""
import math
import os
import sys
import urllib.request

from PIL import Image, ImageDraw, ImageFont

CELL_W, CELL_H = 10, 20
FONT_DIR = "/usr/share/fonts/truetype/dejavu"
TWEMOJI = "https://cdn.jsdelivr.net/gh/jdecked/twemoji@latest/assets/72x72/{}.png"


def xterm256(n):
    """RGB of an xterm-256 colour number."""
    base = [(0, 0, 0), (205, 0, 0), (0, 205, 0), (205, 205, 0), (0, 0, 238), (205, 0, 205), (0, 205, 205), (229, 229, 229),
            (127, 127, 127), (255, 0, 0), (0, 255, 0), (255, 255, 0), (92, 92, 255), (255, 0, 255), (0, 255, 255), (255, 255, 255)]
    if n < 0:
        return (0, 0, 0)
    if n < 16:
        return base[n]
    if n < 232:
        n -= 16
        steps = [0, 95, 135, 175, 215, 255]
        return (steps[n // 36], steps[(n // 6) % 6], steps[n % 6])
    v = 8 + (n - 232) * 10
    return (v, v, v)


def mix(a, b, t):
    return tuple(int(a[i] * (1 - t) + b[i] * t) for i in range(3))


# ---- box drawing: (up, down, left, right) with 1 light, 2 heavy, 3 double ----
BOX = {
    0x2500: (0, 0, 1, 1), 0x2502: (1, 1, 0, 0), 0x2501: (0, 0, 2, 2), 0x2503: (2, 2, 0, 0),
    0x250C: (0, 1, 0, 1), 0x2510: (0, 1, 1, 0), 0x2514: (1, 0, 0, 1), 0x2518: (1, 0, 1, 0),
    0x256D: (0, 1, 0, 1), 0x256E: (0, 1, 1, 0), 0x2570: (1, 0, 0, 1), 0x256F: (1, 0, 1, 0),
    0x251C: (1, 1, 0, 1), 0x2524: (1, 1, 1, 0), 0x252C: (0, 1, 1, 1), 0x2534: (1, 0, 1, 1), 0x253C: (1, 1, 1, 1),
    0x2550: (0, 0, 3, 3), 0x2551: (3, 3, 0, 0), 0x2554: (0, 3, 0, 3), 0x2557: (0, 3, 3, 0),
    0x255A: (3, 0, 0, 3), 0x255D: (3, 0, 3, 0),
}


def draw_box(d, x, y, fg, parts):
    up, down, left, right = parts
    cx, cy = x + CELL_W // 2, y + CELL_H // 2

    def seg(kind, x0, y0, x1, y1, vertical):
        if not kind:
            return
        if kind == 3:                                   # double: two thin lines
            for off in (-2, 2):
                if vertical:
                    d.rectangle([x0 + off, y0, x0 + off, y1], fill=fg)
                else:
                    d.rectangle([x0, y0 + off, x1, y0 + off], fill=fg)
            return
        w = 1 if kind == 1 else 2
        if vertical:
            d.rectangle([x0 - (w - 1) // 2 - (w // 2), y0, x0 + w // 2, y1], fill=fg)
        else:
            d.rectangle([x0, y0 - (w - 1) // 2 - (w // 2), x1, y0 + w // 2], fill=fg)

    seg(up, cx, y, cx, cy, True)
    seg(down, cx, cy, cx, y + CELL_H - 1, True)
    seg(left, x, cy, cx, cy, False)
    seg(right, cx, cy, x + CELL_W - 1, cy, False)


def draw_block(d, x, y, cp, fg, bg):
    """Block elements, drawn exactly so pictures have no gaps. Returns False for other characters."""
    w, h = CELL_W, CELL_H
    if cp == 0x2588:
        d.rectangle([x, y, x + w - 1, y + h - 1], fill=fg)
    elif cp == 0x2580:
        d.rectangle([x, y, x + w - 1, y + h // 2 - 1], fill=fg)
    elif cp == 0x2584:
        d.rectangle([x, y + h // 2, x + w - 1, y + h - 1], fill=fg)
    elif cp == 0x258C:
        d.rectangle([x, y, x + w // 2 - 1, y + h - 1], fill=fg)
    elif cp == 0x2590:
        d.rectangle([x + w // 2, y, x + w - 1, y + h - 1], fill=fg)
    elif cp == 0x2597:
        d.rectangle([x + w // 2, y + h // 2, x + w - 1, y + h - 1], fill=fg)
    elif cp == 0x2596:
        d.rectangle([x, y + h // 2, x + w // 2 - 1, y + h - 1], fill=fg)
    elif cp == 0x259D:
        d.rectangle([x + w // 2, y, x + w - 1, y + h // 2 - 1], fill=fg)
    elif cp == 0x2598:
        d.rectangle([x, y, x + w // 2 - 1, y + h // 2 - 1], fill=fg)
    elif cp in (0x2591, 0x2592, 0x2593):
        d.rectangle([x, y, x + w - 1, y + h - 1], fill=mix(bg, fg, {0x2591: .25, 0x2592: .5, 0x2593: .75}[cp]))
    elif 0x2581 <= cp <= 0x2587:                    # lower eighths
        eighths = cp - 0x2580
        d.rectangle([x, y + h - h * eighths // 8, x + w - 1, y + h - 1], fill=fg)
    elif cp == 0x258F:
        d.rectangle([x, y, x + 1, y + h - 1], fill=fg)
    else:
        return False
    return True


_MISSING = {}


def _shape(font, ch):
    img = Image.new("L", (40, 40))
    ImageDraw.Draw(img).text((4, 4), ch, font=font, fill=255)
    return img.tobytes()


def has_glyph(font, ch):
    """False when the font would draw its placeholder box for `ch`."""
    key = id(font)
    if key not in _MISSING:
        _MISSING[key] = _shape(font, "\U0010FFFD")
    return _shape(font, ch) != _MISSING[key]


def is_emoji(cp, wide):
    return wide and (cp >= 0x1F000 or 0x2600 <= cp <= 0x27BF or 0x2B00 <= cp <= 0x2BFF)


def emoji_image(cache, cp):
    path = os.path.join(cache, "%x.png" % cp)
    if not os.path.exists(path):
        os.makedirs(cache, exist_ok=True)
        try:
            urllib.request.urlretrieve(TWEMOJI.format("%x" % cp), path)
        except Exception:
            return None
    try:
        return Image.open(path).convert("RGBA").resize((CELL_H - 2, CELL_H - 2), Image.LANCZOS)
    except Exception:
        return None


def paint(cells_path, out_path, cache):
    with open(cells_path, encoding="utf-8") as f:
        rows, cols = map(int, f.readline().split())
        grid = [[tuple(int(v) for v in cell.split(",")) for cell in line.split()] for line in f.read().splitlines()]
    img = Image.new("RGB", (cols * CELL_W, rows * CELL_H))
    d = ImageDraw.Draw(img)
    regular = ImageFont.truetype(os.path.join(FONT_DIR, "DejaVuSansMono.ttf"), 16)
    bold = ImageFont.truetype(os.path.join(FONT_DIR, "DejaVuSansMono-Bold.ttf"), 16)
    fallback = ImageFont.truetype(os.path.join(FONT_DIR, "DejaVuSans.ttf"), 15)   # symbols the mono face lacks (☰)
    emoji = []
    for y, row in enumerate(grid):
        for x, (cp, fgn, bgn, flags) in enumerate(row):
            fg, bg = xterm256(fgn), xterm256(bgn)
            if flags & 4:
                fg, bg = bg, fg
            if flags & 2:
                fg = mix(fg, bg, 0.45)
            px, py = x * CELL_W, y * CELL_H
            d.rectangle([px, py, px + CELL_W - 1, py + CELL_H - 1], fill=bg)
            if cp in (0, 32):
                continue
            if draw_block(d, px, py, cp, fg, bg):
                continue
            if cp in BOX:
                draw_box(d, px, py, fg, BOX[cp])
                continue
            wide = bool(flags & 8)
            if is_emoji(cp, wide) and emoji_image(cache, cp) is not None:
                emoji.append((px, py, cp))
                continue
            font = bold if flags & 1 else regular
            ch = chr(cp)
            if not has_glyph(font, ch):
                font = fallback
            width = CELL_W * (2 if wide else 1)
            box = font.getbbox(ch)
            gx = px + (width - (box[2] - box[0])) // 2 - box[0]
            d.text((gx, py + 1), ch, font=font, fill=fg)
    for px, py, cp in emoji:                                  # after the cells, so a wide emoji is not painted over
        pic = emoji_image(cache, cp)
        if pic:
            img.paste(pic, (px + 1, py + 1), pic)
    img = img.convert("P", palette=Image.ADAPTIVE, colors=256)
    img.save(out_path, optimize=True)


# ---- made-up pictures ------------------------------------------------------

def make_pictures(out):
    os.makedirs(out, exist_ok=True)
    # An avatar: a face on a warm background.
    a = Image.new("RGB", (256, 256), (246, 190, 120))
    d = ImageDraw.Draw(a)
    d.ellipse([28, 170, 228, 380], fill=(52, 120, 170))              # shoulders
    d.ellipse([78, 58, 178, 172], fill=(233, 180, 140))              # face
    d.pieslice([70, 40, 186, 140], 180, 360, fill=(70, 45, 30))     # hair
    d.ellipse([104, 102, 116, 114], fill=(40, 30, 25))
    d.ellipse([140, 102, 152, 114], fill=(40, 30, 25))
    d.arc([108, 118, 148, 150], 20, 160, fill=(150, 70, 60), width=4)
    a.save(os.path.join(out, "avatar.png"))
    # A landscape: sky, sun, mountains and a lake.
    w, h = 640, 420
    l = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(l)
    for y in range(h):
        t = y / (h * 0.62)
        d.line([(0, y), (w, y)], fill=mix((70, 130, 200), (250, 190, 120), min(t, 1)))
    d.ellipse([430, 90, 520, 180], fill=(255, 230, 150))
    d.polygon([(0, 300), (140, 150), (250, 260), (360, 120), (520, 280), (640, 200), (640, 330), (0, 330)], fill=(60, 80, 110))
    d.polygon([(330, 150), (360, 120), (392, 152), (372, 146), (358, 160), (345, 150)], fill=(240, 245, 250))
    d.polygon([(0, 330), (100, 250), (200, 320), (330, 240), (470, 330)], fill=(40, 90, 60))
    d.rectangle([0, 330, w, h], fill=(60, 110, 150))
    for i in range(12):
        y = 340 + i * 7
        d.line([(40 + i * 17, y), (160 + i * 23, y)], fill=(120, 170, 200), width=2)
    l.save(os.path.join(out, "landscape.png"))
    # A webcam picture: someone at a desk in front of a window and a plant.
    w, h = 640, 480
    c = Image.new("RGB", (w, h), (205, 196, 182))
    d = ImageDraw.Draw(c)
    d.rectangle([380, 40, 600, 250], fill=(150, 195, 230))              # window
    d.rectangle([380, 40, 600, 250], outline=(240, 236, 228), width=10)
    d.line([(490, 40), (490, 250)], fill=(240, 236, 228), width=8)
    d.rectangle([40, 300, 110, 400], fill=(170, 90, 60))                # plant pot
    for i, (dx, dy) in enumerate([(-30, -60), (0, -90), (30, -55), (-10, -40), (20, -30)]):
        d.ellipse([75 + dx - 22, 300 + dy - 30, 75 + dx + 22, 300 + dy + 30], fill=(60, 130 + i * 8, 70))
    d.ellipse([170, 330, 470, 600], fill=(200, 80, 70))                 # shoulders, red jumper
    d.rectangle([290, 280, 350, 350], fill=(222, 170, 130))             # neck
    d.ellipse([235, 120, 405, 310], fill=(230, 180, 140))               # face
    d.pieslice([225, 95, 415, 250], 180, 360, fill=(45, 35, 30))        # hair
    d.rectangle([225, 170, 245, 250], fill=(45, 35, 30))
    d.rectangle([395, 170, 415, 250], fill=(45, 35, 30))
    d.ellipse([280, 195, 298, 213], fill=(40, 30, 25))                  # eyes
    d.ellipse([342, 195, 360, 213], fill=(40, 30, 25))
    d.arc([290, 225, 350, 275], 20, 160, fill=(160, 70, 60), width=5)   # smile
    d.rectangle([0, 440, w, h], fill=(120, 90, 70))                     # desk
    c.save(os.path.join(out, "webcam.png"))
    # A link card's picture: a small JPEG, as WhatsApp sends them.
    card = Image.new("RGB", (192, 192), (18, 140, 126))
    d = ImageDraw.Draw(card)
    d.rounded_rectangle([36, 52, 156, 140], radius=18, fill=(236, 229, 221))
    d.polygon([(60, 140), (52, 170), (90, 140)], fill=(236, 229, 221))
    for i, w in enumerate((90, 70, 80)):
        d.rounded_rectangle([52, 68 + i * 22, 52 + w, 80 + i * 22], radius=5, fill=(18, 140, 126))
    card.save(os.path.join(out, "linkcard.jpg"), quality=80)


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "pictures":
        make_pictures(sys.argv[2])
    elif len(sys.argv) == 4 and sys.argv[1] == "cells":
        src, out = sys.argv[2], sys.argv[3]
        cache = os.path.join(src, "emoji")
        for name in sorted(os.listdir(src)):
            if name.endswith(".cells"):
                target = os.path.join(out, name[:-6] + ".png")
                paint(os.path.join(src, name), target, cache)
                print("  " + target)
    else:
        print(__doc__)
        sys.exit(2)


if __name__ == "__main__":
    main()
