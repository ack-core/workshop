#!/usr/bin/env python3

# Tool to convert a flat *.vox model to *.png
# Format: https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
#
# 'VOX ' + version
# MAIN
#   PACK      - optional, number of models
#   SIZE/XYZI - one pair per model
#   RGBA      - palette, color [0-254] is mapped to palette index [1-255]

import argparse
import struct
import sys

import png

VOX_READ_MAIN = 20          # 'VOX ' + version + 'MAIN' header
VOX_READ_CHUNK_HEADER = 4   # chunk id, the rest of the header is read with the content


class VoxError(Exception):
    pass


def read_vox(src: str) -> ((int, int, int), [(int, int, int, int)], [(int, int, int, int)]):
    with open(src, mode="rb") as src_file:
        if src_file.read(VOX_READ_MAIN)[:4] != b'VOX ':
            raise VoxError("'{}' is not a *.vox file".format(src))

        current_offset = src_file.tell()
        frame_count = 1

        if src_file.read(VOX_READ_CHUNK_HEADER) == b'PACK':
            data = src_file.read(12)
            frame_count = struct.unpack("<i", data[8:12])[0]
        else:
            src_file.seek(current_offset)

        if frame_count != 1:
            raise VoxError("'{}' has {} frames, expected a single one".format(src, frame_count))

        if src_file.read(VOX_READ_CHUNK_HEADER) != b'SIZE':
            raise VoxError("'{}' has no 'SIZE' chunk".format(src))
        data = src_file.read(20)
        size = struct.unpack("<iii", data[8:20])

        if src_file.read(VOX_READ_CHUNK_HEADER) != b'XYZI':
            raise VoxError("'{}' has no 'XYZI' chunk".format(src))
        data = src_file.read(12)
        count = struct.unpack("<i", data[8:12])[0]
        data = src_file.read(count * 4)
        voxels = [tuple(data[c * 4:c * 4 + 4]) for c in range(0, count)]

        if src_file.read(VOX_READ_CHUNK_HEADER) != b'RGBA':
            raise VoxError("'{}' has no palette ('RGBA' chunk)".format(src))
        data = src_file.read(8 + 1024)
        palette = [(0, 0, 0, 0)] + [tuple(data[8 + c * 4:8 + c * 4 + 4]) for c in range(0, 255)]

    return size, voxels, palette


def convert_vox(src: str, dst: str):
    (sx, sy, sz), voxels, palette = read_vox(src)

    if sz > 1:
        raise VoxError("'{}' has height {}, expected 1".format(src, sz))

    rows = [[0] * (sx * 4) for _ in range(0, sy)]
    for x, y, _, color in voxels:
        # 'y' grows away from the viewer in MagicaVoxel, PNG rows go top down
        rows[sy - 1 - y][x * 4:x * 4 + 4] = palette[color]

    with open(dst, mode="wb") as dst_file:
        writer = png.Writer(width=sx, height=sy, greyscale=False, alpha=True)
        writer.write(dst_file, rows)

    print("'{}' -> '{}' ({}x{}, {} voxels)".format(src, dst, sx, sy, len(voxels)))


def main() -> int:
    parser = argparse.ArgumentParser(prog="vox2png", description="Tool to convert a flat *.vox model to *.png")
    parser.add_argument("-s", "--src", type=str, help="Source *.vox file")
    parser.add_argument("-d", "--dst", type=str, help="Destination *.png file")
    args = parser.parse_args()

    if args.src is None or args.dst is None:
        parser.print_help()
        return 0 if len(sys.argv) == 1 else 1

    try:
        convert_vox(args.src, args.dst)
    except (VoxError, OSError, struct.error, IndexError) as e:
        print("Error: {}".format(e), file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
