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

## WP-3: RatsNest Full Port (PREREQUISITE for WP-4) — COMPLETED (2026-09-18)

**Java refs**: RatsNest.java
**C++ files**: src/interactive/RatsNest.{h,cpp}, RatsNest_test.cpp

**Completed:**
- `RatsNest` constructor — per-net item lists, creates `NetIncompletes` for each net
- `incomplete_count()` / `incomplete_count(int)` — total and per-net airline counts
- `length_violation_count()` / `get_length_violation(int)` — length matching violations
- `get_airlines()` — all airlines across all nets
- `recalculate(int, RoutingBoard*)` / `recalculate(int, vector<Item*>, RoutingBoard*)` — full and item-list recalculation
- `recalculate_length_violations()` — updates length violations
- `hide()` / `show()` / `is_hidden()` — global visibility
- `set_filter(int, bool)` / `is_filtered(int)` — per-net visibility filters
- `AirLine` nested class with `from_item`, `to_item`, corners, net, `get_item_info()`, `to_string()`
- Tests: 5 RatsNestTest cases + 8 NetIncompletesTest cases = 13 green

**Key logic:**
- Per-net item lists, incomplete count, length violations
- Airlines, recalculate, filter, hidden nets

## WP-4: BoardStatistics + Scoring (COMPLETED 2026-09-18)

**Java refs**: BoardStatistics.java + 11 substructs
**C++ files**: src/core/scoring/BoardStatistics.{h,cpp}, src/core/scoring/CMakeLists.txt
**Note**: lives in separate `freerouting_scoring` lib (layering constraint)

### Completed
- `BoardStatistics` main class with all sub-structs as nested types; `freerouting_scoring` lib builds
- 4 gtest tests (CanCreateFromBoard/DSN/SES, ScoreCalculation) + 7 new = 11 green
- **`init_from_board()` full parity**: host fallback `"Freerouting," + FREEROUTING_VERSION`; unit defaults to MM (Java) when none requested, else unit conversion via `UnitHelper::scale`; `board_.areaCm2` (mm×mm/100); `difficulty_` = {pinCount, signalLayerCount, C=max(1,distance), boardAreaCm2, D=C}; `traces_.totalLengthMm` (mm-normalised); `fanout_` via `pin_is_fully_connected`/`pin_is_escaped` over SMD pins (`first_layer()==last_layer()`)
- **Scoring parity**: `traceLengthForCost = totalLengthMm != 0 ? totalLengthMm : totalLength`; costs = length×defaultPreferredDirectionTraceCost + vias×via_costs; penalties = incomplete×unroutedNetPenalty + clearance×clearanceViolationPenalty + bends×bendPenalty; `getNormalizedScore` returns 0 when `maxScore <= 0`
- **File ctor**: DSN scans whole content for `(hostCad)`/`(hostVersion)` quoted values + counts `(layer`/`(component`/`(class`/`(net`/`(wire`/`(via`; SES counts `(path <layer>` layer names + `(component`/`(net`/`(wire`/`(via`
- Fixed ordering bug: `Pin` branch moved before `DrillItem` branch in item counting (pins were misclassified, pinCount always 0)
- Tests verified 422/422 ctest + run_tests PASSED

### Key Java parity notes
- `max_connections = total_item_count - max_net_no` (Java RatsNest) degrades negative on empty boards — mirrored; normalisation clamps to 0
- `pin_is_escaped` approximates Java's `clearanceViolations().isEmpty()` away (no DRC module yet)

## WP-5: Core Tests + CMake + Docs — COMPLETED (2026-09-18)

- gtests for WP-1..4 already registered in `tests/CMakeLists.txt`:
  - WP-1: `ItemContacts_test.cpp` (8 new cases, 14 total)
  - WP-2: `NetIncompletes_test.cpp` (8 cases)
  - WP-3: `RatsNest_test.cpp` (5 cases)
  - WP-4: `BoardStatistics_test.cpp` (11 cases) + `BoardFileDetails_test.cpp` (12 cases)
- `docs/migration/INDEX.md` already reflects WP-3 and WP-4 completion
- ctest: **449/449 PASSED** + `run_tests` PASSED

## WP-6: RoutingJob / Session / BoardFileDetails

Full port of core data model with event wiring.

**COMPLETED (2026-09-18):**
- RoutingJob + Session already ported. `RoutingJob::get_file_format(content)` + `get_file_format_from_path(path)` added (Java `getFileFormat(byte[])`/`getFileFormat(Path)`: `{`→KICAD_DESIGN_JSON, SCR keyword sample, FRB Java-magic 0xACED0005, `(pcb`/`(ses`/`(rules` prefixes, extension table) ✅
- `FileFormat` enum completed to full Java parity: added `KICAD_DESIGN_JSON`, `KICAD_SESSION_JSON` (enum, to_string, file_extension, parse, get_all_formats) ✅
- `BoardFileDetails.{h,cpp,_test.cpp}` ported (Java 224 lines): CRC32 (table-based, verified vs standard vector 0xCBF43926), ctors (file→read bytes→set_data; board→statistics), set_data (size/crc32/format-detect/statistics rebuild + event), set_filename (path split, default-extension append), save_as JSON, listeners. **12 gtest cases** ✅
- **Layering**: BoardFileDetails needs scoring+board → compiles into `freerouting_scoring` lib (NOT the leaf `freerouting_core`), added `../BoardFileDetails.cpp` to scoring CMakeLists. Test link groups now include `freerouting_settings` ✅
- **Fixed pre-existing header collision**: `settings/RouterSettings.h` defined an ad-hoc legacy `RouterScoringSettings` (via_cost/corner_cost/length_cost) AND `settings/RouterScoringSettings.h` defines the real Java-mirror one — same name in `freerouting::settings`. Legacy struct removed; RouterSettings.h now includes the real header (its legacy fields were never read).
- Verified: ctest **434/434** + run_tests PASSED

## WP-7: DataStructures — COMPLETED (2026-09-18)

| Class | Java Lines | Status |
|-------|-----------|--------|
| TimeLimit | 40 | ✅ Ported + tested (6 tests) |
| Stoppable | 17 | ✅ Ported + tested (3 tests) |
| BigIntAux | 113 | ✅ Header-only port + tested (4 tests) |

All 14 datastructures Java files fully migrated. Tests: 9 gtest executables, all green.

## WP-8: Board Model Stubs (Phase 7) — COMPLETED (2026-09-18)

| Task | File | Status |
|------|------|--------|
| 7.2 opt_changed_area() | RoutingBoard.cpp | ✅ DONE — delegates to PullTightAlgo::opt_changed_area, clears changed_area, calls join_graphics_update_box |
| 7.4 insert_via() trace splitting | RoutingBoard.cpp | ✅ DONE — splits traces on padstack layers (from_layer..to_layer-1) via BasicBoard::split_traces |

Also added `ChangedArea::surrounding_box()` for graphics update notification.
All 458 ctest + run_tests PASSED.

