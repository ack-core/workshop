---
name: pr-requested-fix
description: Skill to fix requested changes in PR
disable-model-invocation: true
---

Input: Input: $ARGUMENTS - THE_PR, pull request (PR) on github
We are fixing the code by request from PR reviewer on github.
Do these steps in exact order:

1. Fetch and switch to the branch from THE_PR, overwriting the local branch
2. In THE_PR collect all comments from the repo owner: review bodies, inline comments and conversation comments. Understand what is requested.
   - Study diff and related code to understand what is implemented in PR
   - Check the branch history to understand what has been fixed already
3. Implement the needed fixes
   - Questions for self-check during implementation:
     - is there a simpler implementation? Simplify
4. Commit the fixes
5. Build and run ALL tests (`build_and_run_*_tests.*`) that could be performed on this platform. Check for exit code and `[FAILED * TEST]` in output.
6. Fix the code if tests actually found a problem, fix tests if they are wrong
7. Commit the fixes, go to step 5 if there were fixes
8. Push new commits, using gh cli re-request review from the repo owner
