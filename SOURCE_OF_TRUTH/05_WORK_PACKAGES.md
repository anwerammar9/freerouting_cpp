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
- `complete_expansion_room` is a minimal port of Java `completeExpansionRoom()`: full `ShapeSearchTree.complete_shape()` / blocker #6 NOT ported. Sub-2D room shapes (the unit drill-location box from `IntPoint::surrounding_box()`) are inflated via `IntBox::offset(1)` instead of tree-expanded. Result == Java's empty-board behaviour; obstacle-room splitting still needs complete_shape.
- C++ `ShapeSearchTree` stores only `Item*`, so rooms are never found by overlap queries → `ExpansionDrill` always creates rooms (Java can find existing rooms).
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

### WP-10 remaining (next increments)
- **Increment B (prereq #2, blocker #6)**: `ShapeSearchTree::completeShape()` — full free-space expansion of an incomplete room using the tree (`ThreadLocal<ArrayStack>` port, 45/90-degree room variants). Largest remaining chunk.
- Prereq #3/#4/#5 + SortedRoomNeighbours family (`ObstacleExpansionRoom::create_overlap_door`, `calculateTargetDoors`, `remove_all_doors`), then `MazeSearchAlgo` init/expand rewrite.
- Parking lot note: WP-9's "rooms always created" simplification is now replaced by real reuse; still no room SPLITTING around obstacles (needs completeShape).

## Definition of Done (per WP)

1. Port complete & faithful to Java
2. `cmake --build` green with `-Werror`
3. gtest added + registered in tests/CMakeLists.txt
4. `run_tests` + ctest all green
5. docs/migration/INDEX.md updated
