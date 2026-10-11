#!/usr/bin/env bash
# ============================================================
# Java -> C++ migration loop using OpenCode — one task per iteration.
#
# The work queue is SOURCE_OF_TRUTH/TASKS.md. Each iteration:
#   1. picks the first [ ]/[~] task whose deps are all [x]
#   2. runs opencode on exactly that task
#   3. builds + runs ctest itself (the gate)
#   4. green -> commits "migration(Txxx): ..."; red -> retries with the failure
#      output, then rolls back and marks the task [!] blocked
#
# Usage: ./loop_migration.sh [--max N] [--task T123] [--model provider/model]
#                            [--fallback-model provider/model] [--auto]
#                            [--attempts N] [--timeout MIN] [--dry-run] [--allow-dirty]
#   --auto            pass --auto to opencode (auto-approve tool permissions)
#   --fallback-model  switch to this model when the primary keeps hitting provider errors
#                     e.g. --model freellmapi/auto --fallback-model cheaperinference/glm-5.2
# Stop gracefully: touch STOP   (checked between iterations)
# Extra opencode flags: OPENCODE_EXTRA_ARGS="--variant high" ./loop_migration.sh
# ============================================================

set -u
set -o pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR" || exit 1

TASKS_FILE="SOURCE_OF_TRUTH/TASKS.md"
LOG_DIR="$PROJECT_DIR/.migration-logs"
BUILD_DIR="build"
JOBS="$(nproc 2>/dev/null || echo 4)"

MAX_ITERATIONS=50
MAX_ATTEMPTS=3            # gate failures before a task is marked blocked
MAX_PROVIDER_RETRIES=20   # rate-limit / provider-error retries per attempt
MAX_CONSECUTIVE_BLOCKED=3
TIMEOUT_MIN=90
FORCED_TASK=""
# Strongest model available in this opencode install (see `opencode models`); override with --model.
MODEL="${LOOP_MODEL:-cheaperinference/glm-5.2}"
FALLBACK_MODEL="${LOOP_FALLBACK_MODEL:-freellmapi/auto:smart}"
FALLBACK_AFTER=3          # provider errors on the primary model before switching to the fallback
FREELLMAPI_URL="${FREELLMAPI_URL:-http://127.0.0.1:3001}"
DRY_RUN=0
ALLOW_DIRTY=0
# Maximum reasoning effort unless the caller chose a variant.
[[ "${OPENCODE_EXTRA_ARGS:-}" == *--variant* ]] || OPENCODE_EXTRA_ARGS="${OPENCODE_EXTRA_ARGS:-} --variant high"

COMMIT_PATHS=(src tests tools docs SOURCE_OF_TRUTH CMakeLists.txt JavaToCppClassMap.csv AGENTS.md CLAUDE.md)
PROVIDER_ERROR_RE='Error from provider|Rate limit exceeded|rate_limit_error|overloaded_error|Too Many Requests|insufficient_quota|all_providers_exhausted|No available model|no_available_models|Service Unavailable'

while [[ $# -gt 0 ]]; do
    case "$1" in
        --max)         MAX_ITERATIONS="$2"; shift 2 ;;
        --task)        FORCED_TASK="$2"; shift 2 ;;
        --model)       MODEL="$2"; shift 2 ;;
        --attempts)    MAX_ATTEMPTS="$2"; shift 2 ;;
        --timeout)     TIMEOUT_MIN="$2"; shift 2 ;;
        --dry-run)     DRY_RUN=1; shift ;;
        --allow-dirty) ALLOW_DIRTY=1; shift ;;
        --auto)        OPENCODE_EXTRA_ARGS="${OPENCODE_EXTRA_ARGS:-} --auto"; shift ;;
        --fallback-model) FALLBACK_MODEL="$2"; shift 2 ;;
        [A-Z_]*=*)     export "${1?}"; shift ;;   # allow VAR=value after the command too
        -h|--help)     awk 'NR>1 && /^set -u/{exit} NR>1' "$0"; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; exit 64 ;;
    esac
done

log()  { echo "[$(date '+%H:%M:%S')] $*"; }
die()  { echo "ERROR: $*" >&2; exit 1; }

