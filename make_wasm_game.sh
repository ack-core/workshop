EMSDK=/Users/home/ackcore/2023/emsdk cmake -S . -B ./workshop_wasm_game -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="cmake/toolchain_wasm.cmake" -DPLATFORM="PLATFORM_WASM" -DAPPTYPE="IS_GAME"
make -C workshop_wasm_game -s

# /Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome --ignore-certificate-errors --ignore-urlfetcher-cert-requests --disable-web-security --user-data-dir=/Users/kas/work/tmp

# emsdk: 3.1.47