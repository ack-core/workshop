---
name: pr-requested-fix
description: Skill to fix requested changes in PR
disable-model-invocation: true
---

Input: THE_PR - pull request (PR) on github
We are fixing the code by request from PR reviewer on github.
If at any step it is not possible to continue as described, stop immediately, discard uncommitted changes and report what happened.
Do these steps in exact order:

1. Fetch and switch to the branch from THE_PR, overwriting the local branch
2. In THE_PR collect all comments from the repo owner: review bodies, inline comments and conversation comments. Understand what is requested.
3. Check the branch history to understand what has been fixed already
4. Implement the needed fixes
   - Questions for self-check during implementation:
     - is there a simpler implementation? Simplify
5. Commit the fixes
6. Build and run ALL tests (`build_and_run_*_tests.*`) that could be performed on this platform.
7. Fix the code if tests actually found a problem, fix tests if they are wrong
8. Commit the fixes, go to step 6 if there were fixes
9. Push new commits, using gh cli re-request review from the repo owner
