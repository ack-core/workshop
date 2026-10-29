import json
import os
import re
import subprocess
import time

LOOP_WAIT_MAX_SEC = 60
OPEN_PR_MAX = 1
REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
REPORT_PATH = os.path.join(REPO_ROOT, ".claude", "tmp", "tests-coverage-report.txt")
LOG_PATH = os.path.join(REPO_ROOT, "dev-bot.log")


def run(*cmd):
    result = subprocess.run(cmd, cwd=REPO_ROOT, capture_output=True, text=True)
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


def open_prs_count():
    return len(json.loads(run("gh", "pr", "list", "--state", "open", "--json", "number", "--limit", "1000")))


def log(msg):
    print(msg)
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(f"{msg}\n")


def main():
    while True:
        if is_repo_dirty():
            log("Repo is dirty")
        else:
            if not has_candidates():
                log("Making test coverage report...")
                log(run("claude", "-p", "/tests-coverage", "--allowedTools", "Read", "Glob", "Grep", "Edit(.claude/tmp/tests-coverage-report.txt)"))
            prs_count = open_prs_count()
            log(f"open PRs: {prs_count}")
            if prs_count < OPEN_PR_MAX:
                log("Writing a test...")
                log(run("claude", "-p", "/test-writer", "--allowedTools", "Read", "Glob", "Grep", "Edit", "Bash", "PowerShell"))
        time.sleep(LOOP_WAIT_MAX_SEC)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
