from __future__ import annotations

"""
Tool to adjust an image to existing palette
Example: python adjust_colors.py --palette ../resources/palette.png --source untitled.png 45.235.4t13.1 53.20.3t12.1 147.60.4t0.1
"""

import argparse
import os
import re
import png

from make_palette import hsv_to_rgb_byte, rgb_byte_to_hsv

PALETTE_WIDTH = 8
PALETTE_HEIGHT = 32
TRANSITION_LENGTHS = {"4t": 4, "3t": 3}


def parse_attractor(s):
    """
    Parse attractor string X.Y.<kind><index>.<multiplier>, kind is 4t or 3t (e.g. 0.100.4t1.2).
    """
    m = re.match(r"^(\d+)\.(\d+)\.(4t|3t)(\d+)\.(-?\d+)$", s.strip())
    if not m:
        raise ValueError(f"Invalid attractor string: {s!r}")
    x, y, kind, index, multiplier = m.groups()
    return {
        "raw": s.strip(),
        "x": int(x),
        "y": int(y),
        "kind": kind,
        "index": int(index),
        "multiplier": int(multiplier),
    }


def hue_shortest_delta_deg(h_from_deg, h_to_deg):
    diff = (h_to_deg % 360.0) - (h_from_deg % 360.0)
    if diff > 180:
        diff -= 360
    elif diff < -180:
        diff += 360
    return diff


def rgb_distance_sq(c1, c2):
    return sum((a - b) ** 2 for a, b in zip(c1, c2))


def palette_transition_colors(palette, kind, index):
    """4t lines are left-aligned, 3t lines right-aligned, at rows 1, 3, 5, ..."""
    length = TRANSITION_LENGTHS[kind]
    row = 1 + 2 * index
    if row >= PALETTE_HEIGHT:
        raise ValueError(f"{kind}{index} out of palette range")
    col = 0 if kind == "4t" else PALETTE_WIDTH - length
    return palette[row][col:col + length]


def block_reference_hsv(rgbs):
    """RGB midpoint of the most distant pair of colors in the block."""
    a, b = max(
        ((a, b) for i, a in enumerate(rgbs) for b in rgbs[i + 1:]),
        key=lambda pair: rgb_distance_sq(*pair),
    )
    return rgb_byte_to_hsv(tuple((ca + cb) / 2.0 for ca, cb in zip(a, b)))


def input_colors(palette_colors, attractor_rgb, multiplier):
    """
    HSV deltas of every palette color from the block's reference color,
    scaled by multiplier, applied around the attractor pixel's HSV.
    """
    h_ref, s_ref, v_ref = block_reference_hsv(palette_colors)
    h_att, s_att, v_att = rgb_byte_to_hsv(attractor_rgb)
    colors = []
    for h, s, v in map(rgb_byte_to_hsv, palette_colors):
        colors.append(hsv_to_rgb_byte(
            h_att + multiplier * hue_shortest_delta_deg(h_ref, h),
            s_att + multiplier * (s - s_ref),
            v_att + multiplier * (v - v_ref),
        ))
    return colors


def prepare_attractors(palette, image, attractor_strings):
    height, width = len(image), len(image[0])
    prepared = []
    for s in attractor_strings:
        spec = parse_attractor(s)
        x, y = spec["x"], spec["y"]
        if not (0 <= x < width and 0 <= y < height):
            raise ValueError(
                f"Attractor {spec['raw']!r}: pixel ({x},{y}) out of image bounds {width}x{height}"
            )
        spec["rgb"] = image[y][x]
        spec["palette_colors"] = palette_transition_colors(palette, spec["kind"], spec["index"])
        spec["input_colors"] = input_colors(spec["palette_colors"], spec["rgb"], spec["multiplier"])
        prepared.append(spec)
    return prepared


def convert_image_with_attractors(image, attractors):
    """
    Each pixel is assigned to the attractor whose anchor pixel is closest in RGB,
    then mapped to the palette color whose input color is closest.
    """
    out = []
    for row in image:
        row_out = []
        for rgb in row:
            att = min(attractors, key=lambda a: rgb_distance_sq(rgb, a["rgb"]))
            k = min(range(len(att["input_colors"])),
                    key=lambda k: rgb_distance_sq(rgb, att["input_colors"][k]))
            row_out.append(att["palette_colors"][k])
        out.append(row_out)
    return out


def load_image_rgb(path):
    w, h, rows, info = png.Reader(filename=path).asRGB8()
    return [[tuple(row[i:i + 3]) for i in range(0, w * 3, 3)] for row in rows]


def save_image_rgb(path, image):
    with open(path, "wb") as f:
        writer = png.Writer(width=len(image[0]), height=len(image), greyscale=False, alpha=False)
        writer.write(f, [[v for p in row for v in p] for row in image])


def load_palette_rgb(path):
    w, h, rows, info = png.Reader(filename=path).asRGBA8()
    if w != PALETTE_WIDTH or h != PALETTE_HEIGHT:
        raise ValueError(f"Palette must be {PALETTE_WIDTH}x{PALETTE_HEIGHT}, got {w}x{h}")
    return [[tuple(row[i:i + 3]) for i in range(0, w * 4, 4)] for row in rows]


def main():
    parser = argparse.ArgumentParser(description="Tool to adjust an image to existing palette")
    parser.add_argument("-p", "--palette", required=True, help="Path to palette PNG (8x32)")
    parser.add_argument("-s", "--source", required=True, help="Path to input image PNG to adjust")
    parser.add_argument("attractors", nargs="+", help="Attractor strings X.Y.S.M, e.g. 40.221.4t1.3")
    args = parser.parse_args()

    palette = load_palette_rgb(args.palette)
    image = load_image_rgb(args.source)
    attractors = prepare_attractors(palette, image, args.attractors)
    out = convert_image_with_attractors(image, attractors)

    base, ext = os.path.splitext(args.source)
    save_image_rgb(f"{base}_adjusted{ext}", out)


if __name__ == "__main__":
    main()