## WP-9: Drill Pages + ExpansionDrill rooms — COMPLETED (2026-09-19)

**Java refs**: drill/DrillPage.java (189), drill/DrillPageArray.java, drill/ExpansionDrill.java, AutorouteEngine.completeExpansionRoom()
**C++ files**: src/autoroute/DrillPage.{h,cpp}, DrillPageArray.{h,cpp}, ExpansionDrill.cpp, AutorouteEngine.{h,cpp}, src/board/Pin.h

### Completed
1. `Pin::drill_allowed()` — `first_layer() == last_layer()` (Java Pin.java:348); verified Java `Item.isDrillable` semantics match C++ (base false; Trace=containsNet @ Trace.cpp:290; ConductionArea=`!is_obstacle || containsNet` @ ConductionArea.cpp:152; DrillItem does NOT override → pins/vias are obstacles, not drillable). No DrillItem change needed ✅
2. `AutorouteEngine`: added `board()`, `add_incomplete_expansion_room(IncompleteFreeSpaceExpansionRoom*)` (takes ownership via unique_ptr vector), `complete_expansion_room(IncompleteExpansionRoom*)` → single `CompleteFreeSpaceExpansionRoom`; room lists are now `unique_ptr` vectors; `get_autoroute_search_tree()` now auto-initializes (lazy tree build) ✅
3. `DrillPage` — full port: `get_drills(engine, attach_smd)` with net-no caching + invalidate(); `tile_intersection` (box/box exact via dynamic_cast, else Simplex fallback), `tile_contains` (bounded-corner check → vias same-shape dedup), `split_to_convex_with_holes` (replicates `PolylineArea::split_to_convex` over TileShapes), `calc_pin_center_in_drill`; obstacle cutouts → convex pieces → one `ExpansionDrill` per piece ✅
4. `DrillPageArray` — rewritten from stale draft: raw `RoutingBoard*`, `calc_column_count/row_count/page_size` static helpers, overlapping_pages (floor/ceil index math, shape∩page dimension>1 filter), invalidate/reset/get_page ✅
5. `ExpansionDrill::calculate_expansion_rooms` — real port: `TileShape::get_instance(location)`, `overlapping_objects(shape,-1)` into the `TreeStorableLess` set, per-layer existing `CompleteExpansionRoom` lookup else `IncompleteFreeSpaceExpansionRoom(0, layer, shape)` → engine `complete_expansion_room` (must return exactly 1) ✅
6. Fixed `get_autoroute_search_tree()` null-before-init bug (first get_drills produced no drills) ✅
7. Tests: `DrillPage_test.cpp` (5) + `DrillPageArray_test.cpp` (4) + pre-existing `ExpansionDrill_test.cpp` — registered in tests/CMakeLists.txt AUTOROUTE_TEST_SOURCES; **ctest 467/467 + run_tests PASSED** ✅

### Documented WP-9 simplifications (deliberate)
- `complete_expansion_room` is a minimal port of Java `completeExpansionRoom()`: `ShapeSearchTree::complete_shape()` is now ported and tested as a board API (WP-10 Inc B) but its **engine rewiring** is NOT done — `complete_expansion_room` still inflates sub-2D room shapes via `IntBox::offset(1)` instead of tree-expanding, so obstacle-room splitting is exercised only in the unit tests. Next iteration: rewire `complete_expansion_room`/`ExpansionDrill`.
- C++ `ShapeSearchTree` now stores `SearchTreeObject*` leaves (Items AND ExpansionRooms), fixing the old "stores only Item*" limitation — rooms are found/reused by overlap queries (WP-10 Inc A) ✅
- C++ rooms are IntBox-based; `ExpansionDrill` stores the exact drill piece shape's `bounding_box()` (exact shape only used to compute the drill location / attach_smd pin center).
- `hover (WP-10)`: SortedRoomNeighbours family (obstacle room doors, set_net_dependent/is_net_dependent, calculateTargetDoors, remove_all_doors) NOT part of this WP.

### Key implementation notes
- `IntBox::is_IntOctagon()` returns true ("can be represented") → always use `dynamic_cast`, never `static_cast`, between box/octagon.
- `ShapeSearchTree` is built lazily and not auto-updated; tests insert newly added obstacle items into the engine tree explicitly (mirrors router-maintained database).
- Autoroute gtest link added `freerouting_settings` (AutorouteControl.cpp references `RouterSettings::is_layer_active`) — pre-existing latent link break the new tests exposed.

## WP-10: Expansion room completion (derived from WP-9 analysis) — IN PROGRESS

Port the room-neighbour/completion machinery. Prerequisites to resolve before starting:

1. `ShapeSearchTree` room storage — the C++ tree stores only `Item*`; it needs to also store ExpansionRooms (Java `autorouteSearchTree` holds ShapeTree.Storable items AND rooms). Today `ExpansionDrill` can never find existing rooms.
2. `ShapeSearchTree.completeShape()` / blocker #6 — full free-space room expansion (`complete_expansion_room` is currently minimal empty-board-only).
3. `ItemAutorouteInfo::get_expansion_room()` — stub.
4. `CompleteFreeSpaceExpansionRoom::calculateTargetDoors()` — needs the `Connectable::get_trace_connection_shape()` analogue.
5. `AutorouteEngine::remove_all_doors()`.

Classes: `ExpansionRoom` additions (`ObstacleExpansionRoom::create_overlap_door`, `CompleteFreeSpaceExpansionRoom::set_net_dependent/is_net_dependent`), `SortedRoomNeighbours` (680), `SortedOrthogonalRoomNeighbours` (482), `Sorted45DegreeRoomNeighbours` (810), then `MazeSearchAlgo::init`/`expand_to_room_doors` rewrite.

### Increment A: rooms storable in the search tree (2026-09-19)

**Prerequisite #1 resolved.** Complete free space expansion rooms are now `ShapeTree::Storable`/`board::SearchTreeObject` and are stored in the autoroute search tree.

