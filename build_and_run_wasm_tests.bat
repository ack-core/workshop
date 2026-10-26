@echo off
set "EMSDK=../emsdk"
cmake -S . -B ./workshop_wasm_tests -G"Ninja" -DCMAKE_TOOLCHAIN_FILE="cmake/toolchain_wasm.cmake" -DPLATFORM="PLATFORM_WASM" -DAPPTYPE="IS_TESTS"
cmake --build workshop_wasm_tests