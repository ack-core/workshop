from __future__ import annotations

"""
Tool to generate binary emitter descriptions from *.txt 
Format:
4 bytes  - 'EMTR'
1 uint64 - flags (
    0x1 - looped, 
    0x2 - additiveBlend, 
    0x4 - startShapeFill, 
    0x8 - endShapeFill
)
1 uint32 - randomSeed
1 uint16 - particlesToEmit
1 uint16 - emissionTimeMs
1 uint16 - particleLifeTimeMs
1 uint16 - bakingFrameTimeMs
1 float  - particleStartSpeed
1 uint8  - particleOrientation
1 uint8  - startShapeType
1 uint8  - endShapeType
1 uint8  - reserved
3 float  - startShapeArgs
3 float  - endShapeArgs
3 float  - endShapeOffset
3 float  - minXYZ
3 float  - maxXYZ
2 float  - maxSize
1 uint8 - N
N uint8 - texture
1 uint8 - N
N uint8 - map
"""

import os
import argparse
import struct
import re
from pathlib import Path

def parse_config(text: str) -> dict:
    blocks = {}
    for block, body in re.findall(r'(\w+)\s*{([^}]*)}', text, re.S):
        entries = {}
        for k, t, v in re.findall(r'(\w+)\s*:\s*(\w+)\s*=\s*(.+)', body):
            try:
                v = v.strip()
                if t == 'bool':
                    v = v.lower() == 'true'
                elif t == 'integer':
                    v = int(v)
                elif t == 'number':
                    v = float(v)
                elif t.startswith('vector'):
                    v = list(map(float, v.split()))
                elif t == 'string':
                    v = v.strip('"')
                entries[k] = v
            except (ValueError) as e:
                print("------ Error: ", e)

        blocks[block] = entries
    return blocks

def convert_txt(src: str, dst: str):
    print("---- ", src)

    with open(src, mode="r") as src_file:
        with open(dst, mode="wb") as dst_file:
            src_string = src_file.read()
            src_obj = parse_config(src_string)

            try:
                flags = int(src_obj["emitter"]["looped"]) | (int(src_obj["emitter"]["additiveBlend"]) << 1)

                dst_file.write(b'EMTR')
                dst_file.write(struct.pack("<Q", flags))
                dst_file.write(struct.pack("<I", src_obj["emitter"]["randomSeed"]))
                dst_file.write(struct.pack("<H", src_obj["emitter"]["particlesToEmit"]))
                dst_file.write(struct.pack("<H", src_obj["emitter"]["emissionTimeMs"]))
                dst_file.write(struct.pack("<H", src_obj["emitter"]["particleLifeTimeMs"]))
                dst_file.write(struct.pack("<H", src_obj["emitter"]["bakingFrameTimeMs"]))
                dst_file.write(struct.pack("<f", src_obj["emitter"]["particleStartSpeed"]))
                dst_file.write(struct.pack("<B", src_obj["emitter"]["particleOrientation"]))
                dst_file.write(struct.pack("<B", src_obj["emitter"]["startShapeType"]))
                dst_file.write(struct.pack("<B", src_obj["emitter"]["endShapeType"]))
                dst_file.write(struct.pack("<B", src_obj["emitter"]["shapeDistributionType"]))
                startShapeArgs = src_obj["emitter"]["startShapeArgs"]
                dst_file.write(struct.pack("<fff", startShapeArgs[0], startShapeArgs[1], startShapeArgs[2]))
                endShapeArgs = src_obj["emitter"]["endShapeArgs"]
                dst_file.write(struct.pack("<fff", endShapeArgs[0], endShapeArgs[1], endShapeArgs[2]))
                endShapeOffset = src_obj["emitter"]["endShapeOffset"]
                dst_file.write(struct.pack("<fff", endShapeOffset[0], endShapeOffset[1], endShapeOffset[2]))
                minXYZ = src_obj["emitter"]["minXYZ"]
                dst_file.write(struct.pack("<fff", minXYZ[0], minXYZ[1], minXYZ[2]))
                maxXYZ = src_obj["emitter"]["maxXYZ"]
                dst_file.write(struct.pack("<fff", maxXYZ[0], maxXYZ[1], maxXYZ[2]))
                maxSize = src_obj["emitter"]["maxSize"]
                dst_file.write(struct.pack("<ff", maxSize[0], maxSize[1]))
                texturePath = src_obj["emitter"]["texture"]
                dst_file.write(struct.pack("<B", len(texturePath) + 2))
                dst_file.write(texturePath.encode('utf-8') + b'\x00')

            except (KeyError, ValueError) as e:
                print("------ Error: ", e)



            #print("------ Error: '{}' has no 'SIZE' block".format(src))

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
                convert_txt(fullpath_from, fullpath_to)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to generate binary emitters")
    parser.add_argument("-s", "--src", type=str, required=True, help="Directory containing *.txt")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Directory to output")
    args = parser.parse_args()
    main(**vars(args))