# ------------------------------------------------------------
# TASKS.md parsing
# ------------------------------------------------------------

# Print "<status> <id> <deps>" for every task line, e.g. "x T001 -" / "  T102 T101".
task_table() {
    awk '
        match($0, /^- \[(.)\] (T[0-9]+[a-z]?) \| deps: ([^|]*)\|/) {
            line = substr($0, RSTART, RLENGTH)
            st = substr(line, 4, 1); if (st == " ") st = "_"
            sub(/^- \[.\] /, "", line)
            split(line, parts, " \\| deps: ")
            id = parts[1]
            deps = parts[2]; sub(/[ |]+$/, "", deps); gsub(/ /, "", deps)
            if (deps == "") deps = "-"
            print st, id, deps
        }' "$TASKS_FILE"
}

# First todo/partial task whose deps are all done.
pick_task() {
    task_table | awk '
        { st[$2] = $1; deps[$2] = $3; order[++n] = $2 }
        # A dep that was split (T301 -> T301a, T301b) is done when all its parts are done.
        function dep_done(dep,   m, found) {
            if (dep in st) return st[dep] == "x"
            found = 0
            for (m = 1; m <= n; m++)
                if (order[m] ~ ("^" dep "[a-z]$")) { found = 1; if (st[order[m]] != "x") return 0 }
            return found
        }
        END {
            for (i = 1; i <= n; i++) {
                id = order[i]
                if (st[id] != "_" && st[id] != "~") continue
                ok = 1
                if (deps[id] != "-") {
                    k = split(deps[id], d, ",")
                    for (j = 1; j <= k; j++) if (!dep_done(d[j])) { ok = 0; break }
                }
                if (ok) { print id; exit }
            }
        }'
}

task_status() { task_table | awk -v id="$1" '$2 == id { print $1; exit }'; }

count_status() { task_table | awk -v s="$1" '$1 == s' | wc -l; }

# The task line plus its indented detail lines.
task_block() {
    awk -v id="$1" '
        $0 ~ "^- \\[.\\] " id " \\|" { on = 1; print; next }
        on && /^  / { print; next }
        on { exit }' "$TASKS_FILE"
}

task_title() { task_block "$1" | head -1 | sed -E 's/^- \[.\] [^|]*\|[^|]*\| *//'; }

# Done if the task itself is [x], or it was split and its first part (Txxxa) is [x].
task_outcome() {
    local id="$1" st
    st="$(task_status "$id")"
    if [[ -z "$st" ]]; then
        st="$(task_status "${id}a")"
        [[ "$st" == "x" ]] && { echo "split-done"; return; }
        [[ -n "$st" ]] && { echo "split-partial"; return; }
        echo "missing"; return
    fi
    case "$st" in
        x) echo "done" ;;
        "~") echo "partial" ;;
        "!") echo "blocked" ;;
        *) echo "todo" ;;
    esac
}

mark_blocked() {
    local id="$1" reason="$2"
    # Set the marker and insert a "- blocked:" line right after the task line.
    awk -v id="$id" -v reason="$reason" '
        $0 ~ "^- \\[.\\] " id " \\|" { sub(/^- \[.\]/, "- [!]"); print; print "  - blocked: " reason; next }
        { print }' "$TASKS_FILE" > "$TASKS_FILE.tmp" && mv "$TASKS_FILE.tmp" "$TASKS_FILE"
}

# ------------------------------------------------------------
# Build gate
# ------------------------------------------------------------

ensure_configured() {
    [[ -f "$BUILD_DIR/Makefile" || -f "$BUILD_DIR/build.ninja" ]] && return 0
    log "Build directory not configured — running conan + cmake configure"
    conan install . --build=missing -of "$BUILD_DIR" &&
    cmake -B "$BUILD_DIR" -S . \
        -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/build/Release/generators/conan_toolchain.cmake" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
        -DCMAKE_PREFIX_PATH="$BUILD_DIR/build/Release/generators"
}

