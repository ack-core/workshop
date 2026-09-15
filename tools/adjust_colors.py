#!/usr/bin/env python3

# Tool to adjust an image to existing palette

import argparse
import colorsys
import os
import re
import png

from palette import (
    COLORS_MAJOR,
    COLORS_MINOR,
    COLORS_SPECIAL,
    COLORS_TRANSITION_3,
    COLORS_TRANSITION_4,
    HEIGHT as PALETTE_HEIGHT,
    MAJOR_HEIGHT,
    MAJOR_WIDTH,
    MINOR_HEIGHT,
    MINOR_WIDTH,
    WIDTH as PALETTE_WIDTH,
    _lerp_hsv,
    _offset_hsv,
    hsv_to_rgb_byte,
)

TRANSITION_4_LENGTH = 4
TRANSITION_3_LENGTH = 3



def parse_attractor(s):
    """
    Parse attractor string XXX.YYY.S.M where S is 4q0, 3q1, 4t0, 3t0, 1s0, etc.
    (X.Y.<kind><index>.<multiplier>, e.g. 0.100.4q1.2)
    Returns dict with x, y, kind, index, multiplier_str (parse as int for multiplier).
    """
    _ATTRACTOR_RE = re.compile(
        r"^(\d+)\.(\d+)\.(4q|3q|4t|3t|1s)(\d)\.(.+)$"
    )
    m = _ATTRACTOR_RE.match(s.strip())
    if not m:
        raise ValueError(f"Invalid attractor string: {s!r}")
    x, y, kind, idx_s, mult = m.groups()
    return {
        "raw": s.strip(),
        "x": int(x),
        "y": int(y),
        "kind": kind,
        "index": int(idx_s),
        "multiplier_str": mult,
    }


def rgb_to_hsv_degrees(r, g, b):
    """HSV with hue in degrees [0, 360), s and v in [0, 1]."""
    hn, s, v = colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)
    return (hn * 360.0, s, v)


def hue_shortest_delta_deg(h_from_deg, h_to_deg):
    """Signed shortest arc from h_from to h_to, degrees in (-180, 180]."""
    diff = (h_to_deg % 360.0) - (h_from_deg % 360.0)
    if diff > 180:
        diff -= 360
    elif diff < -180:
        diff += 360
    return diff


def _slot_hsvs_major(entry):
    """
    Ideal HSV of every slot of a 4x3 major rect, row-major (j then i).
    Mirrors palette.draw_major_rect: the middle row is 'hsv_left' -> 'hsv_right',
    the per-column step is added for the upper row and subtracted for the lower one.
    """
    out = []
    for j in range(MAJOR_HEIGHT):
        for i in range(MAJOR_WIDTH):
            t = i / (MAJOR_WIDTH - 1)
            middle = _lerp_hsv(entry["hsv_left"], entry["hsv_right"], t)
            step = (entry["h_step"][i], entry["s_step"][i], entry["v_step"][i])
            out.append(_offset_hsv(middle, step, 1 - j))
    return out


def _slot_hsvs_minor(entry):
    """
    Ideal HSV of every slot of a 3x3 minor rect, row-major (j then i).
    Mirrors palette.draw_minor_rect: 'hsv' is the center, 'hsv_step_h'/'hsv_step_v'
    are the horizontal and vertical increments.
    """
    out = []
    for j in range(MINOR_HEIGHT):
        for i in range(MINOR_WIDTH):
            hsv = _offset_hsv(entry["hsv"], entry["hsv_step_h"], i - 1)
            hsv = _offset_hsv(hsv, entry["hsv_step_v"], j - 1)
            out.append(hsv)
    return out


def _slot_hsvs_transition(entry, length):
    """Ideal HSV of every pixel of a transition line. Mirrors palette.draw_transition_line."""
    return [
        _lerp_hsv(entry["hsv_left"], entry["hsv_right"], i / (length - 1))
        for i in range(length)
    ]


