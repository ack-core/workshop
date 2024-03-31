from __future__ import annotations

"""
Tool to generate optimized static meshes from *.vox 
Format:
4 bytes  - 'VOX '
1 uint32 - 127 (0x7f)
1 uint64 - flags (0x0)
1 uint32 - reserved
3 uint32 - size (x, y, z)
1 uint32 - frame count
1 uint32 - voxel count in frame
voxels
"""

import os
import argparse
import struct

VOX_READ_MAIN = 20
VOX_READ_CHUNK_HEADER = 4

def optimize(data: [(int, int, int, int)], sx: int, sy: int, sz: int) -> (int, bytes):
    output = bytearray()
    for e in data:
        voxel = struct.pack("<hhhBBBBBB", e[0], e[1], e[2], e[3], 0x7e, 0, 0, 0, 0)
        output += voxel

    return len(data), output

def convert_vox(src: str, dst: str):
    print("---- ", src)
    with open(src, mode="rb") as src_file:
        with open(dst, mode="wb") as dst_file:
            src_file.read(VOX_READ_MAIN)
            current_offset = src_file.tell()
            frame_count = 1

            if src_file.read(VOX_READ_CHUNK_HEADER) == b'PACK':
                data = src_file.read(12)
                frame_count = struct.unpack("<i", data[8:12])[0]
            else:
                src_file.seek(current_offset)

            mx, my, mz = (0, 0, 0)

            frame_size = [0] * frame_count
            frame_data = [b''] * frame_count
            frame_bounds = [(0, 0, 0)] * frame_count

            for i in range(0, frame_count):
                if src_file.read(VOX_READ_CHUNK_HEADER) == b'SIZE':
                    data = src_file.read(20)
                    sz, sx, sy = struct.unpack("<iii", data[8:20])

                    mx = sx if sx > mx else mx
                    my = sy if sy > my else my
                    mz = sz if sz > mz else mz

                    if src_file.read(VOX_READ_CHUNK_HEADER) == b'XYZI':
                        data = src_file.read(12)
                        frame_size[i] = struct.unpack("<i", data[8:])[0]
                        frame_data[i] = src_file.read(frame_size[i] * 4)
                        frame_bounds[i] = (sx, sy, sz)
                    else:
                        print("------ Error: '{}' has no 'XYZI' block".format(src))
                        return
                else:
                    print("------ Error: '{}' has no 'SIZE' block".format(src))
                    return

            dst_file.write(b'VOX \x7f\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0')
            dst_file.write(struct.pack("<iii", sx, sy, sz))
            dst_file.write(struct.pack("<i", frame_count))

            for i in range(0, frame_count):
                voxels = [tuple(frame_data[i][c * 4:c * 4 + 4]) for c in range(0, frame_size[i])]
                voxels = [(e[1], e[2], e[0], (31 - e[3] // 8) * 8 + e[3] % 8) for e in voxels]
                count, data = optimize(voxels, frame_bounds[i][0], frame_bounds[i][1], frame_bounds[i][2])
                dst_file.write(struct.pack("<i", count))
                dst_file.write(data)

def main(src: str, dst: str):
    src = os.path.abspath(src)
    dst = os.path.abspath(dst)

    for path, _, files in os.walk(src):
        for file in files:
            if file.endswith(".vox"):
                relpath_to = os.path.join(os.path.relpath(path, src), file)
                fullpath_to = os.path.normpath(os.path.join(dst, relpath_to))
                fullpath_from = os.path.join(path, file)
                convert_vox(fullpath_from, fullpath_to)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to generate optimized static meshes")
    parser.add_argument("-s", "--src", type=str, required=True, help="Directory containing *.vox")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Directory to output")
    args = parser.parse_args()
    main(**vars(args))
