# Freerouting C++ Migration — Single Source of Truth

**Project:** Freerouting Java → C++20 Migration
**Goal:** Headless routing parity (load DSN → route → write SES)
**Java source of truth:** `java/app/freerouting/**/*.java` (Freerouting 2.1.0, pruned to a headless-only subset — GUI, API, analytics and KiCad I/O removed and stubbed; still builds and routes DSN→SES)
**Last updated:** 2026-09-18

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
| Fully migrated | 142 (49%) |
| Partially migrated | 21 (7%) |
| Not migrated | 126 (44%) |
| Critical path | DSN → Route → SES |
| Current headless completion | ~19% (hw48na test) |

## Top Blockers for Headless Routing

1. **MazeSearchAlgo** — grid-based A* stub, needs expansion-room architecture
2. **SortedRoomNeighbours** — 1300+ lines, room completion for maze search
3. **MazeShoveTraceAlgo** — trace shoving during maze search
4. **BatchOptimizer** — post-route optimization
5. **DrillPage / expansion rooms** — room/drill infrastructure (Java-only DrillPage excludes DrillPageArray.cpp)
6. **ShapeSearchTree room creation** — `complete_shape`/`create_complete_room` not ported
7. **SpecctraDsnStreamReader** — full DSN parsing
8. **SessionWriter** — full SES output (currently traces-only)

## Recent Progress (2026-09-18)

- **WP-2 NetIncompletes completed**: verified the existing port against `java/app/freerouting/drc/NetIncompletes.java` (398 lines); added the missing `connected_group_count` / `get_connected_group_count()`; **fixed a real aliasing bug** in `join_connected_sets()` (shared_ptrs were passed by const reference aliasing the members the loop reassigns — from-group items past the first were never repointed, giving extra airlines; now passed by value). 3 new tests (`NoViolationWhenLengthsOk`, `ThreeIsolatedViasProduceTwoAirlines`, `SingleItemHasNoAirLines`).
- **WP-1 Contact Graph completed**: `Item::first_common_layer()` ported (was a -1 stub); `Item::get_connection_items()` + `Item::is_fanout_via()` full ports; `DrillItem::first_layer()/last_layer()` overrides added AND the layer range is now synced from the padstack in the `Via` ctor/`set_padstack` (fixes vias always reporting 0..0); `Trace::normal_contact_point()` extended for DrillItem; `DrillItem::normal_contact_point()` added. 8 new `ItemContactGraphTest` cases.
- `RoutingBoard` facade stubs ported: `combine_traces`, `get_outline`, `contains_trace_tails`, `get_trace_tail` (Java BasicBoard/RoutingBoard); `additional_update_after_change` confirmed faithful no-op.
- `BoardOutline` restored to the build (was a stale non-compiling draft excluded from CMake — fixed namespaces, `Item` base ctor, `split_to_convex`-based `get_tile_shape`).
- Board module 41 → 50 classes done (98% headless); Interactive module 3 → 5 classes done (NetIncompletes, RatsNest now done — 67%).
- Tests: 415/415 ctest + run_tests green (WP-1: 8 `ItemContactGraphTest`, WP-2: 3 new `NetIncompletesTest` cases).
