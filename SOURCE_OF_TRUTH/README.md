# Freerouting C++ Migration — Single Source of Truth

**Project:** Freerouting Java → C++20 Migration
**Goal:** Headless routing parity (load DSN → route → write SES)
**Java source of truth:** `java/app/freerouting/**/*.java` (Freerouting 2.1.0 full upstream)
**Last updated:** 2026-09-16

---

## How to Use This Folder

| File | Contents |
|------|----------|
| `01_MEMORY.md` | Architecture, conventions, patterns, dependency graph, gotchas |
| `02_PROGRESS.md` | Module-by-module migration status with file-level detail |
| `03_MISSING.md` | Detailed gap analysis — what's missing in every module |
| `04_IMPLEMENTATION_PLAN.md` | Phased implementation plan with tasks and effort estimates |
| `05_WORK_PACKAGES.md` | Agent work packages (WP-1 through WP-15) for parallel execution |
| `06_FILE_INDEX.md` | Complete listing of all Java and C++ files |
| `SESSION_TEMPLATE.md` | Template for recording session progress |

## Quick Status

| Metric | Value |
|--------|-------|
| Total Java files | 289 |
| Fully migrated | 131 (45%) |
| Partially migrated | 22 (8%) |
| Not migrated | 136 (47%) |
| Critical path | DSN → Route → SES |
| Current headless completion | ~19% (hw48na test) |

## Top Blockers for Headless Routing

1. **MazeSearchAlgo** — grid-based A* stub, needs expansion-room architecture
2. **SortedRoomNeighbours** — 1300+ lines, room completion for maze search
3. **MazeShoveTraceAlgo** — trace shoving during maze search
4. **BatchOptimizer** — post-route optimization
5. **PullTightAlgo*** — trace tightening (combine/split/pull_tight)
6. **ForcedPadAlgo/ForcedViaAlgo** — forced pad/via placement
7. **SpecctraDsnStreamReader** — full DSN parsing
8. **SessionWriter** — full SES output (currently traces-only)
