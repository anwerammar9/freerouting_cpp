# Implementation Plan: Missing Logic for Headless Routing

## Overview

40 tasks across 7 phases. Estimated 29-39 days. Java source is truth.

## Dependency Graph

```
Phase 1 (Trace Ops) ──┐
                       ├──→ Phase 3 (Maze Search) ──→ Phase 4 (Connection) ──→ Phase 5 (Batch)
Phase 2 (Connectivity) ┘         │
                                  ├──→ Phase 6 (Search Trees)
                                  └──→ Phase 7 (Board Stubs)
```

---

## Phase 1: Trace Operations Foundation (Week 1-2)

| Task | File | Port From | Est. Lines |
|------|------|-----------|------------|
| 1.1 Trace::normalize() + combine() | Trace.cpp, PolylineTrace.cpp | Trace.java | ~150 |
| 1.2 Trace::split() | Trace.cpp, PolylineTrace.cpp | Trace.java | ~100 |
| 1.3 ShoveTraceAlgo::spring_over() | ShoveTraceAlgo.cpp | TraceShover.java:611-875 | ~175 | **DONE 2026-10-02** (WP-10 Inc N) — `spring_over` + `spring_over_obstacles` + `get_ignore_items_at_tie_pins`, with `Item::get_all_contacts*`, `TileShape::cutout(Polyline)`/`bounding_octagon()` and `DrillItem::get_tree_shape_on_layer` as prereqs, 20 new gtests, ctest 673/673 |
| 1.4 ShoveTraceAlgo::insert() board mutation | ShoveTraceAlgo.cpp | ShoveTraceAlgo.java | ~100 | **DONE 2026-10-02** (WP-10 Inc O) — full `TraceShover.insert` port + `ShapeTraceEntries::next_substitute_trace_piece`/`cutout_trace(s)`, 17 new gtests, ctest 690/690 |
| 1.5 ShoveTraceAlgo::check() via/direction | ShoveTraceAlgo.cpp | ShoveTraceAlgo.java | ~100 | **DONE 2026-10-02** (WP-10 Inc O) — full `TraceShover.check` port (shape + line-segment overload), `RoutingBoard::get_shove_failing_obstacle`, ctest 690/690 |

## Phase 2: Item Connectivity (Week 2-3)

> **WP-1/WP-2 status (2026-09-18):** 2.2, 2.3 DONE (`get_connection_items`, `first_common_layer`, `is_fanout_via`, DrillItem padstack layer range, `normal_contact_point`). WP-2 `NetIncompletes` full port DONE: `connected_group_count` added, `join_connected_sets` shared_ptr aliasing bug fixed (take by value). 415/415 ctest.

| Task | File | Port From | Est. Lines |
|------|------|-----------|------------|
| 2.1 Item::get_normal_contacts() base | Item.cpp | Item.java | ~50 |
| 2.2 Item::get_connection_items() | Item.cpp | Item.java:800-880 | ~80 |
| 2.3 Item::first_common_layer() | Item.cpp | Item.java | ~20 |
| 2.4 Pin class completion | Pin.cpp/h | Pin.java (722→25 lines) | ~400 |
| 2.5 Via::is_obstacle() + drill_info | Via.cpp/h | Via.java | ~50 |

## Phase 3: Maze Search Engine (Week 3-5)

