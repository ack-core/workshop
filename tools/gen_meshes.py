from __future__ import annotations

"""
Tool to generate optimized meshes from *.vox 
Format:
4 bytes  - 'VOX '
1 uint32 - 127 (0x7f)
1 uint32 - flags (0x0)
3 uint32 - size (x, y, z)
1 uint32 - N
N uint8  - description as string + zero at the end
1 uint32 - frame count
1 uint32 - voxel count in frame
voxels
"""

import os
import argparse
import struct
import re
from pathlib import Path

VOX_READ_MAIN = 20
VOX_READ_CHUNK_HEADER = 4

class Voxel:
    def __init__(self):
        self.exist = False
        self.color = 0
        self.mask = 0

def optimize(data: [(int, int, int, int)], sx: int, sy: int, sz: int, opt: int) -> (int, bytes):
    output = bytearray()
    matrix = [[[Voxel() for i in range(0, sz + 2)] for i in range(0, sy + 2)] for i in range(0, sx + 2)]
    for v in data:
        e = matrix[v[0] + 1][v[1] + 1][v[2] + 1]
        e.exist = True
        e.color = v[3]
        e.mask = 0b111111

    if opt >= 1:
        for x in range(1, sx + 1):
            for y in range(1, sy + 1):
                for z in range(1, sz + 1):
                    e = matrix[x][y][z]
                    if e.exist:
                        if matrix[x][y][z - 1].exist:
                            e.mask &= 0b111110
                        if matrix[x - 1][y][z].exist:
                            e.mask &= 0b111101
                        if matrix[x][y - 1][z].exist:
                            e.mask &= 0b111011
                        if matrix[x][y][z + 1].exist:
                            e.mask &= 0b110111
                        if matrix[x + 1][y][z].exist:
                            e.mask &= 0b101111
                        if matrix[x][y + 1][z].exist:
                            e.mask &= 0b011111

        for x in range(1, sx + 1):
            for y in range(1, sy + 1):
                for z in range(1, sz + 1):
                    e = matrix[x][y][z]
                    e.exist = e.mask != 0

    if opt >= 2:
        pass

    count = 0
    for x in range(1, sx + 1):
        for y in range(1, sy + 1):
            for z in range(1, sz + 1):
                e = matrix[x][y][z]
                if e.exist:
                    voxel = struct.pack("<hhhBBBBBB", x - 1, y - 1, z - 1, e.color, e.mask, 0, 0, 0, 0)
                    output += voxel
                    count = count + 1

    return count, output

def convert_vox(src: str, cfg: str, dst: str, opt: int):
    print("---- ", src)
    cfgstring = ""
    try:
        with open(cfg, "r", encoding="utf-8") as cfg_file:
            cfgstring = cfg_file.read()
    except OSError as e:
        pass
    
    os.makedirs(os.path.dirname(dst), exist_ok=True)

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

            dst_file.write(b'VOX \x7f\0\0\0\0\0\0\0')
            dst_file.write(struct.pack("<iii", sx, sy, sz))

            # description
            dst_file.write(struct.pack("<i", len(cfgstring) + 1))
            dst_file.write(cfgstring.encode('utf-8') + b'\x00')

            dst_file.write(struct.pack("<i", frame_count))

            for i in range(0, frame_count):
                voxels = [tuple(frame_data[i][c * 4:c * 4 + 4]) for c in range(0, frame_size[i])]
                voxels = [(e[1], e[2], e[0], 7 - (256 - e[3]) % 8 + ((256 - e[3]) // 8) * 8) for e in voxels]
                count, data = optimize(voxels, frame_bounds[i][0], frame_bounds[i][1], frame_bounds[i][2], opt)
                dst_file.write(struct.pack("<i", count))
                dst_file.write(data)

def main(src: str, dst: str, opt: int):
    src = os.path.abspath(src)
    dst = os.path.abspath(dst)

    for path, _, files in os.walk(src):
        for file in files:
            if file.endswith(".vox"):
                relpath_to = os.path.join(os.path.relpath(path, src), file)
                fullpath_to = os.path.normpath(os.path.join(dst, relpath_to))
                fullpath_to = fullpath_to.replace(".vox", ".vxm")
                fullpath_from = os.path.join(path, file)
                fullpath_cfg = fullpath_from.replace(".vox", ".txt")
                convert_vox(fullpath_from, fullpath_cfg, fullpath_to, opt)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to generate optimized meshes")
    parser.add_argument("-s", "--src", type=str, required=True, help="Directory containing *.vox")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Directory to output")
    parser.add_argument("-o", "--opt", type=int, required=True, help="Optimization level 0-2")
    args = parser.parse_args()
    main(**vars(args))
