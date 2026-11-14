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

def collect_prefabs(src: str, game: str, dst: str):
    prefabs = {}
    for root in filter(None, (src, game)):
        for txt in root.rglob("*.txt"):
            print("---- ", txt.as_posix())
            prefabs[txt.relative_to(root.parent).with_suffix("").as_posix()] = txt.read_text(encoding="utf-8")

    with open(dst, mode="wb") as dst_file:
        dst_file.write(b'PREFABS!')
        dst_file.write(struct.pack("<I", len(prefabs)))

        for k, v in prefabs.items():
            dst_file.write(struct.pack("<I", len(k) + 5))
            dst_file.write(k.encode('utf-8') + b'\x00')
            dst_file.write(struct.pack("<I", len(v) + 5))
            dst_file.write(v.encode('utf-8') + b'\x00')


def main(src: str, game: str, dst: str):
    src = Path(src).resolve()
    game = Path(game).resolve() if game else None
    dst = Path(dst).resolve().as_posix()

    collect_prefabs(src, game, dst)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to put all prefabs into the single file prefabs.bin")
    parser.add_argument("-s", "--src", type=str, required=True, help="Directory containing *.txt descriptions")
    parser.add_argument("-g", "--game", type=str, required=False, help="Game directory containing *.txt descriptions")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Directory to output")
    args = parser.parse_args()
    main(**vars(args))
