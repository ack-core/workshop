# Developer bot: an autonomous loop that runs Claude Code skills to develop the engine.

import json
import os
import re
import subprocess
import sys
import time

LOOP_WAIT_MAX_SEC = 1200
OPEN_PR_MAX = 1
REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
REPORT_PATH = os.path.join(REPO_ROOT, "tmp", "tests-coverage-report.txt")
LOG_PATH = os.path.join(REPO_ROOT, "dev-bot.log")


def run(*cmd):
    result = subprocess.run(cmd, cwd=REPO_ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if result.returncode != 0:
        log(result.stdout + result.stderr)
        result.check_returncode()
    return result.stdout


def is_repo_dirty():
    return bool(run("git", "status", "--porcelain").strip())


def has_candidates():
    if not os.path.isfile(REPORT_PATH):
        return False
    with open(REPORT_PATH, encoding="utf-8") as f:
        return re.search(r"Candidates:\s*-+\s*[^-\s]", f.read()) is not None


def open_prs():
    return json.loads(run("gh", "pr", "list", "--state", "open", "--json", "title,url,latestReviews,reviewRequests", "--limit", "1000"))


def repo_owner():
    return run("gh", "repo", "view", "--json", "owner", "--jq", ".owner.login").strip()


def has_requested_changes(pr, owner):
    if any(request.get("login") == owner for request in pr["reviewRequests"]):
        return False
    return any(review["state"] == "CHANGES_REQUESTED" and review["author"]["login"] == owner for review in pr["latestReviews"])


def specific_title_count(titles, marker):
    return sum(marker in title for title in titles)


def log_to_file(msg):
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(f"{msg}\n")


def log(msg):
    print(msg, flush=True)
    log_to_file(msg)


def claude(prompt, *allowed_tools):
    log("----------------------------------------------------------------")
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
    log("----------------------------------------------------------------")


def step():
    if is_repo_dirty():
        log("Repo is dirty")
        return
    prs = open_prs()
    titles = [pr["title"] for pr in prs]
    prs_count = len(titles)
    if prs_count >= OPEN_PR_MAX:
        log("There are maximum opened PRs, maybe changes requested...")
        owner = repo_owner()
        pr_url = next((pr["url"] for pr in prs if has_requested_changes(pr, owner)), None)
        if pr_url:
            log(f"Fixing requested changes in {pr_url}...")
            claude(f"/pr-requested-fix THE_PR={pr_url}", "Read", "Glob", "Grep", "Edit", "Bash", "PowerShell")
        else:
            log("No changes requested...")
        return
    
    log("There is a space for new PRs...")
    
    if not has_candidates() and not specific_title_count(titles, "[TEST]"):
        log("But test coverage report is empty, making a new one...")
        run("git", "switch", "master")
        run("git", "pull", "--ff-only")
        claude("/tests-coverage", "Read", "Glob", "Grep", "Edit(tmp/tests-coverage-report.txt)")
    
    if has_candidates():
        log("Writing a test...")
        claude("/test-writer", "Read", "Glob", "Grep", "Edit", "Bash", "PowerShell")


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
