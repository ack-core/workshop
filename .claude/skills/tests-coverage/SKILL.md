---
name: tests-coverage
description: Skill to make a report about current tests coverage and candidates for new tests
disable-model-invocation: true
---

Single interface/class capability - is ability of doing something that could be logically separated from other abilities:
- All ctors/dtors is a single capability. 
- Ability to launch a task and return the result is other example of capability.
- Ability to save different arguments in the passed lambda (callback test) - single capability.
- Each shader language construct handled in shader.cpp (inout blocks, uniforms, ...) is one capability.

Areas:
- 'math' is testing everything declared/defined in math.h
- 'util' is testing everything declared/defined in util.h/util.cpp
- 'shader' is testing compilation of different shaders using makePlatformShaderSource from shader.h/shader.cpp, read the implementation here to understand how shaders are written.
- 'platform' is testing everything declared/defined by platform.h
- 'rendering' is testing everything declared/defined by rendering.h

In testing folder find tests registered by calling REGISTER_UNIT_TEST/REGISTER_STEP_TEST/REGISTER_SCREENSHOT_TEST macro. Study areas and capabilities being tested. Find what is untested.
Write the report to `tmp/tests-coverage-report.txt` (create the folder if missing, overwrite the file if it exists).
The file must contain exactly one `Candidates:` block with the list of untested capabilities (candidates).
The example below is for format only: do not copy its rows, and `...` stands for more rows of the same form.

```text
Candidates:
---------------------------------------------------------------
util_strstream_methods_test - Testing non-static methods of strstream class
shader_inout_block_test - Testing shader compilation with different possible 'inout' blocks
rendering_red_triangle_on_black_test - Testing ability to draw a single triangle with method 'draw(std::uint32_t)'
...
---------------------------------------------------------------
```

- One line per test: `<test name> - <description of what is tested>`.
- Test name: lowercase snake_case `<area>_<subject>_test`.
- Test name must not match any name already passed to a `REGISTER_*_TEST` macro.
- Exactly 20 lines, with no gaps (fewer only if fewer untested capabilities exist).
- Rank by fundamentality: capability used by others come first. Include completely untested capability.