def block_reference_hsv(slot_hsvs):
    """
    'Main color' of a block: the attractor pixel is placed here and every slot is
    offset from it by the same HSV delta the palette block has.

    The colors of a block span either a line in RGB space (the 4x3 major rect, the
    transition lines) or a flat patch (the 3x3 minor rect); either way its two ends
    are the most distant pair of colors, so their RGB midpoint is the middle of the
    block. Every color is searched, not just the middle row of the major rect: only
    that row is a gradient, the upper and lower ones are fully customizable and may
    well hold the extremes.
    """
    rgbs = [hsv_to_rgb_byte(*hsv) for hsv in slot_hsvs]
    if len(rgbs) == 1:
        return slot_hsvs[0]
    lo, hi = max(
        ((a, b) for a in range(len(rgbs)) for b in range(a + 1, len(rgbs))),
        key=lambda pair: rgb_distance_sq(*rgbs[pair[0]], *rgbs[pair[1]]),
    )
    midpoint = tuple((rgbs[lo][c] + rgbs[hi][c]) / 2.0 for c in range(3))
    return rgb_to_hsv_degrees(*midpoint)


def _input_colors_from_slots(slot_hsvs, reference_hsv, attractor_rgb, multiplier):
    """
    Build input_colors for a block: HSV deltas of every slot from the block's
    reference color, scaled by multiplier, applied around the attractor pixel's HSV.
    S/V clamped to [0, 1]. Same length and order as the block's palette colors.
    """
    h_ref, s_ref, v_ref = reference_hsv
    h_att, s_att, v_att = rgb_to_hsv_degrees(*attractor_rgb)
    colors = []
    for h, s, v in slot_hsvs:
        dh = hue_shortest_delta_deg(h_ref, h)
        H = (h_att + multiplier * dh) % 360.0
        S = max(0.0, min(1.0, s_att + multiplier * (s - s_ref)))
        V = max(0.0, min(1.0, v_att + multiplier * (v - v_ref)))
        colors.append(hsv_to_rgb_byte(H, S, V))
    return colors


def _palette_rect_colors(palette, coords, width, height, what):
    """Read a width x height block of the palette PNG at 'coords', row-major (j then i)."""
    col_start, row_start = coords
    if (
        col_start < 0
        or row_start < 0
        or col_start + width > PALETTE_WIDTH
        or row_start + height > PALETTE_HEIGHT
    ):
        raise ValueError(
            f"{what}: block {width}x{height} at {coords} does not fit into the "
            f"{PALETTE_WIDTH}x{PALETTE_HEIGHT} palette"
        )
    return [
        palette[row_start + j][col_start + i]
        for j in range(height)
        for i in range(width)
    ]


def resolve_block(kind, index):
    """
    Resolve an attractor kind/index to the palette block it points at.
    Returns (entry, width, height, slot_hsvs) with slot_hsvs in row-major order.
    """
    if kind == "4q":
        if index >= len(COLORS_MAJOR):
            raise ValueError(f"4q index {index} out of range for COLORS_MAJOR")
        entry = COLORS_MAJOR[index]
        return (entry, MAJOR_WIDTH, MAJOR_HEIGHT, _slot_hsvs_major(entry))
    if kind == "3q":
        if index >= len(COLORS_MINOR):
            raise ValueError(f"3q index {index} out of range for COLORS_MINOR")
        entry = COLORS_MINOR[index]
        return (entry, MINOR_WIDTH, MINOR_HEIGHT, _slot_hsvs_minor(entry))
    if kind == "4t":
        if index >= len(COLORS_TRANSITION_4):
            raise ValueError(f"4t index {index} out of range for COLORS_TRANSITION_4")
        entry = COLORS_TRANSITION_4[index]
        length = TRANSITION_4_LENGTH
        return (entry, length, 1, _slot_hsvs_transition(entry, length))
    if kind == "3t":
        if index >= len(COLORS_TRANSITION_3):
            raise ValueError(f"3t index {index} out of range for COLORS_TRANSITION_3")
        entry = COLORS_TRANSITION_3[index]
        length = TRANSITION_3_LENGTH
        return (entry, length, 1, _slot_hsvs_transition(entry, length))
    if kind == "1s":
        if index >= len(COLORS_SPECIAL):
            raise ValueError(f"1s index {index} out of range for COLORS_SPECIAL")
        entry = COLORS_SPECIAL[index]
        return (entry, 1, 1, [entry["hsv"]])
    raise ValueError(f"Unknown attractor kind {kind!r}")


