# Progress: Module-by-Module Migration Status

## Summary

| Module | Java Files | Done | Partial | Remaining | Status |
|--------|-----------|------|---------|-----------|--------|
| Logger | 4 | 4 | 0 | 0 | 100% |
| Rules | 10 | 10 | 0 | 0 | 100% |
| Constants | 1 | 1 | 0 | 0 | 100% |
| Geometry | 34 | 34 | 0 | 0 | 100% |
| Board | 51 | 50 | 0 | 1 | 98% headless |
| DataStructures | 14 | 7 | 1 | 6 | 64% |
| Autoroute | 35 | 21 | 0 | 14 | 60% |
| Interactive | 9 | 5 | 1 | 3 | 67% |
| DesignForms | 41 | 1 | 12 | 28 | 32% |
| DRC | 7 | 1 | 1 | 5 | 29% |
| Settings | 21 | 2 | 4 | 15 | 29% |
| Core | 36 | 7 | 1 | 28 | 21% |
| Scoring | 12 | 5 | 0 | 7 | 42% |
| Management | 18 | 2 | 0 | 16 | 11% |
| BoardGraphics | 8 | 0 | 0 | 8 | 0% |
| **TOTAL** | **289** | **145** | **21** | **123** | **50%** |

---

## Module: Geometry (100% — DONE)

All 34 Java files have C++ equivalents. Full `Shape` interface, `Area` + `PolygonArea`/`PolylineArea`/`TileShape`, rational arithmetic via `boost::multiprecision`.

Key classes: `Point`, `IntPoint`, `FloatPoint`, `RationalPoint`, `Vector`, `Line`, `LineSegment`, `Polyline`, `Shape`, `Area`, `TileShape`, `Simplex`, `IntBox`, `IntOctagon`, `Circle`, `PolygonShape`, `PolylineArea`, `Direction`, `Side`, `Rational`

## Module: Rules (100% — DONE)

All 10 Java files mapped. `NetClasses`, `Nets`, `ViaInfos` inlined in C++ headers.

## Module: Board (98% headless)

### Completed (46 classes)
AngleRestriction, FixedState, Unit, Connectable, Component, Components, DrillItem, Layer, Via, Pin, Padstack, Item, Trace, PolylineTrace, BoardOutline, ObstacleArea, ConductionArea, ViaObstacleArea, ComponentObstacleArea, LayerStructure, CoordinateTransform, BasicBoard, RoutingBoard, Communication, ChangedArea, SearchTreeManager, ShapeSearchTree, ShapeSearchTree45Degree, ShapeSearchTree90Degree, BoardLoader, BoardBuilder, ItemSearchTreesInfo, SearchTreeObject, ItemIdentificationNumberGenerator, ItemSelectionFilter, CalcFromSide, CalcShapeAndFromSide, ComponentOutline, BoardObservers, BoardObserverAdaptor, ShapeTraceEntries, ShoveTraceAlgo, ForcedPadAlgo, ForcedViaAlgo, MoveDrillItemAlgo, MoveComponent, OptViaAlgo, PullTightAlgo, PullTightAlgo45, PullTightAlgo90, PullTightAlgoAnyAngle

### Remaining Board Gaps
- `RoutingBoard::opt_changed_area()` — still empty (needs TraceTightener integration)
- Remaining critical stubs: `ShoveTraceAlgo::spring_over()` (returns input unchanged, ShoveTraceAlgo.cpp:235), `ShoveTraceAlgo::insert()` (one-level headless minimal), base `Trace::normalize()/combine()/split()` (PolylineTrace overrides are real)

### Restored Drafts (2026-09-18)
- `BoardOutline` — file existed but was NOT compiled (missing from `src/board/CMakeLists.txt`) and was a stale half-port: wrong namespaces (`class PolylineShape`/`class TileShape` forward-declared in `namespace board`, shadowing the `geometry::planar` types) and no `Item` base constructor call. Fixed header namespaces, wired the `Item` base ctor, reimplemented `get_tile_shape()` on the C++ `split_to_convex()` API, and added `BoardOutline.cpp` to the build.
- Critical Stubs cleared: `combine_traces()`, `get_outline()`, `contains_trace_tails()`, `get_trace_tail()` ported from Java; `additional_update_after_change()` documented as faithful no-op (no autoroute engine wired into the item-change path yet).

