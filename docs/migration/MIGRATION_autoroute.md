# Autoroute Module - Migration Status

**Module**: `src/autoroute/`
**CMake Target**: `freerouting_autoroute`
**Java Files**: 35 | **C++ Impls compiled**: 23 | **Tests registered**: 12 (gtest, via FetchContent)

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
| DrillPage | DrillPage_test | Port of drill/DrillPage.java: obstacle cutouts -> convex pieces -> ExpansionDrill per piece; net-no caching + invalidate; attach_smd pin-center locations |
| DrillPageArray | DrillPageArray_test | Port of drill/DrillPageArray.java: page grid, overlapping_pages, invalidate/reset |
| ExpandableObject | ExpandableObject_test | Interface; `get_shape()` returns `IntBox` (C++ port uses IntBox everywhere) |
| ExpansionDrill | ExpansionDrill_test | Port of drill/ExpansionDrill.java: real calculate_expansion_rooms via engine complete_expansion_room |
| ExpansionRoom | - | Expansion grid (canonical: Complete/FreeSpace/Obstacle/Incomplete rooms + ExpansionDoor) |
| CompleteFreeSpaceExpansionRoom | CompleteFreeSpaceExpansionRoom_test | implements `board::SearchTreeObject` (Storable: tree_shape_count==1, get_tree_shape=IntBox, compare_to by reversed id / -1 for non-rooms; is_obstacle/is_trace_obstacle true; shape_layer=layer) + net_dependent flag + remove_from_tree (WP-10 Inc A) |
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
| IncompleteFreeSpaceExpansionRoom | stale shared_ptr draft | FreeSpaceExpansionRoom (draft; canonical lives in ExpansionRoom.h) |
| OptimizeRouteTask | missing dep | BatchOptimizer/BatchOptimizerMultiThreaded (Java only) |
| TargetItemExpansionDoor | stale shared_ptr draft, conflicts | canonical TargetItemExpansionDoor lives in ExpansionRoom.h |

## WP-9 porting notes (2026-09-19)

- `DrillPage::get_drills()` clusters the obstacle tree entries, cuts convex holes out of the page
  (`split_to_convex_with_holes` replicates `PolylineArea::split_to_convex`; equal obstacle shapes are
  deduped via the bounded-corner `tile_contains` check), then creates one `ExpansionDrill` per piece.
- `ExpansionDrill` stores the piece `bounding_box()` (documented IntBox simplification; the exact piece
  shape is only needed for the drill location and attach_smd pin centers).
- `AutorouteEngine::complete_expansion_room()` is a minimal port of Java `completeExpansionRoom`:
  full `ShapeSearchTree.complete_shape()` free-space expansion is not ported (blocker #6). Sub-2D room
  shapes are inflated with `IntBox::offset(1)` so drill-location units produce 2D rooms. Owns rooms via
  `unique_ptr`, and `get_autoroute_search_tree()` now auto-initializes the lazy tree.
- `Pin::drill_allowed()` added to `src/board/Pin.h` (`first_layer() == last_layer()`); Java
  `Item.isDrillable` semantics confirmed to match C++ for Trace/ConductionArea/DrillItem.
- `ShapeSearchTree` stores only Items, so `ExpansionDrill` always creates rooms (no existing-room search);
  newly added obstacles must be inserted into the engine tree explicitly (as the router does).
- The autoroute test link group now also links `freerouting_settings` (AutorouteControl.cpp needs
  `RouterSettings::is_layer_active`).

## WP-10 Increment A porting notes (2026-09-19): rooms storable in the search tree

- `CompleteFreeSpaceExpansionRoom` now derives from `CompleteExpansionRoom` AND `board::SearchTreeObject`
  (mirrors Java `implements CompleteExpansionRoom, SearchTreeObject`). `get_tree_shape()` returns the
  stored `IntBox` directly (`&shape_`, no heap allocation). `compare_to` mirrors Java: reversed room id
  for rooms, `-1` for any non-room storable (so rooms sort before items, matching Java's TreeSet order).
- `ShapeSearchTree::insert(SearchTreeObject&)` delegates to `ShapeTree::insert(Storable&)` which wires
  `set_search_tree_entries`. `clear()` now removes ALL leaves generically (`ShapeTree::remove(to_array())`);
  the old version did `static_cast<Item*>(leaf->object)` which is UB now that rooms occupy leaves.
- `AutorouteEngine::complete_expansion_room` inserts each completed room into the engine tree (Java
  `addCompleteRoom()` line 535). `clear()` removes every room from the tree first. `init_connection`
  purges `is_net_dependent()` rooms when `maintain_database` is set and the net changes (Java
  `initConnection`); `remove_complete_expansion_room` is the minimal version (no neighbour split — C++
  rooms currently have no doors).
- `ExpansionDrill::calculate_expansion_rooms` now finds and reuses a previously stored room, but only
  when the stored room contains the drill location. This containment guard is a C++-side addition:
  Java's complete rooms are maximal free spaces so the location is always contained, whereas the C++
  interim rooms are ~3x3 boxes around the drill location (complete_shape not ported). Without the guard a
  neighbouring drill could wrongly reuse a foreign micro-room.
- Net-dependent tests also confirmed the existing drill tests stay green: rooms are skipped as cutouts by
  `DrillPage` (non-Item entries) and by `calc_pin_center_in_drill` (non-Pin entries).

## Summary

- **Done**: 23 classes compiled & in the build (66% of Java files)
- **Tests**: 12 gtest executables; **472/472 ctest** + run_tests PASSED
- **Remaining blockers**: BatchOptimizer/BatchOptimizerMultiThreaded (optimization pass),
  SortedRoomNeighbours family + MazeSearchAlgo room/door path (room completion incl. `completeShape`,
  blocker #6), and the settings/scoring infra for BoardHistoryEntry