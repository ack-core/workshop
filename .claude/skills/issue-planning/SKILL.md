---
name: issue-planning
description: Skill to elaborate existing github issue
disable-model-invocation: true
---

Input: $ARGUMENTS - THE_ISSUE, GitHub issue number or URL
We are transforming a github issue from a rough idea to a concrete plan. 
Do not edit any code.

1. Fetch and switch to the fresh master overwriting local changes.
2. Read all information and comments from THE_ISSUE. Pinned comment is only visible via GraphQL `issue{pinnedIssueComment{issueComment{id body author{login}}}}`; pin with the `pinIssueComment` mutation. Your login: `gh api user -q .login`.
3. If THE_ISSUE has a pinned comment from you with `Plan:` in its text, that means the work is in progress, do these steps in exact order:
   3.1. Understand what changes are requested by repo-owner or answers provided. Most recent comment from the repo-owner is most important. 
   3.2. Update the pinned comment-plan accordingly by editing the comment (study the codebase if needed). Remove answered questions from the `Questions` block; if none remain, remove the `Questions` block. 
   3.3. If you have new questions - put them as a new comment, do not add them to the pinned comment-plan 
4. Otherwise, do these steps in exact order:
   4.1. Study issue description, comments and codebase and find the simplest solution for the requested things. 
   4.2. Make a plan in simple English without any slang in this format (`Questions` is optional), post it without indentation and code fence:
      ```text
      <place here a very short description of what is needed>

      Plan:
      <place here a very short description of new things we need to introduce in the codebase and/or changes>

      Questions:
      <place here questions if there are major ambiguities>
      ```
   4.3. Place this plan as a comment in THE_ISSUE and pin it
5. Remove the 'planning' label from THE_ISSUE
