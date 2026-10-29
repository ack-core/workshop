# Guidance to agents when working in this repo

## Project
Code-first C++ 3D engine for small lowpoly/voxel games

## Structure folders
- cmake : toolchains for CMake
- foundation : low level engine interfaces and implementations
- providers : texture, model, font provides
- core : mid level 3d engine interfaces and implementations
- ui : 2d user interface components
- package : platform specific prepared files
- testing : tests suite

## Shell scripts
- make_<platform>_<app> : generate IDE projects 
- build_<platform>_<app> : make and build executables
- build_and_run_<platform>_tests : build and run tests

generated folders:
binary_<platform>_<app> : built platform executables
workshop_<platform>_<app> : generated IDE projects

## Human interaction & communication
- **Be very concise:** No conversational fillers, greetings, apologies, preambles and closing offers
- **Code first:** Start responces directly with required code or structural solutions
- **Do only what was asked:** No extra experiments or verifications beyond the request
- **Do not write python scripts for code editing automation**
- **Be aware of uncommitted user's edits**

## Code values
- **Less code - is better**