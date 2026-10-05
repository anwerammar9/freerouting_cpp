# Missing Logic: Detailed Gap Analysis

## 1. AUTOROUTE MODULE — Core Routing Engine

### 1.1 MazeSearchAlgo — expansion-room core ported (WP-10 Increment I), Java collaborators still open

**Architecture:** the C++ grid A* is gone. `MazeSearchAlgo` is now a port of Java
`MazeSearchEngine` (room/door expansion over a `std::set<MazeListElement>`, the
C++ counterpart of the Java `TreeSet`), 1175 lines C++ vs 1258 lines Java.

| Method | Java Lines | C++ Status | What's Missing |
|--------|-----------|------------|----------------|
| `init()` | 1000-1093 | Ported | `reduce_trace_shapes_at_tie_pins` needs `ShapeSearchTree::reduce_trace_shape_at_tie_pin` |
| `find_connection()` / `occupy_next_element()` | 275-380 | Ported | drill-page and other-layer expansion (`MazeExpansionEngine`) |
| `expand_to_room_doors()` | 383-610 | Ported | drill pages (`MazeExpansionEngine`); the trace shove is ported (WP-10 Inc K) |
| `expand_to_target_doors()` | 612-700 | Ported | `RoutingBoard::check_forced_trace_polyline` thin-room line check |
| `expand_to_door()` | 700-760 | Ported | — |
| `expand_to_door_section()` | 762-900 | Ported | — |
| `door_is_small()` | 900-935 | Ported | `TileShape::bounding_octagon` (derived from the bounding box) |
| `room_shape_is_thick()` | 1095-1125 | Ported | — |
| `check_neck_down_at_dest_pin()` | 1200-1215 | Ported | — |
| `segment_projection()` | 165-205 | Ported | Java compares the endpoints by reference; C++ uses value equality |
| `expand_to_other_layers()` | Java `MazeExpansionEngine` | Ported (WP-10 Inc L) | Ported, but the C++ free-space rooms are still tiny interim boxes around the drill location, so no matching via is found on another layer in production |
| `check_ripup()` | Java `MazeRipupResolver` | Ported (WP-10 Inc J) | The detour discount is lost while `Connection::get()` is still a stub |
| `shove_trace_room()` | Java `MazeTraceShover` | Ported (WP-10 Inc K) | The C++ `ShoveTraceAlgo::check` approximates the Java `TraceShover.check`, and the `IntBox` door shapes no longer reproduce the Java `TileShape` door diagonal, so the shover often collects no door |
| Fanout list filtering | `mazeExpansionList.add` override | Ported (WP-10 Inc R) | `MazeSearchAlgo::passes_fanout_filter()` is the port of the anonymous TreeSet of Java `MazeSearchEngine`, called from `add_to_maze_expansion_list()`. `AutorouteControl::fanout_start_pin_center` is assigned by the new `RoutingBoard::fanout()`; the escape lengths come from `AutorouteControl::max_escape_length_um`/`min_escape_length_um` with the Java defaults 3000/500 um, and the resolution from the new `Communication::get_resolution(Unit)` |
| `MazeFanoutDiagnostics` | Java class | Ported (WP-10 Inc R) | `src/autoroute/MazeFanoutDiagnostics.{h,cpp}` ports all 44 Java lines (`enabled`, `label`, `trace`) and is **compiled and instantiated** by `MazeSearchAlgo` (ctor + `set_control()`, which recreates it because it holds a control reference). All six Java trace call sites in `MazeExpansionEngine` are wired, plus the two rejection traces of the expansion-list filter |

**Consequence for end-to-end routing:** the target-door calculation is wired
(verified in WP-10 Inc J: called from all three `Sorted*RoomNeighbours::complete_expansion_room()`),
but `MazeSearchAlgo::get_instance()` still needs a real start set to produce
expansion doors, and the production engine keeps using the legacy grid A* until
the inserter of Phase 4 exists — the locator half landed in WP-10 Inc M.
`get_instance()` returning `nullptr` on an
empty board is locked in by `MazeSearchAlgo_test.cpp` as the documented pending
behaviour.

