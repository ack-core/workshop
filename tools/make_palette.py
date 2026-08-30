# Tool to make a palette from 'main colors'
# https://pickcoloronline.com/

import colorsys
import png
import os

# 4x3
COLORS_MAJOR = [
    {
        "name" : "forest",
        "hsv_left" : (102, 0.35, 0.42),
        "hsv_right" : (124, 0.29, 0.30),
        "h_step" : (0, 0, 0, 0),
        "s_step" : (0.08, 0.08, 0.08, 0.08),
        "v_step" : (0.08, 0.08, 0.08, 0.08),
        "coords" : (0, 1),
    },
    {
        "name" : "forest1",
        "hsv_left" : (89, 0.96, 0.47),
        "hsv_right" : (108, 0.60, 0.25),
        "h_step" : (0, 0, 0, 0),
        "s_step" : (0.05, 0.05, 0.05, 0.05),
        "v_step" : (0.05, 0.05, 0.05, 0.05),
        "coords" : (0, 5),
    },
    {
        "name" : "grass0",
        "hsv_left" : (61, 0.67, 0.51),
        "hsv_right" : (82, 0.56, 0.42),
        "h_step" : (0, 0, 0, 0),
        "s_step" : (0.05, 0.05, 0.05, 0.05),
        "v_step" : (0.05, 0.05, 0.05, 0.05),
        "coords" : (0, 9),
    },
    {
        "name" : "grass1",
        "hsv_left" : (64, 0.47, 0.68),
        "hsv_right" : (84, 0.63, 0.57),
        "h_step" : (0, 0, 0, 0),
        "s_step" : (0.05, 0.05, 0.05, 0.05),
        "v_step" : (0.05, 0.05, 0.05, 0.05),
        "coords" : (0, 13),
    },
    {
        "name" : "dirt",
        "hsv_left" : (45, 0.51, 0.64),
        "hsv_right" : (38, 0.53, 0.47),
        "h_step" : (0, 0, 0, 0),
        "s_step" : (0.05, 0.05, 0.05, 0.05),
        "v_step" : (0.05, 0.05, 0.05, 0.05),
        "coords" : (0, 17),
    },
    {
        "name" : "volcanic",
        "hsv_left" : (31, 0.28, 0.40),
        "hsv_right" : (47, 0.12, 0.29),
        "h_step" : (0, 0, 0, 0),
        "s_step" : (0.05, 0.05, 0.05, 0.05),
        "v_step" : (0.05, 0.05, 0.05, 0.05),
        "coords" : (0, 21),
    },
]

# 3x3
COLORS_MINOR = [
    {
        "name" : "metal",
        "hsv" : (218, 0.22, 0.75),
        "hsv_step_h" : (0, 0.08, 0.00),
        "hsv_step_v" : (0, 0.00, 0.08),
        "coords" : (5, 1),
    },
    {
        "name" : "wood",
        "hsv" : (39, 0.80, 0.56),
        "hsv_step_h" : (0, 0.12, 0.00),
        "hsv_step_v" : (0, 0.00, 0.08),
        "coords" : (5, 5),
    },
    {
        "name" : "castlestone",
        "hsv" : (38, 0.20, 0.86),
        "hsv_step_h" : (0, 0.07, 0.00),
        "hsv_step_v" : (0, 0.00, 0.07),
        "coords" : (5, 9),
    },
    {
        "name" : "free_1",
        "hsv" : (0, 0.0, 0.5),
        "hsv_step_h" : (0, 0.0, 0.0),
        "hsv_step_v" : (0, 0.0, 0.0),
        "coords" : (5, 13),
    },
    {
        "name" : "free_2",
        "hsv" : (0, 0.0, 0.5),
        "hsv_step_h" : (0, 0.0, 0.0),
        "hsv_step_v" : (0, 0.0, 0.0),
        "coords" : (5, 17),
    },
    {
        "name" : "free_3",
        "hsv" : (0, 0.0, 0.5),
        "hsv_step_h" : (0, 0.0, 0.0),
        "hsv_step_v" : (0, 0.0, 0.0),
        "coords" : (5, 21),
    },
]

COLORS_TRANSITION_4 = [
    {
        "hsv_left": (193, 0.90, 0.83),
        "hsv_right": (183, 0.50, 0.99),
        "coords": (0, 25),
    },
    {
        "hsv_left": (120, 0.08, 0.39),
        "hsv_right": (184, 0.08, 0.65),
        "coords": (0, 27),
    },
    {
        "hsv_left": (22, 0.42, 0.36),
        "hsv_right": (27, 0.42, 0.56),
        "coords": (0, 29),
    },
    {
        "hsv_left": (246, 0.16, 0.25),
        "hsv_right": (334, 0.12, 0.52),
        "coords": (0, 31),
    }
]

COLORS_TRANSITION_3 = [
    {
        "hsv_left": (19, 0.40, 0.70),
        "hsv_right": (19, 0.60, 0.70),
        "coords": (5, 25),
    },
    {
        "hsv_left": (192, 0.40, 0.65),
        "hsv_right": (197, 0.60, 0.65),
        "coords": (5, 27),
    },
    {
        "hsv_left": (53, 0.48, 0.66),
        "hsv_right": (58, 0.38, 0.55),
        "coords": (5, 29),
    },
    {
        "hsv_left": (0, 0.00, 0.50),
        "hsv_right": (0, 0.00, 0.50),
        "coords": (5, 31),
    }
]

