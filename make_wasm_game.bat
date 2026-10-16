@echo off
set "EMSDK=../emsdk"
cmake -S . -B ./workshop_wasm_game -G"Ninja" -DCMAKE_TOOLCHAIN_FILE="cmake/toolchain_wasm.cmake" -DPLATFORM="PLATFORM_WASM" -DAPPTYPE="IS_GAME"
cmake --build workshop_wasm_game