> **Resolved 2026-10-01 (WP-10 Increment M):** the `FoundConnectionLocator` family is ported
> as `src/autoroute/FoundConnectionLocator.h/.cpp` + `FoundConnectionLocator45Degree` +
> `FoundConnectionLocatorAnyAngle` with 13 gtests: `get_instance` dispatch on
> `AngleRestriction`, the door backtrack (`BacktrackElement`, ripped obstacle room
> collection, `currentSectionNo` clamp), the corner generation per angle restriction and the
> connection-item construction. Prerequisites in the same increment:
> `FloatPoint::tangential_points`/`left_tangential_point`/`right_tangential_point`,
> `TileShape::index_of_left_most_corner`/`index_of_right_most_corner`/`shrink`,
> `MazeSearchAlgo::describe_expandable`/`describe_room`, and the `TargetItemExpansionDoor`
> maze-element fix (Java returns a single element; the C++ emulation returned an array-of-one
> and the class had no `shape_` member at all, which broke `ExpansionRoom.cpp`).
> **ctest 655/655 + run_tests PASSED.** `FoundConnectionInserter` was ported in
> Increment P (2026-10-03). Still open in this area:
> **RESOLVED 2026-10-03 (WP-10 Increment P+rewiring):** the `AutorouteEngine::autoroute_connection()` now attempts the expansion-room maze path (`MazeSearchAlgo` + `FoundConnectionLocator` + `FoundConnectionInserter`) first, falling back to the legacy grid A* if the maze path fails. The Java `AutorouteDiagnostic` callbacks have no C++ counterpart (`MazeFanoutDiagnostics` itself is ported, see the table above).
> **RESOLVED 2026-10-04 (WP-10 Increment Q):** multi-segment (3+ corner) route insertion works end to end (`FoundConnectionInserterTest.MultiSegmentConnectionIsInserted`). Four Java-parity bugs fixed along the way: the abbreviated `ShapeTraceEntries::resort()` (early `return` for lists shorter than two entries left a sole own-net entry in place), the non-nullable `CalcFromSide::border_intersection` (Java's is nullable, `(0,0)` was the wrong sentinel), the use-after-free in `PolylineTrace::combine_at_start()`/`combine_at_end()` plus the recursive `combine()`, and the inverted `ClearanceMatrix::get_value(..., /*safety_check=*/true)` margin sign with `RoutingBoard::clearance_value()` omitting the margin entirely. **ctest 708/708 + run_tests PASSED.**

> **Resolved 2026-09-27 (WP-10 Increment I):** the `MazeSearchAlgo::init`/`expand_to_room_doors`
> rewrite landed. Prerequisites added: `IntPoint::get_id()`, `IntBox::get_id()`,
> `ExpandableObject::get_id()` (pure virtual), `ExpansionDoor::get_id()`,
> `TargetItemExpansionDoor::get_id()`/`is_destination_door()`,
> `ExpansionDrill::get_id()`, `DrillPage::get_id()`,
> `IncompleteFreeSpaceExpansionRoom::get_room_id()`, `TileShape::min_width()`/`max_width()`
> dispatchers, `AutorouteControl::bend_costs`, `RouterSettings::get_bend_cost()`,
> `Pin::min_width()`/`get_trace_neckdown_halfwidth()`,
> `AutorouteEngine::get_maintain_database()`/`get_rooms_with_target_items()`, and the const
> `CompleteExpansionRoom::target_doors()` overload. `MazeListElement` now carries
> `ExpandableObject*` backtracking, `CompleteExpansionRoom*` next room, a `FloatLine`
> shape entry, a `ripup_cost`, and the Java `compareTo` order (sorting value, expansion
> value, door id, section number). `MazeSearchAlgo` gained `get_instance(start, dest, engine, ctrl)`,
> `find_connection()`, `occupy_next_element()`, `expand_to_room_doors()`,
> `expand_to_target_doors()`, `expand_to_door()`, `expand_to_door_section()`,
> `door_is_small()`, `room_shape_is_thick()`, `check_neck_down_at_dest_pin()`,
> `segment_projection()` and `reconstruct_path()`, with 12 gtests in
> `MazeSearchAlgo_test.cpp` and 12 in the new `MazeSearchElement_test.cpp`.
> `Result::destination_door` is now an `ExpandableObject*` as in Java.
> **ctest 572/572 + run_tests PASSED.**

### 1.2 Missing Autoroute Classes (11 files, ~3400 lines)

| Class | Lines | Purpose |
|-------|-------|---------|
| `LocateFoundConnectionAlgo45Degree` | 238 | 45° connection location |
| `LocateFoundConnectionAlgoAnyAngle` | 363 | Any-angle connection location |
| `BatchAutorouterThread` | 546 | Multi-threaded routing worker |
| `BatchOptimizer` | 397 | Post-route optimization |
| `BatchOptimizerMultiThreaded` | 294 | Multi-threaded optimizer |
| `BoardHistory` | ~200 | Backtracking/convergence |
| `BatchAutorouterV19` | 490 | Legacy comparison router |
| `MazeFanoutDiagnostics` | ~120 | Fanout telemetry |

> **Resolved 2026-10-01 (WP-10 Increment L):** `MazeExpansionEngine` (~250 Java lines) is ported as `src/autoroute/MazeExpansionEngine.h/.cpp` with `src/autoroute/MazeExpansionEngine_test.cpp` (11 gtests): `expand_to_drill()`, `expand_to_drill_page()`, `expand_to_drills_of_page()`, `expand_to_other_layers()` and `check_layer_with_any_matching_via()`, wired into `MazeSearchAlgo::occupy_next_element()` and `expand_to_room_doors()`. Two module-layering lessons: (1) the Java per-`Via` drill cache cannot be ported as a member, because `ExpansionDrill` is an autoroute type and a `Via` of the board module may not hold one — a `unique_ptr` member made every board TU that destroys a `Via` pull in `AutorouteEngine.o` → `AutorouteControl.o` → `freerouting_settings` and broke 6 link targets; the cache now lives in `MazeExpansionEngine` keyed by via. (2) `ItemAutorouteInfo::get_expansion_room()` was still a `return nullptr` stub (so no obstacle expansion room was ever built) — ported in this increment, which also fixed 5 pre-existing `ItemAutorouteInfoTest`/`SortedRoomNeighboursTest` failures. **ctest 628/628 + run_tests PASSED.** Still open: `MazeFanoutDiagnostics` and the fanout `mazeExpansionList.add` filter; `expand_to_drill()` omits the Java backtrack-door intersection exception in the thin-room condition; `BoardRules` has no global via-rule list, so `get_default_via_rule()` falls back to the first net class; `Network.cpp` still skips DSN via-rule scopes.

`MazeListElement` (130 Java lines) is **ported** as the second class in
`src/autoroute/MazeSearchElement.h/.cpp` with `src/autoroute/MazeSearchElement_test.cpp`.

> **Resolved 2026-09-27 (WP-10 Increment H):** the `AutorouteEngine.removeCompleteExpansionRoom` neighbour-split half is ported in `src/autoroute/AutorouteEngine.cpp` (now public like the Java method, with `src/autoroute/AutorouteEngine_test.cpp`, 8 gtests): per door of the removed room the door is removed from the neighbour, the room/neighbour intersection is computed, and a 1-dimensional intersection creates a new incomplete free space room (half-plane shape behind the neighbour's `touching_sides` border line, contained shape = the intersection) registered with `add_incomplete_expansion_room()` and connected by a dimension-1 `ExpansionDoor` on both sides; then `remove_all_doors()`, `remove_from_tree()` and the database erase (with the Java warning for an unregistered room). `completeNeighbourRooms()` (completes incomplete free space neighbours, restarts the door iteration, calculates the doors of an obstacle neighbour once) and `getFirstIncompleteExpansionRoom()` are ported too. Two notes: `ExpansionDoor::other_room()` must be called through an `ExpansionRoom*` (the `CompleteExpansionRoom*` overload `dynamic_cast`s and returns `nullptr` for a door to an incomplete room — the counterpart of the Java `(ExpansionRoom) room` cast), and Java's `drillPageArray.invalidate(roomShape)` line was not portable because the C++ engine had no `DrillPageArray` owner (it would need `rules::BoardRules::get_default_via_diameter()`); both are now in place in WP-10 Increment L. **ctest 547/547 + run_tests PASSED.**

> **Resolved 2026-09-27 (WP-10 Increment G):** `SortedOrthogonalRoomNeighbours` (482 Java lines) is ported as `src/autoroute/SortedOrthogonalRoomNeighbours.h/.cpp` with `src/autoroute/SortedOrthogonalRoomNeighbours_test.cpp` (13 gtests): `SortedOrthogonalRoomNeighbour` (4 border lines = bottom, right, top, left; first/last touching side, `edge_interior_touches_obstacle` flags), `calculate_neighbours()` (2-D overlap doors, dimension-1 doors, own-net target doors, `IntBox` guard for the room *and* the neighbour tree shapes), `add_sorted_neighbour()` (counterclock ordering with the `TreeSet` duplicate drop and the `get_id()` tie-break), `remove_border_line()`, `try_remove_edge()` (room enlargement through `complete_shape` with the area-increase guard and the largest dimension-2 door as ignore object/shape), `calculate_new_incomplete_rooms()` (free space + obstacle room), `calculate_incomplete_rooms_with_empty_neighbours()` and the `calculate()` recursion. The dispatch is now a named `select_calculation_mode()`: 8 bounding directions → 45 degree, **4 → orthogonal**, otherwise any-angle base. Supporting change: the Java-parity 2-argument `ExpansionDoor(ExpansionRoom*, ExpansionRoom*)` constructor. Contained divergence: Java `completeShape` returns an empty result for a rootless tree while the C++ version falls back to the whole board, so `try_remove_edge()` skips the enlargement for an empty search tree. **ctest 539/539 + run_tests PASSED.**

> **Resolved 2026-09-27 (WP-10 Increment F):** `Sorted45DegreeRoomNeighbours` (810 Java lines) is ported as `src/autoroute/Sorted45DegreeRoomNeighbours.h/.cpp` with `src/autoroute/Sorted45DegreeRoomNeighbours_test.cpp` (12 gtests): `Sorted45DegreeRoomNeighbour` (8 border lines = 4 axis + 4 diagonal, first/last touching side with the Java corner rules, `edge_interior_touches_obstacle` flags), `calculate_neighbours()` (2-D overlap doors, dimension-1 doors, own-net target doors), `add_sorted_neighbour()` (counterclock ordering with the `TreeSet` duplicate drop), `remove_not_touching_border_lines()`, `try_remove_edge_line()` (room enlargement through `complete_shape` with the area-increase guard), `calculate_new_incomplete_rooms()` (free space + obstacle room) and the `calculate()` recursion. `SortedRoomNeighbours::calculate()` now dispatches on the search tree bounding directions (8 → 45 degree, new `ShapeTree::get_bounding_directions()` accessor; 4 → orthogonal in Increment G). The C++ `IntOctagon::is_empty()`/`dimension()` divergence (degenerate octagons count as empty) is contained with local `java_is_empty()`/`java_dimension()` helpers. **ctest 526/526 + run_tests PASSED.**

> **Resolved 2026-09-26 (WP-10 Increment E):** `SortedRoomNeighbours` (base, 680 Java lines) is ported as `src/autoroute/SortedRoomNeighbours.h/.cpp` with `src/autoroute/SortedRoomNeighbours_test.cpp` (15 gtests): `SortedRoomNeighbour::compare_to()` counterclock ordering, `calculate_neighbours()` (own-net object collection, 2-D overlap doors for obstacle rooms, 1-D door creation, 0-D corner classification), both `insert_door_ok` overloads, and `calculate_incomplete_rooms_with_empty_neighbours()`. This needed the room `get_shape_tile()` generalization (`ExpansionRoom` + Simplex shapes/`set_shape_tile`), `SearchTreeObject::get_id()`, and the `TileShape` helpers `equals_corner`/`prev_no`/`next_no`/`touching_sides`/`intersection_of`. A real divergence was found and fixed: `TileShape::intersection_of` simplified 0/1-D results into degenerate (empty) octagons, which silently dropped the incomplete rooms created for empty neighbours. **ctest 515/515 + run_tests PASSED.**

> **Resolved 2026-09-22 (WP-9 + WP-10 Inc A + Inc B + Inc C + Inc D):** `DrillPage`, `DrillPageArray`, and `ExpansionDrill::calculate_expansion_rooms()` are now ported and tested, complete free space expansion rooms are `SearchTreeObject`s stored in the autoroute search tree, `ShapeSearchTree::complete_shape()` (Java `completeShapeUnlocked`/`restrainShape`/`divideLargeRoom`) is ported and tested as a board API (`src/board/ShapeSearchTree_complete_shape_test.cpp`, 9 gtests), and `AutorouteEngine::complete_expansion_room`/`ExpansionDrill` are rewired onto `complete_shape` (Inc C). The net-connectable room-door prereqs are ported (Inc D): `ItemAutorouteInfo::get_expansion_room()` (was a stub), `ObstacleExpansionRoom::create_overlap_door()`, `CompleteFreeSpaceExpansionRoom::calculateTargetDoors()` (dead-`Connectable` divergence: connectable = trace/via/pin/ConductionArea, connection shape approximated by the item tree shape), `TargetItemExpansionDoor` real shape + dimension 2, and `AutorouteEngine::remove_all_doors()` wired into `remove_incomplete_expansion_room()` (ctest 495/499 — 4 pre-existing DRC failures — + run_tests). What is still missing is the *use* of drill pages and these doors inside `MazeSearchAlgo` (Java `expand_to_room_doors`/`expand_to_other_layers` rooms with via doors), the room-neighbour variants (both the 45 degree and the orthogonal/90 degree variants are done in Inc F/G), and the `MazeSearchAlgo::init`/`expand_to_room_doors` rewrite (both closed by Increment I above;
what remains is the door-calculation wiring, see 1.1).

### 1.3 BatchAutorouter — Missing Convergence Logic

Java `runBatchLoop()` (lines 549-686) contains:
- Board history recording after each pass
- Score comparison and backtracking
- Stop conditions: `STOP_AT_PASS_MINIMUM=8`, check every `STOP_AT_PASS_MODULO=4`
- Board restoration up to `BOARD_RANK_LIMIT=50`
- C++ has none of this — simple loop stops after 2 no-progress passes

### 1.4 AutorouteEngine — Grid vs Expansion Rooms

`MazeSearchAlgo` itself no longer belongs in this section: since WP-10 Increment I it is the
Java expansion-room search (see 1.1). What is still on the old grid is the *engine driver*.

C++ `autoroute_connection()` (lines 119-416):
- Uniform grid A* on single layer
- Manhattan distance heuristic
- Binary blocked/unblocked cells
- Creates PolylineTrace directly

Java uses:
- Expansion room graph traversal
- Multi-layer routing with via insertion
- Ripup and trace shoving
- Neckdown handling
- Fanout routing — **ported in WP-10 Increment R** (`RoutingBoard::fanout()` + the `mazeExpansionList.add` filter)
- Integration with FoundConnectionInserter (`AutorouteEngine::autoroute_connection()` rewiring) — **done (WP-10 Increment Q)**

---

## 2. BOARD MODULE — Stub Methods

### 2.1 Critical Stubs (blocks routing)

| Method | File:Line | Issue |
|--------|-----------|-------|
| `ShoveTraceAlgo::spring_over()` | — | **RESOLVED 2026-10-02** (WP-10 Inc N): full `TraceShover.springOver` port, see 02_PROGRESS |
| `ShoveTraceAlgo::insert()` | ShoveTraceAlgo.cpp | **RESOLVED 2026-10-02** (WP-10 Inc O): full `TraceShover.insert` port incl. the board mutation, see 02_PROGRESS |
| `ShoveTraceAlgo::check()` | ShoveTraceAlgo.cpp | **RESOLVED 2026-10-02** (WP-10 Inc O): full `TraceShover.check` port (shape + line-segment overload), see 02_PROGRESS |
| `RoutingBoard::opt_changed_area()` | RoutingBoard.cpp:67-74 | Empty body |
| `Item::get_normal_contacts()` | Item.cpp:43-48 | Returns empty (Trace/DrillItem/ConductionArea override) |
| `Item::first_common_layer()` | Item.cpp:86-91 | WAS returning -1; resolved 2026-09-18 |

**Resolved 2026-09-18:** `RoutingBoard::combine_traces()`, `get_outline()`, `contains_trace_tails()`, `get_trace_tail()` ported (BasicBoard.java:1307 `getTraceTail`, 684 `combineTraces`, RoutingBoard.java:1188 `containsTraceTails`; Java `getOutline` scans the item list for a `BoardOutline`). `BoardOutline` restored as a compilable build member (was excluded from `src/board/CMakeLists.txt` with broken namespace forward-declarations). `additional_update_after_change()` is a faithful no-op until an autoroute engine is wired into the item-change path (Java RoutingBoard.java:96 returns when `autorouteEngine == null`). Note base `Trace::normalize()/combine()/split()` still return false/empty — the `PolylineTrace` overrides are real.

**Resolved 2026-09-18 (WP-1 Contact Graph):** `Item::first_common_layer()` ported (was returning -1 unconditionally); `Item::get_connection_items()` ported (was returning `{}`); `Item::is_fanout_via()` added; `DrillItem::first_layer()/last_layer()` overrides added and layer range now synced from the padstack in the `Via` ctor / `set_padstack` (fixes vias always reporting 0..0); `Trace::normal_contact_point()` extended for DrillItem; `DrillItem::normal_contact_point()` added. Verified by 8 new `ItemContactGraphTest` cases and a diagnostics run (14/14 green, 413/413 ctest + run_tests green).

**Resolved 2026-10-02 (WP-10 Inc N, `TraceShover.springOver` prerequisites):** `ShoveTraceAlgo::spring_over()` and `spring_over_obstacles()` are full Java ports (Java TraceShover.java:611-875), together with `get_ignore_items_at_tie_pins()`; the prereqs `Item::get_all_contacts()`/`get_all_contacts(int)`, `TileShape::cutout(Polyline)`, `TileShape::bounding_octagon()` and `DrillItem::get_tree_shape_on_layer()` were ported in the same increment. `ShoveTraceAlgo::check()`/`insert()` and the `ShapeTraceEntries` cutout were closed in WP-10 Inc O (2026-10-02); what is still missing is the `RoutingBoard` forced-trace insertion, which is the remaining part of Phase 4.4.

**Resolved 2026-09-18 (WP-2 NetIncompletes):** `connected_group_count` / `get_connected_group_count()` added (was missing vs `NetIncompletes.java:46,141,155`). Fixed real aliasing bug in `join_connected_sets()`: passed `const shared_ptr<...>&` aliasing `from_item`/`to_item` members that the join reassigns, so from-group items beyond the first were never repointed (3 isolated vias → 3 airlines, expected 2); now takes shared_ptrs by value. 415/415 ctest + run_tests green.

### 2.2 Missing Board Classes

| Class | Java Lines | Purpose |
|-------|-----------|---------|
| Pin (complete) | 722 | Only 25 lines in C++ — missing padstack, shapes, exit restrictions |
| ShapeSearchTree90Degree methods | 287 | create_incomplete_room, create_complete_room, cutout_drill_page |
| ShapeSearchTree45Degree methods | 457 | Same as 90° + border_line_of_obstacle |

---

## 3. DESIGIFORMS MODULE — I/O Gaps

### 3.1 Write Path Almost Entirely Missing

| Component | Java Write | C++ Write | Status |
|-----------|-----------|-----------|--------|
| SessionWriter | Full session | Traces only | STUB |
| Structure | Full DSN write | None | MISSING |
| Network | Full DSN write | None | MISSING |
| Library | Full DSN write | None | MISSING |
| Placement | Write-only | Read-only | INVERTED |
| Wiring | Read/write | Header-only | MISSING |
| DsnFile | Read/write | Header-only | MISSING |

### 3.2 Read Path Missing Logic

| Component | Missing in C++ |
|-----------|---------------|
| Scanner | No keyword recognition, no lexical states |
| Structure | Skips keepouts, autoroute settings, snap_angle, control, rules |
| Network | Skips via infos/rules, net classes, class pairs |
| Library | Only reads circle/rect shapes |
| CoordinateTransform | Entire class missing (199 lines) |

---

## 4. ENTRY POINT — CLI/DRC Gaps

| Gap | Impact |
|-----|--------|
| DRC mode is a stub | Never calls DesignRulesChecker |
| `GlobalSettings::load()` returns nullptr | Settings persistence broken |
| `GlobalSettings::save_as_json()` returns false | Settings can't be saved |
| No async job scheduling | No job queue support |
| `session.add_job()` commented out | Session-job link broken |
| `set_input()` doesn't read file | Manual output path required |
| `-dl` sets wrong field | Bug |
| Generic `--property=value` unimplemented | CLI limited |
