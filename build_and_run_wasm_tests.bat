@echo off
set "EMSDK=../emsdk"
cmake -S . -B ./workshop_wasm_tests -G"Ninja" -DCMAKE_TOOLCHAIN_FILE="cmake/toolchain_wasm.cmake" -DPLATFORM="PLATFORM_WASM" -DAPPTYPE="IS_TESTS" || exit /b 1
cmake --build workshop_wasm_tests || exit /b 1
pushd binary_wasm_tests
node testing.js
set "RESULT=%ERRORLEVEL%"
popd
exit /b %RESULT%
