EMSDK=/Users/kas/work/2023/emsdk cmake -S . -B ./workshop_wasm -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="cmake/toolchain_wasm.cmake" -DPLATFORM="PLATFORM_WASM"
make -C workshop_wasm -s

# /Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome --ignore-certificate-errors --ignore-urlfetcher-cert-requests --disable-web-security --user-data-dir=/Users/kas/work/tmp