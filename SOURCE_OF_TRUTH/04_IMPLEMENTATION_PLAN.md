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
| 1.3 ShoveTraceAlgo::spring_over() | ShoveTraceAlgo.cpp | ShoveTraceAlgo.java:375-550 | ~175 |
| 1.4 ShoveTraceAlgo::insert() board mutation | ShoveTraceAlgo.cpp | ShoveTraceAlgo.java | ~100 |
| 1.5 ShoveTraceAlgo::check() via/direction | ShoveTraceAlgo.cpp | ShoveTraceAlgo.java | ~100 |

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

| Task | File | Port From | Est. Lines |
|------|------|-----------|------------|
| 3.1 SortedRoomNeighbours (base) | NEW .h/.cpp | SortedRoomNeighbours.java | ~200 |
| 3.2 Sorted45DegreeRoomNeighbours | NEW .h/.cpp | Sorted45DegreeRoomNeighbours.java (810) | ~810 |
| 3.3 SortedOrthogonalRoomNeighbours | NEW .h/.cpp | SortedOrthogonalRoomNeighbours.java (482) | ~482 |
| 3.4 DrillPage | NEW .h/.cpp | DrillPage.java (189) | ~189 |
| 3.5 MazeShoveTraceAlgo | NEW .h/.cpp | MazeShoveTraceAlgo.java (316) | ~316 |
| 3.6 MazeListElement | NEW .h/.cpp | MazeListElement.java (130) | ~130 |
| 3.7 MazeSearchAlgo::init() rewrite | MazeSearchAlgo.cpp | MazeSearchAlgo.java:886-976 | ~90 |
| 3.8 expand_to_room_doors() rewrite | MazeSearchAlgo.cpp | MazeSearchAlgo.java:298-469 | ~170 |
| 3.9 expand_to_other_layers() | MazeSearchAlgo.cpp | MazeSearchAlgo.java:748-880 | ~130 |
| 3.10 check_ripup() | MazeSearchAlgo.cpp | MazeSearchAlgo.java:1003-1085 | ~80 |
| 3.11 shove_trace_room() | MazeSearchAlgo.cpp | MazeSearchAlgo.java:1093-1144 | ~50 |
| 3.12 Fix distance calculation | MazeSearchAlgo.cpp:174 | Weighted distance | ~30 |

## Phase 4: Connection Location & Insertion (Week 5-6)

| Task | File | Port From | Est. Lines |
|------|------|-----------|------------|
| 4.1 LocateFoundConnectionAlgo45Degree | NEW .h/.cpp | Java (238) | ~238 |
| 4.2 LocateFoundConnectionAlgoAnyAngle | NEW .h/.cpp | Java (363) | ~363 |
| 4.3 Complete LocateFoundConnectionAlgo | LocateFoundConnectionAlgo.cpp | Java (427) | ~200 |
| 4.4 Complete InsertFoundConnectionAlgo | InsertFoundConnectionAlgo.cpp | Java | ~100 |

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
| 6.1 ShapeSearchTree90Degree rooms | ShapeSearchTree.cpp | ShapeSearchTree90Degree.java (287) | ~287 |
| 6.2 ShapeSearchTree45Degree rooms | ShapeSearchTree.cpp | ShapeSearchTree45Degree.java (457) | ~457 |

## Phase 7: Board Model Stubs (Parallel)

| Task | File | Port From | Est. Lines | Status |
|------|------|-----------|------------|--------|
| 7.1 additional_update_after_change() | RoutingBoard.cpp | RoutingBoard.java | ~30 | DONE (no-op — no autoroute engine wired yet) |
| 7.2 opt_changed_area() | RoutingBoard.cpp | RoutingBoard.java | ~30 | TODO |
| 7.3 combine_traces() | RoutingBoard.cpp | RoutingBoard.java | ~15 | DONE |
| 7.4 insert_via() trace splitting | RoutingBoard.cpp | BasicBoard.java | ~20 | TODO |
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
