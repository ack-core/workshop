from __future__ import annotations

"""
Tool to make engine resources list
Example of output:
    meshes/dev/8x8x8 8 8 8
    textures/ui/panel rgba 358 358
"""

import typing
import argparse
import os
import struct

PNG_READ_LENGTH = 26
PNG_SIGN = b'\x89PNG\r\n\x1a\n'
PNG_REQUIRED_DEPTH = 8
PNG_COLOR_TYPE_GRAYSCALE = 0
PNG_COLOR_TYPE_RGBA = 6

VOX_READ_MAIN = 20
VOX_READ_CHUNK_HEADER = 4

def list_png(root: str, file: str, out: typing.BinaryIO):
    with open(os.path.join(root, file), mode="rb") as f:
        header = f.read(PNG_READ_LENGTH)
        try:
            sign, _, _, w, h, depth, color_type = struct.unpack(">8siiiiBB", header)
            if sign == PNG_SIGN and depth == PNG_REQUIRED_DEPTH:
                color_types = {
                    PNG_COLOR_TYPE_GRAYSCALE: "grayscale",
                    PNG_COLOR_TYPE_RGBA: "rgba"
                }

                if color_type in color_types:
                    file = file.replace(".png", "")
                    msg = "{} {} {} {}\r\n".format(file, color_types[color_type], w, h)
                    print("---->", msg, end="")
                    out.write(msg.encode("utf-8"))

        except (Exception,):
            pass

def list_vox(root: str, file: str, out: typing.BinaryIO):
    with open(os.path.join(root, file), mode="rb") as f:
        _ = f.read(VOX_READ_MAIN)
        if f.read(VOX_READ_CHUNK_HEADER) == b'PACK':
            data = f.read(12)
            frame_count = struct.unpack("<i", data[8:12])[0]
            mx, my, mz = (0, 0, 0)

            for i in range(0, frame_count):
                if f.read(VOX_READ_CHUNK_HEADER) == b'SIZE':
                    data = f.read(20)
                    sz, sx, sy = struct.unpack("<iii", data[8:20])

                    mx = sx if sx > mx else mx
                    my = sy if sy > my else my
                    mz = sz if sz > mz else mz

                    if f.read(VOX_READ_CHUNK_HEADER) == b'XYZI':
                        data = f.read(12)
                        voxel_count = struct.unpack("<i", data[8:])[0]
                        f.read(voxel_count * 4)

                    else:
                        return
                else:
                    return

            file = file.replace(".vox", "")
            msg = "{} {} {} {}\r\n".format(file, mx, my, mz)
            print("---->", msg, end="")
            out.write(msg.encode("utf-8"))

def main(root: str, dst: str, ext: str):
    list_functions = {
        ".vox": list_vox,
        ".png": list_png
    }

    root = os.path.abspath(root)
    dst = os.path.abspath(dst)

    with open(dst, mode="wb") as f:
        for path, _, files in os.walk(root):
            for file in files:
                if file.endswith(ext) and ext in list_functions:
                    relpath = os.path.relpath(path, root)
                    if not relpath.startswith("."):
                        list_functions[ext](root, os.path.join(relpath, file), f)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to form list of resources")
    parser.add_argument("-r", "--root", type=str, required=True, help="Path where to search resources")
    parser.add_argument("-d", "--dst", type=str, required=True, help="File to write generated output")
    parser.add_argument("-e", "--ext", type=str, required=True, help="Extension. For example '.png'")
    args = parser.parse_args()
    main(**vars(args))
