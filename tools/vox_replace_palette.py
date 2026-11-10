from __future__ import annotations

"""
Tool to replace the palette of every *.vox file in tools/tmp,
each voxel gets the index of the nearest RGB color in the new palette
Format: https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
"""

import argparse
import glob
import io
import os
import struct
import sys
import png

from adjust_colors import rgb_distance_sq
from voxscale import VOX_READ_MAIN, VoxError, read_chunks

PALETTE_COLORS = 256


def load_palette(path: str) -> bytes:
    w, h, rows, _ = png.Reader(filename=path).asRGBA8()
    if w * h != PALETTE_COLORS:
        raise VoxError("'{}' has {} colors, {} expected".format(path, w * h, PALETTE_COLORS))
    return bytes(v for row in reversed(list(rows)) for v in row)


def rgb_list(rgba: bytes) -> [(int, int, int)]:
    return [tuple(rgba[i:i + 3]) for i in range(0, (PALETTE_COLORS - 1) * 4, 4)]


def replace_palette(path: str, palette: bytes):
    with open(path, mode="rb") as f:
        data = f.read()

    if data[:4] != b'VOX ':
        raise VoxError("'{}' is not a *.vox file".format(path))

    main_chunks = read_chunks(data[VOX_READ_MAIN:])
    if len(main_chunks) != 1 or main_chunks[0].name != b'MAIN':
        raise VoxError("'{}' has no 'MAIN' chunk".format(path))

    main_chunk = main_chunks[0]
    children = read_chunks(main_chunk.children)
    rgba = next((c for c in children if c.name == b'RGBA'), None)
    if rgba is None:
        raise VoxError("'{}' has no palette ('RGBA' chunk)".format(path))

    new_colors = rgb_list(palette)
    nearest = [1 + min(range(len(new_colors)), key=lambda k: rgb_distance_sq(c, new_colors[k]))
               for c in rgb_list(rgba.content)]
    remap = bytes([0] + nearest)

    rgba.content = palette
    for chunk in children:
        if chunk.name == b'XYZI':
            voxels = bytearray(chunk.content)
            voxels[7::4] = voxels[7::4].translate(remap)
            chunk.content = bytes(voxels)

    out = io.BytesIO()
    for chunk in children:
        chunk.write(out)
    main_chunk.children = out.getvalue()

    with open(path, mode="wb") as f:
        f.write(data[:VOX_READ_MAIN])
        main_chunk.write(f)

    print("'{}' palette replaced".format(path))


def main() -> int:
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description="Tool to replace the palette of *.vox files in tools/tmp")
    parser.add_argument("-p", "--palette", type=str, default=os.path.join(script_dir, "..", "resources", "palette.png"),
                        help="Path to the palette png (256 colors), resources/palette.png by default")
    args = parser.parse_args()

    try:
        palette = load_palette(args.palette)
        for path in glob.glob(os.path.join(script_dir, "tmp", "*.vox")):
            replace_palette(path, palette)
    except (VoxError, OSError, struct.error, png.Error) as e:
        print("Error: {}".format(e), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
