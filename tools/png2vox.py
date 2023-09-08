from __future__ import annotations

"""
Tool to create vox file from png image
"""

import argparse
import struct
import png

from array import array

class Color:
    def __init__(self, r: int, g: int, b: int, a: int):
        self._rgba = array("B", [r, g, b, a])
        self._hash = struct.unpack("<i", self._rgba)

    def __hash__(self):
        return self._hash

    def __eq__(self, other: Color):
        return self._rgba == other._rgba

class Palette:
    def __init__(self, img: array):
        self._palette = {}
        for i in range(0, 1024, 4):
            color = Color(img[i], img[i + 1], img[i + 2], img[i + 3])
            self._palette[color] = i / 4

    def get_color_index(self, color: Color):
        return self._palette.get(color, 0)

def main(src: str, palette: str, dst: str):
    src_reader = png.Reader(filename=src)
    src_w, src_h, data, _ = src_reader.asRGBA8()
    src_img = sum([array('B', e) for e in data], array('B'))

    palette_reader = png.Reader(filename=palette)
    palette_img = sum([array('B', e) for e in palette_reader.asRGBA8()[2]], array('B'))
    palette = Palette(img=palette_img)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to create vox from png")
    parser.add_argument("-s", "--src", type=str, required=True, help="Path to the png source")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Path to the *.vox output")
    parser.add_argument("-p", "--palette", type=str, required=True, help="Path to the palette")
    args = parser.parse_args()
    main(**vars(args))
