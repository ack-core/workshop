---
name: pr-advance
description: Skill to advance unmergeable PR
disable-model-invocation: true
---

Input: THE_PR - pull request (PR) on github
We are investigating/fixing what is needed for PR to be mergeable.
Do these steps in exact order:

1. Fetch and switch to the branch from THE_PR, overwriting the local branch
2. If PR is in conflict state:
   - rebase it onto origin/master after `git fetch`
   - resolve conflicts during rebase
3. If PR has failed checks:
   - Fix the code that broke them
4. Build and run ALL tests (`build_and_run_*_tests.*`) that could be performed on this platform. Check for exit code and `[FAILED * TEST]` in output.
5. Fix the code if tests actually found a problem, fix tests if they are wrong
6. Commit the fixes, go to step 4 if there were fixes
7. Push the branch (with `--force-with-lease` after rebase)
8. Using gh cli add a very short comment in the PR about what you have done. If nothing changed, comment why the PR is still unmergeable.
