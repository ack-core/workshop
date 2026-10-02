#!/usr/bin/env python3

# Tool to scale a *.vox model up by an integer factor, every voxel becomes a cube
# Format: https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
#
# 'VOX ' + version
# MAIN
#   PACK      - optional, number of models
#   SIZE/XYZI - one pair per model
#   RGBA      - palette, color [0-254] is mapped to palette index [1-255]
#
# 'SIZE' and 'XYZI' are rewritten, every other chunk is copied as is

import argparse
import io
import struct
import sys

VOX_MAX_SIZE = 256          # MagicaVoxel refuses anything larger
VOX_HEADER_SIZE = 12        # chunk id + content size + children size
VOX_READ_MAIN = 8           # 'VOX ' + version
DEFAULT_FACTOR = 3


class VoxError(Exception):
    pass


class VoxChunk:
    def __init__(self, name: bytes, content: bytes, children: bytes = b""):
        self.name = name
        self.content = content
        self.children = children    # kept raw, only 'MAIN' children are looked into

    def write(self, out):
        out.write(self.name)
        out.write(struct.pack("<ii", len(self.content), len(self.children)))
        out.write(self.content)
        out.write(self.children)


def read_chunks(data: bytes) -> [VoxChunk]:
    chunks = []
    offset = 0
    while offset < len(data):
        name = data[offset:offset + 4]
        content_size, children_size = struct.unpack("<ii", data[offset + 4:offset + VOX_HEADER_SIZE])
        offset += VOX_HEADER_SIZE
        content = data[offset:offset + content_size]
        offset += content_size
        children = data[offset:offset + children_size]
        offset += children_size

        if len(content) != content_size or len(children) != children_size:
            raise VoxError("chunk '{}' is truncated".format(name.decode(errors="replace")))
        chunks.append(VoxChunk(name, content, children))

    return chunks


def scale_size(content: bytes, factor: int, src: str) -> bytes:
    size = [s * factor for s in struct.unpack("<iii", content[:12])]
    if max(size) > VOX_MAX_SIZE:
        raise VoxError("'{}' scaled is {}x{}x{}, the limit is {}".format(src, *size, VOX_MAX_SIZE))

    return struct.pack("<iii", *size)


def scale_voxels(content: bytes, factor: int) -> (bytes, int):
    count = struct.unpack("<i", content[:4])[0]
    voxels = []
    for c in range(0, count):
        x, y, z, color = content[4 + c * 4:4 + c * 4 + 4]
        for dz in range(0, factor):
            for dy in range(0, factor):
                for dx in range(0, factor):
                    voxels.append(bytes((x * factor + dx, y * factor + dy, z * factor + dz, color)))

    return struct.pack("<i", len(voxels)) + b"".join(voxels), len(voxels)


def scale_vox(src: str, dst: str, factor: int):
    with open(src, mode="rb") as src_file:
        data = src_file.read()

    if data[:4] != b'VOX ':
        raise VoxError("'{}' is not a *.vox file".format(src))

    version = data[4:VOX_READ_MAIN]
    main_chunks = read_chunks(data[VOX_READ_MAIN:])
    if len(main_chunks) != 1 or main_chunks[0].name != b'MAIN':
        raise VoxError("'{}' has no 'MAIN' chunk".format(src))

    main_chunk = main_chunks[0]
    children = read_chunks(main_chunk.children)
    models = 0
    total = 0
    for chunk in children:
        if chunk.name == b'SIZE':
            chunk.content = scale_size(chunk.content, factor, src)
            models += 1
        elif chunk.name == b'XYZI':
            chunk.content, count = scale_voxels(chunk.content, factor)
            total += count
        elif chunk.name not in (b'PACK', b'RGBA'):
            # Scene graph chunks ('nTRN' etc.) hold translations that would need scaling too
            print("Warning: chunk '{}' copied unchanged".format(chunk.name.decode(errors="replace")),
                  file=sys.stderr)

    out = io.BytesIO()
    for chunk in children:
        chunk.write(out)
    main_chunk.children = out.getvalue()

    with open(dst, mode="wb") as dst_file:
        dst_file.write(b"VOX ")
        dst_file.write(version)
        main_chunk.write(dst_file)

    print("'{}' -> '{}' (x{}, {} model(s), {} voxels)".format(src, dst, factor, models, total))


def main() -> int:
    parser = argparse.ArgumentParser(prog="voxscale", description="Tool to scale a *.vox model up by an integer factor")
    parser.add_argument("-s", "--src", type=str, required=True, help="Source *.vox file")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Destination *.vox file")
    parser.add_argument("-f", "--factor", type=int, default=DEFAULT_FACTOR,
                        help="Scale factor, {} by default".format(DEFAULT_FACTOR))
    args = parser.parse_args()

    if args.factor < 1:
        parser.error("factor must be at least 1")

    try:
        scale_vox(args.src, args.dst, args.factor)
    except (VoxError, OSError, struct.error, ValueError) as e:
        print("Error: {}".format(e), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
