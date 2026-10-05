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
| Fully migrated | 145 (50%) |
| Partially migrated | 21 (7%) |
| Not migrated | 123 (43%) |
| Critical path | DSN → Route → SES |
| Current headless completion | ~19% (hw48na test) |

## Top Blockers for Headless Routing

1. **`MazeSearchAlgo` connection location and insertion** — the expansion-room search itself is ported (**WP-10 Inc I** 2026-09-27: Java `MazeSearchEngine` room/door expansion over a `std::set<MazeListElement>`), the ripup costs are ported (**WP-10 Inc J** 2026-09-27: `MazeRipupResolver` + the deterministic `MazeRandom`), the target-door calculation is wired in all three `Sorted*RoomNeighbours::complete_expansion_room()` (verified 2026-09-27, the old blocker text was stale) and the **connection location is ported** (**WP-10 Inc M** 2026-10-01: `FoundConnectionLocator` + the 45° and any-angle variants, 13 gtests) — what is still missing is a maze connection route reaching the board: `FoundConnectionInserter` is not ported and the production `AutorouteEngine::autoroute_connection()` still uses the legacy grid A* with the `LocateFoundConnectionAlgo`/`InsertFoundConnectionAlgo` stubs
2. **SortedRoomNeighbours** — room completion for maze search (**base ported 2026-09-26**, WP-10 Inc E: neighbour ordering, 1-D/2-D door creation, empty-neighbour incomplete rooms; **45° variant ported 2026-09-27**, WP-10 Inc F: touching sides, room enlargement, new incomplete rooms + bounding-direction dispatch; **orthogonal/90° variant ported 2026-09-27**, WP-10 Inc G: touching sides, room enlargement, new incomplete rooms, `select_calculation_mode` dispatch — the whole family is done, and the maze search that uses it is ported in Inc I; the door-calculation wiring was verified present in Inc J, same as blocker 1)
3. ~~`MazeTraceShover`~~ — **RESOLVED 2026-09-30 (WP-10 Inc K)**: trace shoving during maze search is ported and wired into `MazeSearchAlgo::expand_to_room_doors()`. Together with `MazeRipupResolver` (WP-10 Inc J) the ripup cost of a shoveable room is now really reserved and the delayed occupation is really reached. The remaining part of this blocker, the drill-page/other-layer expansion (`MazeExpansionEngine`), is item 5
4. **BatchOptimizer** — post-route optimization
5. **DrillPage / expansion rooms** — room/drill infrastructure (**WP-9 done 2026-09-19**: DrillPage, DrillPageArray, ExpansionDrill::calculate_expansion_rooms, AutorouteEngine.complete_expansion_room; **WP-10 Inc A done 2026-09-19**: rooms are SearchTreeObjects stored in the autoroute search tree — found/reused by drills, net-dependent purge on net change; MazeSearchAlgo not yet wired to drill pages)
6. **ShapeSearchTree room creation** — `complete_shape` ported as a board API (**WP-10 Inc B** 2026-09-22) and rewired into the engine (**WP-10 Inc C** 2026-09-22: `AutorouteEngine.complete_expansion_room` runs through `complete_shape`, obstacle-restrained room splitting); door mechanics ported (**WP-10 Inc D** 2026-09-22: `get_expansion_room`, `create_overlap_door`, `calculateTargetDoors`, `remove_all_doors`); door *creation* from a room shape ported for the base variant (**WP-10 Inc E** 2026-09-26: `SortedRoomNeighbours`), for the default 45° route angle (**WP-10 Inc F** 2026-09-27: `Sorted45DegreeRoomNeighbours`) and for 90° (**WP-10 Inc G** 2026-09-27: `SortedOrthogonalRoomNeighbours` + `select_calculation_mode` dispatch on the search tree bounding directions) and the room-database housekeeping is ported (**WP-10 Inc H** 2026-09-27: `removeCompleteExpansionRoom` neighbour split, `completeNeighbourRooms`, `getFirstIncompleteExpansionRoom`); the maze search that consumes the doors is ported (**WP-10 Inc I** 2026-09-27: `MazeSearchAlgo` = Java `MazeSearchEngine` room/door expansion over a `std::set<MazeListElement>`) and the ripup costs are ported (**WP-10 Inc J** 2026-09-27: `MazeRipupResolver`, `MazeRandom`, `RouterSettings::get_start_ripup_costs`) — the `calculate_target_doors()` call was verified in Inc J; still missing: the inserter of Phase 4 (the locator landed in **WP-10 Inc M** 2026-10-01)
7. **SpecctraDsnStreamReader** — full DSN parsing
8. **SessionWriter** — full SES output (currently traces-only)

## Recent Progress (2026-09-18)

- **WP-2 NetIncompletes completed**: verified the existing port against `java/app/freerouting/drc/NetIncompletes.java` (398 lines); added the missing `connected_group_count` / `get_connected_group_count()`; **fixed a real aliasing bug** in `join_connected_sets()` (shared_ptrs were passed by const reference aliasing the members the loop reassigns — from-group items past the first were never repointed, giving extra airlines; now passed by value). 3 new tests (`NoViolationWhenLengthsOk`, `ThreeIsolatedViasProduceTwoAirlines`, `SingleItemHasNoAirLines`).
- **WP-1 Contact Graph completed**: `Item::first_common_layer()` ported (was a -1 stub); `Item::get_connection_items()` + `Item::is_fanout_via()` full ports; `DrillItem::first_layer()/last_layer()` overrides added AND the layer range is now synced from the padstack in the `Via` ctor/`set_padstack` (fixes vias always reporting 0..0); `Trace::normal_contact_point()` extended for DrillItem; `DrillItem::normal_contact_point()` added. 8 new `ItemContactGraphTest` cases.
- `RoutingBoard` facade stubs ported: `combine_traces`, `get_outline`, `contains_trace_tails`, `get_trace_tail` (Java BasicBoard/RoutingBoard); `additional_update_after_change` confirmed faithful no-op.
- `BoardOutline` restored to the build (was a stale non-compiling draft excluded from CMake — fixed namespaces, `Item` base ctor, `split_to_convex`-based `get_tile_shape`).
- Board module 41 → 50 classes done (98% headless); Interactive module 3 → 5 classes done (NetIncompletes, RatsNest now done — 67%).
- Tests: 415/415 ctest + run_tests green (WP-1: 8 `ItemContactGraphTest`, WP-2: 3 new `NetIncompletesTest` cases).
