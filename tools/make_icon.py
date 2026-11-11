#!/usr/bin/env python3
"""Draws the plugin's NAS icon and writes components/nas_power/icon.h: one A8 mask, how much of each pixel the icon
covers. The screen paints the mask in any theme colour, so the icon follows light, dark and the accent, with no
picture to fetch. Tessera's icon font has no NAS, so the plugin brings this one.

The shape is "nas" of Material Design Icons (https://pictogrammers.com/library/mdi/icon/nas/, Apache License 2.0),
the icon Home Assistant shows for `mdi:nas`: a lying case with its power light and three drive bays. Its path is only
boxes and one circle, so it is drawn here from its numbers, cut to what it covers of the 24 x 24 grid.

    pip install pillow
    python3 tools/make_icon.py [proof.png]
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

LEFT, TOP, RIGHT, BOTTOM = 2, 5, 22, 19   # what the icon covers of its 24 x 24 grid
UNIT = 4    # pixels of the mask per unit of the grid; the tile scales the mask down to its size
OVER = 8    # drawn this many times larger, then averaged down: smooth edges
WIDTH, HEIGHT = (RIGHT - LEFT) * UNIT, (BOTTOM - TOP) * UNIT

mask = Image.new('L', (WIDTH * OVER, HEIGHT * OVER), 0)
draw = ImageDraw.Draw(mask)
S = UNIT * OVER


def box(x0, y0, x1, y1, fill, radius=0.0):
    draw.rounded_rectangle([(x0 - LEFT) * S, (y0 - TOP) * S, (x1 - LEFT) * S - 1, (y1 - TOP) * S - 1],
                           radius=radius * S, fill=fill)


box(2, 5, 22, 19, 255, radius=2)        # the case
draw.ellipse([(3.5 - LEFT) * S, (7 - TOP) * S, (5.5 - LEFT) * S - 1, (9 - TOP) * S - 1], fill=0)   # the power light
box(7, 7, 20, 17, 0)                    # the window the bays stand in
for x in (8, 12, 16):                   # three drive bays, each with its latch cut out
    box(x, 8, x + 3, 16, 255)
    box(x + 1, 9, x + 2, 10, 0)

alpha = mask.resize((WIDTH, HEIGHT), Image.LANCZOS)
out = Path(__file__).resolve().parent.parent / 'components' / 'nas_power' / 'icon.h'
data = alpha.tobytes()
lines = [
    '#pragma once',
    '// Made by tools/make_icon.py. Do not edit by hand.',
    '// The shape is "nas" of Material Design Icons (pictogrammers.com), Apache License 2.0.',
    '#include <cstdint>',
    '',
    'namespace esphome::nas_power {',
    '',
    f'static constexpr int ICON_WIDTH = {WIDTH};',
    f'static constexpr int ICON_HEIGHT = {HEIGHT};',
    '',
    '// The NAS icon as a mask: ICON_WIDTH * ICON_HEIGHT bytes, row by row, how much of each pixel it covers.',
    'static const uint8_t ICON_NAS[] = {',
]
for i in range(0, len(data), 20):
    lines.append('  ' + ', '.join(f'0x{b:02x}' for b in data[i:i + 20]) + ',')
lines += ['};', '', '}  // namespace esphome::nas_power', '']
out.write_text('\n'.join(lines))
print(f'{out}: {WIDTH}x{HEIGHT}')

if len(sys.argv) > 1:   # a sheet to look at: the icon large and at the widths a tile shows
    sheet = Image.new('RGB', (560, 190), 'white')
    x = 10
    for width in (240, 80, 56, 44, 32, 24):
        size = (width, width * HEIGHT // WIDTH)
        ink = Image.new('RGB', size, (20, 20, 20))
        sheet.paste(ink, (x, 10), alpha if size == alpha.size else alpha.resize(size, Image.LANCZOS))
        x += width + 12
    sheet.save(sys.argv[1])
