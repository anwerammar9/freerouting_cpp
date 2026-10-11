# Migration Prompts

The remaining migration is a task queue in `SOURCE_OF_TRUTH/TASKS.md`. The normal way to run it:

```bash
./loop_migration.sh --dry-run      # preview the next task + the exact prompt opencode gets
./loop_migration.sh --max 10       # run up to 10 tasks (each: opencode -> build -> ctest -> commit)
./loop_migration.sh --task T117    # force one specific task
./loop_migration.sh --model anthropic/claude-sonnet-5-5 --timeout 120
./loop_migration.sh --auto --max 20                 # --auto = opencode auto-approves tool permissions
OPENCODE_EXTRA_ARGS="--variant high" ./loop_migration.sh   # extra flags passed to `opencode run` (VAR=... goes BEFORE the command)
touch STOP                         # graceful stop between tasks (rm STOP to resume)
```

### Free models via FreeLLMAPI (local router, ~/freellmapi)

FreeLLMAPI pools the free tiers of many providers behind one OpenAI-compatible endpoint
(`http://127.0.0.1:3001/v1`). It is registered in `~/.config/opencode/opencode.json` as provider
`freellmapi` with models `auto`, `auto:smart` and `auto:fast`.

```bash
# start the router (if not running)
cd ~/freellmapi/server && FREEAPI_ENV_PATH=~/freellmapi/.env nohup node dist/index.js > ~/freellmapi/server.log 2>&1 &

# run the loop on free models, falling back to the paid model when the free pool is exhausted
export FREELLMAPI_API_KEY=freellmapi-...      # from the dashboard Keys page
./loop_migration.sh --auto --max 20 --model freellmapi/auto:smart --fallback-model cheaperinference/glm-5.2
```

The loop checks that the router answers before each session. After 3 provider errors in a row it
switches to `--fallback-model` (or, without one, backs off up to 30 min). Every task still has to pass
the build + ctest gate, so a weak free model can't commit broken work. At worst it burns attempts
and the task gets marked `[!]`.

Exit codes: `0` all done or STOP · `1` max iterations reached · `2` only blocked tasks left ·
`3` three tasks blocked in a row (needs human review). Logs: `.migration-logs/<time>-<task>-try<N>.log`.

When a task is `[!]` blocked: read its `- blocked:` line and log, fix the cause (or rewrite/split the
task), set it back to `[ ]`, commit, and restart the loop.

---

## Manual single-task prompt (any agent)

```
You are continuing the Freerouting Java -> C++20 migration. Java in java/app/freerouting/** is truth.
Read SOURCE_OF_TRUTH/01_MEMORY.md, SOURCE_OF_TRUTH/CONVENTIONS.md and the "Rules for every task"
section of SOURCE_OF_TRUTH/TASKS.md. Then do task <TXXX> from TASKS.md and nothing else.
Build: cmake --build build -j$(nproc). Test: ctest --test-dir build --output-on-failure.
When green, update the task in TASKS.md ([x] + "- result:" line) and docs/migration/MIGRATION_<module>.md.
```

## Commit prompt (manual sessions only — the loop commits by itself)

```
Build and run ctest one final time. If green, stage src/ tests/ tools/ docs/ SOURCE_OF_TRUTH/ and
commit as "migration(<TXXX>): <task title>" with a body listing ported classes and the test count.
Do not push.
```