# Runs build + ctest, appends output to $1, writes a failure excerpt to $2. Returns 0 on green.
run_gate() {
    local logf="$1" failf="$2" rc
    : > "$failf"
    {
        echo; echo "================ GATE: build ================"
    } >> "$logf"
    # Re-run cmake configure so new CMakeLists entries are picked up.
    cmake "$BUILD_DIR" >> "$logf" 2>&1
    if ! cmake --build "$BUILD_DIR" -j"$JOBS" > "$failf.build" 2>&1; then
        cat "$failf.build" >> "$logf"
        { echo "BUILD FAILED:"; grep -E 'error|Error' "$failf.build" | head -60; } > "$failf"
        rm -f "$failf.build"
        return 1
    fi
    tail -3 "$failf.build" >> "$logf"; rm -f "$failf.build"

    echo "================ GATE: ctest ================" >> "$logf"
    ctest --test-dir "$BUILD_DIR" -j"$JOBS" --timeout 600 --output-on-failure > "$failf.test" 2>&1
    rc=$?
    tail -40 "$failf.test" >> "$logf"
    if [[ $rc -ne 0 ]]; then
        { echo "TESTS FAILED:"; tail -80 "$failf.test"; } > "$failf"
        rm -f "$failf.test"
        return 1
    fi
    log "Gate: $(grep -E 'tests passed' "$failf.test")"
    rm -f "$failf.test"
    return 0
}

tree_changed() { [[ -n "$(git status --porcelain -- "${COMMIT_PATHS[@]}" 2>/dev/null)" ]]; }

commit_task() {
    local id="$1" msg="$2"
    git add -A -- "${COMMIT_PATHS[@]}" 2>/dev/null
    if git diff --cached --quiet; then
        log "Nothing to commit for $id"
        return 0
    fi
    git commit -q -m "migration($id): $msg

Automated by loop_migration.sh (opencode)." || die "git commit failed"
    log "Committed: $(git log --oneline -1)"
    local stray
    stray="$(git status --porcelain | grep -v '^??' | head -5)"
    [[ -n "$stray" ]] && log "WARNING: changes outside commit paths left uncommitted:"$'\n'"$stray"
}

rollback() {
    log "Rolling back working tree changes"
    git reset -q --hard HEAD
    git clean -fdq -- src tests tools docs
}

# ------------------------------------------------------------
# Prompt
# ------------------------------------------------------------

