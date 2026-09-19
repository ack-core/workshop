from __future__ import annotations

"""
Tool to generate optimized grounds from *.png
"""

import os
import argparse
import png
import struct
import re
import random
from PIL import Image
from gen_meshes import make_vxm_data as make_vxm_data
import time
import traceback

class Palette:
    def __init__(self, img: bytes):
        self._data = {}
        for i in range(0, 256):
            self._data[int.from_bytes(img[i * 4:i * 4 + 4])] = i;
        pass

    def has_index(self, color: bytes):
        return int.from_bytes(color) in self._data;

    def get_index(self, color: bytes):
        return self._data[int.from_bytes(color)]

def get_paletted_texture(texture: str, palette: Palette):
    src_reader = png.Reader(filename=texture)
    src_w, src_h, src_data, _ = src_reader.asRGBA8()

    src_data = bytes().join([bytes(e) for e in src_data])
    color_idx_data = [0 for _ in range(src_w * src_h)]
    for i in range(0, src_w * src_h):
        bb = src_data[i * 4:i * 4 + 4]
        if palette.has_index(bb):
            color_idx_data[i] = palette.get_index(bb)
        else:
            color_idx_data[i] = 0

    return color_idx_data, src_w, src_h

def generate_place(resroot: str, src_txt: str, src_texture: str, src_vg: str, dst: str, palette: Palette):
    print("---- ", dst)

    cfgstring = ""
    vg_data = bytes()
    try:
        with open(src_txt, "r", encoding="utf-8") as cfg_file:
            cfgstring = cfg_file.read()
    except OSError as e:
        pass

    pattern = r"(color\s*:\s*integer\s*=\s*(0x[0-9a-fA-F]+))"
    matches = re.findall(pattern, cfgstring)
    vg_color_array = [int(match[1], 16) for match in matches]
    
    pattern = r'(source\s*:\s*string\s*=\s*"([^"\n]*)")'
    matches = re.findall(pattern, cfgstring)
    vg_source_array = [match[1] for match in matches]

    if len(vg_color_array) > 8:
        print("---- Error: too many vegetations")
        return

    try:
        with Image.open(src_vg) as img:
            vg_data = img.convert('RGBA').tobytes()
    except OSError as e:
        pass

    try:
        tx_data, tx_w, tx_h = get_paletted_texture(src_texture, palette)

        random.seed(100)

        mapw = tx_w + 1
        maph = tx_h + 1
        maps_data = bytearray(mapw * maph * 4)
        for x in range(0, mapw):
            for y in range(0, maph):
                mapi = mapw * y * 4 + x * 4
                vgi = tx_w * y * 4 + x * 4

                bb = vg_data[vgi:vgi + 4]
                rgb = int.from_bytes(bb[:3], byteorder='little')
                maps_data[mapi + 0] = 0x0  # heightmap
                maps_data[mapi + 1] = 0x0 # vegetation bit
                maps_data[mapi + 2] = random.getrandbits(8)

                for i in range(0, len(vg_color_array)):
                    if rgb == vg_color_array[i]:
                        maps_data[mapi + 1] = maps_data[mapi + 1] | (1 << i)                        

        with open(dst, mode="wb") as f:
            f.write(b'GROUND\0\0\0\0\0\0\0\0\0\0')

            meshw = tx_w - 1
            meshh = tx_h - 1

            # size
            f.write(struct.pack("<iii", tx_w, 1, tx_h))

            # description
            f.write(struct.pack("<i", len(cfgstring) + 1))
            f.write(cfgstring.encode('utf-8') + b'\x00')

            # vertexes and indexes
            f.write(struct.pack("<ii", 4, 6))
            v = [
                (-0.5, -0.5, 0.0, 0.0),
                (meshw + 0.5, -0.5, 1.0, 0.0),
                (-0.5, meshh + 0.5, 0.0, 1.0),
                (meshw + 0.5, meshh + 0.5, 1.0, 1.0)
            ]
            for i in range(0, 4):
                f.write(struct.pack("<ffffffff", v[i][0], -0.5, v[i][1], 0.0, 1.0, 0.0, v[i][2], v[i][3]))
            f.write(struct.pack("<IIIIII", 0, 1, 2, 2, 1, 3))

            f.write(struct.pack("<i", len(vg_source_array))) # vg entries count
            texture_headers = f.tell() 
            f.write(struct.pack("<ii", 0, 0)) # texture offset + size
            f.write(struct.pack("<ii", 0, 0)) # maps offset + size, 0 if absent
            for i in range(0, len(vg_source_array)):
                f.write(struct.pack("<ii", 0, 0)) # vg source data offset + size

            # grayscale texture with color indexes
            tx_offset = f.tell()
            tx_writer = png.Writer(width=tx_w, height=tx_h, greyscale=True, alpha=False, compression=1)
            tx_writer.write_array(f, pixels=tx_data)
            tx_size = f.tell() - tx_offset

            mp_offset = f.tell()
            mp_writer = png.Writer(width=mapw, height=maph, greyscale=False, alpha=True, compression=1)
            mp_writer.write_array(f, pixels=maps_data)
            mp_size = f.tell() - mp_offset

            vg_positions = []
            for i in range(0, len(vg_source_array)):
                vg_offset = f.tell()

                if vg_source_array[i].startswith("auxiliary/textures"):
                    spath = vg_source_array[i] + ".png"
                    data, w, h = get_paletted_texture(os.path.join(resroot, spath), palette)
                    writer = png.Writer(width=w, height=h, greyscale=True, alpha=False, compression=1)
                    writer.write_array(f, pixels=data)
                if vg_source_array[i].startswith("auxiliary/meshes"):
                    spath = vg_source_array[i] + ".vox"
                    frames = make_vxm_data(os.path.join(resroot, spath), 1)
                    if frames:
                        f.write(struct.pack("<i", len(frames)))
                        for frame in range(0, len(frames)):
                            f.write(struct.pack("<i", frames[frame][0]))
                            f.write(frames[frame][1])
                    else:
                        raise ValueError("No frames loaded for " + spath)

                vg_size = f.tell() - vg_offset
                vg_positions.append((vg_offset, vg_size));

            f.seek(texture_headers)
            f.write(struct.pack("<ii", tx_offset, tx_size))
            f.write(struct.pack("<ii", mp_offset, mp_size))
            for i in range(0, len(vg_source_array)):
                f.write(struct.pack("<ii", vg_positions[i][0], vg_positions[i][1]))

    except (Exception,) as e:
        print("---- Error: '{}'".format(e))
        traceback.print_exc()

