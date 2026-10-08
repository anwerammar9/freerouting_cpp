# Autoroute Module - Migration Status

**Module**: `src/autoroute/`
**CMake Target**: `freerouting_autoroute`
**Java Files**: 35 | **C++ Impls compiled**: 26 | **Tests registered**: 16 (gtest, via FetchContent)

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
| SortedRoomNeighbours | SortedRoomNeighbours_test | Base any-angle room neighbour collection + 1-D/2-D door creation; `calculate` uses `select_calculation_mode` to dispatch to the 45° (8 directions) or orthogonal (4 directions) variant (WP-10 Inc E/F/G) |
| Sorted45DegreeRoomNeighbours | Sorted45DegreeRoomNeighbours_test | 45° room completion: touching sides, door creation, room enlargement, new incomplete rooms (WP-10 Inc F) |
| SortedOrthogonalRoomNeighbours | SortedOrthogonalRoomNeighbours_test | 90° room completion: 4 touching sides, door creation, room enlargement, new incomplete rooms (WP-10 Inc G) |
| CompleteFreeSpaceExpansionRoom | CompleteFreeSpaceExpansionRoom_test | implements `board::SearchTreeObject` (Storable: tree_shape_count==1, get_tree_shape=IntBox, compare_to by reversed id / -1 for non-rooms; is_obstacle/is_trace_obstacle true; shape_layer=layer) + net_dependent flag + remove_from_tree (WP-10 Inc A) |
| FoundConnectionInserter | FoundConnectionInserter_test | Port of Java `autoroute/path/FoundConnectionInserter.java`: `get_instance`, `insert_trace` (spring-over + forced polyline insert with the stub-tail cleanup), `insert_neckdown`, `try_neck_down`, `insert_fanout_micro_neckdown`, `insert_via` (via-mask search + `ForcedViaAlgo::check`), the fanout diagnostics, and the two `connect_to_trace` calls on `first_corner`/`last_corner` (WP-10 Inc P). Replaces the deleted `InsertFoundConnectionAlgo` stub. |
| ItemAutorouteInfo | ItemAutorouteInfo_test | Item routing state; `get_expansion_room` (WP-10 Inc D), `reset_doors` |
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

## WP-10 Increment B porting notes (2026-09-22): `ShapeSearchTree::complete_shape()`

Ported `ShapeSearchTree.completeShapeUnlocked()` as a tested **board-level** API (board lib cannot depend on
autoroute; the engine rewiring is the next iteration). New public structs in `ShapeSearchTree.h`:

- `CompleteShapeInput { const TileShape* shape; const TileShape* contained_shape; int layer; int net_no;
  SearchTreeObject* ignore_object; const TileShape* ignore_shape; }` — `shape == nullptr` means "whole board box".
- `CompleteShapePiece { unique_ptr<TileShape> shape; unique_ptr<TileShape> contained_shape; }`.

`complete_shape()` flow (mirrors Java):
1. `start_shape = board.getBoundingBox()`, intersected with `input.shape` when present; pushed as one piece.
2. Deterministic leaf walk over the tree using a local `std::vector<TreeNode*>` stack. **Documented deviation**:
   Java returns empty when `root == null`; here an empty tree completes the room to the whole board box
   (matches expected empty-board drill behaviour — the old engine shortcut).
3. Leaves sorted by `compare_to` before processing (Java `Collections.sort(overlappingLeaves)`).
4. Obstacle loop: skip objects that are not a trace obstacle on the input net / different layer / the ignore
   object. Compute `intersection_of_two_shapes`; if 2D and it is a free-space room fully inside the ignore
   shape, ignore (reuse); else `restrain_shape` into `new_result` and union the pruned
   `IntOctagon` bounding shape.
5. `divide_large_room(board_box, result)` before returning.

Private helper ports with comments on fidelity:
- `intersection_of_two_shapes`: `to_Simplex()` both, `Simplex::intersection`, `simplify()`.
- `shape_contains`: exact containment via `RegularTileShape::contains(RegularTileShape)` when both operands
  are regular shapes; otherwise conservative `false` (only used to decide free-space-room reuse).
- `restrain_shape`: obstacle → `Simplex`; iterate `border_line(i)` segments where the room shape is
  intersected in its interior; pick cut line by max `distance_to_the_left` (Java `distanceToTheLeft`,
  verified byte-identical to C++ `TileShape::distance_to_the_left`); intersect room with the opposite
  halfplane; else-branch looks for a collinear cut, splits contained shape, recurses on the rest.
- `divide_large_room`: sections via `IntBox::divide_into_sections` when `width/height > max_section_width_`
  (Java `divideLargeRoom`, Production board only).

Supporting ports:
- `TileShape::is_intersected_interior_by(const LineSegment&)` + 3-arg overload (TileShape.java:923-1022).
- `LineSegment(const TileShape&, int p_no)` constructor (mirrors the PolylineShape ctor).
- `BasicBoard::get_bounding_box()` virtual (defined in the .cpp — header only forward-declares IntBox), plus
  `RoutingBoard` by-value override. Fixed a downstream break in `core/scoring/BoardStatistics.cpp` that bound
  the by-value result to `const auto&`.
- `SearchTreeObject::is_free_space_room()` virtual (default `false`), overridden `true` in
  `CompleteFreeSpaceExpansionRoom`.

Testing notes:
- `ShapeSearchTree_complete_shape_test.cpp` (9 gtests). `FakeObstacle` must retain the leaves the tree
  hands to `set_search_tree_entries` (the tree only references them; a no-op impl leaves a dangling root →
  segfault — the same Storable contract the real `CompleteFreeSpaceExpansionRoom` already honours).
- Invariant-based assertions used where exact piece geometry is hard to hand-derive (pieces must not overlap
  the obstacle interior, contained-shape corners covered).
- A free-space-room obstacle with the *contained* shape strictly inside it yields an empty result — this is
  faithful Java behaviour (`restrainShape` finds no cut line), not a bug.

## WP-10 Increment C porting notes (2026-09-22): engine rewiring onto `complete_shape`

