---
name: test-coverage
description: Skill to make a report about current tests coverage and candidates for new tests
disable-model-invocation: true
---

Single interface/class/function capability - is ability of doing something that could be logically separated from other abilities:
- All ctors/dtors is a single capability. 
- Ability to launch a task and return the result is other example of capability.
- Ability to save different arguments in the passed lambda (callback test) - single capability.
- Each shader language construct handled in shader.cpp (inout blocks, uniforms, ...) is one capability.
- Each possible way of rendering in RenderingInterface is single capability

Areas:
- 'math' is testing everything declared/defined in math.h
- 'util' is testing everything declared/defined in util.h/util.cpp
- 'shader' is testing compilation of different shaders using makePlatformShaderSource from shader.h/shader.cpp, read the implementation here to understand how shaders are written.
- 'platform' is testing everything declared/defined by platform.h
- 'rendering' is testing everything declared/defined by rendering.h and primitives rendering at different state of RenderingInterface

We are adding untested capabilities (candidates) to the file `tmp/tests-coverage-report.txt`.
The file contains exactly one `Candidates:` block with the list of untested capabilities (candidates).
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
- Test name must not match any name already passed to a `REGISTER_*_TEST` macro or any other test in the list.

In testing folder find tests registered by calling REGISTER_UNIT_TEST/REGISTER_STEP_TEST/REGISTER_SCREENSHOT_TEST macro. Study areas and capabilities being tested. 
Find up to 3 untested capabilities (candidates) which could be easily tested through existing code.
Ensure that candidates aren't in the list and there are no open pull requests which already implement them (Use `gh pr list --repo ack-core/workshop --state open --json title,body`)

Create file `tmp/tests-coverage-report.txt` if it's missing (create the folder if missing). Fix the file format if it's not as described in the example.
Append new candidates at the end of the list in the file.
