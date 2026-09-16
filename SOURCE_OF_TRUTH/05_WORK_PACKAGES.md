# Work Packages: Agent Execution Plan

Each WP is self-contained for parallel agent execution. See `04_IMPLEMENTATION_PLAN.md` for the full phased plan.

## WP-1: Contact Graph (PREREQUISITE for WP-2)

**Java refs**: Item.java:546, Trace.java:172, DrillItem.java:264/342, ConductionArea.java
**C++ files**: Item.{h,cpp}, Trace.{h,cpp}, Via.h, ConductionArea.{h,cpp}

**Tasks:**
1. `Item::get_connected_set()` — recursive over get_normal_contacts(), skip ConductionArea
2. `DrillItem::get_ratsnest_corners()` — single center point
3. `DrillItem::get_normal_contacts()` — search overlapping objects with tolerance
4. Extend `Trace::get_normal_contacts(Point, bool)` for DrillItem/ConductionArea

## WP-2: NetIncompletes Full Port (PREREQUISITE for WP-3)

**Java refs**: NetIncompletes.java (~450 lines)
**C++ files**: NetIncompletes.{h,cpp} — replace stub

**Key logic:**
- Per-item filter, connected-set analysis, Delaunay triangulation
- AirLines joining disjoint connected sets
- `calc_length_violation()` via Trace::get_length()

## WP-3: RatsNest Full Port (PREREQUISITE for WP-4)

**Java refs**: RatsNest.java
**C++ files**: RatsNest.{h,cpp} (new)

**Key logic:**
- Per-net item lists, incomplete count, length violations
- Airlines, recalculate, filter, hidden nets

## WP-4: BoardStatistics + Scoring

**Java refs**: BoardStatistics.java + 11 substructs
**C++ files**: src/core/scoring/BoardStatistics.{h,cpp} (new)
**Note:** Must go in separate `freerouting_scoring` lib (layering constraint)

## WP-5: Core Tests + CMake + Docs

Register gtests for WP-1..4, update docs/migration/INDEX.md, verify ctest green.

## WP-6: RoutingJob / Session / BoardFileDetails

Full port of core data model with event wiring.

## WP-7..15: Remaining Modules (Parallel)

| WP | Module | Key Remaining Classes |
|----|--------|----------------------|
| WP-7 | datastructures | TimeLimit, UndoableObjects, Stoppable, BigIntAux |
| WP-8 | management | RoutingJobScheduler, TextManager remainder |
| WP-9 | settings | GlobalSettings impl, RouterOptimizerSettings, SettingsMerger |
| WP-10 | drc | DesignRulesChecker, DrcReport, UnconnectedItems |
| WP-11 | designforms | SpecctraDsnStreamReader, SpecctraSesFileWriter, Parser |
| WP-12 | boardgraphics | All 8 (GUI, low priority) |
| WP-13 | autoroute | BatchOptimizer, DrillPage, BoardHistory, SortedRoomNeighbours |
| WP-14 | interactive | HeadlessBoardManager enhancement, Route |
| WP-15 | board | ForcedViaAlgo, MoveDrillItemAlgo, OptViaAlgo, PullTight* |

## Definition of Done (per WP)

1. Port complete & faithful to Java
2. `cmake --build` green with `-Werror`
3. gtest added + registered in tests/CMakeLists.txt
4. `run_tests` + ctest all green
5. docs/migration/INDEX.md updated
