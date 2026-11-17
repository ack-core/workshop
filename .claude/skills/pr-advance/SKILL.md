---
name: pr-advance
description: Skill to advance an unmergeable PR
disable-model-invocation: true
---

Input: $ARGUMENTS - THE_PR, pull request (PR) on github
We are investigating/fixing what is needed for PR to be mergeable.
Do these steps in exact order:

1. Fetch and switch to the branch from THE_PR, overwriting the local branch
2. If the PR's branch is in conflict state:
   1. Rebase it onto origin/master after `git fetch`
   2. Resolve conflicts during rebase, go to step 5
3. If the PR has failed checks caused by the changes in the branch (`gh run view <run-id> --log-failed`):
   1. Fix the code that broke them, go to step 5
4. If the PR has failed checks which are unrelated to the branch changes:
   1. Re-run them with `gh run rerun <run-id> --failed`
   2. Wait until the re-run jobs are `queued` or `in_progress` (`gh run view <run-id> --json status`), go to step 8
5. Build and run ALL tests (`build_and_run_*_tests.*`) that could be performed on this platform. Check for exit code and `[FAILED * TEST]` in output.
6. Fix the code if tests actually found a problem, fix tests if they are wrong
7. Commit the fixes, go to step 5 if there were fixes
8. Push the branch (with `--force-with-lease` after rebase)
9. Using gh cli add a very short comment in the PR about what you have done. If nothing was pushed or re-run, comment why the PR is still unmergeable.