COLORS_SPECIAL = [
    {
        "hsv": (0, 0.00, 0.50),
        "coords": (4, 0),
    },
    {
        "hsv": (0, 0.00, 0.50),
        "coords": (4, 4),
    },
    {
        "hsv": (0, 0.00, 0.50),
        "coords": (4, 8),
    },
    {
        "hsv": (0, 0.00, 0.50),
        "coords": (4, 12),
    },
    {
        "hsv": (0, 0.00, 0.50),
        "coords": (4, 16),
    },
    {
        "hsv": (0, 0.00, 0.50),
        "coords": (4, 20),
    },
    {
        "hsv": (0, 0.00, 0.50),
        "coords": (4, 24),
    },
    {
        "hsv": (28, 0.28, 0.80),
        "coords": (4, 26),
    },
    {
        "hsv": (0, 0.00, 0.50),
        "coords": (4, 28),
    },
    {
        "hsv": (0, 0.00, 0.50),
        "coords": (4, 30),
    },
]

# Image dimensions
WIDTH = 8
HEIGHT = 32
STRIDE = 4   # 3 rows + 1 gap

MAJOR_WIDTH = 4
MAJOR_HEIGHT = 3
MINOR_WIDTH = 3
MINOR_HEIGHT = 3


def hsv_to_rgb_byte(h_deg, s, v):
    h_deg = h_deg % 360
    h_norm = h_deg / 360.0
    s = max(0.0, min(1.0, s))
    v = max(0.0, min(1.0, v))
    r, g, b = colorsys.hsv_to_rgb(h_norm, s, v)
    return (round(r * 255), round(g * 255), round(b * 255))


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


def _offset_hsv(hsv, step, times):
    return (hsv[0] + times * step[0], hsv[1] + times * step[1], hsv[2] + times * step[2])


def draw_major_rect(image, entry):
    # 4x3: the middle row goes from 'hsv_left' to 'hsv_right'; every column has
    # its own increment, taken from 'h_step'/'s_step'/'v_step', which is added
    # for the upper row and subtracted for the lower one.
    col_start, row_start = entry["coords"]
    for i in range(MAJOR_WIDTH):
        t = i / (MAJOR_WIDTH - 1)
        middle = _lerp_hsv(entry["hsv_left"], entry["hsv_right"], t)
        step = (entry["h_step"][i], entry["s_step"][i], entry["v_step"][i])
        for j in range(MAJOR_HEIGHT):
            hsv = _offset_hsv(middle, step, 1 - j)
            image[row_start + j][col_start + i] = hsv_to_rgb_byte(*hsv)


def draw_minor_rect(image, entry):
    # 3x3: 'hsv' is the center, 'hsv_step_h'/'hsv_step_v' are the horizontal
    # and vertical increments, so the top-left pixel is
    # 'hsv' - 'hsv_step_h' - 'hsv_step_v'.
    col_start, row_start = entry["coords"]
    step_h = entry["hsv_step_h"]
    step_v = entry["hsv_step_v"]
    for j in range(MINOR_HEIGHT):
        for i in range(MINOR_WIDTH):
            hsv = _offset_hsv(entry["hsv"], step_h, i - 1)
            hsv = _offset_hsv(hsv, step_v, j - 1)
            image[row_start + j][col_start + i] = hsv_to_rgb_byte(*hsv)


def draw_transition_line(image, entry, length):
    col_start, row = entry["coords"]
    for i in range(length):
        t = i / (length - 1)
        hsv = _lerp_hsv(entry["hsv_left"], entry["hsv_right"], t)
        image[row][col_start + i] = hsv_to_rgb_byte(*hsv)


def draw_specials(image):
    for entry in COLORS_SPECIAL:
        col, row = entry["coords"]
        image[row][col] = hsv_to_rgb_byte(*entry["hsv"])


def generate_palette():
    image = [[(0, 0, 0) for _ in range(WIDTH)] for _ in range(HEIGHT)]

    for entry in COLORS_MAJOR:
        draw_major_rect(image, entry)

    for entry in COLORS_MINOR:
        draw_minor_rect(image, entry)

    for entry in COLORS_TRANSITION_4:
        draw_transition_line(image, entry, 4)

    for entry in COLORS_TRANSITION_3:
        draw_transition_line(image, entry, 3)

    draw_specials(image)

    image_data = []
    for row in image:
        flat_row = []
        for r, g, b in row:
            flat_row.extend([r, g, b])
        image_data.append(flat_row)

    return image_data

def main():
    # Get the directory where this script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_path = os.path.join(script_dir, 'palette.png')
    
    # Generate palette data
    image_data = generate_palette()
    
    # Write PNG file
    with open(output_path, 'wb') as f:
        writer = png.Writer(width=WIDTH, height=HEIGHT, greyscale=False, alpha=False)
        writer.write(f, image_data)
    
    print(f"Palette generated successfully: {output_path}")

if __name__ == '__main__':
    main()