def main(src: str, dst: str, palette: str):
    src = os.path.abspath(src)
    dst = os.path.abspath(dst)
    palette = os.path.abspath(palette)
    resroot = os.path.normpath(os.path.join(src, ".."))

    try:
        palette_reader = png.Reader(filename=palette)
        palette_w, palette_h, palette_data, _ = palette_reader.asRGBA8()

        if palette_w * palette_h == 256:
            palette_object = Palette(bytes().join([bytes(e) for e in palette_data]))

            for path, _, files in os.walk(src):
                for file in files:
                    if file.endswith(".png"):
                        relpath_to = os.path.join(os.path.relpath(path, src), file)
                        fullpath_to = os.path.normpath(os.path.join(dst, relpath_to))
                        fullpath_to = fullpath_to.replace(".png", ".grd")
                        fullpath_texture_from = os.path.join(path, file)
                        fullpath_txt_from = fullpath_texture_from.replace(".png", ".txt")
                        fullpath_vg_from = fullpath_texture_from.replace(".png", ".vg.tga")
                        st = time.perf_counter()
                        generate_place(resroot, fullpath_txt_from, fullpath_texture_from, fullpath_vg_from, fullpath_to, palette_object)
                        print("time: ", time.perf_counter() - st)

        else:
            print("------ Error: '{}' has incorrect size".format(palette))

    except (Exception,) as e:
        print("---- Error: '{}'".format(e))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to generate optimized grounds from *.png")
    parser.add_argument("-s", "--src", type=str, required=True, help="Directory containing ground sources")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Directory to output")
    parser.add_argument("-p", "--palette", type=str, required=True, help="Path to the palette")
    args = parser.parse_args()
    main(**vars(args))
