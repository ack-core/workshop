from __future__ import annotations

"""
Tool to fill white pixels of resources/palette.png with interpolated colors
"""

import colorsys
import png
import os

WHITE = (255, 255, 255)
BLACK = (0, 0, 0)


def hsv_to_rgb_byte(h_deg, s, v):
    h_deg = h_deg % 360
    h_norm = h_deg / 360.0
    s = max(0.0, min(1.0, s))
    v = max(0.0, min(1.0, v))
    r, g, b = colorsys.hsv_to_rgb(h_norm, s, v)
    return (round(r * 255), round(g * 255), round(b * 255))


def rgb_byte_to_hsv(rgb):
    h, s, v = colorsys.rgb_to_hsv(*(c / 255.0 for c in rgb))
    return (h * 360.0, s, v)


def _hue_shortest_path(h_left, h_right, t):
    diff = h_right - h_left
    if diff > 180:
        diff -= 360
    elif diff < -180:
        diff += 360
    return (h_left + t * diff) % 360


def _lerp_hsv(hsv_left, hsv_right, t):
    h_l, s_l, v_l = hsv_left
    h_r, s_r, v_r = hsv_right
    return (
        _hue_shortest_path(h_l, h_r, t),
        (1 - t) * s_l + t * s_r,
        (1 - t) * v_l + t * v_r,
    )


def interpolate(image):
    height, width = len(image), len(image[0])

    def px(r, c):
        return image[r][c][:3] if 0 <= r < height and 0 <= c < width else None

    def is_source(*colors):
        return all(p not in (None, BLACK, WHITE) for p in colors)

    def fill(a, b, cells):
        a_hsv, b_hsv = rgb_byte_to_hsv(a), rgb_byte_to_hsv(b)
        for i, (r, c) in enumerate(cells, 1):
            rgb = hsv_to_rgb_byte(*_lerp_hsv(a_hsv, b_hsv, i / (len(cells) + 1)))
            image[r][c] = rgb + image[r][c][3:]

    for r in range(height):
        for c in range(width):
            if px(r, c) != WHITE:
                continue
            if px(r, c + 1) == WHITE and is_source(px(r, c - 1), px(r, c + 2)):
                fill(px(r, c - 1), px(r, c + 2), [(r, c), (r, c + 1)])
            elif is_source(px(r, c - 1), px(r, c + 1)):
                fill(px(r, c - 1), px(r, c + 1), [(r, c)])
            elif is_source(px(r - 1, c), px(r + 1, c)):
                fill(px(r - 1, c), px(r + 1, c), [(r, c)])


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    path = os.path.join(script_dir, '..', 'resources', 'palette.png')

    width, height, rows, info = png.Reader(filename=path).asRGBA8()
    image = [[tuple(row[i:i + 4]) for i in range(0, width * 4, 4)] for row in rows]

    interpolate(image)

    physical = info.get('physical')
    with open(path, 'wb') as f:
        writer = png.Writer(width=width, height=height, greyscale=False, alpha=True, bitdepth=8,
                            x_pixels_per_unit=physical and physical.x,
                            y_pixels_per_unit=physical and physical.y,
                            unit_is_meter=bool(physical and physical.unit_is_meter))
        writer.write(f, [[v for p in row for v in p] for row in image])

    print(f"Palette updated: {path}")


if __name__ == '__main__':
    main()
