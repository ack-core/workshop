from __future__ import annotations

"""
Tool to correct a png in accordance with palette
"""

import png
import argparse

from array import array

PALETTE_INDEX_MAX = 30

class Palette:
    def __init__(self, img: array):
        self._data = img
        pass

    def get_color(self, index: int, brightness: int):
        index = min(max(index, 0), PALETTE_INDEX_MAX)
        brightness = min(max(brightness, 0), 255)
        offset = (8 + index * 8 + brightness // 32) * 4
        return self._data[offset:offset + 4]

def main(file: str, palette: str, material: int, mask: str):
    if file.endswith(".png"):
        try:
            file_reader = png.Reader(filename=file)
            file_w, file_h, data, _ = file_reader.asRGBA8()
            file_img = sum([array('B', e) for e in data], array('B'))
            file_size = file_w * file_h

            if palette.endswith(".png"):
                try:
                    palette_reader = png.Reader(filename=palette)
                    palette_w, palette_h, palette_data, _ = palette_reader.asRGBA8()

                    if palette_w * palette_h == 256:
                        palette_img = sum([array('B', e) for e in palette_data], array('B'))
                        palette = Palette(img=palette_img)

                        material_indexes = None
                        if mask:
                            mask_reader = png.Reader(filename=mask)
                            mask_data = mask_reader.asRGBA8()[2]
                            mask_img = sum([array('B', e) for e in mask_data], array('B'))
                            material_indexes = [mask_img[i * 4] // 8 for i in range(0, file_size)]

                        for i in range(0, file_size):
                            pixel_start = i * 4
                            pixel_end = i * 4 + 4
                            color = file_img[pixel_start:pixel_end]
                            brightness = (2 * color[0] + 3 * color[1] + color[2]) // 6
                            m = material if material else material_indexes[i]
                            color = palette.get_color(m, brightness)
                            file_img[pixel_start:pixel_end] = array('B', color)

                        out_path = file.replace(".png", "_corrected.png")
                        try:
                            with open(out_path, 'wb') as f:
                                writer = png.Writer(width=file_w, height=file_h, greyscale=False, alpha=True)
                                writer.write_array(f, pixels=file_img)

                        except EnvironmentError:
                            print("can't open {} for writing".format(out_path))

                    else:
                        print("{} has incorrect size".format(palette))

                except FileNotFoundError:
                    print("{} not found".format(palette))
                except png.Error:
                    print("{} is invalid".format(palette))

            else:
                print("{} is not a *.png file".format(palette))

        except FileNotFoundError:
            print("{} not found".format(file))
        except png.Error:
            print("{} is invalid".format(file))

    else:
        print("{} is not a *.png file".format(file))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to correct a png")
    parser.add_argument("-f", "--file", type=str, required=True, help="Path to the png that is corrected")
    parser.add_argument("-p", "--palette", type=str, required=True, help="Path to the palette")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--material", type=int, help="Index of the material")
    group.add_argument("--mask", type=str, help="Path to the png with a material mask")
    args = parser.parse_args()
    main(**vars(args))
