from __future__ import annotations

"""
Tool to create vox file from png image
"""

import png
import typing
import argparse
import struct

from array import array

class Color:
    rgba = array('B', [0, 0, 0, 0])

    def __init__(self, r: int, g: int, b: int, a: int):
        self.rgba = array('B', [r, g, b, a])
        self._hash = struct.unpack("<i", self.rgba)[0]

    def __hash__(self):
        return self._hash

    def __eq__(self, other: Color):
        return self.rgba == other.rgba

class Palette:
    def __init__(self, img: array):
        self._color_to_index = {}
        self._colors = []

        for i in range(0, 1024, 4):
            index = i // 4
            r, g, b, a = img[i:i + 4]
            color = Color(r, g, b, a)
            self._color_to_index[color] = index
            self._colors.append(color)

    def get_color_index(self, color: Color):
        return self._color_to_index.get(color, 0)

    def get_colors(self):
        result = array('B')
        for c in self._colors:
            result.frombytes(c.rgba)

        return result.tobytes()

class VoxChunk:
    def __init__(self, name: bytes, content: bytes):
        self._name = name
        self._content_size = 0
        self._children_size = 0
        self._children_chunks = []
        self._content_data = content

    def add_child(self, chunk: VoxChunk):
        self._children_chunks.append(chunk)
        self._children_size += chunk._content_size + 12

    def write(self, out: typing.BinaryIO):
        out.write(self._name)
        out.write(struct.pack("<i", len(self._content_data)))
        out.write(struct.pack("<i", self._children_size))
        out.write(self._content_data)

        for child in self._children_chunks:
            child.write(out)


def main(src: str, palette: str, dst: str):
    if src.endswith(".png"):
        try:
            src_reader = png.Reader(filename=src)
            src_w, src_h, data, _ = src_reader.asRGBA8()
            src_img = sum([array('B', e) for e in data], array('B'))

            if palette.endswith(".png"):
                try:
                    palette_reader = png.Reader(filename=palette)
                    palette_w, palette_h, palette_data, _ = palette_reader.asRGBA8()

                    if palette_w * palette_h == 256:
                        palette_img = sum([array('B', e) for e in palette_data], array('B'))
                        palette = Palette(img=palette_img)

                        with open(dst, mode="wb") as f:
                            f.write(b"VOX ")
                            f.write(struct.pack("<i", 150))
                            main_chunk = VoxChunk(b"MAIN", b"")

                            pack_chunk = VoxChunk(b"PACK", struct.pack("<i", 1))
                            main_chunk.add_child(pack_chunk)

                            size_chunk = VoxChunk(b"SIZE", struct.pack("<iii", 1, 1, 1))
                            main_chunk.add_child(size_chunk)

                            voxels = array('B')
                            voxels.frombytes(struct.pack("<i", 1))
                            voxels.fromlist([0, 0, 0, 1])

                            data_chunk = VoxChunk(b"XYZI", voxels.tobytes())
                            main_chunk.add_child(data_chunk)

                            rgba_chunk = VoxChunk(b"RGBA", palette.get_colors())
                            main_chunk.add_child(rgba_chunk)
                            main_chunk.write(f)

                    else:
                        print("{} has incorrect size".format(palette))

                except FileNotFoundError:
                    print("{} not found".format(palette))
                except png.Error:
                    print("{} is invalid".format(palette))

            else:
                print("{} is not a *.png file".format(palette))

        except FileNotFoundError:
            print("{} not found".format(src))
        except png.Error:
            print("{} is invalid".format(src))

    else:
        print("{} is not a *.png file".format(src))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to create vox from png")
    parser.add_argument("-s", "--src", type=str, required=True, help="Path to the png source")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Path to the *.vox output")
    parser.add_argument("-p", "--palette", type=str, required=True, help="Path to the palette")
    args = parser.parse_args()
    main(**vars(args))