1. `CompleteFreeSpaceExpansionRoom` implements `board::SearchTreeObject` (was `CompleteExpansionRoom` only): `tree_shape_count()==1`, `get_tree_shape()` returns the room's IntBox, `compare_to()` (rooms ordered by reversed id; any non-room → -1), `is_obstacle/is_trace_obstacle==true`, `shape_layer()`==layer ✅
2. `set_net_dependent()`/`is_net_dependent()` + `remove_from_tree(ShapeSearchTree&)` (generic `ShapeTree::remove` on stored leaves) ✅
3. `ShapeSearchTree::insert(SearchTreeObject&)` overload (delegates to `ShapeTree::insert(Storable&)`); `clear()` now removes all leaves generically instead of `static_cast<Item*>` (rooms would have been UB) ✅
4. `AutorouteEngine::complete_expansion_room` inserts the completed room into the engine tree (mirrors Java `addCompleteRoom()` line 535); `clear()` removes all rooms from the tree; `remove_net_dependent_rooms()` + `remove_complete_expansion_room()` (Java `initConnection` purge / `removeCompleteExpansionRoom`) ✅
5. `ExpansionDrill` existing-room lookup is now real: a recomputed drill on the same location reuses the stored room. Guard added: reuse only if the stored room contains the drill location (Java's complete rooms are maximal free spaces so the location is always inside; the C++ interim micro-rooms require the explicit containment check) ✅
6. Tests: `CompleteFreeSpaceExpansionRoom_test.cpp` (5: Storable contract, compare_to ordering, room-in-tree round trip + clear, net-dependent purge, drill room stored & reused after recalculation) — registered in tests/CMakeLists.txt; **ctest 472/472 + run_tests PASSED** ✅

### Increment B: `ShapeSearchTree::complete_shape()` (blocker #6, 2026-09-22)

**Prerequisite #2 resolved at the board level.** Full port of Java `ShapeSearchTree.completeShapeUnlocked()` as a tested board API.

1. New board-level structs in `ShapeSearchTree.h`: `CompleteShapeInput` (`shape` may be null = whole board box, `contained_shape`, `layer`, `net_no`, `ignore_object`, `ignore_shape`) and `CompleteShapePiece` (`shape` + `contained_shape` unique_ptrs) ✅
2. `complete_shape()` port: start shape = board box ∩ input shape; deterministic leaf walk via local vector stack (`root==nullptr` → whole-board deviation documented in code); leaves sorted by `compare_to`; obstacle loop with free-space-room ignore check (`is_free_space_room()` + `ignore_shape` containment) and `IntOctagon` bounding-shape union pruning ✅
3. Private helpers: `intersection_of_two_shapes` (Simplex-based + simplify), `shape_contains` (exact for RegularTileShape pairs, conservative false otherwise), `restrain_shape` (exact port of Java `restrainShape`: Simplex obstacle, cut-line selection via `distance_to_the_left`, opposite-halfplane branch, collinear second branch + recursion; contained shape kept as Simplex-mirroring clone), `divide_large_room` (uses `IntBox::divide_into_sections`) ✅
4. Supporting ports: `TileShape::is_intersected_interior_by(LineSegment&)` + 3-arg (TileShape.java:923-1022), `LineSegment(const TileShape&, int)` ctor, `BasicBoard::get_bounding_box()` virtual, `RoutingBoard` override, `SearchTreeObject::is_free_space_room()` virtual (`true` in `CompleteFreeSpaceExpansionRoom`) ✅
5. Tests: `ShapeSearchTree_complete_shape_test.cpp` (9: empty-tree small-room exact, whole-board divided into 4, single obstacle restrains, net/layer filtering, ignore-object skip, free-space room ignored when contained in ignore shape, free-space room restrains without ignore shape, straddling obstacle splits room) — **ctest 481/481 + run_tests PASSED** ✅
6. NOT rewired this iteration (next iteration scope): `AutorouteEngine::complete_expansion_room`/`ExpansionDrill` to call `complete_shape` and the Java 45/90-degree room-variant subclasses.

### Increment C: engine rewiring onto `complete_shape` (2026-09-22)

**Engine rewiring complete.** `AutorouteEngine::complete_expansion_room` now runs through `ShapeSearchTree::complete_shape()` instead of the interim empty-board inflation.

1. `complete_expansion_room` port of Java `completeExpansionRoom()`: door scan finds the first dimension-2 door to a `CompleteFreeSpaceExpansionRoom` → `ignore_object` + `from_door_shape` (= door shape); `CompleteShapeInput` built with bounded start + contained shapes; `remove_incomplete_expansion_room` (erases from the engine list when registered; no-op for drill stack rooms); first/rest result loop — first 2-D piece added directly, later pieces re-run through `complete_shape` (Java recalc) ✅
2. Bounded start mapping (documented deviation): `shape = contained = room box` for 2-D rooms; sub-2-D (unit drill shape) rooms get `offset(1.0)` start with the original point as contained. Prevents whole-board start from splitting an empty board into 4 sections (does not break empty-board drill tests) while still restraining rooms around obstacles ✅
3. Each 2-D `CompleteShapePiece` → `CompleteFreeSpaceExpansionRoom(layer, generate_room_id_no(), bounding_box)`, inserted into the engine search tree (Java `addCompleteRoom`) ✅
4. Tests: `CompleteFreeSpaceExpansionRoomTest.RoomIsRestrainedAroundObstacle` — whole-board room on a net-2 wall → multi-piece completion, every piece avoids the wall interior, at least one piece differs from the raw box (would fail on the old passthrough). **ctest 482/482 + run_tests PASSED** ✅
5. NOT done (future increments): Java SortedRoomNeighbours door calculation (`calculateDoors` after `addCompleteRoom`), room splitting for the 45/90-degree room variants, `ExpansionDrill` callers beyond the exact-1 invariant.

### Increment D: net-connectable room doors (prereqs #3/#4/#5, 2026-09-22)

**Prereqs #3, #4, #5 + `ObstacleExpansionRoom::create_overlap_door` ported.** Doors between obstacle (item) rooms, from free-space rooms to the routeable items of the routing net, and their cleanup.

1. `ItemAutorouteInfo::get_expansion_room()` port of Java `getExpansionRoom()`: room array sized to `item->tree_shape_count(tree)` with resize-preserving (`std::vector::resize(count, nullptr)`), out-of-range → `FRLogger` warn + nullptr, lazy `new ObstacleExpansionRoom(item, index, tree)`; `reset_doors()` now walks the rooms; destructor frees the room array ✅
2. `ObstacleExpansionRoom` port: new tree-based ctor `ObstacleExpansionRoom(Item*, int, ShapeSearchTree*)` (layer = `item->shape_layer(index)`, shape = tree shape bounding box, room id = `(item->get_id() << 10) | index`); the 4-arg ctor now computes the Java room id instead of `0`; `create_overlap_door(ObstacleExpansionRoom*)` — door_exists guard → both `is_routable()` → `shares_net` → same-item requires `PolylineTrace` + consecutive indices → `ExpansionDoor(this, other, 2)` added to both ✅
3. `CompleteFreeSpaceExpansionRoom::calculate_target_doors(TreeEntry, net_no, ShapeSearchTree*)`: `set_net_dependent()` first; C++ `Connectable` is a dead draft implemented by no real item → connectable guard mapped to `board::Item` that is a trace/via/pin/ConductionArea (documented divergence); `contains_net`; connection shape = item tree shape (exact `Connectable::get_trace_connection_shape` not ported; drills approximated by tree shape instead of the Java center-point shape); room-shape intersection via `IntBox::intersects(to_Simplex())`; creates `TargetItemExpansionDoor` into `target_doors()` ✅
4. `TargetItemExpansionDoor` now carries a real door shape: ctor takes the `ShapeSearchTree*` and stores `room.get_shape().intersection(item_shape->bounding_box())`, `get_shape()` overridden, dimension 2 (Java `getDimension()==2`); `item` stays public for `MazeSearchAlgo::is_destination` ✅
5. `AutorouteEngine::remove_all_doors(ExpansionRoom*)` public port of Java `removeAllDoors()`: walk the doors of the room and every door-connected incomplete free-space room, removing each door from both sides, then erase the walked incomplete rooms from the engine database — destruction deferred until after the whole door walk (Java relies on the GC; the C++ rooms are owned by the engine vector, so deleting during iteration would be UAF). Wired as the first step of `remove_incomplete_expansion_room` (Java `removeIncompleteExpansionRoom()` line 368) ✅
6. NOT done (future increment): the `MazeSearchAlgo::init`/`expand_to_room_doors` rewrite (the `SortedRoomNeighbours` base and the 45 degree variant that *create* these doors are done in Inc E/F; the `removeCompleteExpansionRoom` neighbour split is done in Inc H).
7. Tests: `ItemAutorouteInfo_test.cpp` (10: room creation/caching, out-of-range null, per-shape rooms, overlap door same-net + different-net denial + same-trace consecutive/non-adjacent, target doors for overlapping net item incl. shape + dimension, skipped for disjoint/other-net/non-item, remove_all_doors clears both sides, completing a registered room erases it from the database). **ctest 495/499 + run_tests PASSED** — the 4 failing ctest are pre-existing untracked `DesignRulesChecker_test.cpp` (in-progress DRC rework, no autoroute dependency) ✅

### Increment E: `SortedRoomNeighbours` (base) — neighbour collection + 1-D doors (2026-09-26)

**Base room-neighbour class ported** (`src/autoroute/SortedRoomNeighbours.h/.cpp`, 806-line Java source). Doors are now *created* from a room shape; the 45° subclass is ported in Increment F, `try_remove_edge` / `calculate_new_incomplete_rooms` / the orthogonal subclass stay deferred.

1. `SortedRoomNeighbour` port of the Java inner class: `compare_to()` (side no → distance to the side corner → last-corner distance → neighbour-corner direction tie-break, Java `c_dist_tolerance` = 1), lazily cached `first_corner()`/`last_corner()` (Java precalculated corner caches), `C_DIST_TOLERANCE` constant ✅
2. `SortedRoomNeighbours::calculate_neighbours(ExpansionRoom*, net_no, ShapeSearchTree*, room_id_no)`: creates a `CompleteFreeSpaceExpansionRoom` for an `IncompleteFreeSpaceExpansionRoom` from room (or uses the `ObstacleExpansionRoom` itself), collects `overlapping_tree_entries`, sorts them deterministically by `get_id()` + shape index (Java `TreeSet` order, documented deviation for parity), skips the room itself, routes own-net non-trace-obstacle objects into `own_net_objects` (delayed target doors), and per entry: 2-D → `create_overlap_door` for routable items (free-space overlap → trace log only), 1-D → `touching_sides` + sorted neighbour + `insert_door_ok` + dimension-1 `ExpansionDoor` on both sides, 0-D → `equals_corner` / `contains_on_border_line_no` corner classification with the Java "prefer the previous border line" rule ✅
3. `insert_door_ok` both overloads: 3-arg (room pair + door shape) → `door_exists` guard → both-obstacle ⇒ `shares_net`, both-free-space ⇒ true, mixed ⇒ door-line parallelity check; 2-arg (obstacle room + door line) ⇒ `arr[room_index + 1]` of the polyline must be parallel. Java's nullable door line is modelled as `std::optional<Line>` (rejected when disengaged) ✅
4. `calculate_incomplete_rooms_with_empty_neighbours(ObstacleExpansionRoom*, AutorouteEngine*)`: per border line → `insert_door_ok` → `get_instance(line.opposite())` half-plane as new room shape + `intersection_of` as contained shape → `AutorouteEngine::add_incomplete_expansion_room` + dimension-1 door both sides ✅
5. `ExpansionRoom` generalization needed by the port: `ExpansionRoom::get_shape_tile()` (Simplex view of the room shape, overridden by the rooms carrying an exact Simplex), `CompleteFreeSpaceExpansionRoom`/`ObstacleExpansionRoom`/`IncompleteFreeSpaceExpansionRoom` Simplex constructors + `set_shape_tile()`, `SearchTreeObject::get_id()` (deterministic neighbour tie-break) ✅
6. `TileShape` supporting ports: `equals_corner`, `prev_no`, `next_no`, `touching_sides`, static `intersection_of` (delegated to by `ShapeSearchTree::intersection_of_two_shapes`) ✅
7. **Divergence found & fixed**: `TileShape::intersection_of` used to `simplify()` every result. Java `Simplex.intersection()` does *not* simplify, and `Simplex::simplify()` only checks 45° multiples + `corner_is_bounded` — so a valid 0/1-dimensional intersection (e.g. octagon ∩ opposite half-plane = a valid segment) was converted into a degenerate `IntOctagon` with `dimension() == -1` and `is_empty() == true`, which made `calculate_incomplete_rooms_with_empty_neighbours` silently create no rooms. Now only 2-dimensional results are simplified ✅
8. Tests: `SortedRoomNeighbours_test.cpp` (15: empty board, unexpected room type → nullptr, 1-D neighbour + door + ownership release, 2-D free-space overlap ignored, point touch corner flags, counterclock sorting of 3 sides, own-net objects vs trace-obstacle neighbours, obstacle-room 2-D overlap door same net / ignored other net / 1-D door to same-net item room, `insert_door_ok` door+net+point-door+existing-door cases, `insert_door_ok` fork parallelity, empty-neighbour incomplete rooms, null-argument guard, corner accessor caching) — **ctest 515/515 + run_tests PASSED** ✅
9. NOT done (next increments): `SortedOrthogonalRoomNeighbours` and the `removeCompleteExpansionRoom` neighbour-split half. `try_remove_edge_line`/`calculate_new_incomplete_rooms` are done for the 45 degree variant in Increment F. (Both items have since landed: Inc G and Inc H.)

### Increment F: `Sorted45DegreeRoomNeighbours` — 45° room completion (2026-09-27)

**45 degree room-neighbour class ported and wired in** (`src/autoroute/Sorted45DegreeRoomNeighbours.h/.cpp`, 810-line Java source). This is the default route angle of the Java autorouter, so the maze search now gets the 45 degree room completion.

1. `Sorted45DegreeRoomNeighbour` port of the Java inner class: 8 border lines (0 bottom, 1 lower-right diagonal, 2 right, 3 upper-right diagonal, 4 top, 5 upper-left diagonal, 6 left, 7 lower-left diagonal), `calculate_first_touching_side()` / `calculate_last_touching_side()` with the Java `equals_corner` + `corner_distance` rules, and the `edge_interior_touches_obstacle` bit vector that is filled while walking the tree entries ✅
2. `calculate_neighbours(ExpansionRoom*, net_no, ShapeSearchTree*, room_id_no)`: new `CompleteFreeSpaceExpansionRoom` for an `IncompleteFreeSpaceExpansionRoom` from room, `ObstacleExpansionRoom` used as-is, deterministic entry order, own-net free-space entries → `calculate_target_doors` immediately, dimension > 1 + obstacle room → `create_overlap_door` for routable items, dimension < 0 → skip, otherwise `add_sorted_neighbour` + dimension-1 door when `insert_door_ok` ✅
3. `add_sorted_neighbour()`: counterclock `lower_bound` insert with the Java `TreeSet` "comparing-equal neighbour is not inserted twice" rule; `last_touching_side < 0` drops a neighbour that contains the room shape ✅
4. `remove_not_touching_border_lines()` / `try_remove_edge_line()`: the room octagon sides with no touching obstacle are set to `±CRIT_INT` and re-normalized; the enlarged shape is then completed with `ShapeSearchTree::complete_shape` (largest 2-dimensional door as `ignore_shape`) and only applied when the area grows (endless-loop guard), which is the Java `tryRemoveEdgeLine` behaviour ✅
5. `calculate_new_incomplete_rooms()`: free-space variant (8 `first_touching_side` cases + door creation for a 2-dimensional contained shape) and `calculate_new_incomplete_rooms_for_obstacle_expansion_room()` / `calculate_edge_incomplete_rooms_of_obstacle_expansion_room()` with the Java `fromSideIndex == toSideIndex` early return ✅
6. `calculate()`: the Java recursion — `try_remove_edge_line` restarts the calculation, a completed from room falls back to `complete_shape` on the completed room, the new incomplete rooms drive the retry loop, and a non-null result releases the owned completed room ✅
7. **Dispatch ported**: `SortedRoomNeighbours::calculate()` now selects `Sorted45DegreeRoomNeighbours` when the autoroute search tree has 8 bounding directions, using the new public `ShapeTree::get_bounding_directions()` accessor; a tree with 4 directions is handled by the orthogonal variant of Increment G. `AutorouteEngine::add_complete_room` needed no change because it already calls the base `calculate` ✅
8. **Divergence contained**: C++ `IntOctagon::is_empty()` treats degenerate octagons as empty (Java compares against the `IntOctagon.EMPTY` singleton), so C++ `dimension()` returns -1 for 1-dimensional shapes. Local `java_is_empty()` / `java_dimension()` helpers reproduce the Java branch behaviour; the shared geometry class stays untouched ✅
9. **Known approximation**: the C++ `ExpansionDoor` has no own door shape, so the intersection box of the door rooms is passed as the `ignore_shape` of `complete_shape` ✅
10. Tests: `Sorted45DegreeRoomNeighbours_test.cpp` (11: empty tree, unexpected room type, left neighbour first/last touching side 5/7 + the interior-touch flag 6 + dimension-1 door, containing neighbour creates its door but is dropped from the sorted neighbours, non-overlapping neighbour skipped, counterclock sorting of the 4 axis-side neighbours, room enlargement by edge removal, second enlargement rejected, new incomplete rooms between neighbours (2-dimensional doors, board bounded), `calculate` enlargement + completion, and the base-class dispatch to the 45 degree implementation) — **ctest 526/526 + run_tests PASSED** ✅
11. NOT done (next increments): the Java `removeCompleteExpansionRoom` neighbour-split half (landed in Increment H) and the `MazeSearchAlgo::init`/`expand_to_room_doors` rewrite (`SortedOrthogonalRoomNeighbours` landed in Increment G).

### Increment G: `SortedOrthogonalRoomNeighbours` — 90° room completion (2026-09-27)

**Orthogonal room-neighbour class ported and wired in** (`src/autoroute/SortedOrthogonalRoomNeighbours.h/.cpp`, 482-line Java source). This completes the `SortedRoomNeighbours` family: base (any-angle), 45° and 90° are all ported, and the calculation mode is selected from the search tree bounding directions.

1. `SortedOrthogonalRoomNeighbour` port of the Java inner class: 4 border lines (0 bottom, 1 right, 2 top, 3 left), `calculate_first_touching_side()` / `calculate_last_touching_side()`, the `edge_interior_touches_obstacle` flags filled while walking the tree entries, and a `compare_to()` with the Java side/corner ordering plus the `get_id()` tie-break for the `TreeSet` ✅
2. `calculate_neighbours(ExpansionRoom*, net_no, ShapeSearchTree*, room_id_no)`: new `CompleteFreeSpaceExpansionRoom` for an `IncompleteFreeSpaceExpansionRoom` from room, `ObstacleExpansionRoom` used as-is, deterministic entry order (by `get_id()` + shape index), own-net free-space entries → `calculate_target_doors` immediately, dimension > 1 + obstacle room → `create_overlap_door`, dimension < 0 → skip, otherwise `add_sorted_neighbour` + dimension-1 door when `insert_door_ok`. Unlike the 45° port this variant needs the `IntBox` form of the shapes, so a non-box room shape *or* a non-box neighbour tree shape aborts the calculation like in Java ✅
3. `remove_border_line()` / `try_remove_edge()`: the first side without an interior obstacle touch is set to `±CRIT_INT`; the enlarged shape is completed with `ShapeSearchTree::complete_shape` (largest 2-dimensional door as ignore object + shape) and only applied when the area grows (endless-loop guard) ✅
4. `calculate_new_incomplete_rooms()`: free-space variant (the 4 `first_touching_side` cases with the Java skip conditions) and the obstacle-room variant, plus `insert_incomplete_room()` (new room + contained shape = room ∩ new room + engine registration + 2-dimensional door) ✅
5. `calculate_incomplete_rooms_with_empty_neighbours()`: the 4 half-plane rooms between the room border lines and the board border, each with a dimension-1 door ✅
6. `calculate()`: the Java recursion — `try_remove_edge` restarts the calculation after `remove_all_doors`, a result with no sorted neighbours falls back to the empty-neighbour rooms of an obstacle room, and a non-null owned result releases the completed room ✅
7. **Dispatch extended**: the direction count check became the named `select_calculation_mode()` returning `CalculationMode::ORTHOGONAL` (4 directions), `DEGREE_45` (8) or `ANY_ANGLE` (everything else), so the any-angle base class is only used for a genuinely non-orthogonal/non-45-degree tree ✅
8. **Supporting port**: the Java-parity 2-argument `ExpansionDoor(ExpansionRoom*, ExpansionRoom*)` constructor (Java creates the doors of the new incomplete rooms from the room pair; the C++ door computes its shape as the intersection of the 2 room shapes) ✅
9. **Divergence contained**: Java `ShapeSearchTree.completeShape` returns an empty result for a tree without a root, while the C++ `complete_shape` falls back to the whole board; `try_remove_edge()` therefore skips the enlargement when the search tree is empty, which keeps the Java "no enlargement" behaviour ✅
10. Tests: `SortedOrthogonalRoomNeighbours_test.cpp` (13: empty tree + ownership hand-off, unexpected room type / null arguments, non-box room shape rejected, non-box neighbour shape aborts, left neighbour touching side 3 + flag + dimension-1 door, counterclock sorting of the 4 side neighbours, room enlargement by edge removal, second enlargement rejected (area guard), enlargement skipped for an obstacle from room, new incomplete rooms between neighbours with 2-dimensional doors, `calculate` enlargement + completion, 4 half-plane rooms for an obstacle room with empty neighbours, `compare_to` side/corner ordering + id tie-break, `select_calculation_mode` for 45°/90°/6-direction/null trees) — **ctest 539/539 + run_tests PASSED** ✅
11. NOT done (next increments): the `MazeSearchAlgo::init`/`expand_to_room_doors` rewrite (the `removeCompleteExpansionRoom` neighbour-split half landed in Increment H).

### Increment H: `AutorouteEngine` room-database housekeeping (2026-09-27)

**The three remaining room-database methods of the Java `AutorouteEngine` ported** (`src/autoroute/AutorouteEngine.h/.cpp` + new `src/autoroute/AutorouteEngine_test.cpp`).

1. `remove_complete_expansion_room()` is now the full port of Java `removeCompleteExpansionRoom()` (`AutorouteEngine.java:377-412`) and public like the Java method. The neighbour-split half: per door of the room the door is removed from the neighbour, the intersection of the exact Simplex room/neighbour shapes is computed, and a 1-dimensional intersection creates a new `IncompleteFreeSpaceExpansionRoom` whose shape is the half plane behind the neighbour's `touching_sides` border line (`border_line().opposite()` + `TileShape::get_instance`) and whose contained shape is the intersection; the new room is registered with `add_incomplete_expansion_room()` and gets a dimension-1 `ExpansionDoor` on both sides. Then `remove_all_doors()`, `remove_from_tree()`, the database erase with the Java warning for an unregistered room, and the `unique_ptr` release of the engine-owned room ✅
2. `complete_neighbour_rooms()` port of Java `completeNeighbourRooms()` (`AutorouteEngine.java:567-592`): an incomplete free space neighbour is completed, the neighbour is skipped, and the door iteration restarts on the updated door list; an obstacle room neighbour gets its doors calculated only once (`all_doors_calculated()` / `set_doors_calculated()`) ✅
3. `get_first_incomplete_expansion_room()` port of Java `getFirstIncompleteExpansionRoom()` ✅
4. **Java cast gotcha honoured**: `ExpansionDoor::other_room()` must be called through an `ExpansionRoom*` — the `CompleteExpansionRoom*` overload `dynamic_cast`s the result and returns `nullptr` for a door to an incomplete room, which is the C++ counterpart of the explicit `(ExpansionRoom) room` cast in the Java code. Without it `complete_neighbour_rooms` silently completes nothing ✅
5. **Contained divergence**: Java also calls `drillPageArray.invalidate(roomShape)`; the C++ engine owns no `DrillPageArray` (it would need `rules::BoardRules::get_default_via_diameter()`), so that single line is not ported ✅
6. Tests: `AutorouteEngine_test.cpp` (8: first incomplete room null/added/cleared, neighbour split creates the half-plane incomplete room with a dimension-1 door and empties the search tree, 2-D overlap neighbour skipped, null room ignored, unregistered room warning path, `complete_neighbour_rooms` completes an incomplete free space neighbour into the tree, obstacle neighbour doors calculated only once, null room ignored) — **ctest 547/547 + run_tests PASSED** ✅

### WP-10 remaining (next increments)
- **Increment D (2026-09-22) DONE**: net-connectable room doors — `get_expansion_room`, `create_overlap_door`, `calculate_target_doors`, `remove_all_doors`. 495/499 ctest (4 pre-existing DRC failures) + run_tests green. See Increment D above.
- **Increment E (2026-09-26) DONE**: `SortedRoomNeighbours` (base) — neighbour collection in counterclock order, 1-D/2-D door creation, own-net object collection, `calculate_incomplete_rooms_with_empty_neighbours`, plus the room `get_shape_tile()` generalization and the `TileShape` helper ports. 515/515 ctest + run_tests green. See Increment E above.
- **Increment F (2026-09-27) DONE**: `Sorted45DegreeRoomNeighbours` — 45° room completion (touching sides, door creation, room enlargement, new incomplete rooms) plus the bounding-direction dispatch from `SortedRoomNeighbours::calculate`. 526/526 ctest + run_tests green. See Increment F above.
- **Increment G (2026-09-27) DONE**: `SortedOrthogonalRoomNeighbours` — 90° room completion (touching sides, door creation, room enlargement, new incomplete rooms) plus the `select_calculation_mode` dispatch and the Java-parity 2-argument `ExpansionDoor` constructor. 539/539 ctest + run_tests green. See Increment G above.
- **Increment H (2026-09-27) DONE**: `AutorouteEngine` room-database housekeeping — `remove_complete_expansion_room` incl. the neighbour split, `complete_neighbour_rooms`, `get_first_incomplete_expansion_room`. 547/547 ctest + run_tests green. See Increment H above.
- **Increment I (2026-09-27) DONE**: the `MazeSearchAlgo` expansion-room core — the grid A* is replaced by a 1175-line port of Java `MazeSearchEngine` (`get_instance`/`init`/`find_connection`/`occupy_next_element`/`expand_to_room_doors`/`expand_to_target_doors`/`expand_to_door`/`expand_to_door_section`/`door_is_small`/`room_shape_is_thick`/`check_neck_down_at_dest_pin`/`segment_projection`), the queue is a `std::set<MazeListElement>` with the Java `compareTo` order, and the expansion cost is the Java `weightedDistance` plus a normalized bend-cost penalty. New stable-id prerequisites (`IntPoint`/`IntBox`/`ExpandableObject`/door/target-door/drill/drill-page/incomplete-room `get_id`, `TileShape::min_width`/`max_width`), `AutorouteControl::bend_costs`, `Pin::min_width`/`get_trace_neckdown_halfwidth`, `AutorouteEngine::get_maintain_database`/`get_rooms_with_target_items`. Deferred with a code comment and a tracking entry: `MazeExpansionEngine` (drill pages/other layers), `MazeRipupResolver`, `MazeTraceShover`, `MazeFanoutDiagnostics`, the tie-pin shape reduction and `check_forced_trace_polyline`. 12 new gtests in `MazeSearchAlgo_test.cpp` + 12 in the new `MazeSearchElement_test.cpp` — **ctest 572/572 + run_tests PASSED** ✅
- **Increment J (2026-09-27) DONE**: `MazeRipupResolver` (task 3.10) — `check_ripup`/`check_leaving_ripped_item`/`calc_fanout_via_ripup_cost_factor` plus the private `enter_through_small_door`, the header-only `MazeRandom` (exact `java.util.Random` port, verified against the JDK 25 of this workspace) and `RouterSettings::get_start_ripup_costs()`. `MazeSearchAlgo::expand_to_room_doors()` calls the resolver in both branches; the unported `MazeTraceShover` branch is emulated exactly as Java does (the delayed `MazeListElement` with `already_checked`/`room_ripped`/ripup cost is enqueued when the shove would have failed). 31 new gtests in `MazeRipupResolver_test.cpp` — **ctest 603/603 + run_tests PASSED**, and `Issue022-AutoRouter_interrupted.dsn` still routes end to end ✅
- **Task 3.14 verified DONE (2026-09-27)**: the "wire `calculate_target_doors()`" blocker was stale — the call already exists in all three `Sorted*RoomNeighbours::complete_expansion_room()` (`SortedRoomNeighbours.cpp:648`, `Sorted45DegreeRoomNeighbours.cpp:435`, `SortedOrthogonalRoomNeighbours.cpp:336`) and is unit-tested in `ItemAutorouteInfo_test.cpp` ✅
- **Increment K (2026-09-30) DONE**: `MazeTraceShover` (tasks 3.5/3.11) — `check_shove_trace_line()`, `shove_trace_room()`, `end_points_matching()` and the public `DoorSection` result struct, wired into `MazeSearchAlgo::expand_to_room_doors()` in place of the "shove not ported" divergence: the left and right shove sections are now really attempted and the delayed `MazeListElement` is enqueued only when a section reports `false`. 14 new gtests in `MazeTraceShover_test.cpp` — **ctest 617/617 + run_tests PASSED** ✅
- **Increment L (2026-10-01) DONE**: `MazeExpansionEngine` (task 3.9) is ported as `src/autoroute/MazeExpansionEngine.h/.cpp` with 11 gtests — `expand_to_drill`, `expand_to_drill_page`, `expand_to_drills_of_page`, `expand_to_other_layers`, `check_layer_with_any_matching_via`, wired into `MazeSearchAlgo::occupy_next_element()`/`expand_to_room_doors()`. Prereqs in the same increment: `AutorouteEngine` owns the `DrillPageArray` (Java page width `max(5 * via_diameter, 10000)`, invalidated on room removal), `AutorouteControl::via_rule` + `rebuild_via_info`, `ForcedViaAlgo::check_layer` with `p_trace_half_width`/`p_trace_clearance_class`, `BoardRules::get_default_via_rule`, the three `Pin` trace-exit-restriction methods. **Layering lesson**: the Java per-`Via` drill cache cannot become a `Via` member — `ExpansionDrill` is an autoroute type, and a `unique_ptr` member made every board TU that destroys a `Via` pull `AutorouteEngine.o` → `AutorouteControl.o` → `freerouting_settings`, breaking 6 link targets; the cache lives in `MazeExpansionEngine` keyed by via. Also ported the long-standing `ItemAutorouteInfo::get_expansion_room` stub (fixed 5 pre-existing failures) and fixed a use-after-free in `remove_complete_expansion_room`. **ctest 628/628 + run_tests PASSED** ✅
- **Increment M (2026-10-01) DONE**: the `FoundConnectionLocator` family (Phase-4 tasks 4.1-4.3) — `FoundConnectionLocator.h/.cpp` (get_instance angle dispatch, the door backtrack with `BacktrackElement`, the ripped obstacle room collection, the `currentSectionNo` clamp and the Java warnings, the connection-item construction), `FoundConnectionLocator45Degree` and `FoundConnectionLocatorAnyAngle` (`calculate_next_trace_corners`, `calc_door_left/right_corner`, the turn helpers). Prereqs in the same increment: `FloatPoint::tangential_points`/`left_tangential_point`/`right_tangential_point`, `TileShape::index_of_left_most_corner`/`index_of_right_most_corner`/`shrink`, `MazeSearchAlgo::describe_expandable`/`describe_room`, and the `TargetItemExpansionDoor` fix (single `MazeSearchElement` + the missing `shape_` member that broke `ExpansionRoom.cpp`). 13 + 6 + 8 = 27 new gtests — **ctest 655/655 + run_tests PASSED** ✅
- **Increment N (2026-10-02) DONE**: the `FoundConnectionInserter` prerequisites, part 1 — `ShoveTraceAlgo::spring_over()`/`spring_over_obstacles()`/`get_ignore_items_at_tie_pins()` are now the full Java `TraceShover` ports (java TraceShover.java:611-875). Prereqs in the same increment: `Item::get_all_contacts()`/`get_all_contacts(int)` (Java `Item.getAllContacts`, the C++ `Connectable` is a dead draft so the Java interface test is reproduced with `Trace`/`DrillItem`/`ConductionArea` casts), `TileShape::cutout(Polyline)`, `TileShape::bounding_octagon()`, `DrillItem::get_tree_shape_on_layer()` (added to the **live** `DrillItem` of `board/Via.h` — `board/DrillItem.h` is a stale uncompiled duplicate whose class name collides with it, so the two headers must never be included together) and the missing `BasicBoard::insert_trace`/`check_polyline_trace`/`get_max_generated_id`/`max_trace_half_width_`/`min_trace_half_width_` (the tree did not compile before). Java's nullable-return-plus-identity contract is mapped to `SpringOverResult{std::optional<Polyline>, bool changed}`. Two geometry quirks were confirmed as **Java parity, not bugs**, and are pinned by tests: `Polyline::first_corner()` is `corner(0)` = the junction of lines 0 and 1 (not the first point), and `TileShape::entrance_points()`/`cutout()` only scan the lines 1..n-2, so a 2-corner polyline is never cut. 7 new gtests in `ShoveTraceAlgo_test.cpp`, 6 in `ItemContacts_test.cpp`, 7 in `TileShape_test.cpp` — **ctest 673/673 + run_tests PASSED** ✅
- **Increment O (2026-10-02) DONE**: the `FoundConnectionInserter` prerequisites, part 2 — `ShoveTraceAlgo::check()` (shape + line-segment overload) and `ShoveTraceAlgo::insert()` are full Java `TraceShover` ports with the real board mutation, together with `ShapeTraceEntries::next_substitute_trace_piece()`, the board-mutating `cutout_trace()`/`fast_cutout_trace()` (list **and** search tree, because the C++ `RoutingBoard::remove_item` is list-only like Java's `removeItem` counterpart) and `RoutingBoard::get_shove_failing_obstacle()`. 17 new gtests (24 in `ShoveTraceAlgo_test.cpp`) — **ctest 690/690 + run_tests PASSED** ✅
- Increment O Java-parity findings now pinned by tests, **do not "fix" them**: (1) `LineSegment(const Polyline&, int)` needs the neighbouring lines, so a 2-line polyline degenerates to the (0,0)-(1,0) line — test helpers must build 3+ lines; (2) `IntBox`/`IntOctagon` `border_line()` return *unit* lines (Java parity), so the cut-out pieces carry degenerate first/last lines while the infinite-line corner maths stays correct; (3) the substitute piece is deliberately **not** on the board during the segment recursion — Java only calls `addItem` after it; (4) shoving a *crossing* foreign trace always fails on the first substitute segment, because the foreign net's own cut pieces are own-net obstacles whose entrance chain starts at stack level 0 and `calculate_stack_levels()` requires the end level to be 1; (5) `MoveDrillItemAlgo::shove_vias` returns **true** when it gives up, so `insert` then fails through the non-empty `shove_via_list`. Only deliberate divergence: Java's `SearchTreeManager::reuseEntriesAfterCutout` entry recycling is not ported (fresh entries for the cut pieces).
- **Increment P (2026-10-03) DONE**: the `FoundConnectionInserter` itself plus the `RoutingBoard` forced-trace prereqs (Phase-4 task 4.4, half done). Board side: `RoutingBoard::check_forced_trace_polyline()`/`insert_forced_trace_polyline()`/`insert_forced_trace_segment()`/`connect_to_trace()`/`clear_shove_failing_obstacle()` + `get_shove_failing_layer()`; `ShoveTraceAlgo::spring_over_obstacles` now takes a nullable `const std::set<Pin*>*` (Java passes `null`) instead of an empty set; `ForcedViaAlgo::check` gained Java's start-trace-circle clearance test; `DrillItem::is_on_layer` and `BasicBoard::mutable_rules` were added. Autoroute side: `FoundConnectionInserter.h/.cpp` (`get_instance`, `insert_trace`, `insert_neckdown`, `try_neck_down`, `insert_fanout_micro_neckdown`, `insert_via`, the fanout diagnostics), `AutorouteControl::fanout_start_pin_name`, and the stale `InsertFoundConnectionAlgo.{h,cpp}` were **deleted** (replaced by `FoundConnectionInserter.cpp` in `src/autoroute/CMakeLists.txt`). **Geometry parity fix found by the new tests**: `Polyline::Polyline(const IntPoint&, const IntPoint&)` built a single line, but Java `new Polyline(fromCorner, toCorner)` builds **three** lines (the segment plus two 45-degree stub lines) and `lines.length < 3` marks a polyline empty, so every single-segment forced insert silently returned its from corner; the constructor is now the full Java port. Same trap: Java `new Polyline(Point[])` is `Polyline::from_corners`, not the C++-only `Polyline(std::vector<IntPoint>)`. 12 + 5 = 17 new gtests (`RoutingBoardForcedTrace_test.cpp`, `FoundConnectionInserter_test.cpp`) — **ctest 707/707 + run_tests PASSED** ✅
- Increment P Java-parity findings, **do not "fix" them**: (1) `insert_forced_trace_polyline` passes `CalcFromSide(combined_polyline, corner_count() - trace_shapes.size() - 1 + i, shape)`, which is `i - 2` (negative for the first two shapes) whenever no trace was picked for combining; Java does exactly the same, and `ShapeEntrySide`/`CalcFromSide` resolves a negative index via its "first corner inside shape" fallback, so the shove check is weaker than `check_forced_trace_polyline` (which uses `i + 1`); (2) `ShapeTraceEntries::store_items` refuses only `is_shove_fixed()` items, **not** `FixedState::USER_FIXED` ones, so a user-fixed trace is not by itself a blocker; (3) `FoundConnectionInserter::insert_trace`'s tail-cleanup loop removes every contact-free trace ending on an intermediate corner, so a synthetic route without a pin at its start corner deletes itself again — tests must anchor the start with a pin.
- Increment P **open items**: multi-segment (3+ corner) routes still fail to insert end to end in the gtest harness — the trace combine + `PolylineTrace::normalize` split path needs the same investigation against the Java baseline (**DONE in Increment Q, 2026-10-04**); `AutorouteEngine::autoroute_connection()` now tries the maze expansion path first (MazeSearchAlgo + FoundConnectionLocator + FoundConnectionInserter), falling back to the legacy grid A*; `AutorouteControl::fanout_start_pin_name` is declared but never assigned (Java sets it in the board's control setup) — **assigned in Increment R, 2026-10-05**; `ForcedViaInserter.check` still lacks Java's hole-only-layer / hole-shape clearance branch.
- **Increment R (2026-10-05) DONE**: the fanout pass. `RoutingBoard::fanout()` is declared in `src/board/RoutingBoard.h` and **defined in the autoroute module** (`src/autoroute/RoutingBoardFanout.cpp`), because the board module may not reference `AutorouteControl`/`AutorouteEngine`; the header forward-declares `autoroute::AutorouteAttemptResult` (existing convention, same as `datastructures::TimeLimit`). Full port of the guard clauses, the distance-squared target sort, the fanout control fields, the ≤4-target closest-target strategy and the `opt_changed_area` pull-tight call. Board prerequisites: `Item::net_count()`/`get_net_no(int)`/`get_connected_set(int)`/`get_unconnected_set(int)`, `Communication::get_resolution(Unit)`, `Pin::relative_location()`/`name()`/`get_center()` with the new lazy `DrillItem::center_is_set_` cache, `RouterSettings::get_trace_pull_tight_accuracy()`. `MazeSearchAlgo::passes_fanout_filter()` ports the anonymous `mazeExpansionList` TreeSet (escape-length rejection on the start layer, min-escape drill rejection), and `MazeFanoutDiagnostics` is now **compiled and instantiated** (recreated in `set_control()` because it holds a control reference) with all six Java `MazeExpansionEngine` trace sites plus the two filter rejections. Divergences (all in-code comments + `03_MISSING.md`): the Java `fallbackToBoardVias` via-rule combination is missing (no fanout settings, no global via-rule list in C++), the engine is a local instance (Java `init_autoroute(..., false)` always creates one), `Pin::get_center()` corrects against the untransformed padstack shape, pins refer to their component by index (the compiled `Components::get()` is index-based), and Java's inverted `getResolution(Unit)` scaling is reproduced verbatim. 18 new gtests (`RoutingBoardFanout_test.cpp`, `Pin_center_test.cpp`, 6 filter/diagnostics cases in `MazeSearchAlgo_test.cpp`) — **ctest 726/726 + run_tests PASSED** ✅
- **Next**: the free-space rooms are still tiny interim boxes around the drill location, which limits `expand_to_other_layers()` in production, and the tie-pin shape reduction (`reduce_trace_shapes_at_tie_pins`) is still a documented divergence. Next candidates: `RoutingBoard::autoroute_connection()` as a real board-level API (today `AutorouteEngine::autoroute_connection` carries the maze-first wiring, so the Java board facade API of the same name is still missing), the `fallbackToBoardVias` fanout via-rule combination (needs `RouterSettings::FanoutSettings` + `BoardRules::via_rules`), and `ForcedViaInserter.check`'s hole-only-layer / hole-shape clearance branch.
- Parking lot note: WP-9's "rooms always created" simplification is now replaced by real reuse; room expansion/splitting around obstacles now uses `complete_shape` (Increment C), door creation from a room shape is ported (Increments E/F/G), the maze search that consumes those doors is ported (Increment I), the ripup costs of that search are ported (Increment J) and the trace shoving of that search is ported (Increment K). The connection location half of Phase 4 landed in Increment M; what is still missing is the insertion (Phase 4.4) and its engine rewiring.

## Definition of Done (per WP)

1. Port complete & faithful to Java
2. `cmake --build` green with `-Werror`
3. gtest added + registered in tests/CMakeLists.txt
4. `run_tests` + ctest all green
5. docs/migration/INDEX.md updated
