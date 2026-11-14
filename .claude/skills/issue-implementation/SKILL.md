---
name: issue-implementation
description: Skill to implement a feature requested in github issue
disable-model-invocation: true
---

Input: $ARGUMENTS - THE_ISSUE, GitHub issue number or URL
We are implementing a feature described in the github issue.
Do these steps in exact order:

1. Fetch and switch to the fresh master overwriting local changes.
2. Read all information (including comments) from the github issue to understand what is the feature about. 
3. Make a new branch with name `dev/<what_is_feature_about>` and switch to it
4. Study feature-related code and make a short step-by-step implementation plan with what and where to change.
   - Questions for self-check during planning:
     - Which interfaces need changes, could we minimize changes? Minimize
     - Can existing code be reused without breaking interfaces? Expose it in the right header if needed
5. Implement the feature according to the plan
   - Questions for self-check during implementation:
     - Does the implementation follow patterns used in the code around? Follow patterns in existing code
     - Is there a simpler implementation? Simplify
6. Commit the changes
7. Add a test if there is a testing suite for the changed interface/class/function. If there is a test but it does not cover changes - improve the test.
8. Commit changes in the tests if there are any
9. Build and run ALL tests (`build_and_run_*_tests.*`) that could be performed on this platform. Check for exit code and `[FAILED * TEST]` in output.
10. Fix the code if tests actually found a problem
11. Commit the fixes, go to step 9 if there were fixes
12. Push the branch
13. Using gh cli create a PR on github with a short description about what was implemented. Start the description with single line `Resolves #N` where N is THE_ISSUE number. 
14. Lock conversation in the PR, request review from the repo owner