### Contact Graph completed (2026-09-18, WP-1)
- `Item::first_common_layer()` — WAS a stub returning -1, now full Java port (`Item.java:320` `firstCommonLayer`)
- `DrillItem::first_layer()/last_layer()` — overrides added; layer range is now synced from the padstack in the `Via` ctor and `set_padstack` (Java DrillItem derives `firstLayer`/`lastLayer` from the padstack). Fixed a real bug: vias always reported layers 0..0.
- `Item::get_connection_items(StopConnectionOption)` — WAS a stub returning `{}`, now full port (`Item.java:741` `getConnectionItems`)
- `Item::is_fanout_via(const std::set<Item*>*)` — new (`Item.java:1250` `isFanoutVia`)
- `Trace::normal_contact_point(Item*)` — DrillItem branch added (`DrillItem.java:339`)
- `DrillItem::normal_contact_point(Item*)` — new override (`DrillItem.java:326`)
- Tests: 8 new `ItemContactGraphTest` cases in `ItemContacts_test.cpp` (14 total green)

## Module: Autoroute (60%)

### Compiled & Tested (21 classes)
AutorouteAttemptState, AutorouteControl, AutorouteEngine, BatchAutorouter, BoardUpdateStrategy, Connection, DestinationDistance, ExpandableObject, ExpansionRoom, InsertFoundConnectionAlgo, ItemAutorouteInfo, ItemRouteResult, ItemSelectionStrategy, LocateFoundConnectionAlgo, MazeSearchAlgo, MazeSearchElement, MazeSearchRoute, NamedAlgorithmType, PerformanceProfiler, RoutingFailureLog, TaskState

### Not Migrated (14 classes)
| Class | Lines | Priority | Notes |
|-------|-------|----------|-------|
| MazeShoveTraceAlgo | 316 | CRITICAL | Trace shoving during maze search |
| Sorted45DegreeRoomNeighbours | 810 | CRITICAL | 45° room completion |
| SortedOrthogonalRoomNeighbours | 482 | CRITICAL | Orthogonal room completion |
| SortedRoomNeighbours | 680 | CRITICAL | Base room completion |
| DrillPage | 189 | CRITICAL | Via drill intermediary |
| LocateFoundConnectionAlgo45Degree | 238 | CRITICAL | 45° connection location |
| LocateFoundConnectionAlgoAnyAngle | 363 | CRITICAL | Any-angle connection |
| BatchAutorouterThread | 546 | HIGH | Multi-threaded routing |
| BatchOptimizer | 397 | HIGH | Post-route optimization |
| BatchOptimizerMultiThreaded | 294 | MEDIUM | Multi-threaded optimizer |
| BoardHistory | ~200 | HIGH | Backtracking/convergence |
| MazeListElement | 130 | HIGH | Priority queue element pooling |
| BatchAutorouterV19 | 490 | LOW | Legacy comparison router |
| OptimizeRouteTask | 67 | MEDIUM | Optimization task |

