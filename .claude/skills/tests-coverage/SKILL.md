---
name: tests-coverage
description: Skill to make a report about current tests coverage and candidates for new tests
disable-model-invocation: true
---

In testing folder find tests registered by calling REGISTER_UNIT_TEST/REGISTER_STEP_TEST/REGISTER_SCREENSHOT_TEST macro. 
Single interface/class capability - is ability of doing something: all ctors/dtors is a single capability. 
Ability to launch a task and return the result is other example of capability.
Ability to save different arguments in the passed lambda (callback test) - single capability.
Count capabilities from the area headers only, don't look at implementations except for shaders.
Each shader language construct handled in shader.cpp (inout blocks, uniforms, ...) is one capability.

Areas:
'math' is testing everything declared/defined in math.h
'util' is testing everything declared/defined in util.h/util.cpp
'shader' is testing compilation of different shaders using makePlatformShaderSource from shader.h/shader.cpp, read the implementation here to understand how shaders are written.
'platform' is testing everything declared/defined by platform.h
'rendering' is testing everything declared/defined by rendering.h

Study areas being tested: what approximate percentage of their capabilities is tested, what capabilities are untested.
Area % = tested capabilities ÷ all capabilities declared in the area

Write the report to `.claude/tmp/tests-coverage-report.txt` (create the folder if missing, overwrite the file if it exists).
The file must contain exactly two blocks, `Area:` and `Candidates:`, separated by one empty line, with nothing before, between or after them.
The example below is for format only: do not copy its rows, and `...` stands for more rows of the same form.

```text
Area:
rendering - 10%
util - 2%
...

Candidates:
util_strstream_methods_test - Testing non-static methods of strstream class
shader_inout_block_test - Testing shader compilation with different possible 'inout' blocks
rendering_red_triangle_on_black_test - Testing ability to draw a single triangle with method 'draw(std::uint32_t)'
...
```

Area block:
- One line per area: `<area> - <integer>%`.
- Sorted by coverage, highest first; ties in alphabetical order.

Candidates block:
- One line per test: `<test name> - <description of what is tested>`.
- Test name: lowercase snake_case `<area>_<subject>_test`.
- Test name must not match any name already passed to a REGISTER_*_TEST macro.
- Exactly 10 lines, with no gaps (fewer only if fewer untested capabilities exist).
- Rank by fundamentality: capability used by others come first. Include completely untested capability.
