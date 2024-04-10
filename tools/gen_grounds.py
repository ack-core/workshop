from __future__ import annotations

"""
Tool to generate optimized grounds from *.png
"""

import os
import argparse
import png
import struct


class Palette:
    def __init__(self, img: bytes):
        self._data = {int.from_bytes(img[i * 4:i * 4 + 4]): i for i in range(0, 256)}
        pass

    def get_index(self, color: bytes):
        return self._data[int.from_bytes(color)]


def generate_place(src: str, dst: str, palette: Palette):
    print("---- ", src)
    try:
        src_reader = png.Reader(filename=src)
        src_w, src_h, src_data, _ = src_reader.asRGBA8()
        src_data = bytes().join([bytes(e) for e in src_data])
        idx_data = [palette.get_index(src_data[i * 4:i * 4 + 4]) for i in range(0, src_w * src_h)]

        with open(dst, mode="wb") as f:
            f.write(b'GROUND\0\0\0\0\0\0\0\0\0\0')

            # size
            f.write(struct.pack("<iii", src_w, 1, src_h))
            mw = src_w - 1
            mh = src_h - 1

            # vertexes and indexes
            f.write(struct.pack("<ii", 4, 6))
            v = [
                (-0.5, -0.5, 0.0, 0.0),
                (mw + 0.5, -0.5, 1.0, 0.0),
                (-0.5, mh + 0.5, 0.0, 1.0),
                (mw + 0.5, mh + 0.5, 1.0, 1.0)
            ]
            for i in range(0, 4):
                f.write(struct.pack("<ffffffff", v[i][0], -0.5, v[i][1], 0.0, 1.0, 0.0, v[i][2], v[i][3]))
            f.write(struct.pack("<IIIIII", 0, 1, 2, 2, 1, 3))

            # grayscale texture with color indexes
            f.write(b'\0\0\0\0')
            writer = png.Writer(width=src_w, height=src_h, greyscale=True, alpha=False)
            writer.write_array(f, pixels=idx_data)

    except (Exception,) as e:
        print("---- Error: '{}'".format(e))

def main(src: str, dst: str, palette: str):
    src = os.path.abspath(src)
    dst = os.path.abspath(dst)
    palette = os.path.abspath(palette)

    try:
        palette_reader = png.Reader(filename=palette)
        palette_w, palette_h, palette_data, _ = palette_reader.asRGBA8()

        if palette_w * palette_h == 256:
            palette_object = Palette(bytes().join([bytes(e) for e in palette_data]))

            for path, _, files in os.walk(src):
                for file in files:
                    if file.endswith(".png"):
                        relpath_to = os.path.join(os.path.relpath(path, src), file)
                        fullpath_to = os.path.normpath(os.path.join(dst, relpath_to))
                        fullpath_to = fullpath_to.replace(".png", ".grd")
                        fullpath_from = os.path.join(path, file)
                        generate_place(fullpath_from, fullpath_to, palette_object)

        else:
            print("------ Error: '{}' has incorrect size".format(palette))

    except (Exception,) as e:
        print("---- Error: '{}'".format(e))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to generate optimized grounds from *.png")
    parser.add_argument("-s", "--src", type=str, required=True, help="Directory containing ground sources")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Directory to output")
    parser.add_argument("-p", "--palette", type=str, required=True, help="Path to the palette")
    args = parser.parse_args()
    main(**vars(args))
