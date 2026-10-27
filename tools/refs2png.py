#!/usr/bin/env python3

# Tool to convert rendering tests references saved as *.tga to *.png and move them to testing resources
# Launched from the repository root
#
# <binary>/data/rendering_tests/<name>_ref.tga -> testing/resources/rendering_tests/<name>_ref.png
# TGA: uncompressed 32-bit BGRA, origin top-left or bottom-left

import argparse
import os
import struct
import sys

import png

REFS_SRC_DIR = os.path.join("data", "rendering_tests")
REFS_DST_DIR = os.path.join("testing", "resources", "rendering_tests")
TGA_HEADER = 18


class TgaError(Exception):
    pass


def read_tga(src: str) -> (int, int, [[int]]):
    with open(src, mode="rb") as src_file:
        data = src_file.read()

    if len(data) < TGA_HEADER:
        raise TgaError("'{}' is too short".format(src))

    id_length, image_type = data[0], data[2]
    width, height, bpp, descriptor = struct.unpack("<HHBB", data[12:18])

    if image_type != 2 or bpp != 32:
        raise TgaError("'{}' is not an uncompressed 32-bit *.tga".format(src))

    offset = TGA_HEADER + id_length
    stride = width * 4

    if len(data) < offset + stride * height:
        raise TgaError("'{}' has not enough pixel data".format(src))

    top_down = (descriptor & 0x20) != 0
    rows = []

    for y in range(0, height):
        start = offset + (y if top_down else height - 1 - y) * stride
        bgra = data[start:start + stride]
        row = bytearray(stride)
        row[0::4] = bgra[2::4]
        row[1::4] = bgra[1::4]
        row[2::4] = bgra[0::4]
        row[3::4] = bgra[3::4]
        rows.append(row)

    return width, height, rows


def convert_tga(src: str, dst: str):
    width, height, rows = read_tga(src)

    with open(dst, mode="wb") as dst_file:
        writer = png.Writer(width=width, height=height, greyscale=False, alpha=True)
        writer.write(dst_file, rows)

    print("'{}' -> '{}' ({}x{})".format(src, dst, width, height))


def move_refs(src_dir: str, dst_dir: str):
    if not os.path.isdir(src_dir):
        return

    os.makedirs(dst_dir, exist_ok=True)

    for name in sorted(os.listdir(src_dir)):
        base, ext = os.path.splitext(name)
        if ext.lower() == ".tga":
            src = os.path.join(src_dir, name)
            convert_tga(src, os.path.join(dst_dir, base + ".png"))
            os.remove(src)


def main() -> int:
    parser = argparse.ArgumentParser(prog="refs2png", description="Tool to convert rendering tests references from *.tga to *.png")
    parser.add_argument("-b", "--binary", type=str, help="Binary directory where tests save references")
    args = parser.parse_args()

    if args.binary is None:
        parser.print_help()
        return 0 if len(sys.argv) == 1 else 1

    try:
        move_refs(os.path.join(args.binary, REFS_SRC_DIR), REFS_DST_DIR)
    except (TgaError, OSError, struct.error) as e:
        print("Error: {}".format(e), file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
