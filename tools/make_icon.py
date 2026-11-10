#!/usr/bin/env python3
"""Draws the plugin's own NAS icon and writes components/nas_power/icon.h: one A8 mask, how much of each pixel the
icon covers. The screen paints the mask in any theme colour, so the icon follows light, dark and the accent, with no
picture to fetch. Tessera's icon font has no NAS, so the plugin brings this one.

    pip install pillow
    python3 tools/make_icon.py [proof.png]
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

SIZE = 80   # pixels a side; the tile scales the mask down to its height
OVER = 8    # drawn this many times larger, then averaged down: smooth edges
U = SIZE * OVER / 24   # the icon is drawn on a 24 x 24 grid

mask = Image.new('L', (SIZE * OVER, SIZE * OVER), 0)
draw = ImageDraw.Draw(mask)


def box(x0, y0, x1, y1, r, fill):
    draw.rounded_rectangle([x0 * U, y0 * U, x1 * U - 1, y1 * U - 1], radius=r * U, fill=fill)


box(4, 2, 20, 22, 2.5, 255)             # the case
for x in (6.4, 12.6):                   # two drive bays, cut out, each with its handle left standing
    box(x, 4.4, x + 5, 15, 0.9, 0)
    box(x + 1.3, 6, x + 3.7, 8.6, 0.6, 255)
draw.ellipse([6.9 * U, 17.4 * U, 9.3 * U, 19.8 * U], fill=0)   # the power light
box(11.4, 17.9, 17.6, 19.3, 0.7, 0)     # the vent

alpha = mask.resize((SIZE, SIZE), Image.LANCZOS)
out = Path(__file__).resolve().parent.parent / 'components' / 'nas_power' / 'icon.h'
data = alpha.tobytes()
lines = [
    '#pragma once',
    '// Made by tools/make_icon.py. Do not edit by hand.',
    '#include <cstdint>',
    '',
    'namespace esphome::nas_power {',
    '',
    f'static constexpr int ICON_SIZE = {SIZE};',
    '',
    '// The NAS icon as a mask: ICON_SIZE * ICON_SIZE bytes, row by row, how much of each pixel it covers.',
    'static const uint8_t ICON_NAS[] = {',
]
for i in range(0, len(data), 24):
    lines.append('  ' + ', '.join(f'0x{b:02x}' for b in data[i:i + 24]) + ',')
lines += ['};', '', '}  // namespace esphome::nas_power', '']
out.write_text('\n'.join(lines))
print(f'{out}: {SIZE}x{SIZE}')

if len(sys.argv) > 1:   # a sheet to look at: the icon large and at the sizes a tile shows
    sheet = Image.new('RGB', (520, 260), 'white')
    x = 10
    for side in (240, 80, 44, 32, 24):
        ink = Image.new('RGB', (side, side), (20, 20, 20))
        sheet.paste(ink, (x, 10), alpha.resize((side, side), Image.LANCZOS) if side != SIZE else alpha)
        x += side + 12
    sheet.save(sys.argv[1])
