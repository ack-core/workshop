@echo off
set "EMSDK=../emsdk"
cmake -S . -B ./workshop_wasm_editor -G"Ninja" -DCMAKE_TOOLCHAIN_FILE="cmake/toolchain_wasm.cmake" -DPLATFORM="PLATFORM_WASM" -DAPPTYPE="IS_EDITOR" || exit /b 1
cmake --build workshop_wasm_editor || exit /b 1
pushd binary_editor
node testing.js
set "RESULT=%ERRORLEVEL%"
popd
exit /b %RESULT%
