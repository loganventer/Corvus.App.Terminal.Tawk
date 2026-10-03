#!/usr/bin/env python3
"""Draws tawk's logo files as PNGs with a transparent background.

    make_logos.py OUT_DIR

Needs Pillow and the DejaVu fonts (DejaVuSans-Bold.ttf and
DejaVuSansMono-Bold.ttf), in /usr/share/fonts/truetype/dejavu or the folder
named by TAWK_SHOT_FONTS. Everything is drawn four times too large and
scaled down, so edges are smooth.
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

FONT_DIR = os.environ.get("TAWK_SHOT_FONTS", "/usr/share/fonts/truetype/dejavu")
GREEN = (92, 196, 126, 255)
BLUE = (152, 198, 222, 255)
TEAL = (90, 169, 184, 255)
NIGHT_TOP = (44, 54, 68, 255)
NIGHT_BOTTOM = (22, 27, 36, 255)
INK = (27, 32, 42, 255)
WHITE = (255, 255, 255, 255)
BLACK = (0, 0, 0, 255)
CLEAR = (0, 0, 0, 0)
SCALE = 4

# The symbol on a 200 by 200 grid: four rows of bars, split like bricks, and a tail under the last.
BAR_H, GAP, RADIUS = 30, 12, 5
ROWS = [[(0, 150), (162, 200)], [(0, 110), (124, 200)], [(0, 48), (62, 200)], [(0, 200)]]
TAIL = [(62, 150), (62, 200), (112, 150)]
# The small one, for 16 pixels: three plain bars.
SMALL_BAR_H, SMALL_GAP = 40, 16
SMALL_TAIL = [(62, 150), (62, 200), (118, 150)]


def blend(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(4))


def draw_symbol(draw, x, y, size, colours):
    """`colours(row, part)` gives the colour of each bar; the tail takes the last row's first."""
    k = size / 200
    for r, parts in enumerate(ROWS):
        top = y + r * (BAR_H + GAP) * k
        for p, (x0, x1) in enumerate(parts):
            draw.rounded_rectangle([x + x0 * k, top, x + x1 * k, top + BAR_H * k], RADIUS * k, fill=colours(r, p))
    draw.polygon([(x + px * k, y + py * k) for px, py in TAIL], fill=colours(len(ROWS) - 1, 0))


def draw_small_symbol(draw, x, y, size, colour):
    k = size / 200
    for r in range(3):
        top = y + r * (SMALL_BAR_H + SMALL_GAP) * k
        draw.rounded_rectangle([x, top, x + 200 * k, top + SMALL_BAR_H * k], RADIUS * k, fill=colour)
    draw.polygon([(x + px * k, y + py * k) for px, py in SMALL_TAIL], fill=colour)


def tile(size):
    """The dark rounded square behind the app icon, lighter at the top; its corners are transparent."""
    img = Image.new("RGBA", (size, size), CLEAR)
    shade = Image.new("RGBA", (size, size), CLEAR)
    d = ImageDraw.Draw(shade)
    for row in range(size):
        d.line([(0, row), (size, row)], fill=blend(NIGHT_TOP, NIGHT_BOTTOM, row / max(1, size - 1)))
    mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, size - 1, size - 1], size * 0.22, fill=255)
    img.paste(shade, (0, 0), mask)
    return img


def symbol(size, colours):
    big = size * SCALE
    img = Image.new("RGBA", (big, big), CLEAR)
    pad = big * 0.07                                     # a little room around it, clear on every side
    draw_symbol(ImageDraw.Draw(img), pad, pad, big - 2 * pad, colours)
    return img.resize((size, size), Image.LANCZOS)


def icon(size):
    """The app icon: green bars on the left, blue on the right, on the dark tile."""
    big = size * SCALE
    img = tile(big)
    inset = big * 0.19
    draw_symbol(ImageDraw.Draw(img), inset, inset + big * 0.02, big - 2 * inset,
                lambda r, p: TEAL if r == 3 else (GREEN if p == 0 else BLUE))
    return img.resize((size, size), Image.LANCZOS)


def small_icon(size):
    big = max(size * SCALE, 256)
    img = tile(big)
    inset = big * 0.2
    draw_small_symbol(ImageDraw.Draw(img), inset, inset + big * 0.02, big - 2 * inset, GREEN)
    return img.resize((size, size), Image.LANCZOS)


def lockup(mark, word_colour, font_file, height=256):
    """A mark with the name beside it."""
    big = height * SCALE
    font = ImageFont.truetype(os.path.join(FONT_DIR, font_file), int(big * 0.78))
    left, top, right, bottom = font.getbbox("tawk")
    gap = int(big * 0.22)
    img = Image.new("RGBA", (big + gap + (right - left) + int(big * 0.04), big), CLEAR)
    img.alpha_composite(mark.resize((big, big), Image.LANCZOS), (0, 0))
    ImageDraw.Draw(img).text((big + gap - left, (big - (bottom - top)) // 2 - top), "tawk", font=font, fill=word_colour)
    return img.resize((img.width // SCALE, height), Image.LANCZOS)


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    green = lambda r, p: GREEN
    files = {
        "logo-symbol.png": symbol(512, green),
        "logo-symbol-32.png": symbol(32, green),
        "logo-glyph-black.png": symbol(512, lambda r, p: BLACK),
        "logo-glyph-white.png": symbol(512, lambda r, p: WHITE),
        "logo-icon.png": icon(512),
        "logo-icon-small.png": small_icon(256),
        "logo-icon-32.png": small_icon(32),
        "logo-icon-16.png": small_icon(16),
        # The name beside the app icon, for light pages, and beside the symbol in white, for dark ones.
        "logo-lockup.png": lockup(icon(1024), INK, "DejaVuSans-Bold.ttf"),
        "logo-lockup-dark.png": lockup(symbol(1024, green), WHITE, "DejaVuSansMono-Bold.ttf"),
    }
    for name, img in files.items():
        assert img.mode == "RGBA" and img.getpixel((0, 0))[3] == 0, name + " must have a transparent corner"
        img.save(os.path.join(out, name), optimize=True)
        print("  " + os.path.join(out, name))


if __name__ == "__main__":
    main()