**Prerequisites resolved (WP-10 Inc A/D/E):** complete free space rooms are stored in the autoroute search tree (Inc A); `ItemAutorouteInfo::get_expansion_room()`, `ObstacleExpansionRoom::create_overlap_door()`, `CompleteFreeSpaceExpansionRoom::calculateTargetDoors()`, `TargetItemExpansionDoor` real shape/dimension, and `AutorouteEngine::remove_all_doors()` are ported and tested (Inc D, 2026-09-22); `SortedRoomNeighbours` (base) with 1-D/2-D door creation and `calculate_incomplete_rooms_with_empty_neighbours()` is ported and tested (Inc E, 2026-09-26), together with the room `get_shape_tile()` generalization, the 45-degree variant with its `try_remove_edge_line`/`calculate_new_incomplete_rooms` plus the bounding-direction dispatch is ported and tested (Inc F, 2026-09-27), and the orthogonal/90-degree variant with its `try_remove_edge`/`calculate_new_incomplete_rooms` completes the `SortedRoomNeighbours` family (Inc G, 2026-09-27). The `AutorouteEngine.removeCompleteExpansionRoom` neighbour-split half was closed in WP-10 Inc H (2026-09-27), and the `MazeSearchAlgo` expansion-room core (tasks 3.6, 3.7, 3.8, 3.12) was closed in WP-10 Inc I (2026-09-27). Task 3.10 (`MazeRipupResolver`, with the deterministic `MazeRandom` and the `RouterSettings::get_start_ripup_costs()` accessor) was closed in WP-10 Inc J (2026-09-27), and task 3.14 was verified as already done (the target-door calculation is called from all three neighbour classes). Tasks 3.5/3.11 (`MazeTraceShover` + `shove_trace_room`) were closed in WP-10 Inc K (2026-09-30). Task 3.9 (drill-page/other-layer expansion, the Java `MazeExpansionEngine`) was closed in WP-10 Inc L, and `MazeFanoutDiagnostics` + the fanout `mazeExpansionList.add` filter + `RoutingBoard::fanout()` were closed in **WP-10 Inc R (2026-10-05)**. Still open: the tie-pin shape reduction (`reduce_trace_shapes_at_tie_pins`) and the Java `fallbackToBoardVias` fanout via-rule combination (needs `RouterSettings::FanoutSettings` and a global `BoardRules` via-rule list).

| Task | File | Port From | Est. Lines |
|------|------|-----------|------------|
| 3.1 SortedRoomNeighbours (base) | NEW .h/.cpp | SortedRoomNeighbours.java | ~200 | **DONE 2026-09-26 (WP-10 Inc E)** — `try_remove_edge`/`calculate_new_incomplete_rooms` deferred |
| 3.2 Sorted45DegreeRoomNeighbours | NEW .h/.cpp | Sorted45DegreeRoomNeighbours.java (810) | ~810 | **DONE 2026-09-27 (WP-10 Inc F)** — 11 gtests, `SortedRoomNeighbours::calculate` dispatches on the tree bounding directions |
| 3.3 SortedOrthogonalRoomNeighbours | NEW .h/.cpp | SortedOrthogonalRoomNeighbours.java (482) | ~482 | **DONE 2026-09-27 (WP-10 Inc G)** — 13 gtests, `select_calculation_mode` dispatches 4 bounding directions to it |
| 3.4 DrillPage (+ DrillPageArray, ExpansionDrill::calculateExpansionRooms) | NEW .h/.cpp | DrillPage.java (189) | ~189 | **DONE 2026-09-19 (WP-9)** |
| 3.5 MazeTraceShover | NEW .h/.cpp | MazeTraceShover.java (316) | 454 | **DONE 2026-09-30** (WP-10 Inc K) — `check_shove_trace_line`/`shove_trace_room`/`end_points_matching` + `DoorSection`, 14 gtests, ctest 617/617 |
| 3.6 MazeListElement | 2nd class in MazeSearchElement.h/.cpp | MazeListElement.java (130) | ~130 | **DONE 2026-09-27 (WP-10 Inc I)** — `std::set` queue, Java `compareTo` order, `ripup_cost`; 12 gtests in the new `MazeSearchElement_test.cpp` |
| 3.7 MazeSearchAlgo::init() rewrite | MazeSearchAlgo.cpp | MazeSearchEngine.java:1000-1093 | ~95 | **DONE 2026-09-27 (WP-10 Inc I)** — tie-pin shape reduction deferred (needs `ShapeSearchTree::reduce_trace_shape_at_tie_pin`) |
| 3.8 expand_to_room_doors() rewrite | MazeSearchAlgo.cpp | MazeSearchEngine.java:383-610 | ~230 | **DONE 2026-09-27 (WP-10 Inc I)** — ripup/trace-shove/drill-page branches deferred, not stubbed silently |
| 3.9 expand_to_other_layers() | NEW MazeExpansionEngine.h/.cpp | MazeExpansionEngine.java | ~250 | **NOT STARTED** — also owns `expand_to_drill_page`/`expand_to_drill`; needs a `DrillPageArray` owner in the engine |
| 3.10 check_ripup() | NEW MazeRipupResolver.h/.cpp | MazeRipupResolver.java | ~180 | **DONE 2026-09-27 (WP-10 Inc J)** — `check_ripup`/`check_leaving_ripped_item`/`calc_fanout_via_ripup_cost_factor` plus the header-only `MazeRandom` (java.util.Random parity) and `RouterSettings::get_start_ripup_costs()`; 31 gtests |
| 3.11 shove_trace_room() | NEW MazeTraceShover.h/.cpp | MazeTraceShover.java (316) | — | **DONE 2026-09-30** (WP-10 Inc K) — wired into `MazeSearchAlgo::expand_to_room_doors()`, replacing the "shove not ported" divergence |
| 3.12 Fix distance calculation | MazeSearchAlgo.cpp | Weighted distance | ~30 | **DONE 2026-09-27 (WP-10 Inc I)** — `shapeEntryMiddle.weighted_distance(...)` plus the normalized bend-cost penalty |
| 3.13 `AutorouteEngine.removeCompleteExpansionRoom` neighbour split + `completeNeighbourRooms` | AutorouteEngine.cpp | AutorouteEngine.java:377-412, 567-592 | ~60 | **DONE 2026-09-27 (WP-10 Inc H)** — 8 gtests; only the `drillPageArray.invalidate()` line is deferred (no `DrillPageArray` owner in the engine) |
| 3.14 Wire room door calculation into the engine | AutorouteEngine.cpp | SortedRoomNeighbours.complete / AutorouteEngine.addCompleteRoom | ~60 | **DONE (verified 2026-09-27, WP-10 Inc J)** — `calculate_target_doors()` is called from all three `Sorted*RoomNeighbours::complete_expansion_room()` (`SortedRoomNeighbours.cpp:648`, `Sorted45DegreeRoomNeighbours.cpp:435`, `SortedOrthogonalRoomNeighbours.cpp:336`) and unit-tested in `ItemAutorouteInfo_test.cpp`; the old tracking text was stale |

