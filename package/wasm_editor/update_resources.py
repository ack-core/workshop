import os
import sys
import shutil

binaryRoot = os.path.dirname(os.path.abspath(__file__))
resourcesRoot = os.path.normpath(binaryRoot + "/../resources")
toolsRoot = os.path.normpath(binaryRoot + "/../tools")

sys.path.insert(1, toolsRoot)

import gen_meshes
import gen_emitters
import gen_grounds
import gen_prefabs

def main():
    print("Updating engine resources...")
    gen_prefabs.main(resourcesRoot + "/prefabs", None, binaryRoot + "/data/prefabs.bin")
    gen_meshes.main(resourcesRoot + "/meshes", binaryRoot + "/data/meshes", 1)
    gen_emitters.main(resourcesRoot + "/emitters", binaryRoot + "/data/emitters")
    gen_grounds.main(resourcesRoot + "/grounds", binaryRoot + "/data/grounds", resourcesRoot + "/palette.png")
    print("Copying raycast dir")
    shutil.copytree(resourcesRoot + "/raycast", binaryRoot + "/data/raycast", dirs_exist_ok = True)
    print("Copying collision dir")
    shutil.copytree(resourcesRoot + "/collision", binaryRoot + "/data/collision", dirs_exist_ok = True)
    print("Finished")

if __name__ == "__main__":
    main()
