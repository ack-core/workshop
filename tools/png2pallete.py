"""
Tool to correct a png in accordance with palette
"""

import argparse
import png
from array import array

class Palette:
    def __init__(self, img: array):
        self._data = img
        pass

    def get_color(self, index: int, brightness: int):
        index = min(max(index, 0), 30)
        brightness = min(max(brightness, 0), 255)
        offset = (8 + index * 8 + brightness // 32) * 4
        return self._data[offset:offset + 4]

def main(file: str, palette: str, material: int, mask: str):
    file_reader = png.Reader(filename=file)
    file_w, file_h, data, _ = file_reader.asRGBA8()
    file_img = sum([array('B', e) for e in data], array('B'))

    palette_reader = png.Reader(filename=palette)
    palette_img = sum([array('B', e) for e in palette_reader.asRGBA8()[2]], array('B'))
    palette = Palette(img=palette_img)

    material_indexes = None
    if mask:
        mask_reader = png.Reader(filename=mask)
        mask_img = sum([array('B', e) for e in mask_reader.asRGBA8()[2]], array('B'))
        material_indexes = [mask_img[i * 4] // 8 for i in range(0, file_w * file_h)]

    for i in range(0, file_w * file_h):
        pixel_start = i * 4
        pixel_end = i * 4 + 4
        color = file_img[pixel_start:pixel_end]
        brightness = (2 * color[0] + 3 * color[1] + color[2]) // 6

        m = material if material else material_indexes[i]
        color = palette.get_color(m, brightness)

        file_img[pixel_start:pixel_end] = array('B', color)

    with open(file + "1", 'wb') as f:
        writer = png.Writer(width=file_w, height=file_h, greyscale=False, alpha=True)
        writer.write_array(f, pixels=file_img)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to correct a png")
    parser.add_argument("-f", "--file", type=str, required=True, help="Path to the png that is corrected")
    parser.add_argument("-p", "--palette", type=str, required=True, help="Path to the palette")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--material", type=int, help="Index of the material")
    group.add_argument("--mask", type=str, help="Path to the png with a material mask")
    args = parser.parse_args()
    main(**vars(args))