## Phase 4: Connection Location & Insertion (Week 5-6)

| Task | File | Port From | Est. Lines |
|------|------|-----------|------------|
| 4.1 LocateFoundConnectionAlgo45Degree | `FoundConnectionLocator45Degree.cpp` | Java (238) | ~238 | **DONE 2026-10-01 (WP-10 Inc M)** — `FoundConnectionLocator45Degree.h/.cpp`, 13 locator gtests |
| 4.2 LocateFoundConnectionAlgoAnyAngle | `FoundConnectionLocatorAnyAngle.cpp` | Java (363) | ~363 | **DONE 2026-10-01 (WP-10 Inc M)** — `FoundConnectionLocatorAnyAngle.h/.cpp`, 13 locator gtests |
| 4.3 Complete LocateFoundConnectionAlgo | `FoundConnectionLocator.cpp` | Java (427) | ~200 | **DONE 2026-10-01 (WP-10 Inc M)** — backtrack, `BacktrackElement`, ripped-room collection, `get_instance` angle dispatch |
| 4.4 Complete InsertFoundConnectionAlgo | `FoundConnectionInserter.cpp` | Java (`path/FoundConnectionInserter.java`) | ~400 | **PARTIAL 2026-10-03 (WP-10 Inc P)** — `FoundConnectionInserter.h/.cpp` ported (get_instance, insert_trace, insert_neckdown, try_neck_down, insert_fanout_micro_neckdown, insert_via, fanout diagnostics) and the `RoutingBoard` forced-trace prereqs added (`check_forced_trace_polyline`, `insert_forced_trace_polyline`, `insert_forced_trace_segment`, `connect_to_trace`, `clear_shove_failing_obstacle`); **still OPEN**: `AutorouteEngine::autoroute_connection()` rewiring is DONE (the maze expansion path is tried first, falling back to the grid A*); multi-segment route insertion is DONE as of **2026-10-04 (WP-10 Inc Q)**, which also fixed the `ShapeTraceEntries::resort()` port, the nullable `CalcFromSide::border_intersection`, the `PolylineTrace::combine()` use-after-free and the inverted clearance safety margin; **still OPEN**: `ForcedViaInserter` hole-shape parity (`MazeFanoutDiagnostics` and the `AutorouteControl::fanout_start_pin_name` assignment landed in **WP-10 Inc R, 2026-10-05**) |

## Phase 5: Batch Router & Convergence (Week 6-7)