def prepare_attractors(palette, source_grid, width, height, attractor_strings):
    """
    For each attractor string, parse it and attach palette_colors (from the palette PNG)
    and input_colors (derived from the attractor pixel; same length and order).
    Returns a list of dicts, one per string.
    """
    prepared = []
    for s in attractor_strings:
        spec = parse_attractor(s)
        x, y = spec["x"], spec["y"]
        if not (0 <= x < width and 0 <= y < height):
            raise ValueError(
                f"Attractor {spec['raw']!r}: pixel ({x},{y}) out of image bounds "
                f"{width}x{height}"
            )
        attractor_rgb = source_grid[y][x]
        kind = spec["kind"]
        idx = spec["index"]
        try:
            multiplier = int(spec["multiplier_str"])
        except ValueError as e:
            raise ValueError(
                f"Attractor {spec['raw']!r}: multiplier must be an integer, "
                f"got {spec['multiplier_str']!r}"
            ) from e

        entry, block_w, block_h, slot_hsvs = resolve_block(kind, idx)
        palette_colors = _palette_rect_colors(
            palette, entry["coords"], block_w, block_h, f"{kind}{idx}"
        )
        input_colors = _input_colors_from_slots(
            slot_hsvs, block_reference_hsv(slot_hsvs), attractor_rgb, multiplier
        )

        prepared.append(
            {
                **spec,
                "multiplier": multiplier,
                "block_size": (block_w, block_h),
                "palette_colors": palette_colors,
                "input_colors": input_colors,
            }
        )
    return prepared


def write_attractor_debug_png(attractor, path="tmp.png"):
    """
    Write an 8x4 PNG: left 4x4 = palette_colors, right 4x4 = input_colors (same layout).

    The block is drawn at the top-left corner of its quad, row-major, so 4q takes
    4x3, 3q takes 3x3, 4t/3t take a single row and 1s a single pixel; the rest is black.
    """
    block_w, block_h = attractor["block_size"]
    rows = [[(0, 0, 0) for _ in range(8)] for _ in range(4)]

    def fill_quad(col0, colors):
        for j in range(block_h):
            for i in range(block_w):
                rows[j][col0 + i] = colors[j * block_w + i]

    fill_quad(0, attractor["palette_colors"])
    fill_quad(4, attractor["input_colors"])

    flat_rows = []
    for row in rows:
        flat = []
        for r, g, b in row:
            flat.extend([r, g, b])
        flat_rows.append(flat)

    with open(path, "wb") as f:
        writer = png.Writer(width=8, height=4, greyscale=False, alpha=False)
        writer.write(f, flat_rows)


def rgb_distance_sq(r1, g1, b1, r2, g2, b2):
    """Squared Euclidean distance in RGB space (avoids sqrt for comparison)."""
    return (r1 - r2) ** 2 + (g1 - g2) ** 2 + (b1 - b2) ** 2


def convert_image_with_attractors(image_grid, width, height, attractors):
    """
    Each pixel is assigned to the attractor whose anchor pixel is closest in RGB.
    Then the pixel is mapped to palette_colors[k] where k minimizes distance to
    input_colors within that attractor (nearest input slot).
    """
    ref = [image_grid[a["y"]][a["x"]] for a in attractors]
    out_grid = []
    for py in range(height):
        row_out = []
        for px in range(width):
            r, g, b = image_grid[py][px]
            best_ai = 0
            best_ad = rgb_distance_sq(r, g, b, *ref[0])
            for ai in range(1, len(attractors)):
                d = rgb_distance_sq(r, g, b, *ref[ai])
                if d < best_ad:
                    best_ad = d
                    best_ai = ai
            att = attractors[best_ai]
            ic = att["input_colors"]
            pc = att["palette_colors"]
            best_k = 0
            best_kd = rgb_distance_sq(r, g, b, *ic[0])
            for k in range(1, len(ic)):
                kd = rgb_distance_sq(r, g, b, *ic[k])
                if kd < best_kd:
                    best_kd = kd
                    best_k = k
            row_out.append(pc[best_k])
        out_grid.append(row_out)
    return out_grid


