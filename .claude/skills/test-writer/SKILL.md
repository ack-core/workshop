---
name: test-writer
description: Skill to write a test
disable-model-invocation: true
---

We are adding a new test to the test suite (`testing` folder).
If at any step it is not possible to continue as described, stop immediately and report why.
Do these steps in exact order:

1. In the file `.claude/tmp/tests-coverage-report.txt` get the first line in the 'Candidates' block. It has the format `<test name> - <description of what is tested>`, one line per test.
   - Test name has format `<area>_<subject>_test`, each area has its `testing/<area>_tests.cpp` file where tests are implemented.
2. Switch the repo to the fresh master. Make a branch named `test/<area>_<subject>` or similar and switch to it.
3. Study some existing tests and how they work. Implement the described test and register it with suitable `REGISTER_*(...)` macro
   - If the test is more than 60 (for unit tests) and 120 (for step/rendering tests) lines of code split it into two logically different tests with their own class/function names.
   - Questions for self-check during implementation:
     - is there a simpler implementation? Simplify
     - is it already tested somewhere or repeating itself? Test what aren’t covered yet
4. Commit the implementation
5. Build and run tests (`build_and_run_*_tests.*`) that could be performed on this platform.
6. Fix tests if the test is wrong, fix the code if the new tests actually found a problem.
7. Commit the fixes, go to step 5 if there were fixes
8. Push the branch to origin
9. Remove taken line from `.claude/tmp/tests-coverage-report.txt`. Remove that file if there are no more lines in the 'Candidates' block
10. Using gh cli create a PR to master. Make PR's title with format `[TEST] <Very short title made from subject>`. In the PR's body place very short description of what is tested.
