from __future__ import annotations

"""
Tool to put all prefab descriptions into the single file prefabs.bin
Format:
8 bytes  - 'PREFABS!'
1 uint32 - Number of prefabs
N entry  - Prefab data

Prefab format:
1 uint32 - N + 4 + 1
N uint8  - path + zero at the end
1 uint32 - M + 4 + 1
M uint8  - description data + zero at the end
"""

import os
import argparse
import struct
import re
from pathlib import Path

def collect_prefabs(src: str, dst: str):
    prefabs = {}
    fullpath_to = os.path.normpath(os.path.join(dst, "prefabs.bin"))
    for path, _, files in os.walk(src):
        for file in files:
            if file.endswith(".txt"):
                fullpath_from = os.path.join(path, file)
                relpath_from = os.path.normpath(os.path.join(os.path.basename(src), os.path.join(os.path.relpath(path, src), file)))
                print("---- ", fullpath_from)
                with open(fullpath_from, mode="r", encoding="utf-8") as src_file:
                   prefabs[relpath_from.replace(".txt", "")] = src_file.read()

    with open(dst, mode="wb") as dst_file:
        dst_file.write(b'PREFABS!')
        dst_file.write(struct.pack("<I", len(prefabs)))

        for k, v in prefabs.items():
            dst_file.write(struct.pack("<I", len(k) + 5))
            dst_file.write(k.encode('utf-8') + b'\x00')
            dst_file.write(struct.pack("<I", len(v) + 5))
            dst_file.write(v.encode('utf-8') + b'\x00')


def main(src: str, dst: str):
    src = os.path.abspath(src)
    dst = os.path.abspath(dst)

    collect_prefabs(src, dst)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to put all prefabs into the single file prefabs.bin")
    parser.add_argument("-s", "--src", type=str, required=True, help="Directory containing *.txt descriptions")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Directory to output")
    args = parser.parse_args()
    main(**vars(args))