### Critical Simplifications in Existing Code
- `MazeSearchAlgo::init()` — uses bounding box centers, ignores search_tree
- `MazeSearchAlgo::expand_to_room_neighbours()` — hardcoded `dist = 100.0`
- `AutorouteEngine::autoroute_connection()` — grid-based A* (fundamentally different from Java's expansion rooms)
- `AutorouteEngine::route_all()` — stub (returns true immediately)
- `BatchAutorouter::route_all()` — no backtracking, no convergence detection

## Module: DesignForms (32%)

### Compiled (9 files)
Scanner, Structure, Network, Library, Placement, SessionWriter, Circle, Polygon, Keyword (partial)

### Critical Missing
- `DsnFile.cpp` — declared but no implementation
- `Wiring.cpp` — header exists, impl missing
- `SpecctraDsnStreamReader` (1467 lines) — full DSN parsing
- `SpecctraSesFileWriter` (416 lines) — full SES output
- `CoordinateTransform` (199 lines) — DSN↔board coordinates
- `Parser` (198 lines) — DSN parser core
- Full write paths for Structure, Network, Library, Placement

## Module: DRC (29%)

### Done: ClearanceViolation only
### Missing: DesignRulesChecker (466 lines), DrcReport, DrcViolation, DrcViolationItem, DrcPosition, UnconnectedItems

## Module: Settings (29%)

### Done: RouterSettings, RuntimeEnvironment
### Missing: GlobalSettings (impl), RouterOptimizerSettings (2123 lines), RouterScoringSettings (2544 lines), SettingsMerger, all 9 settings sources

## Module: Core (21%)

### Done: FileFormat, RoutingJob, RoutingStage, StopRequestState, Package, Session, BoardFileDetails, BoardFileDetailsUpdatedEvent, BoardFileDetailsUpdatedEventListener, RoutingJob*Event/Listener (events submodule)
### Missing: Padstack/Padstacks, BoardLibrary (4980 lines), Packages, StoppableThread, RouterCounters, RouterJobResourceUsage

## Module: Scoring (42%)

### Compiled & Tested (14 classes)
BoardStatistics, BoardStatisticsBoard, BoardStatisticsLayers, BoardStatisticsItems, BoardStatisticsComponents, BoardStatisticsPads, BoardStatisticsNets, BoardStatisticsConnections, BoardStatisticsTraces, BoardStatisticsBends, BoardStatisticsVias, BoardStatisticsClearanceViolations, BoardStatisticsDifficulty, BoardStatisticsFanout

### Not Migrated (9 classes)
| Class | Lines | Priority | Notes |
|-------|-------|----------|-------|
| BoardStatisticsBoard | 16 | LOW | Data holder (areaCm2 added) |
| BoardStatisticsLayers | 15 | LOW | Data holder |
| BoardStatisticsItems | 28 | LOW | Data holder |
| BoardStatisticsComponents | 13 | LOW | Data holder |
| BoardStatisticsPads | 13 | LOW | Data holder |
| BoardStatisticsNets | 15 | LOW | Data holder |
| BoardStatisticsConnections | 12 | LOW | Data holder |
| BoardStatisticsTraces | 34 | LOW | Data holder (totalLengthMm added) |
| BoardStatisticsBends | 19 | LOW | Data holder |

### Completed (5 classes)
- BoardStatistics (main class) - full port from Java BoardStatistics.java
- BoardStatisticsBoard (data holder)
- BoardStatisticsLayers (data holder)
- BoardStatisticsDifficulty (data holder)
- BoardStatisticsFanout (data holder)

### Critical Missing
- None — `init_from_board()` (item counting, trace analysis, bends, vias, difficulty, fanout, unit conversion), `calculateScore()/getMaximumScore()/getNormalizedScore()`, and the DSN/SES file constructor are all implemented and tested (11 BoardStatistics gtest cases, 422/422 ctest).
- Open parity gap: `pin_is_escaped` approximates Java's `clearanceViolations().isEmpty()` check away (DRC module not yet ported).

## Module: Interactive (67%)

### Done: HeadlessBoardManager, InteractiveSettings, RatsNestItemInfo, NetIncompletes, RatsNest
### Partial: BoardManager (interface)
### Missing: Route (624 lines), ThreadActionListener

### NetIncompletes completed (2026-09-18, WP-2)
- Verified the existing port against `java/app/freerouting/drc/NetIncompletes.java` (398 lines): filter, `calculate_net_items`, Delaunay triangulation, `Edge::Less` tie-breakers, airline creation, `calc_length_violation` are all faithful.
- Added `connected_group_count_` / `get_connected_group_count()` — was missing entirely (`NetIncompletes.java:46,141,155`).
- **Fixed aliasing bug in `join_connected_sets()`**: parameters were `const shared_ptr<...>&` bound to `from_item`/`to_item` members that the loop reassigns; repointing the from-item mid-loop invalidated `p_from`, so the remaining from-group items were skipped → too many airlines (3 isolated vias → 3, expected 2). Fixed by taking shared_ptrs by value.
- Tests: `NoViolationWhenLengthsOk` (+group count), `ThreeIsolatedViasProduceTwoAirlines` (3-island MST), `SingleItemHasNoAirLines` — 8/8 NetIncompletesTest, 415/415 ctest, run_tests all PASSED.

## Module: Management (11%)

### Done: SessionManager, TextManager
### Missing: RoutingJobScheduler (395 lines), RoutingJobSchedulerActionThread (264 lines), ReflectionUtil, VersionChecker, analytics (8 files), gson adapters (4 files)
