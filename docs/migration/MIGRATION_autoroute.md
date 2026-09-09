# Autoroute Module - Migration Status

**Module**: `src/autoroute/`
**CMake Target**: `freerouting_autoroute`
**Java Files**: 35 | **C++ Impls compiled**: 21 | **Tests registered**: 9 (gtest, via FetchContent)

---

## Compiled, linked & tested (2026-09)

The full routing engine now builds into the `freerouting_autoroute` static library and is covered by
gtest-based companion tests registered in `tests/CMakeLists.txt` (googletest pulled in via FetchContent).

| C++ Class | Test | Notes |
|-----------|------|-------|
| AutorouteAttemptState | AutorouteAttemptState_test | Enum + helpers (C++ adds NOT_ROUTED vs Java) |
| AutorouteControl | - | Routing parameters |
| AutorouteEngine | - | Routing coordinator |
| BatchAutorouter | - | Batch routing |
| BoardUpdateStrategy | BoardUpdateStrategy_test | Enum + helpers |
| Connection | - | Connection handling |
| DestinationDistance | - | Cost functions |
| ExpandableObject | ExpandableObject_test | Interface; `get_shape()` returns `IntBox` (C++ port uses IntBox everywhere) |
| ExpansionRoom | - | Expansion grid (canonical: Complete/FreeSpace/Obstacle/Incomplete rooms + ExpansionDoor) |
| InsertFoundConnectionAlgo | - | Insert connection algo |
| ItemAutorouteInfo | - | Item routing state |
| ItemRouteResult | ItemRouteResult_test | Fields made non-const for assignability; `operator<=>`/`operator==` explicit |
| ItemSelectionStrategy | ItemSelectionStrategy_test | Enum + helpers |
| LocateFoundConnectionAlgo | - | Locate connection algo |
| MazeSearchAlgo | - | Core maze search |
| MazeSearchElement | - | Search nodes |
| MazeSearchRoute | - | Trace construction |
| NamedAlgorithmType | NamedAlgorithmType_test | Enum + helpers |
| PerformanceProfiler | PerformanceProfiler_test | Uses FR_LOG_DEBUG macros |
| RoutingFailureLog | RoutingFailureLog_test | Uses `board::Item` (get_id/get_net_nos); thread-safe with per-info mutex |
| TaskState | TaskState_test | Enum + helpers (to_string/get_task_description/is_active_state added) |

## Not yet compiled (blocked on unported dependencies or stale drafts)

| Class | Reason | Depends on |
|-------|--------|------------|
| BoardHistoryEntry | missing deps | settings/RouterScoringSettings, core/scoring/BoardStatistics (not ported) |
| DrillPageArray | missing dep | DrillPage (Java only, never ported) |
| IncompleteFreeSpaceExpansionRoom | stale shared_ptr draft | FreeSpaceExpansionRoom (draft; canonical lives in ExpansionRoom.h) |
| OptimizeRouteTask | missing dep | BatchOptimizer/BatchOptimizerMultiThreaded (Java only) |
| TargetItemExpansionDoor | stale shared_ptr draft, conflicts | canonical TargetItemExpansionDoor lives in ExpansionRoom.h |

## Summary

- **Done**: 21 classes compiled & in the build (60% of Java files)
- **Tests**: 9 gtest executables -> ~107 test cases; all 116 ctest entries pass
- **Remaining blockers**: BatchOptimizer/BatchOptimizerMultiThreaded (optimization pass), DrillPage/DrillPageArray,
  and the settings/scoring infra for BoardHistoryEntry