def assert_out_grid_in_attractor_palette_colors(out_grid, attractors):
    """Every RGB in out_grid must appear in some attractor's palette_colors."""
    allowed = set()
    for a in attractors:
        allowed.update(a["palette_colors"])
    for py, row in enumerate(out_grid):
        for px, rgb in enumerate(row):
            if rgb not in allowed:
                raise ValueError(
                    f"Adjusted pixel ({px}, {py}) color {rgb} is not in any "
                    f"attractor palette_colors"
                )


def save_image_rgb(path, grid, width, height):
    """Write RGB PNG from grid[row][col] = (r, g, b)."""
    image_data = []
    for row in grid:
        flat_row = []
        for r, g, b in row:
            flat_row.extend([r, g, b])
        image_data.append(flat_row)
    with open(path, "wb") as f:
        writer = png.Writer(width=width, height=height, greyscale=False, alpha=False)
        writer.write(f, image_data)


def hsv_distance_sq(r1, g1, b1, r2, g2, b2):
    """
    Squared distance in HSV. Hue uses shortest arc on the circle (colorsys: h in [0, 1)).
    S and V are linear in [0, 1].
    """
    h1, s1, v1 = colorsys.rgb_to_hsv(r1 / 255.0, g1 / 255.0, b1 / 255.0)
    h2, s2, v2 = colorsys.rgb_to_hsv(r2 / 255.0, g2 / 255.0, b2 / 255.0)
    dh = abs(h1 - h2)
    dh = min(dh, 1.0 - dh)
    ds = s1 - s2
    dv = v1 - v2
    return dh * dh + ds * ds + dv * dv


def load_palette_rgb(path):
    """Load palette PNG and return 2D list of (r, g, b) per pixel. Index as [row][col]. Must be 8x32."""
    reader = png.Reader(filename=path)
    w, h, rows, info = reader.read()
    if w != PALETTE_WIDTH or h != PALETTE_HEIGHT:
        raise ValueError(
            f"Palette must be {PALETTE_WIDTH}x{PALETTE_HEIGHT}, got {w}x{h}"
        )
    palette = []
    for row in rows:
        row = list(row)
        pixel_row = []
        for col in range(w):
            if info.get("alpha"):
                r, g, b = row[col * 4], row[col * 4 + 1], row[col * 4 + 2]
            else:
                r, g, b = row[col * 3], row[col * 3 + 1], row[col * 3 + 2]
            pixel_row.append((r, g, b))
        palette.append(pixel_row)
    return palette


def load_image_rgb(path):
    """
    Load input PNG and return (width, height, pixel_grid) where pixel_grid[row][col] = (r, g, b).
    """
    reader = png.Reader(filename=path)
    w, h, rows, info = reader.read()
    grid = []
    for row in rows:
        row = list(row)
        pixel_row = []
        for col in range(w):
            if info.get("alpha"):
                r, g, b = row[col * 4], row[col * 4 + 1], row[col * 4 + 2]
            else:
                r, g, b = row[col * 3], row[col * 3 + 1], row[col * 3 + 2]
            pixel_row.append((r, g, b))
        grid.append(pixel_row)
    return (w, h, grid)



def main():
    parser = argparse.ArgumentParser(description="Adjust image colors to palette.")
    parser.add_argument("-p", "--palette", required=True, help="Path to palette PNG (e.g. 8x32)")
    parser.add_argument("-s", "--source", required=True, dest="source", help="Path to input image PNG to adjust")
    parser.add_argument("attractors", nargs="+", help="Attractor strings X.Y.S.M, e.g. 40.221.4t1.3")
    args = parser.parse_args()

    palette = load_palette_rgb(args.palette)
    width, height, image_grid = load_image_rgb(args.source)

    attractors = prepare_attractors(
        palette, image_grid, width, height, args.attractors
    )
    assert all(
        len(a["palette_colors"]) == len(a["input_colors"]) for a in attractors
    )

    #write_attractor_debug_png(attractors[0])

    out_grid = convert_image_with_attractors(image_grid, width, height, attractors)
    assert_out_grid_in_attractor_palette_colors(out_grid, attractors)
    base, ext = os.path.splitext(args.source)
    save_image_rgb(f"{base}_adjusted{ext}", out_grid, width, height)


if __name__ == "__main__":
    main()
