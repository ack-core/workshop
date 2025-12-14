from __future__ import annotations

"""
Tool to generate binary emitter descriptions from *.txt 
Format:

4 bytes  - 'EMTR'
1 uint32 - N + 4
N uint8  - description as string + zero at the end
1 uint32 - map width, map data is optional (width and height can be 0)
1 uint32 - map height
N uint8  - rgba map data, where N = width * height * 4

"""

import os
import argparse
import struct
import re
from pathlib import Path
from PIL import Image

def convert_emitter(src: str, map: str, dst: str):
    print("---- ", src)
    map_image = None
    map_width = 0
    map_height = 0

    try:
        map_image = Image.open(map)
        map_image = map_image.convert("RGBA")
        map_width, map_height = map_image.size
    except OSError as e:
        pass
    
    os.makedirs(os.path.dirname(dst), exist_ok=True)

    with open(src, mode="r") as src_file:
        with open(dst, mode="wb") as dst_file:
            src_string = src_file.read()

            try:
                dst_file.write(b'EMTR')
                dst_file.write(struct.pack("<I", len(src_string) + 1 + 4))
                dst_file.write(src_string.encode('utf-8') + b'\x00')

                # next comes map data
                dst_file.write(struct.pack("<I", map_width))
                dst_file.write(struct.pack("<I", map_height))
                if map_image is not None:
                    dst_file.write(map_image.tobytes())

            except (KeyError, ValueError) as e:
                print("------ Error: ", e)



def main(src: str, dst: str):
    src = os.path.abspath(src)
    dst = os.path.abspath(dst)

    for path, _, files in os.walk(src):
        for file in files:
            if file.endswith(".txt"):
                relpath_to = os.path.join(os.path.relpath(path, src), file)
                fullpath_to = os.path.normpath(os.path.join(dst, relpath_to))
                fullpath_to = fullpath_to.replace(".txt", ".bin")
                fullpath_from = os.path.join(path, file)
                fullpath_map = fullpath_from.replace(".txt", ".tga");
                convert_emitter(fullpath_from, fullpath_map, fullpath_to)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to generate binary emitters")
    parser.add_argument("-s", "--src", type=str, required=True, help="Directory containing *.txt (and *.tga)")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Directory to output")
    args = parser.parse_args()
    main(**vars(args))