build_prompt() {
    local id="$1" attempt="$2" failf="$3"
    cat <<EOF
You are continuing the Freerouting Java -> C++20 migration (repo: $PROJECT_DIR).
Java in java/app/freerouting/** is the source of truth. Goal: headless DSN -> route -> SES parity.

YOUR TASK FOR THIS SESSION IS EXACTLY ONE TASK: $id. Do not work on any other task.

Read first (and nothing else from SOURCE_OF_TRUTH unless the task says so):
- SOURCE_OF_TRUTH/01_MEMORY.md     (architecture, gotchas)
- SOURCE_OF_TRUTH/CONVENTIONS.md   (C++20 style, naming, tests)
- the "Rules for every task" section at the top of $TASKS_FILE
- the Java files named in the task and the C++ files you touch

The task (verbatim from $TASKS_FILE):
-----
$(task_block "$id")
-----

Workflow:
1. Read the Java code, then the existing C++ code. Port faithfully; keep Java quirks.
2. New classes: .h + .cpp + _test.cpp; add sources to src/<module>/CMakeLists.txt and the test to tests/CMakeLists.txt.
3. Build:  cmake --build build -j$JOBS
4. Test:   ctest --test-dir build -j$JOBS --output-on-failure
5. Fix until green. Never delete, skip or weaken existing tests.
6. Update $TASKS_FILE for $id: [x] + "- result:" line when done; [~] + "- progress:" line if partially
   done; split into ${id}a/${id}b if too big; [!] + "- blocked:" line if impossible. Add a short note to
   docs/migration/MIGRATION_<module>.md.
7. Do NOT run git commit / git reset / git checkout. The loop script commits after verifying the build.
EOF
    if [[ "$attempt" -gt 1 && -s "$failf" ]]; then
        cat <<EOF

IMPORTANT — RETRY $attempt/$MAX_ATTEMPTS: the previous session left the working tree in a state that
FAILS the gate. Your work so far is still in the working tree. Fix these failures first:
-----
$(cat "$failf")
-----
EOF
    fi
}

# ------------------------------------------------------------
# Model backend health (FreeLLMAPI local router)
# ------------------------------------------------------------

# For freellmapi/* models: make sure the local router answers; otherwise use the
# fallback model (if any) or wait for it to come back.
check_model_backend() {
    [[ "$MODEL" == freellmapi/* ]] || return 0
    local waited=0
    until curl -sf -m 5 -o /dev/null "$FREELLMAPI_URL/v1/models" \
            -H "Authorization: Bearer ${FREELLMAPI_API_KEY:-}"; do
        if [[ -n "$FALLBACK_MODEL" && "$FALLBACK_MODEL" != freellmapi/* ]]; then
            log "FreeLLMAPI not reachable at $FREELLMAPI_URL — using fallback $FALLBACK_MODEL"
            MODEL="$FALLBACK_MODEL"; return 0
        fi
        [[ $waited -ge 600 ]] && die "FreeLLMAPI not reachable at $FREELLMAPI_URL (start it: ~/freellmapi, npm start -w server)"
        log "FreeLLMAPI not reachable at $FREELLMAPI_URL — retrying in 30s"
        sleep 30; waited=$((waited + 30))
    done
}

# ------------------------------------------------------------
# One opencode session, with provider-error backoff
# ------------------------------------------------------------

run_opencode() {
    local id="$1" prompt="$2" logf="$3"
    local retry=0 wait=60 rc
    local -a args

    while :; do
        args=(run --title "migration $id")
        [[ -n "$MODEL" ]] && args+=(--model "$MODEL")
        # shellcheck disable=SC2206
        [[ -n "${OPENCODE_EXTRA_ARGS:-}" ]] && args+=(${OPENCODE_EXTRA_ARGS})
        check_model_backend
        local start_lines=0
        [[ -f "$logf" ]] && start_lines=$(wc -l < "$logf")
        timeout -k 60 "$((TIMEOUT_MIN * 60))" opencode "${args[@]}" "$prompt" 2>&1 | tee -a "$logf"
        rc=${PIPESTATUS[0]}
        [[ $rc -eq 124 ]] && log "opencode timed out after ${TIMEOUT_MIN} min (work so far is kept)"

        if tail -n +"$((start_lines + 1))" "$logf" | tail -30 | grep -qE "$PROVIDER_ERROR_RE"; then
            retry=$((retry + 1))
            [[ $retry -gt $MAX_PROVIDER_RETRIES ]] && die "Provider errors persist after $MAX_PROVIDER_RETRIES retries — see $logf"
            if [[ -n "$FALLBACK_MODEL" && "$MODEL" != "$FALLBACK_MODEL" && $retry -ge $FALLBACK_AFTER ]]; then
                log "Primary model $MODEL keeps failing — switching to fallback $FALLBACK_MODEL for the rest of this run"
                MODEL="$FALLBACK_MODEL"; wait=60
                continue
            fi
            log "Provider error / rate limit detected — waiting ${wait}s (retry $retry/$MAX_PROVIDER_RETRIES)"
            sleep "$wait"
            wait=$(( wait * 2 > 1800 ? 1800 : wait * 2 ))
            [[ -f STOP ]] && { log "STOP file found"; exit 0; }
            continue
        fi
        return "$rc"
    done
}

# ------------------------------------------------------------
# Main loop
# ------------------------------------------------------------

[[ -f "$TASKS_FILE" ]] || die "$TASKS_FILE not found"
mkdir -p "$LOG_DIR"

if [[ $DRY_RUN -eq 1 ]]; then
    id="${FORCED_TASK:-$(pick_task)}"
    [[ -z "$id" ]] && { echo "No eligible task."; exit 0; }
    echo "Picked task: $id — $(task_title "$id")"
    echo "Tasks: done=$(count_status x) partial=$(count_status '~') todo=$(count_status _) blocked=$(count_status '!')"
    echo "================ PROMPT ================"
    build_prompt "$id" 1 /dev/null
    exit 0
fi

command -v opencode >/dev/null || die "opencode not found in PATH"
if [[ $ALLOW_DIRTY -eq 0 && -n "$(git status --porcelain)" ]]; then
    git status --short | head -20
    die "Working tree is dirty. Commit/stash first, or pass --allow-dirty."
fi
ensure_configured || die "Build configuration failed"

consecutive_blocked=0

for ((ITERATION = 1; ITERATION <= MAX_ITERATIONS; ITERATION++)); do
    [[ -f STOP ]] && { log "STOP file found — exiting (rm STOP to resume)"; exit 0; }

    if [[ -n "$FORCED_TASK" ]]; then
        TASK="$FORCED_TASK"; FORCED_TASK=""
        [[ -n "$(task_status "$TASK")" ]] || die "Task $TASK not found in $TASKS_FILE"
    else
        TASK="$(pick_task)"
    fi
    if [[ -z "$TASK" ]]; then
        blocked=$(count_status '!'); todo=$(( $(count_status _) + $(count_status '~') ))
        if [[ $todo -eq 0 && $blocked -eq 0 ]]; then
            log "All tasks in $TASKS_FILE are done."; exit 0
        fi
        log "No eligible task: $todo todo/partial waiting on deps, $blocked blocked. Resolve the blocked tasks."
        exit 2
    fi

    TITLE="$(task_title "$TASK")"
    STAMP="$(date '+%Y%m%d-%H%M%S')"
    FAIL_FILE="$LOG_DIR/$STAMP-$TASK.fail"
    echo
    echo "============================================================"
    echo " Iteration $ITERATION/$MAX_ITERATIONS — $TASK: $TITLE"
    echo " done=$(count_status x) partial=$(count_status '~') todo=$(count_status _) blocked=$(count_status '!')"
    echo "============================================================"

    green=0
    for ((ATTEMPT = 1; ATTEMPT <= MAX_ATTEMPTS; ATTEMPT++)); do
        LOG_FILE="$LOG_DIR/$STAMP-$TASK-try$ATTEMPT.log"
        log "Attempt $ATTEMPT/$MAX_ATTEMPTS — log: $LOG_FILE"
        run_opencode "$TASK" "$(build_prompt "$TASK" "$ATTEMPT" "$FAIL_FILE")" "$LOG_FILE"

        if ! tree_changed; then
            log "Session produced no changes."
            echo "The previous session made no changes to the repository. Make progress on the task." > "$FAIL_FILE"
            continue
        fi
        if run_gate "$LOG_FILE" "$FAIL_FILE"; then green=1; break; fi
        log "Gate FAILED:"; head -15 "$FAIL_FILE"
    done

    if [[ $green -eq 1 ]]; then
        outcome="$(task_outcome "$TASK")"
        case "$outcome" in
            done|split-done) commit_task "$TASK" "$TITLE"; consecutive_blocked=0 ;;
            partial|split-partial|todo) commit_task "$TASK" "partial — $TITLE"; consecutive_blocked=0 ;;
            blocked) commit_task "$TASK" "blocked — $TITLE"; consecutive_blocked=$((consecutive_blocked + 1)) ;;
            missing) log "WARNING: $TASK vanished from $TASKS_FILE"; commit_task "$TASK" "$TITLE (task line removed?)" ;;
        esac
        log "$TASK outcome: $outcome"
    else
        rollback
        mark_blocked "$TASK" "gate failed after $MAX_ATTEMPTS attempts on $STAMP (logs: .migration-logs/$STAMP-$TASK-try*.log)"
        commit_task "$TASK" "blocked — gate failed after $MAX_ATTEMPTS attempts"
        consecutive_blocked=$((consecutive_blocked + 1))
    fi
    rm -f "$FAIL_FILE"

    if [[ $consecutive_blocked -ge $MAX_CONSECUTIVE_BLOCKED ]]; then
        log "$consecutive_blocked tasks blocked in a row — stopping for human review."
        exit 3
    fi
    sleep 2
done

log "Reached MAX_ITERATIONS=$MAX_ITERATIONS. done=$(count_status x) todo=$(count_status _) blocked=$(count_status '!')"
exit 1
