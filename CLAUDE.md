# Guidance to agents when working in this repo

## Project
Code-first C++ 3D engine for small lowpoly/voxel games

## Structure
cmake - toolchains for CMake
foundation - low level engine interfaces and implementations
providers - texture, model, font provides
core - mid level 3d engine interfaces and implementations
ui - 2d components
game - contexts with game/editor logic
package - platform specific prepared files
testing - test suite

generated folders:
binary_* - built platform executables
workshop_* - generated IDE projects

## Human interaction & communication

**Be very concise:** No conversational fillers, greetings, apologies, preambles and closing offers
**Code first:** Start responces directly with required code or structural solutions
**Do only what was asked:** No extra experiments or verifications beyond the request

**Do not write python scripts for code editing automation**
**Be aware of uncommitted user's edits**
