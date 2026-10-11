# Freerouting C++ Migration — Single Source of Truth

**Project:** Freerouting Java → C++20 Migration
**Goal:** Headless routing parity (load DSN → route → write SES)
**Java source of truth:** `java/app/freerouting/**/*.java` (Freerouting 2.1.0, pruned to a headless-only subset — GUI, API, analytics and KiCad I/O removed and stubbed; still builds and routes DSN→SES)
**Last updated:** 2026-10-10

---

## Work Queue

**`TASKS.md` is the work queue.** The remaining migration is split into ~70 small tasks
(≈ one Java class or ≤ 700 Java lines each), ordered by dependencies. They are run one per iteration by
`./loop_migration.sh` (opencode), which builds, runs ctest and commits each task itself.

```bash
./loop_migration.sh --dry-run        # show the next task and its prompt
./loop_migration.sh --max 5          # run 5 tasks
./loop_migration.sh --task T117      # force a specific task
touch STOP                           # stop gracefully after the current task
```

## How to Use This Folder

| File | Contents |
|------|----------|
| `TASKS.md` | **The task queue — what to do next** |
| `ROADMAP.md` | Roadmap + execution playbook for smaller models: gates, milestones M0–M8, findings, benchmark summary, kickoff prompt |
| `plans/` | **Per-task specs** (one file per milestone; every open task has a `- spec:` pointer): Java methods + line ranges, C++ targets, reuse, steps, tests, acceptance |
| `01_MEMORY.md` | Architecture, conventions, patterns, dependency graph, gotchas |
| `CONVENTIONS.md` | C++20 patterns, naming, testing |
| `02_PROGRESS.md` | Historical, module-by-module status with per-increment logs (large) |
| `03_MISSING.md` | Historical gap analysis (large; superseded by TASKS.md) |
| `04_IMPLEMENTATION_PLAN.md` | Historical phased plan (Phases 1–7) |
| `05_WORK_PACKAGES.md` | Historical work packages WP-1 … WP-10 (Inc A–V) |
| `06_FILE_INDEX.md` | Listing of Java and C++ files |
| `SESSION_TEMPLATE.md` | Template for recording session progress |

## Quick Status (2026-10-10)

| Metric | Value |
|--------|-------|
| Build | green (conan + cmake Release) |
| Tests | ctest **887/887** (incl. `AllTests` = `build/tests/data/run_tests`) |
| Class Map | 375 rows: **ported 203, header_only 32, partial 2, pending 90, out_of_scope 48** (goal `pending = 0`) |
| Done | geometry, rules, datastructures, logger, board model, expansion rooms, maze search, connection locator/inserter, trace shove, fanout, `RoutingBoard::autoroute` (WP-1 … WP-10 Inc V) |
| Critical path | DSN → Route → SES |

### Remaining, by size (Java lines vs C++ today)

| Area | Java | C++ | Tasks |
|------|------|-----|-------|
| DSN reader (`io/specctra/parser`) | ~11.4k | ~2.2k | T101–T118 |
| Autoroute pipeline (batch loop, pass runner, optimizer, fanout, history) | ~6k | 352 | T401–T416 |
| Settings + CLI | ~3.9k | ~450 | T301–T305 |
| SES writer | 552 + write paths | 88 | T201–T204 |
| Board gaps (drill holes, Pin, facades) | — | — | T501–T505 |
| End-to-end parity per board | — | — | T601–T609 |

`JavaToCppClassMap.csv` currently over-reports "missing" classes (renamed ports); T002 fixes it.