| Task | File | Port From | Est. Lines |
|------|------|-----------|------------|
| 5.1 BoardHistory | NEW .h/.cpp | BoardHistory.java | ~150 |
| 5.2 route_all() convergence loop | BatchAutorouter.cpp | BatchAutorouter.java:549-686 | ~140 |
| 5.3 autoroute_pass() | BatchAutorouter.cpp | BatchAutorouter.java:326-503 | ~180 |
| 5.4 autoroute_item() | BatchAutorouter.cpp | BatchAutorouter.java:697-775 | ~80 |
| 5.5 getAutorouteItems() enhance | BatchAutorouter.cpp | BatchAutorouter.java:151-194 | ~40 |
| 5.6 BatchOptimizer | NEW .h/.cpp | BatchOptimizer.java | ~300 |

## Phase 6: Search Tree Room Creation (Parallel with Phase 3)

| Task | File | Port From | Est. Lines |
|------|------|-----------|------------|
| 6.0 Rooms storable in search tree (WP-10 Inc A) | ExpansionRoom.h/cpp, ShapeSearchTree.h/cpp, AutorouteEngine.cpp | CompleteFreeSpaceExpansionRoom.java, SearchTreeObject.java, AutorouteEngine.addCompleteRoom/removeCompleteExpansionRoom | ~150 | **DONE 2026-09-19 (WP-10 Inc A)** |
| 6.0b ShapeSearchTree.completeShape (WP-10 Inc B, blocker #6) | ShapeSearchTree.h/cpp, TileShape, LineSegment, BasicBoard/RoutingBoard | ShapeSearchTree.completeShapeUnlocked/restrainShape/divideLargeRoom (Java ~620 desc / ~900) | ~300 | **DONE 2026-09-22 as board API + 9 gtests**; engine rewiring DONE as **WP-10 Inc C** (complete_expansion_room via complete_shape) |
| 6.1 ShapeSearchTree90Degree rooms | ShapeSearchTree.cpp | ShapeSearchTree90Degree.java (287) | ~287 |
| 6.2 ShapeSearchTree45Degree rooms | ShapeSearchTree.cpp | ShapeSearchTree45Degree.java (457) | ~457 |

## Phase 7: Board Model Stubs (Parallel)

| Task | File | Port From | Est. Lines | Status |
|------|------|-----------|------------|--------|
| 7.1 additional_update_after_change() | RoutingBoard.cpp | RoutingBoard.java | ~30 | DONE (no-op — no autoroute engine wired yet) |
| 7.2 opt_changed_area() | RoutingBoard.cpp | RoutingBoard.java | ~30 | DONE (delegates to PullTightAlgo) |
| 7.3 combine_traces() | RoutingBoard.cpp | RoutingBoard.java | ~15 | DONE |
| 7.4 insert_via() trace splitting | RoutingBoard.cpp | BasicBoard.java | ~20 | DONE (calls split_traces on padstack layers) |
| 7.5 get_outline() | RoutingBoard.cpp | BasicBoard.java | ~10 | DONE |
| 7.6 contains_trace_tails/get_trace_tail | RoutingBoard.cpp | BasicBoard.java | ~30 | DONE |

Plus: `BoardOutline` restored to the build (fixed `geometry::planar` namespaces, `Item` base ctor, `split_to_convex()`-based `get_tile_shape`). Tests: `src/board/RoutingBoard_stubs_test.cpp` (8 cases).

---

## Summary

| Phase | Tasks | New Files | Est. Lines | Effort | Dependencies |
|-------|-------|-----------|------------|--------|--------------|
| 1: Trace Ops | 5 | 0 | ~525 | 3-4 days | None |
| 2: Connectivity | 5 | 0 | ~600 | 3-4 days | None |
| 3: Maze Search | 12 | 5 | ~2500 | 10-14 days | Phase 1+2 |
| 4: Connection | 4 | 2 | ~1000 | 4-5 days | Phase 3 |
| 5: Batch Router | 6 | 1 | ~800 | 4-5 days | Phase 3+4 |
| 6: Search Trees | 2 | 0 | ~744 | 3-4 days | None |
| 7: Board Stubs | 6 | 0 | ~135 | 2-3 days | None |
| **Total** | **40** | **8** | **~6304** | **29-39 days** | |
