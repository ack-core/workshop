
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CROSSCOMPILING TRUE)

set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)

if ("$ENV{EMSDK}" STREQUAL "")
	message(FATAL_ERROR "env-variable EMSDK must be a path to Emscripten SDK root")
else()
	set(EMSDK_ROOT "$ENV{EMSDK}")
endif()

set(CMAKE_C_COMPILER "${EMSDK_ROOT}/upstream/bin/clang")
set(CMAKE_CXX_COMPILER "${EMSDK_ROOT}/upstream/bin/clang")
set(CMAKE_LINKER "${EMSDK_ROOT}/upstream/bin/wasm-ld")
set(CMAKE_SYSROOT "${EMSDK_ROOT}/upstream/emscripten/cache/sysroot")

set(CMAKE_CXX_FLAGS_INIT "-target wasm32-unknown-emscripten -DEMSCRIPTEN -fignore-exceptions -fvisibility=default -std=c++17 -fshort-wchar -O2 -mbulk-memory -c")
set(CMAKE_CXX_LINK_FLAGS "-L${EMSDK_ROOT}/upstream/emscripten/cache/sysroot/lib/wasm32-emscripten")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -ldlmalloc -lc++-noexcept -lc++abi-noexcept -lcompiler_rt --import-undefined --import-memory --strip-debug --no-entry")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} --no-export-dynamic  --export-if-defined=__wasm_call_ctors")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> <LINK_FLAGS> <CMAKE_CXX_LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

set(CMAKE_AR "${EMSDK_ROOT}/upstream/bin/llvm-ar")
set(CMAKE_RANLIB "${EMSDK_ROOT}/upstream/bin/llvm-ranlib")

set(CMAKE_C_OUTPUT_EXTENSION .o)
set(CMAKE_CXX_OUTPUT_EXTENSION .o)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .wasm)
set(CMAKE_SKIP_INSTALL_RULES TRUE)

