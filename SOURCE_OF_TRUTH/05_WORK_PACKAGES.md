# Work Packages: Agent Execution Plan

Each WP is self-contained for parallel agent execution. See `04_IMPLEMENTATION_PLAN.md` for the full phased plan.

## WP-1: Contact Graph (PREREQUISITE for WP-2)

**Java refs**: Item.java:546, Trace.java:172, DrillItem.java:264/342, ConductionArea.java
**C++ files**: Item.{h,cpp}, Trace.{h,cpp}, Via.h, ConductionArea.{h,cpp}

**Completed (2026-09-18):**
1. `Item::get_connected_set()` — recursive over get_normal_contacts(), skip ConductionArea ✅
2. `DrillItem::get_ratsnest_corners()` — single center point ✅
3. `DrillItem::get_normal_contacts()` — search overlapping objects with tolerance ✅
4. `Trace::get_normal_contacts(Point, bool)` — DrillItem/ConductionArea branches ✅

**Additional gaps closed this session:**
- `Item::first_common_layer()` — full Java port (max first / min last layer, -1 if none)
- `DrillItem::first_layer()/last_layer()` — overrides added AND layer range now synced from the padstack in `Via` ctor / `set_padstack` (fixed real bug: vias always reported layers 0..0)
- `Item::get_connection_items(StopConnectionOption)` — full port (fork/stub detection, trace single-contact check, via stop/fanout-stop)
- `Item::is_fanout_via(const std::set<Item*>*)` — full port (java Item.isFanoutVia)
- `Trace::normal_contact_point(Item*)` — DrillItem branch added
- `DrillItem::normal_contact_point(Item*)` — new override (DrillItem+Trace cases)
- Tests: 8 new `ItemContactGraphTest` cases (14 total green)

## WP-2: NetIncompletes Full Port (PREREQUISITE for WP-3)

**Java refs**: NetIncompletes.java (398 lines), AirLine.java (55 lines)
**C++ files**: NetIncompletes.{h,cpp}, NetIncompletes_test.cpp

**Completed (2026-09-18):**
1. Verified `NetIncompletes.{h,cpp}` is already a faithful port — filter (is_tail / zero contacts), `calculate_net_items`, PlanarDelaunayTriangulation, `Edge::Less` with coordinate tie-breakers, airline creation, `calc_length_violation` via `Trace::get_length()`
2. Added missing `connected_group_count_` / `get_connected_group_count()` — port of Java `uniqueConnectedSets` count (set identity), set to net-item count when ≤ 1 ✅
3. **Fixed aliasing bug in `join_connected_sets()`** — was taking `const shared_ptr<...>&` bound to `from_item`/`to_item` members; repointing those very members mid-loop silently changed `p_from`, so from-group items were skipped (3 isolated vias → 3 airlines, should be 2). Now takes shared_ptrs **by value**. Java is immune because reassigning a field never changes the caller's captured reference. ✅
4. Tests: added `NoViolationWhenLengthsOk` group-count assert, `ThreeIsolatedViasProduceTwoAirlines` (MST), `SingleItemHasNoAirLines` — 8 NetIncompletesTest cases green (415/415 ctest)

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

## WP-4: BoardStatistics + Scoring (IN PROGRESS)

**Java refs**: BoardStatistics.java + 11 substructs
**C++ files**: src/core/scoring/BoardStatistics.{h,cpp}, src/core/scoring/CMakeLists.txt
**Note**: Must go in separate `freerouting_scoring` lib (layering constraint)

### Completed
- `BoardStatistics` main class created with all sub-structs as nested types
- `BoardStatistics.h` and `BoardStatistics.cpp` created
- `freerouting_scoring` library created and builds successfully
- 4 gtest tests created and passing (CanCreateFromBoard, CanCreateFromDSN, CanCreateFromSES, ScoreCalculation)
- Test registered in `tests/CMakeLists.txt`

### TODO
- `init_from_board()` needs full implementation with item counting, trace analysis, bend calculation, via classification
- `calculateScore()`, `getMaximumScore()`, `getNormalizedScore()` need proper scoring logic
- File-based constructors need full SES/DSN parsing
- Unit conversion needs proper implementation

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