`AutorouteEngine::complete_expansion_room` is now a port of `AutorouteEngine.completeExpansionRoom()` running
through `ShapeSearchTree::complete_shape()`, instead of the interim empty-board inflation (was "must be exactly 1
room = the raw box").

Flow (mirrors Java):
1. Door scan: first door with `get_dimension() == 2` to a neighbour `CompleteFreeSpaceExpansionRoom` supplies
   `ignore_object` (the other room) and `from_door_shape` (`door.get_shape()`).
2. `CompleteShapeInput` built with `complete_shape`'s bounded start/contained shapes, the room layer, `net_no_`
   (obstacle filtering in the leaf walk) and the ignore fields.
3. `remove_incomplete_expansion_room(p_room)` — Java `removeIncompleteExpansionRoom()`, but only the engine-list
   erase (no neighbour-tool removal, C++ rooms have no doors); no-op for the stack-allocated rooms the drill
   passes in.
4. First/rest loop over the returned 2-D pieces: the first is added directly, every further piece is re-run
   through `complete_shape` (the Java recalc — the first completion may have changed its shape such that it now
   intersects the others); every resulting 2-D piece becomes a `CompleteFreeSpaceExpansionRoom(layer,
   generate_room_id_no(), piece.shape->bounding_box())` inserted into the engine search tree (Java
   `addCompleteRoom()`).

Starting shape mapping (documented deviation, keeps empty-board drill behaviour identical):
- dimension ≥ 2 room: `CompleteShapeInput.shape = contained = room box` — the completion cannot grow beyond the
  room's own free space. Java's whole-board start would split an *empty* board into 4 sections via
  `divideLargeRoom`; bounding by the room box keeps "exactly one room" for the empty-board drill cases, while
  still splitting around obstacles that the room box intersects.
- sub-2-D room (the unit `TileShape::get_instance(location)` drill shape): start = `room_shape.offset(1.0)`
  (small 3x3 box), contained = the original degenerate point — same observable result as the old inflation, but
  routed through `complete_shape`.

Testing notes:
- `CompleteFreeSpaceExpansionRoomTest.RoomIsRestrainedAroundObstacle`: whole-board room (0,0,10000,10000) vs a
  net-2 wall (4000,0,6000,10000) → non-empty completion, every 2-D piece must avoid the wall interior
  (Simplex intersection dimension < 2), at least one piece differs from the raw box. The old passthrough
  implementation returns the raw box (overlapping the wall) and fails.
- `ctest 482/482` (6 CompleteFreeSpaceExpansionRoom tests, 9 complete_shape tests) + `run_tests` PASSED.

## WP-10 Increment D: net-connectable room doors (2026-09-22)

Port of the door-mechanics prerequisites for the room/door maze search (`ItemAutorouteInfo.getExpansionRoom`,
`ObstacleExpansionRoom.createOverlapDoor`, `CompleteFreeSpaceExpansionRoom.calculateTargetDoors`,
`AutorouteEngine.removeAllDoors`).

- `ItemAutorouteInfo::get_expansion_room(p_index, ShapeSearchTree*)` — was a stub returning nullptr. Room array
  is sized to `item->tree_shape_count(tree)`, resized preserving already created rooms (`std::vector::resize(count,
  nullptr)`), out-of-range index logs an `FRLogger` warning and returns nullptr, lazy creation of
  `new ObstacleExpansionRoom(item, index, tree)`. `reset_doors()` walks the array; the destructor frees the rooms.
- `ObstacleExpansionRoom` — new tree-based constructor `(Item*, int, ShapeSearchTree*)` computing layer
  (`item->shape_layer(index)`), shape (bounding box of `item->get_tree_shape(tree, index)`) and the Java room id
  `(item->get_id() << 10) | index`; the 4-arg constructor previously hard-coded room id 0 and is now fixed to the
  same formula.
- `ObstacleExpansionRoom::create_overlap_door(ObstacleExpansionRoom*)` — port of Java `createOverlapDoor()`:
  existing-door guard (`door_exists`) → both items `is_routable()` → `shares_net` → for rooms of the same item the
  item must be a `PolylineTrace` and the shape indices consecutive (`|index_a - index_b| == 1`) →
  `new ExpansionDoor(this, other, 2)` stored on both sides.
- `CompleteFreeSpaceExpansionRoom::calculate_target_doors(TreeEntry, net_no, ShapeSearchTree*)` — port of Java
  `calculateTargetDoors()`. `set_net_dependent()` is called unconditionally first (Java does the same). The Java
  `instanceof Connectable` guard is mapped to `board::Item` (documented divergence — the C++ `Connectable`
  interface is a dead draft implemented by no real item): the entry object must be a trace/via/pin or
  `ConductionArea`, must `contains_net(net_no)`, and its tree shape must intersect the room shape
  (`IntBox::intersects(tree_shape->to_Simplex())`). The connection shape is approximated by the item's tree shape
  instead of Java's exact `Connectable::getTraceConnectionShape` (drills return a center point there). Creates a
  `TargetItemExpansionDoor` and stores it via `add_target_door`.
- `TargetItemExpansionDoor` — ctor now takes the `ShapeSearchTree*` and computes the real door shape
  `room.get_shape().intersection(item tree shape bounding box)`; `get_shape()` overridden (was inheriting
  `EMPTY` from the base door shape computation of two rooms, the second room being null); dimension now 2
  (Java `getDimension()==2`); `item` stays public — `MazeSearchAlgo::is_destination` reads `target_door->item`.
- `AutorouteEngine::remove_all_doors(ExpansionRoom*)` — public port of Java `removeAllDoors()`; wired as the
  first step of `remove_incomplete_expansion_room` (Java `removeIncompleteExpansionRoom()` line 368). Because the
  C++ incomplete rooms are owned by the engine's `unique_ptr` vector (Java relies on the garbage collector), the
  door-transitive walk collects every door-connected incomplete free-space room and destroys them only after the
  whole walk, so no door iteration touches a deleted room; the walk removes each door from both sides
  (`other->remove_door(door)`, then `current->clear_doors()`).
- Deliberately NOT ported in this increment (all closed later): the Java `SortedRoomNeighbours` door-*creation*
  family (base class in WP-10 Inc E, 45° variant in Inc F, `SortedOrthogonalRoomNeighbours` in Inc G) and the
  neighbour-split half of Java `removeCompleteExpansionRoom` (WP-10 Inc H) — these needed the `TileShape` +
  `contained_shape` room generalization of Increment E.

## WP-10 Increment E: `SortedRoomNeighbours` (base) — 2026-09-26

Port of `java/app/freerouting/autoroute/expansion/SortedRoomNeighbours.java` (806 lines) as
`src/autoroute/SortedRoomNeighbours.h` / `.cpp`. This is the class that actually *creates* the doors of a room
shape; the 45°/orthogonal subclasses and the room-enlargement half are still missing.

### Ported

- `SortedRoomNeighbour` (Java inner class) — `compare_to()` in four stages (side number, distance to the side
  corner, distance to the last corner, neighbour-corner line direction for the "short 1-dim. touch at a fork link"
  tie-break), the Java `c_dist_tolerance` of 1, and the lazily precalculated `first_corner()`/`last_corner()`
  (`std::optional<IntPoint>` mirrors the Java `precalculatedFirstCorner` cache).
  **Deviation:** when the tie-break resolves to 0 the C++ comparator falls back to the search tree object ids
  (`SearchTreeObject::get_id()`), because the Java `TreeSet` would otherwise keep an arbitrary one of several
  geometrically equal neighbours.
- `calculate_neighbours(ExpansionRoom*, net_no, ShapeSearchTree*, room_id_no)`:
  - creates a `CompleteFreeSpaceExpansionRoom` when the from room is an `IncompleteFreeSpaceExpansionRoom`
    (borrowed by the result, handed to the caller via `release_completed_room()`), uses the room itself for an
    `ObstacleExpansionRoom`, warns and returns `nullptr` for any other type (Java throws);
  - walks `overlapping_tree_entries` sorted by `(get_id(), shape_index)` for determinism, skipping the room itself;
  - own-net objects that are no trace obstacle go into `own_net_objects` (the door calculation for them is
    delayed until the room shape stops changing);
  - intersection dimension 2 → `ObstacleExpansionRoom::create_overlap_door()` for routable items (a free-space
    room overlapping a tree object only produces a trace log, as in Java);
  - dimension 1 → `TileShape::touching_sides`, a sorted neighbour entry, then `insert_door_ok` and a
    dimension-1 `ExpansionDoor` added to both rooms;
  - dimension 0 → `equals_corner`/`contains_on_border_line_no` corner classification with the Java preference
    for the *previous* border line of the neighbour corner;
  - dimension < 0 → debug log and skip.
- `insert_door_ok` overloads: `(ExpansionRoom*, ExpansionRoom*, const TileShape&)` — `door_exists` guard, both
  obstacle rooms ⇒ `shares_net`, both free-space rooms ⇒ true, mixed ⇒ fork-parallelity check on the door line;
  `(ObstacleExpansionRoom*, const Line&)`. Java's nullable door line is `std::optional<Line>`; a disengaged one
  is rejected with a warning (a point-shaped door has no door line). `Polyline::arr` holds the closing start
  line at index 0 and the closing end line last, so `arr[room_index + 1]` is the trace line following the room.
- `calculate_incomplete_rooms_with_empty_neighbours(ObstacleExpansionRoom*, AutorouteEngine*)`: per border line
  of the room shape that passes `insert_door_ok`, a half-plane room shape (`line.opposite()`) plus the
  intersection with the room shape as contained shape are registered with the engine and connected by a
  dimension-1 door. Null arguments return early (Java has no such guard).

### Supporting ports required by the above

- `ExpansionRoom::get_shape_tile()` — the Simplex view of the room shape (Java returns a `TileShape`);
  `CompleteFreeSpaceExpansionRoom`/`ObstacleExpansionRoom`/`IncompleteFreeSpaceExpansionRoom` gained Simplex
  constructors, `set_shape_tile()` and the override. `get_tree_shape()` therefore returns the exact Simplex
  instead of the old `IntBox` (the room bounding box is still available through `get_shape()`).
- `SearchTreeObject::get_id()` — deterministic comparator tie-break (test fakes updated).
- `TileShape::equals_corner`, `TileShape::prev_no`, `TileShape::next_no`, `TileShape::touching_sides`, and the
  static `TileShape::intersection_of` (which `ShapeSearchTree::intersection_of_two_shapes` now delegates to).

### Divergence found and fixed while porting

`TileShape::intersection_of` simplified *every* result, but Java `Simplex.intersection()` does not simplify.
`Simplex::simplify()` only tests whether the border lines are 45° multiples and whether the corners are
bounded, so a valid 0/1-dimensional intersection (for example an octagon room shape intersected with the
opposite half-plane of its own border line, which is exactly the empty-neighbour case) was converted into a
degenerate `IntOctagon` reporting `dimension() == -1` and `is_empty() == true`. The result was that
`calculate_incomplete_rooms_with_empty_neighbours` silently created no rooms at all. Only 2-dimensional results
are simplified now; sub-2-D results stay `Simplex`, which is what Java returns.

### Testing notes

- `SortedRoomNeighbours_test.cpp` (15 tests): empty board, unexpected room type, 1-D neighbour with door on both
  sides and ownership release, 2-D free-space overlap ignored, point touch with corner flags and no door,
  counterclock sorting across three sides, own-net objects vs. trace-obstacle neighbours, obstacle-room 2-D
  overlap door for a same-net item / ignored other net / 1-D door to a same-net item room, all `insert_door_ok`
  branches (net check, free-space pair, point-shaped door, existing door), fork-parallelity for both trace
  segments, incomplete rooms for empty neighbours (2 parallel border lines of the octagon room shape) incl.
  `remove_all_doors` cleanup, null-argument guard, and corner accessor caching.
- **ctest 515/515 + run_tests PASSED.** The 4 previously failing `RatsNestTest` cases are fixed as well: they
  modelled unrouted pins as isolated vias, while Java `NetIncompletes` skips `item->is_tail()` and an isolated
  via is a tail (SMD pins are `DrillItem`s without that restriction).
- Gotcha: `ExpansionDoor::other_room` is overloaded on `ExpansionRoom*` and `CompleteExpansionRoom*`; for an
  `ObstacleExpansionRoom*` argument the *complete*-room overload wins and returns `nullptr` for an incomplete
  neighbour, so the base overload has to be selected explicitly.

Testing notes:
- `ItemAutorouteInfo_test.cpp` (10 tests): room creation/caching/out-of-range; per-shape rooms for a two-segment
  trace; overlap doors (same net ↔ created, different nets ↔ denied, same trace consecutive ↔ created,
  non-adjacent ↔ denied), each with the existing-door guard; target doors for an overlapping same-net item incl.
  door shape and dimension, and skips for disjoint rooms / other nets / non-item Storables; engine
  `remove_all_doors` clearing both door sides and `complete_expansion_room` removing the registered incomplete
  room from the database.
- **ctest 495/499** (4 failing tests are the pre-existing untracked `DesignRulesChecker_test.cpp` from the
  in-progress DRC rework — no autoroute dependency) + `run_tests` PASSED.

## WP-10 Increment F: `Sorted45DegreeRoomNeighbours` — 2026-09-27

Port of `java/app/freerouting/autoroute/expansion/Sorted45DegreeRoomNeighbours.java` (810 lines) as
`src/autoroute/Sorted45DegreeRoomNeighbours.h` / `.cpp`. 45° is the default route angle of the Java autorouter,
so this is the variant the maze search needs.

### Ported
- `Sorted45DegreeRoomNeighbour`: the 8 border lines (0 bottom, 1 lower-right diagonal, 2 right, 3 upper-right
  diagonal, 4 top, 5 upper-left diagonal, 6 left, 7 lower-left diagonal), `calculate_first_touching_side()` /
  `calculate_last_touching_side()` with the Java `equals_corner` + `corner_distance` rules, and the
  `edge_interior_touches_obstacle` bit vector filled during the tree walk
- `calculate_neighbours()`: completed room creation, deterministic entry order, immediate
  `calculate_target_doors` for own-net free-space entries, 2-D overlap doors for obstacle rooms, dimension-1
  doors for the touching neighbours
- `add_sorted_neighbour()`: counterclock insert with the Java `TreeSet` duplicate drop; a neighbour that
  contains the room shape (`last_touching_side < 0`) is dropped
- `remove_not_touching_border_lines()` + `try_remove_edge_line()`: free room sides are set to `±CRIT_INT` and
  re-normalized, the result is completed with `ShapeSearchTree::complete_shape` (largest 2-dimensional door as
  `ignore_shape`) and applied only when the area grows (endless-loop guard)
- `calculate_new_incomplete_rooms()` (free space, 8 `first_touching_side` cases),
  `calculate_new_incomplete_rooms_for_obstacle_expansion_room()`,
  `calculate_edge_incomplete_rooms_of_obstacle_expansion_room()`
- `calculate()`: Java recursion — `try_remove_edge_line` restart, `complete_shape` fallback for a completed from
  room, retry loop over the new incomplete rooms, ownership release of the completed room

### Dispatch

`SortedRoomNeighbours::calculate()` selects the 45° implementation when the autoroute search tree has
8 bounding directions, using the new public `datastructures::ShapeTree::get_bounding_directions()`.
A 4-direction tree dispatches to `SortedOrthogonalRoomNeighbours` (WP-10 Inc G).
`AutorouteEngine::add_complete_room` needed no change because it already calls the base `calculate`.

### Divergences

- C++ `IntOctagon::is_empty()` treats degenerate octagons as empty, while Java compares against the
  `IntOctagon.EMPTY` singleton; C++ `dimension()` therefore returns -1 for 1-dimensional shapes where Java
  returns 1. The port uses local `java_is_empty()` / `java_dimension()` helpers, so the Java branch behaviour
  is preserved without changing the shared geometry class.
- The C++ `ExpansionDoor` has no own door shape, so `try_remove_edge_line` passes the intersection box of the
  door rooms as the `ignore_shape` of `complete_shape`.
- Java `ShapeSearchTree.completeShape()` returns an empty result for a tree without a root; the C++
  `complete_shape()` uses the whole board, so the empty-tree case is handled explicitly in the port.

### Testing notes

- `Sorted45DegreeRoomNeighbours_test.cpp` (11 tests): empty tree, unexpected room type, left neighbour with
  first/last touching side 5/7 and the interior-touch flag 6 plus its dimension-1 door, containing neighbour
  (door created, neighbour dropped), non-overlapping neighbour skipped, counterclock sorting of the 4 axis-side
  neighbours, room enlargement by edge removal, second enlargement rejected, new incomplete rooms between
  neighbours (2-dimensional doors, bounded by the board), `calculate` enlargement + completion, and the
  base-class dispatch to the 45° implementation.
- Verified values for the 30-unit cut-square room: the left neighbour shares the edge `x == 0` between
  `y == 30..70`, `first_touching_side == 5`, `last_touching_side == 7`, only side 6's interior touches.
- Note: rooms inserted into a test search tree must outlive the tree (the tree keeps pointers to them).
- **ctest 526/526** + `run_tests` PASSED.

## WP-10 Increment H: `AutorouteEngine` room-database housekeeping — 2026-09-27

Port of the three remaining room-database methods of `java/app/freerouting/autoroute/maze/AutorouteEngine.java`
(the C++ engine is the post-refactor counterpart of the Java `MazeSearchAlgo`).

### Ported
- `remove_complete_expansion_room()` (Java `removeCompleteExpansionRoom`, `AutorouteEngine.java:377-412`), now
  public like in Java, with the neighbour-split half: for every door the door is removed from the neighbour, the
  room/neighbour intersection is computed from the exact Simplex shapes, and a **1-dimensional** intersection
  creates a new incomplete free space room — shape = half plane behind the neighbour's `touching_sides` border
  line (`border_line().opposite()`), contained shape = the intersection, registered with
  `add_incomplete_expansion_room()` and connected to the neighbour by a dimension-1 `ExpansionDoor` on both
  sides. Afterwards `remove_all_doors()`, `remove_from_tree()`, the database erase (with the Java warning when
  the room is not registered) and the engine-owned `unique_ptr` release
- `complete_neighbour_rooms()` (Java `completeNeighbourRooms`, `AutorouteEngine.java:567-592`): completes the
  incomplete free space neighbours and restarts the door iteration on the updated door list; an obstacle room
  neighbour gets its doors calculated only once (`all_doors_calculated()` / `set_doors_calculated()`)
- `get_first_incomplete_expansion_room()` (Java `getFirstIncompleteExpansionRoom`)

### Gotchas and divergences
- `ExpansionDoor::other_room()` **must** be called through an `ExpansionRoom*`: the `CompleteExpansionRoom*`
  overload `dynamic_cast`s the result and therefore returns `nullptr` for a door to an incomplete room. This is
  the C++ counterpart of the explicit `(ExpansionRoom) room` cast in the Java `completeNeighbourRooms()` code,
  and the reason the naive port silently completed nothing.
- Java additionally calls `drillPageArray.invalidate(roomShape)`. The C++ engine does not own a `DrillPageArray`
  (wiring one needs `rules::BoardRules::get_default_via_diameter()`, still missing), so that line is not ported.
- A 1-dimensional room intersection is created as an `IncompleteFreeSpaceExpansionRoom` with a half-plane
  shape and the segment as contained shape — the same pattern as
  `SortedRoomNeighbours::calculate_incomplete_rooms_with_empty_neighbours()`.

### Testing notes

- `AutorouteEngine_test.cpp` (8 tests): first incomplete room `nullptr` on an empty database / added /
  cleared, neighbour split creates the half-plane incomplete room with a dimension-1 door and empties the search
  tree, 2-D overlap neighbour skipped, `nullptr` room ignored, unregistered room handled (warning path),
  `complete_neighbour_rooms()` completes an incomplete free space neighbour into the tree,
  `complete_neighbour_rooms()` calculates an obstacle neighbour's doors only once, `nullptr` room ignored.
- The half-plane result relies on the engine's default 45° search tree: the `touching_sides` border line of a
  45° room yields a diagonal half plane.
- **ctest 547/547** + `run_tests` PASSED.

## WP-10 Increment I — `MazeSearchAlgo` room/door expansion core (2026-09-27)

Port of `java/app/freerouting/autoroute/maze/MazeSearchEngine.java` (1258 lines) into
`src/autoroute/MazeSearchAlgo.h/.cpp` (247 + 1175 lines). The 227-line grid-based A* is
**deleted**, not patched.

### Ported surface
| C++ | Java |
|-----|-------|
| `MazeSearchAlgo::get_instance(start, dest, engine, ctrl, stoppable)` | `MazeSearchEngine.getInstance` |
| `init()` | same, incl. the tie-pin reduction (WP-10 Inc T) |
| `find_connection()` / `occupy_next_element()` | same |
| `expand_to_room_doors()` | `expandToRoomDoors` |
| `expand_to_target_doors()` | `expandToTargetDoors` |
| `expand_to_door()` / `expand_to_door_section()` | same |
| `door_is_small()` / `room_shape_is_thick()` | same |
| `check_neck_down_at_dest_pin()` | same |
| `segment_projection()` | `segmentProjection` |
| `describe_expandable()` / `describe_room()` | same (logging) |
| `MazeSearchAlgo::Result::destination_door` is an `ExpandableObject*` | `Result.destinationDoor` |

`MazeListElement` (the second class of `MazeSearchElement.h/.cpp`) is the Java class:
`ExpandableObject* door`/`backtrack_door`, `CompleteExpansionRoom* next_room`, `FloatLine
shape_entry`, `ripup_cost`, and `compare_to()` with the Java tie-break chain
**sorting value → expansion value → door id → section number**. The queue is a
`std::set`, the counterpart of the Java `TreeSet`: equal elements collapse into one entry
and `begin()` is the cheapest element.

### New prerequisite APIs
`IntPoint::get_id()`, `IntBox::get_id()`, `ExpandableObject::get_id()` (pure virtual),
`ExpansionDoor::get_id()` (order-independent room-id pair), `TargetItemExpansionDoor::get_id()`
and `is_destination_door()`, `ExpansionDrill::get_id()`, `DrillPage::get_id()`,
`IncompleteFreeSpaceExpansionRoom::get_room_id()` (`31 * shape_id + layer`),
`TileShape::min_width()`/`max_width()` (dispatched to IntBox/IntOctagon/Simplex),
`AutorouteControl::bend_costs`, `RouterSettings::get_bend_cost(int)`,
`Pin::min_width(int)`, `Pin::get_trace_neckdown_halfwidth(int)`,
`AutorouteEngine::get_maintain_database()`, `AutorouteEngine::get_rooms_with_target_items()`
(room-id ordered), and a const `CompleteExpansionRoom::target_doors()` overload.

### Gotchas and divergences
- **Java compares `FloatPoint` by reference** in `segmentProjection` (`firstProjection.a ==
  toSegment.a`). The only way that is true is the endpoint reuse inside
  `FloatLine::segment_projection`, so the C++ port uses value equality, which expresses the
  same intent and is the only sane translation for a value type.
- `TileShape` has no `bounding_octagon()`. `door_is_small()` derives the octagon from the
  bounding box, which is identical for the `IntBox`-based C++ doors.
- The C++ `Connectable` interface is a dead draft implemented by no real item. As in
  `calculate_target_doors()`, "connectable" means `is_trace() || is_via() || is_pin() ||
  ConductionArea`, and the connection shape is the item tree shape.
- `FloatLine` has no default constructor (Java has no no-arg `FloatLine` either), so
  `MazeListElement` is not default-constructible: `occupy_next_element()` holds the popped
  element in a `std::optional` and `expand_to_door()` holds the new shape entry in one too.
- The C++ `AutorouteControl` default constructor leaves the per-layer arrays empty, where Java
  would raise an exception. `MazeSearchAlgo` therefore reads them through bounds-checked
  accessors (`is_layer_active`, `compensated_trace_half_width_for_layer`,
  `bend_cost_for_layer`, `trace_costs_for_layer`) that log and fall back to the neutral value.
- `rules::AngleRestriction::NINETY_DEGREE` is spelled `RIGHT` in the C++ rules module.

### Deliberately deferred (documented in the code, never silently stubbed)
- `MazeExpansionEngine` — `expand_to_other_layers`, `expand_to_drill_page`,
  `expand_to_drill`, `expand_to_drills_of_page`. Until then a drill reached from a
  non-drill door is not expanded.
- `MazeRipupResolver` — ported in WP-10 Increment J.
- `MazeTraceShover` — ported in WP-10 Increment K.
- `RoutingBoard::check_forced_trace_polyline` — the thin-room line check in
  `expand_to_target_doors` is logged and skipped.
- ~~`ShapeSearchTree::reduce_trace_shape_at_tie_pin`~~ — ported in WP-10 Increment T (2026-10-06).
- Fanout: the `mazeExpansionList.add` escape-length filter needs
  `AutorouteControl::fanout_start_pin_center`, which does not exist in C++ yet. The Java seed of
  the RNG from `ctrl.ripupCosts` is ported in Increment J (`MazeRandom`).

### Current end-to-end status
The target-door calculation **is** wired: `calculate_target_doors()` is called from all three
`Sorted*RoomNeighbours::complete_expansion_room()` (`SortedRoomNeighbours.cpp:648`,
`Sorted45DegreeRoomNeighbours.cpp:435`, `SortedOrthogonalRoomNeighbours.cpp:336`) and unit-tested —
task 3.14 was verified done in WP-10 Increment J, the old blocker text was stale. Phase 4.4 landed in WP-10 Increment P: `FoundConnectionInserter` is ported
(`FoundConnectionInserter.h/.cpp`, the stale `InsertFoundConnectionAlgo` stub is deleted) and the
`RoutingBoard` forced-trace prereqs (`check_forced_trace_polyline`, `insert_forced_trace_polyline`,
`insert_forced_trace_segment`, `connect_to_trace`) are in. What still blocks a maze route from
reaching the board is the `AutorouteEngine::autoroute_connection()` rewiring off the legacy grid
A* — that is now the only remaining wiring gap.

### Testing notes
- `MazeSearchAlgo_test.cpp` (12 tests): `Result` defaults, `ALREADY_RIPPED_COSTS`, construction
  state, control round-trip, `get_instance` rejecting a missing engine, `get_instance` failing
  without start doors, `init()` failing without an engine, `find_connection()` without an
  expansion list, `occupy_next_element()` on an empty list, and `reconstruct_path()` following a
  backtrack chain / stopping on a self-reference.
- `MazeSearchElement_test.cpp` (12 tests): element defaults, `reset()`, `reset_section_array`,
  the full `compare_to` tie-break chain, the `TreeSet` duplicate collapse in a `std::set`, the
  sorting order, and insertion-order-independent determinism.
- `IncompleteFreeSpaceExpansionRoomTest.Construction` was updated: it asserted the old
  placeholder `get_room_id() == 0`, which the ported Java `getId()` (`31 * shape_id + layer`)
  replaces.
- **ctest 572/572** + `run_tests` PASSED.

## WP-10 Increment J — `MazeRipupResolver` (2026-09-27)

### Ported surface
- NEW `src/autoroute/MazeRipupResolver.h/.cpp` (74 + 385 lines) — the 269-line Java
  `MazeRipupResolver` with `check_ripup()`, `check_leaving_ripped_item()`,
  `calc_fanout_via_ripup_cost_factor()` (`FANOUT_COST_CONSTANT = 20000`) and the private
  `enter_through_small_door()`, including the `CHECK_RIPUP` trace line.
- NEW `src/autoroute/MazeRandom.h` — header-only 48-bit LCG, a bit-exact `java.util.Random` port
  (`0x5DEECE66D` / `0xB`, `next(bits)` on the post-increment state, `nextDouble()` from 26+27 bits).
- `MazeSearchAlgo` owns the resolver and exposes `ripup_resolver()`, `search_tree()` and
  `random_generator()`; `set_control()` re-seeds the generator from `control.ripup_costs`, the
  counterpart of the Java constructor's `randomGenerator.setSeed(ctrl.ripupCosts)`.
- `MazeSearchAlgo::expand_to_room_doors()` now uses the resolver on both paths: a thick free space
  room behind a small door is entered only when `check_leaving_ripped_item()` allows it, and an
  obstacle room is entered with the calculated ripup costs.
- `RouterSettings::get_start_ripup_costs()` — needed by the `preserveFanoutProtection` threshold.

### Gotchas and divergences
- **The shove branch is emulated, not skipped**: Java tries `shoveTraceRoom()` and, when it fails,
  enqueues a delayed `MazeListElement` (`already_checked = true`, `room_ripped = true`,
  `ripup_cost = ripupCosts`) so the room can be shoved through another door section, then returns.
  The C++ port keeps that whole path; since WP-10 Increment K the shove attempt itself is real
  (`MazeTraceShover::shove_trace_room`).
- `Connection::get()` is still a C++ stub (returns the precalculated connection or `nullptr`), so
  an uncalculated connection contributes no detour discount — `detour` stays 1 and the ripup cost
  is not divided. This is the one numeric divergence of `check_ripup()`.
- The C++ doors/rooms carry `IntBox` shapes, so the small-door check polyline is derived from the
  door bounding box rather than the exact Java `TileShape`.
- `RouterSettings` holds a plain `int start_ripup_costs = 100` where Java has a nullable `Integer`
  (unset = 1); the resolver falls back to 1 when the settings pointer is null.
- The ported `Trace::get_normal_contacts(point, ...)` only detects a contact at the corners of a
  trace, so fanout-protection tests must place the contact's end corner on the contact point.

### Testing notes
- `MazeRipupResolver_test.cpp` (31 tests, 3 suites): the `MazeRandom` sequences of the workspace
  JDK 25 for 6 seeds plus `next(32)`/`next(8)`; fanout protection for a shove-fixed 2-corner
  contact and its rejection for 1 or 3 corners; the ripup costs 50000 (plain), 5000 (fanout
  protected), 49782 (randomized, pass 4, seed 1000) and 49838 (seed 1001) — all taken from a
  throwaway Java program running the Java cost formula; already-ripped and shoved-room rejections;
  the small-door guards.
- **ctest 603/603** + `run_tests` PASSED; `tests/data/Issue022-AutoRouter_interrupted.dsn` still
  routes end to end (23%, 12 nets in the SES).

## `MazeTraceShover` (WP-10 Increment K, 2026-09-30)

- **Port**: `src/autoroute/MazeTraceShover.h` (61 lines) + `MazeTraceShover.cpp` (454 lines) from
  the 316-line Java `freerouting/autoroute/maze/MazeTraceShover.java`. Public surface:
  `check_shove_trace_line()` (static), `shove_trace_room()` (static) and `end_points_matching()`
  (static), plus the `DoorSection { ExpansionDoor* door; int section_index; FloatLine line; }`
  result struct that the caller expands into.
- **Wiring**: `MazeSearchAlgo::expand_to_room_doors()` previously logged a "shove not ported"
  divergence and unconditionally enqueued the delayed ripup element. It now calls
  `MazeSearchAlgo::shove_trace_room()` (declared in the header, implemented right after
  `room_shape_is_thick()`), which tries the left door section and then the right one via
  `expand_to_door_section()`. The delayed element with `already_checked = true`,
  `room_ripped = true` and the calculated ripup cost is enqueued only when a section returns
  `false`, which is the Java `shoveTraceRoom` contract.
- **Porting traps hit** (both cost a build cycle):
  1. The canonical C++ `DrillItem` is defined in **`src/board/Via.h`**; the stale
     `src/board/DrillItem.h` is not in any target and does not even compile. The shover needs the
     real `DrillItem` for `end_points_matching()`, so the include must be `board/Via.h` and the
     center comes from `item->get_center()`.
  2. `ExpansionDoor::other_room()` is **overloaded** (`ExpansionDrill*`, `CompleteExpansionRoom*`,
     `ExpansionRoom*`). Java's `(ExpansionRoom) room` cast means the base overload, so both call
     sites need `static_cast<ExpansionRoom*>(p_obstacle_room)`; without it the C++ overload
     resolution picks the `CompleteExpansionRoom*` variant and returns `nullptr` for drill
     pages, which would have silently disabled the shove for every drill-page door.
- **Divergences** (documented in the code and pinned by tests):
  1. `board::ShoveTraceAlgo::check` is only an approximation of the Java `TraceShover.check` — it
     has no via/spring-over recursion — so a shove Java accepts can be refused here. The port
     keeps the Java structure and the "refused = return true" contract, so the difference only
     changes *which* door sections are produced, never the control flow contract.
  2. The C++ expansion doors carry an `IntBox` shape instead of an exact `TileShape`, so
     `diagonal_corner_segment()` is the bounding box diagonal and `polar_line_segment()` snaps to
     a border line. For a door that is seen almost edge on this no longer reproduces the Java
     geometry, and the `from_door_compare_distance` filter then rejects every candidate, so the
     C++ shover typically returns an empty door list.
  3. Building the shove line from `tracePolyline.lines[traceCornerNo + 1]` and `[+ 2]` degenerates
     for a *straight* trace — all three lines are collinear and the `LineSegment` constructor
     runs off to infinity. This is a property of the Java construction, not of the port, so the
     end-to-end test uses an L shaped trace.
- **Tests**: NEW `src/autoroute/MazeTraceShover_test.cpp`, 14 gtests, registered in
  `tests/CMakeLists.txt`:
  - 6 for `end_points_matching`: the trace itself, a drill item at a trace end (and one mid-trace
    which must not match), a drill item of another net, a bending same-net continuation (both
    directions), a disjoint same-net trace, and a non-trace same-net item (`ConductionArea`).
  - 7 for the `check_shove_trace_line` guard chain, one per Java early return: a drill as the
    from door, a non-`PolylineTrace` obstacle item, a trace with another half width, a trace of
    another clearance class, `from_door_shape.max_width() < 2 * compensatedTraceHalfWidth`, a
    stale `trace_corner_no`, and an inner door section (which must return `false` so another
    section is tried).
  - 1 end-to-end: `AShoveIsAttemptedFromTheOuterMostDoorSection` drives the whole path (section
    check → `check_trace_segment` → `ShoveTraceAlgo::check` → the per-door loop) on an empty
    board, with three extra obstacle-room doors straddling / below / above the shove line, and
    pins both that the function reports success and that divergence 2 above yields an empty door
    list rather than a Java-matching collection.
- **ctest 617/617** + `run_tests` PASSED.

## WP-10 Increment L — `MazeExpansionEngine` (2026-10-01)

- `src/autoroute/MazeExpansionEngine.h/.cpp` ports the ~250-line Java
  `MazeExpansionEngine`: `expand_to_drill()`, `expand_to_drill_page()`,
  `expand_to_drills_of_page()`, `expand_to_other_layers()` and
  `check_layer_with_any_matching_via()`, wired into `MazeSearchAlgo::occupy_next_element()`
  and `expand_to_room_doors()`.
- Prerequisite ports in the same increment: `AutorouteEngine` owns the `DrillPageArray`
  (built with the Java page width `max(5 * via_diameter, 10000)`, invalidated when a
  complete expansion room is removed), `AutorouteControl::via_rule` + a Java-faithful
  `rebuild_via_info()`, `ForcedViaAlgo::check_layer()` with `p_trace_half_width` /
  `p_trace_clearance_class`, `BoardRules::get_default_via_rule()`, the three
  `Pin` trace-exit-restriction methods, and a non-owning
  `ViaRule::contains_padstack(Padstack*)` overload.
- `BoardRules::get_default_via_diameter()` was removed again: `freerouting_rules` may not
  depend on geometry or Boost, so the default via diameter is computed in the
  `AutorouteEngine` constructor.
- **Module-layering lesson (the drill cache).** Java caches the `ExpansionDrill` on the
  `Via`. `ExpansionDrill` is an autoroute type, so a `Via` in the board module may not hold
  one. A `std::unique_ptr<ExpansionDrill>` member on `Via` made every board TU that destroys
  a `Via` reference autoroute symbols, dragging `AutorouteEngine.o` → `AutorouteControl.o` →
  `freerouting_settings` into board-only test links and breaking 6 link targets. Caching in
  `ItemAutorouteInfo` hit the same cycle, because `ExpansionDrill.o` itself references
  `AutorouteEngine`. Final design: the cache lives in
  `MazeExpansionEngine::get_autoroute_drill_info(via, tree)`, keyed by via, so board never
  references autoroute.
- **Two real bugs found and fixed:**
  1. `ItemAutorouteInfo::get_expansion_room()` was still a `return nullptr` stub, so no
     obstacle expansion room was ever created. Now ported (room array resized on a changed
     tree shape count, the Java out-of-range warning, lazy room creation) together with
     `reset_doors()`. This also fixed 5 pre-existing failures in `ItemAutorouteInfoTest` and
     `SortedRoomNeighboursTest`.
  2. `AutorouteEngine::remove_complete_expansion_room()` read `p_room->get_shape()` *after*
     the owning `unique_ptr` had been erased — a use-after-free that segfaulted both new
     tests. The shape is now copied up front and `invalidate_drill_pages()` takes the
     `IntBox` directly.
- Tests: `src/autoroute/MazeExpansionEngine_test.cpp`, 11 gtests → **ctest 628/628** +
  `run_tests` (`AllTests`) PASSED.
- **Open divergences, documented in the code:** `MazeFanoutDiagnostics` is not ported
  (**superseded: ported, compiled and wired in WP-10 Increment R**, see the Increment R section
  above);
  `expand_to_drill()` omits Java's backtrack-door intersection exception in the thin-room
  condition; the C++ free-space rooms are still tiny interim boxes around the drill location,
  so `expand_to_other_layers()` finds no matching via on another layer in production
  (**resolved in WP-10 Increment U below**);
  `BoardRules` has no global via-rule list, so `get_default_via_rule()` falls back to the
  first net class; `Network.cpp` still skips DSN via-rule scopes.

## WP-10 Increment M — `FoundConnectionLocator` family (2026-10-01)

Port of the three Java `autoroute/path/FoundConnectionLocator*` classes:

- `src/autoroute/FoundConnectionLocator.h/.cpp` — `get_instance()` (dispatches on
  `AutorouteControl::angle_restriction`: 45° → `FoundConnectionLocator45Degree`, 90°/NONE →
  `FoundConnectionLocatorAnyAngle`), the door backtrack through the `BacktrackElement` array
  (`next_room`, `door`, `ripup_cost`, `room_ripped`, `section_no`), the ripped obstacle room
  collection, the `currentSectionNo` clamp to the door section count with the Java warnings,
  the start-corner adjustment into the start room and the `ItemRouteResult` construction.
- `src/autoroute/FoundConnectionLocator45Degree.h/.cpp` — the 45° corner generation including
  the 2-dimensional-door branch (`TileShape::shrink()` + `nearest_point_approx`).
- `src/autoroute/FoundConnectionLocatorAnyAngle.h/.cpp` — `calculate_next_trace_corners()` with
  the visibility-range advance over the doors, `calc_door_left_corner()` /
  `calc_door_right_corner()`, `left_turn_next_corner()` / `right_turn_next_corner()`,
  `right_left_tangential_point()` / `left_right_tangential_point()` and the clearance-violation
  corner correction.

Prerequisite ports in the same increment:

- `FloatPoint::tangential_points()` + `left_tangential_point()` / `right_tangential_point()`
  (Java `FloatPoint.tangentialPoints` incl. the 90° turn taken when `dy > dx`, and the
  `discriminant <= 0` "pole inside the circle" case returning nothing).
- `TileShape::index_of_left_most_corner()` / `index_of_right_most_corner()` / `shrink()`.
- `MazeSearchAlgo::describe_expandable()` / `describe_room()` — the Java diagnostic text.
- **`TargetItemExpansionDoor` fix**: Java returns a single `MazeSearchElement` for this
  expandable, but the C++ emulation returned an array-of-one whose element was never written
  (garbage `backtrack_door`, `section_no_of_backtrack_door`, `room_ripped`), and the class had
  no `shape_` member at all, so `ExpansionRoom.cpp` did not compile. It now owns one
  `MazeSearchElement` and overrides `maze_search_element_count()`, `get_maze_search_element()`
  and `reset()`.

Divergences, all documented in the code:

- `FoundConnectionLocator::is_valid()` replaces the Java nullable `connectionItems` — a
  `std::vector<ItemRouteResult>` cannot be null, so an unroutable connection is an empty
  vector.
- The dead C++ `Connectable` interface is replaced by the `is_trace()` / `is_via()` /
  `is_pin()` / `ConductionArea` guard already used by `MazeSearchAlgo::calculate_target_doors`,
  with the item tree shape as connection shape.
- `ExpandableObject::get_shape()` returns an `IntBox`, so the room door corners come from
  `get_shape_tile()`.
- `AutorouteDiagnostic` does not exist in C++, so every Java diagnostic callback is dropped.

**Test-fixture lesson (worth keeping):** a 1-dimensional door between *touching* rooms has
duplicated corners in the `IntBox` representation (`corner_approx` returns ll/ur combinations),
so `calc_door_left_corner()` and `calc_door_right_corner()` can return the same point and the
any-angle locator degenerates to a single corner. Real maze rooms overlap, so their doors are
2-dimensional. The any-angle test therefore uses overlapping rooms that are also clearly wider
than twice the trace half width — a door exactly as tall as the trace has no tangent line at
all, so Java would degenerate the same way.

Tests: `src/autoroute/FoundConnectionLocator_test.cpp` 13 gtests, `src/geometry/planar/Point_test.cpp`
6 gtests, `src/geometry/planar/TileShape_test.cpp` 8 gtests → **ctest 655/655** + `run_tests`
(`AllTests`) PASSED.

## Summary

- **Done**: 33 classes compiled & in the build; the `MazeSearchAlgo` expansion-room core
  (Increment I), the `MazeRipupResolver` ripup costs (J), `MazeTraceShover` trace shoving (K),
  `MazeExpansionEngine` drill-page / other-layer expansion (L), the `FoundConnectionLocator`
  family (M), the `FoundConnectionInserter` (P/Q), the fanout pass (R), the
  `ForcedViaInserter` hole-shape clearance branch (S) and the tie-pin trace shape reduction
  (`reduce_trace_shapes_at_tie_pins`, `ShapeSearchTree::change_item_shape` /
  `reduce_trace_shape_at_tie_pin`, T) are ported; the whole-plane `shape == null` free space
  rooms (`AutorouteEngine::complete_expansion_room` `WholePlaneShapeTag` overload, `ExpansionRoom`
  `is_whole_plane()`, whole-plane maze-start and expansion-drill rooms, U) are ported
- **Tests**: 29 gtest executables; **755/755 ctest** + run_tests PASSED (2 added by WP-10 Inc U)
- **Remaining blockers**: (1) Phase 4.4 — `FoundConnectionInserter` is ported (WP-10 Inc P) and the
  `AutorouteEngine::autoroute_connection()` rewiring (maze expansion path first, legacy grid A* as
  fallback) landed with it; multi-segment (3+ corner) route insertion is verified end to end as of
  **WP-10 Inc Q (2026-10-04)**, which also fixed the `ShapeTraceEntries::resort()` port, the nullable
  `CalcFromSide::border_intersection`, the `PolylineTrace::combine()` use-after-free and the inverted
  clearance safety margin (see `MIGRATION_board.md` / `MIGRATION_rules.md` porting notes),
  (2) ~~the tie-pin shape reduction~~ **is DONE (WP-10 Inc T, 2026-10-06)**;
  `check_forced_trace_polyline` landed with Inc P, (3) the `ForcedViaInserter` hole-only-layer /
  hole-shape clearance branch **is DONE (WP-10 Inc S, 2026-10-06)**, (6) the Java `fallbackToBoardVias` fanout via-rule combination
  (needs `RouterSettings::FanoutSettings` and a global `BoardRules` via-rule list), (4) ~~the free-space rooms are still tiny interim boxes, which limits
  `expand_to_other_layers()`~~ **is DONE (WP-10 Inc U, 2026-10-07)** — the drill / maze-start rooms are whole-plane
  `shape == null` rooms again, (5) `BatchOptimizer`/`BatchOptimizerMultiThreaded`, and the
  settings/scoring infra for `BoardHistoryEntry`
- **Note**: `MazeFanoutDiagnostics` is compiled (`src/autoroute/CMakeLists.txt`), instantiated by
  `MazeSearchAlgo` (constructor + `set_control()`, which must recreate it because it holds a
  control reference) with all six Java `MazeExpansionEngine` trace sites plus the two fanout
  expansion-list filter rejections, and it is also used by `FoundConnectionInserter`'s fanout
  diagnostics — all as of WP-10 Increment R (2026-10-05).

## WP-10 Increment R (2026-10-05) — fanout pass

### `RoutingBoard::fanout()`

Declared in `src/board/RoutingBoard.h`, **defined in the autoroute module**
(`src/autoroute/RoutingBoardFanout.cpp`), because the Java method needs `AutorouteControl` +
`AutorouteEngine` + `AutorouteAttemptResult` and the board module may not reference the autoroute
module. The header forward-declares `autoroute::AutorouteAttemptResult`, following the existing
convention already used for `settings::RouterSettings`, `datastructures::Stoppable` and
`datastructures::TimeLimit`.

Ported from Java `RoutingBoard.java:990-1122`: the `ALREADY_CONNECTED` guards (multi-layer pin,
multi-net pin, connected set that leaves the start layer), the `NO_UNCONNECTED_NETS` guard, the
distance-squared sort of the unconnected set, the `is_fanout` / `fanout_start_pin_name` /
`fanout_start_pin_center` / `fanout_start_pin_layer` / `remove_unconnected_vias = false` control
setup, the ripup settings, the closest-target-then-whole-set strategy for at most four targets and
the whole-set strategy above that, and the `opt_changed_area` pull-tight call.

### `MazeSearchAlgo::passes_fanout_filter()`

Port of the anonymous `mazeExpansionList` TreeSet of Java `MazeSearchEngine`, now called from
`add_to_maze_expansion_list()`: entries on the start layer farther than
`AutorouteControl::max_escape_length_um` (default 3000 um) are dropped, drills closer than
`AutorouteControl::min_escape_length_um` (default 500 um) are dropped, and both limits are scaled by
the board resolution (Java's `Communication.getResolution(Unit)` with its inverted `Unit.scale`
argument order, reproduced verbatim so the limits match Java). All other-layer entries pass
through, and a null control passes through like Java's empty control fields.

### Documented divergences

1. The Java `fallbackToBoardVias` branch is missing: the C++ `RouterSettings` has no fanout
   settings and `BoardRules` has no global via-rule list, so the via-rule combination cannot be
   reproduced yet.
2. Java's `init_autoroute(..., false)` always creates a new engine, so the C++ port creates a
   local `AutorouteEngine` instead of reusing a control member.
3. `RoutingBoard::opt_changed_area(...)` returns early when the changed area's tidy region is
   null, which is what the fanout call passes.
4. `MazeFanoutDiagnostics::enabled()` gates on a `fanout_start_pin_name` prefix (`U27-`), so the
   fanout traces only appear for that naming pattern, exactly like Java.

### Tests

`src/autoroute/RoutingBoardFanout_test.cpp` (6 gtests: null pin, multi-layer pin, multi-net pin,
connected multi-layer item, empty unconnected set, and a maze attempt that must leave the guard
clauses) plus 6 filter/diagnostics cases in `MazeSearchAlgo_test.cpp` →
**ctest 726/726** + run_tests PASSED.

## WP-10 Increment T (2026-10-06) — tie-pin trace shape reduction

Closes the last documented divergence of the `MazeSearchAlgo::init()` port (Java
`MazeSearchEngine.init` lines 970-971 call `reduceTraceShapesAtTiePins(startItems, ...)` /
`(destItems, ...)` before the item loops).

### Ported surface

| C++ | Java |
|-----|------|
| `reduce_trace_shapes_at_tie_pins(set<Item*>, net_no, search_tree*)` (anonymous namespace in `MazeSearchAlgo.cpp`), called for the start **and** dest item lists at the top of `init` | `MazeSearchEngine.reduceTraceShapesAtTiePins(Collection<Item>, int, ShapeSearchTree)` |
| `ShapeSearchTree::reduce_trace_shape_at_tie_pin(Pin*, PolylineTrace*)` + `ShapeSearchTree::change_item_shape(Item*, int, shared_ptr<TileShape>)` (in the **board** module, see `MIGRATION_board.md`) | `ShapeSearchTree.reduceTraceShapeAtTiePin` / `changeItemShapeUnlocked` |

The helper only reduces traces of **foreign** nets (`!trace->contains_net(control_.net_no)`)
that are normal contacts of a `Pin` with `net_count() > 1`; the own-net trace through the same
tie pin keeps its shape. Non-`Pin` items and single-net pins are skipped exactly as in Java.

### Documented divergences

1. Java throws a `NullPointerException` when the pin has no tree shape on the trace layer
   (`getTreeShapeOnLayer` returning null); the port logs a warning and returns without
   changing the trace (`DrillItem::get_tree_shape_on_layer` warns on an out-of-range layer).

### Tests

`MazeSearchAlgoTest.InitReducesForeignTraceShapesAtTiePins` in `src/autoroute/MazeSearchAlgo_test.cpp`
(foreign-net trace through a `{1,2}` tie pin loses the pin center in the engine search tree and
keeps the piece with its far corner; the own-net trace is untouched; `validate_entries` holds for
both) → **ctest 753/753** + run_tests PASSED.

## WP-10 Increment U — whole-plane free space expansion rooms (2026-10-07)

The `shape == null` cases of the Java `IncompleteFreeSpaceExpansionRoom` (the whole-board room a
drill or maze start completes against) are ported for real; the interim bounded micro-box mapping
from Increment C is gone.

### `ExpansionRoom` whole-plane support

- New internal `WholePlaneShapeTag` ctor set in `src/autoroute/ExpansionRoom.h/.cpp`: the room is
  constructed with shape `IntBox::EMPTY` and completes against the board box —
  `complete_room(ShapeSearchTree&)` special-cases `IntBox::EMPTY` through
  `IntBox::get_contained_box(this, tree)` → `get_board_box()`, and `is_whole_plane()` is true.
- The **public** resize arm of the ctor now creates a whole-plane room when the incoming box has
  grown to the board box (`IntBox` containment), so expansion recalculations that reach the board
  bounds fold back into the whole-board room exactly like Java's
  `IncompleteFreeSpaceExpansionRoom` constructors.
- `AutorouteEngine.h` merely **forward-declares** `ExpansionRoom` (the layout change is confined to
  the `.cpp`); this keeps the module layering from pulling the autoroute engine into board files.

### Wiring

- `AutorouteEngine::complete_expansion_room` gained a `WholePlaneShapeTag` overload (Java
  `completeExpansionRoom(ShapeSearchTree, WholePlaneShape, ...)`) and the real overload now
  rewrites the whole-plane null-shape case to a `CompleteShapeInput` with `shape_type == NULL`
  and `IntBox::get_contained_box` as the shape — i.e. the Java `completeShapeUnlocked` null-start
  path, whose `divide_large_room` run splits an obstacle-free whole-board room into 4 sections.
- `MazeSearchAlgo::init`'s maze-start rooms call the whole-plane overload;
  `ExpansionDrill::calculate_expansion_rooms` uses the whole-plane drill room (reuse a stored room
  at the same location, then the engine whole-plane call). The interim identical-mixture
  containment guard is removed.

### Java-parity consequence (a test "regression" that is actually the fix)

Java `DrillPage.getDrills` (~line 125) calculates the expansion rooms of a drill and keeps the
drill **only when each layer ends with exactly one complete free space room**. On an obstacle-free
board the whole-board incomplete room completes into **4 sections** — never one — so the drill is
dropped. The C++ port now reproduces this, so 10 previously-green empty-board drill tests
(`DrillPage`, `DrillPageArray`, `MazeExpansionEngine`) legitimately failed for the right reason.
They were reworked to **restrained** boards (a net-2 `FakeObstacle` wall inserted into the
`ShapeSearchTree`), not to a fake "always succeeds" path.

### Test-rework lessons (the valuable part)

1. **Fake obstacles must own the inserted leaves.** `ShapeTree::insert(ShapeSearchTreeObject&)`
   transfers the leaf `unique_ptr`s to the object through `set_search_tree_entries()`. A fake that
   ignores them leaves dangling pointers in `MinAreaTree` → SIGSEGV in `MinAreaTree::overlaps`
   during `complete_shape`. The `FakeObstacle` stores them in a `search_tree_entries_` member,
   mirroring `board::Item::search_trees_info_`. (A stale `FakeStorable` in `CompleteFreeSpaceExpansionRoom_test`
   kept its no-op `set_search_tree_entries` on purpose: its `tree_shape_count` is 0 by design.)
2. **`check_layer_with_any_matching_via` bails `NOT_DRILLABLE` when `search_->autoroute_engine()`
   is null** — the test that verifies via insertion through `expand_to_other_layers` therefore
   sets the search's engine through the new public `MazeSearchAlgo::set_autoroute_engine()`
   (also called by `get_instance`; the setter must live in the public section next to
   `set_control`, the private accessors area is for friend/internal use).
3. Wall geometry used for the restrained rooms: `IntBox(4000,0,6000,10000)` gives contained rooms
   `(0,0,4000,10000)` / `(6000,0,10000,10000)`. The sandwich walls `IntBox(0,0,10000,470)` +
   `IntBox(0,530,10000,10000)` with a drill at `(500,500)` give the thin room `(0,470,10000,530)`
   (`min_width()` 60 < 100 → thin → drill rejected), and thick walls give the thick room
   `(0,0,4000,10000)`.
4. **Stale-binary trap:** piping a build through `grep -E " error"` can hide a compile failure and
   leave the old test binary running. After a failing build, rebuild cleanly before judging a
   re-run; a misleading failure was traced to exactly this.

### Divergence (still documented, still parity-equivalent)

The C++ `complete_shape` on a **rootless** tree returns the whole board box while Java returns an
empty result for the same `ShapeSearchTree` when no root shape is set. Both paths still produce a
multipiece completion that never equals one room per layer, so the drill-drop outcome of
`DrillPage::get_drills` is identical.

### Tests

`CompleteFreeSpaceExpansionRoom_test.cpp` grew by 2 (`WholePlaneRoomCompletesToRestrainedFreeSpace`,
`WholePlaneRoomNeverCompletesToOneRoomOnObstacleFreeTree`), `MazeExpansionEngine_test.cpp` by 2
(`ExpandToDrillAddsElementForThickRoom`/`ExpandToDrillRejectsThinRoom`) with
`ExpandToOtherLayersAddsViaElementOnFreeSpaceDrill` renamed from `WaitsForCompleteRoomsOnFreeSpaceDrill`
inverting its expectation (+1 element) → **ctest 755/755** + run_tests PASSED.

- **2026-10-08 (T417)**: `AutorouteEngine::autoroute_connection` is now a faithful port of Java
  `AutorouteEngine.autorouteConnection` (+ `resetAllDoors`, item-data cleanup in `clear()`). The old
  grid-A* fallback after a failed maze search was not Java and caused every E2E timeout (a search-tree
  query per grid cell, up to 3M cells per connection). `BatchAutorouter` now routes each item through
  `RoutingBoard::autoroute` (interim until the T401–T409 pipeline port); its previous loop used a
  default, board-less `AutorouteControl`, so `MazeSearchAlgo::get_instance` could never succeed.
  Remaining reason for 0 routed connections: DSN pin geometry (see MIGRATION_designforms.md).
