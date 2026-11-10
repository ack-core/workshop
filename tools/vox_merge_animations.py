from __future__ import annotations

"""
Tool to merge frames of every *.vox file in tools/tmp (lexical order) into a single *.vox,
palette is taken from the first file
Format: https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
"""

import argparse
import glob
import io
import os
import struct
import sys

from voxscale import VOX_READ_MAIN, VoxChunk, VoxError, read_chunks


def read_vox(path: str) -> (bytes, [VoxChunk]):
    with open(path, mode="rb") as f:
        data = f.read()

    if data[:4] != b'VOX ':
        raise VoxError("'{}' is not a *.vox file".format(path))

    main_chunks = read_chunks(data[VOX_READ_MAIN:])
    if len(main_chunks) != 1 or main_chunks[0].name != b'MAIN':
        raise VoxError("'{}' has no 'MAIN' chunk".format(path))

    return data[:VOX_READ_MAIN], read_chunks(main_chunks[0].children)


def merge_vox(paths: [str], dst: str):
    if not paths:
        raise VoxError("no *.vox files to merge")

    header, first_children = read_vox(paths[0])
    rgba = next((c for c in first_children if c.name == b'RGBA'), None)
    if rgba is None:
        raise VoxError("'{}' has no palette ('RGBA' chunk)".format(paths[0]))

    frames = []
    for path in paths:
        frames += [c for c in read_vox(path)[1] if c.name in (b'SIZE', b'XYZI')]
    count = len(frames) // 2

    out = io.BytesIO()
    for chunk in [VoxChunk(b'PACK', struct.pack("<i", count))] + frames + [rgba]:
        chunk.write(out)

    with open(dst, mode="wb") as f:
        f.write(header)
        VoxChunk(b'MAIN', b"", out.getvalue()).write(f)

    print("{} file(s) -> '{}' ({} frame(s))".format(len(paths), dst, count))


def main() -> int:
    parser = argparse.ArgumentParser(description="Tool to merge frames of *.vox files in tools/tmp")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Destination *.vox file")
    args = parser.parse_args()

    tmp_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "tmp")
    try:
        merge_vox(sorted(glob.glob(os.path.join(tmp_dir, "*.vox"))), args.dst)
    except (VoxError, OSError, struct.error) as e:
        print("Error: {}".format(e), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
