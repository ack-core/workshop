# Developer bot: an autonomous loop that runs Claude Code skills to develop the engine.

import json
import os
import re
import subprocess
import sys
import time

LOOP_WAIT_MAX_SEC = 60
OPEN_PR_MAX = 3
CANDIDATES_MIN = 10
USAGE_LIMIT_MAX = 0.5
REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
REPORT_PATH = os.path.join(REPO_ROOT, "tmp", "tests-coverage-report.txt")
LOG_PATH = os.path.join(REPO_ROOT, "dev-bot.log")
usage_windows = {}


def run(*cmd):
    result = subprocess.run(cmd, cwd=REPO_ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if result.returncode != 0:
        log(result.stdout + result.stderr)
        result.check_returncode()
    return result.stdout


def is_repo_dirty():
    return bool(run("git", "status", "--porcelain").strip())


def candidates():
    if not os.path.isfile(REPORT_PATH):
        return 0
    with open(REPORT_PATH, encoding="utf-8") as f:
        blocks = re.split(r"^---.*$", f.read(), flags=re.MULTILINE)
    return sum(bool(line.strip()) for line in blocks[1].splitlines()) if len(blocks) > 2 else 0


def remove_gone_branches():
    run("git", "fetch", "--prune")
    refs = run("git", "for-each-ref", "--format=%(refname:short) %(upstream:track)", "refs/heads")
    gone = [line.split()[0] for line in refs.splitlines() if line.endswith("[gone]")]
    if gone:
        log(f"Removing gone branches: {', '.join(gone)}")
        run("git", "switch", "master")
        run("git", "branch", "-D", *gone)


def open_prs():
    return json.loads(run("gh", "pr", "list", "--state", "open", "--json", "title,url,latestReviews,reviewRequests,mergeable,statusCheckRollup", "--limit", "1000"))


def assigned_issues():
    return run("gh", "issue", "list", "--assignee", "@me", "--json", "number", "--jq", ".[].number", "--limit", "1000").split()


def planning_issues():
    return run("gh", "issue", "list", "--label", "planning", "--search", "no:assignee", "--json", "number", "--jq", ".[].number", "--limit", "1000").split()


def issues_with_prs():
    return run("gh", "pr", "list", "--state", "all", "--json", "closingIssuesReferences", "--jq", ".[].closingIssuesReferences[].number", "--limit", "1000").split()


def repo_owner():
    return run("gh", "repo", "view", "--json", "owner", "--jq", ".owner.login").strip()


def has_requested_changes(pr, owner):
    if any(request.get("login") == owner for request in pr["reviewRequests"]):
        return False
    return any(review["state"] == "CHANGES_REQUESTED" and review["author"]["login"] == owner for review in pr["latestReviews"])


def is_unmergeable(pr):
    failed = ("FAILURE", "ERROR", "TIMED_OUT", "CANCELLED", "STARTUP_FAILURE", "ACTION_REQUIRED")
    return pr["mergeable"] == "CONFLICTING" or any(check.get("conclusion") in failed or check.get("state") in failed for check in pr["statusCheckRollup"])


def is_usage_over_limit():
    return any(window["utilization"] >= USAGE_LIMIT_MAX and window["resetsAt"] > time.time() for window in usage_windows.values())


def specific_title_count(titles, marker):
    return sum(marker in title for title in titles)


def log_to_file(msg):
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(f"{msg}\n")


def log(msg):
    print(msg, flush=True)
    log_to_file(msg)

def claude(prompt, *allowed_tools):
    run("git", "switch", "--discard-changes", "master")
    run("git", "clean", "-fd")
    log("--- agent run -------------------------------------------------------------")
    cmd = ("claude", "-p", prompt, "--output-format", "stream-json", "--verbose", "--thinking", "adaptive", "--thinking-display", "summarized", "--allowedTools", *allowed_tools)
    with subprocess.Popen(cmd, cwd=REPO_ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace") as proc:
        try:
            for line in proc.stdout:
                try:
                    event = json.loads(line)
                except json.JSONDecodeError:
                    event = None
                if not isinstance(event, dict):
                    log(line.rstrip())
                elif event.get("type") == "assistant":
                    for block in event.get("message", {}).get("content", []):
                        if block.get("type") == "text":
                            log(block.get("text"))
                        elif block.get("type") == "thinking":
                            log(block.get("thinking"))
                        elif block.get("type") == "tool_use":
                            log_to_file(f"> {block.get('name')} {json.dumps(block.get('input'))[:200]}")
                elif event.get("type") == "rate_limit_event":
                    usage_windows.update(event.get("rate_limit_info", {}).get("unifiedWindows", {}))
                    for name, window in usage_windows.items():
                        log(f"LIMITS {name} = {window['utilization']}")
                elif event.get("type") == "result":
                    if event.get("is_error"):
                        log(f"Error: {event.get('result') or event.get('subtype')}")
                    for denial in event.get("permission_denials") or []:
                        log(f"Denied: {denial.get('tool_name')} {json.dumps(denial.get('tool_input'))[:200]}")
        except BaseException:
            proc.kill()
            raise
    if proc.returncode != 0:
        raise subprocess.CalledProcessError(proc.returncode, cmd)
    log("--- agent end -------------------------------------------------------------")


def step():
    if is_usage_over_limit():
        log("Claude usage is over limit")
        return

    if is_repo_dirty():
        log("Repo is dirty")
        return
    
    log("Preparing...")
    run("git", "switch", "master")
    run("git", "pull", "--ff-only")
    remove_gone_branches()
    prs = open_prs()
    titles = [pr["title"] for pr in prs]

    log("Maybe there are issues to elaborate...")
    issues = planning_issues()
    for issue in issues:
        log(f"Elaborating issue #{issue}...")
        claude(f"/issue-planning THE_ISSUE={issue}", "Read", "Glob", "Grep", "Bash", "PowerShell")
    if issues:
        return

    log("Maybe there are unimplemented issues...")
    issues = assigned_issues()
    if issues:
        implemented = False
        linked = issues_with_prs()
        for issue in issues:
            if issue not in linked:
                log(f"Looking into issue #{issue} to implement...")
                claude(f"/issue-implementation THE_ISSUE={issue}", "Read", "Glob", "Grep", "Edit", "Write", "Bash", "PowerShell")
                implemented = True

        if implemented:
            return

    log("Maybe some PRs need to advance...")
    if specific_title_count(titles, "[TEST]") >= OPEN_PR_MAX:
        owner = repo_owner()
        pr_urls = [pr["url"] for pr in prs if has_requested_changes(pr, owner)]
        for pr_url in pr_urls:
            log(f"Fixing requested changes in {pr_url}...")
            claude(f"/pr-requested-fix THE_PR={pr_url}", "Read", "Glob", "Grep", "Edit", "Write", "Bash", "PowerShell")
        if not pr_urls:
            for pr in prs:
                if is_unmergeable(pr):
                    log(f"Failed checks or conflicts in {pr['url']}, advancing...")
                    claude(f"/pr-advance THE_PR={pr['url']}", "Read", "Glob", "Grep", "Edit", "Bash", "PowerShell")

    elif candidates():
        log("There is a space for new PRs, writing a test...")
        claude("/test-writer", "Read", "Glob", "Grep", "Edit", "Bash", "PowerShell")

    log("Checking if there is enough candidates for new tests...")
    if candidates() < CANDIDATES_MIN:
        log("Test coverage report is short, appending candidates...")
        claude("/test-coverage", "Read", "Glob", "Grep", "Edit(tmp/tests-coverage-report.txt)", "Write(tmp/tests-coverage-report.txt)", "Bash(gh pr list:*)")

def main():
    sys.stdout.reconfigure(errors="replace")
    while True:
        try:
            step()
        except Exception as e:
            log(f"Error: {e}")
        time.sleep(LOOP_WAIT_MAX_SEC)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
