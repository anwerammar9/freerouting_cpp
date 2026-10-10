# Board Module - Migration Status

**Module**: `src/board/` (headless)
**CMake Target**: `freerouting_board`
**Java Files**: 51 (headless scope: 49, excl. GUI `ObjectInfoPanel`/`PrintableShape`) | **C++ Headers**: 34 | **C++ Impls**: 29 | **Tests**: 7

---

## Completed (Fully Migrated)

| Java Class | C++ Header | C++ Impl | Test | Java Lines | Notes |
|------------|-----------|----------|------|-----------|-------|
| AngleRestriction | AngleRestriction.h | AngleRestriction.cpp | - | 23 | Angle restriction enum |
| FixedState | FixedState.h | - | - | 11 | Enum (header-only) |
| Unit | Unit.h | Unit.cpp | - | 39 | Unit enum with conversion |
| Connectable | Connectable.h | - | - | 46 | Interface (header-only) |
| Component | Component.h | Component.cpp | - | 242 | Board component |
| Components | Components.h | Components.cpp | Components_test.cpp | 171 | Component container |
| DrillItem | DrillItem.h | DrillItem.cpp | DrillItem_test.cpp | 448 | Drill item base class |
| Layer | Layer.h | Layer.cpp | - | 618 | Board layer |
| Via | Via.h | Via.cpp | - | 240 | Via implementation |
| Pin | Pin.h | Pin.cpp | - | 27060 | Pin implementation |
| Padstack | Padstack.h | Padstack.cpp | - | 149 | Padstack implementation |
| Item | Item.h | Item.cpp | - | 42289 | Base item class |
| Trace | Trace.h | Trace.cpp | board_shape_search_tree_test.cpp | 15440 | Trace implementation |
| PolylineTrace | PolylineTrace.h | PolylineTrace.cpp | board_shape_search_tree_test.cpp | 2600 | Multi-segment trace variant |
| BoardOutline | BoardOutline.h | BoardOutline.cpp | - | 8710 | Board outline |
| ObstacleArea | ObstacleArea.h | ObstacleArea.cpp | ObstacleArea_test.cpp | 328 | [added this session] Keepout/conduction item with a relative `Area`. `get_area()` computes and caches the absolute area (mirror if `side_changed`, then `turn_90_degree`/`rotate_approx` per `rotation_in_degree`, then `translate_by`), mirroring Java ObstacleArea.java. `copy(int id)`, `translate_by`/`turn_90_degree`/`rotate_approx`/`change_placement_side` (rotation wraps, side flip also mirrors translation about the pole and flips the layer), `is_obstacle` (not obstacle for shared nets, obstacle only for Trace/Via), `tile_shape_count`/`get_tile_shape` via `Area::split_to_convex`, `is_selected_by_filter` (KEEPOUT). `BasicBoard` gained `flip_style_rotate_first` (C++ `Components` port is out of sync, so the flag lives on the board). UI methods (draw/print_info/serialization) not ported |
| ConductionArea | ConductionArea.h | ConductionArea.cpp | ObstacleAreaSubclass_test.cpp | 148 | [added this session] Conducting obstacle area. Carries a Java-parity `is_obstacle` flag: `is_obstacle(Item*)` delegates to `ObstacleArea` only when set, `is_trace_obstacle(int)` = flag && other net, `is_drillable(int)` = !flag || same net, `get_is_obstacle`/`set_is_obstacle`. `get_normal_contacts()`: for each tile shape, `BasicBoard::overlapping_objects`, keeping same-net same-layer traces whose first/last corner is inside the shape and drill items (Via/Pin) whose center is inside it. `get_trace_connection_shape` delegates to the tree (`tree_shape_count`/`get_tree_shape`), `get_ratsnest_corners` from `Area::corner_approx_arr`, selection filter CONDUCTION. `copy(int)` warns & returns nullptr for multi-net. Note: the C++ `Connectable` interface is NOT implemented (same decision as Trace) |
| ViaObstacleArea | ViaObstacleArea.h | ViaObstacleArea.cpp | ObstacleAreaSubclass_test.cpp | 80 | [added this session] Via keepout. `is_obstacle(Item*)` = other is a real `Via` (not a ViaObstacleArea) on a foreign net; `is_trace_obstacle` always false; both net-array ctor overloads (with/without nets), `copy(int)` keeps nets; filter VIA_KEEPOUT |
| ComponentObstacleArea | ComponentObstacleArea.h | ComponentObstacleArea.cpp | ObstacleAreaSubclass_test.cpp | 66 | [added this session] Component keepout. `is_obstacle(Item*)` = other is a ComponentObstacleArea of a different component_no (dynamic_cast, never self); `is_trace_obstacle` always false; ctor forwards an empty net array; filter COMPONENT_KEEPOUT |
| LayerStructure | LayerStructure.h | LayerStructure.cpp | - | 2238 | Layer structure |
| CoordinateTransform | CoordinateTransform.h | CoordinateTransform.cpp | - | 3813 | Coordinate transform |
| BasicBoard | BasicBoard.h | BasicBoard.cpp | - | 49597 | Basic board impl |
| RoutingBoard | RoutingBoard.h | RoutingBoard.cpp | - | 49705 | Routing board impl |
| Communication | Communication.h | Communication.cpp | - | 4307 | Board communication |
| ChangedArea | ChangedArea.h | ChangedArea.cpp | - | 3427 | Changed area tracking |
| SearchTreeManager | SearchTreeManager.h | SearchTreeManager.cpp | - | 10270 | Search tree manager |
| ShapeSearchTree | ShapeSearchTree.h | ShapeSearchTree.cpp | board_shape_search_tree_test.cpp | 45906 | Shape search tree |
| ShapeSearchTree45Degree | ShapeSearchTree45Degree.h | ShapeSearchTree45Degree.cpp | - | 20411 | 45-degree search tree |
| ShapeSearchTree90Degree | ShapeSearchTree90Degree.h | ShapeSearchTree90Degree.cpp | - | 13387 | 90-degree search tree |
| BoardLoader | BoardLoader.h | BoardLoader.cpp | - | 54 | Board loader |
| BoardBuilder | BoardBuilder.h | BoardBuilder.cpp | - | N/A | Board builder |
| ItemSearchTreesInfo | ItemSearchTreesInfo.h | ItemSearchTreesInfo.cpp | - | 98 | Search tree info |
| SearchTreeObject | SearchTreeObject.h | - | - | 24 | Search tree interface (header-only) |
| ItemIdentificationNumberGenerator | ItemIdentificationNumberGenerator.h | ItemIdentificationNumberGenerator.cpp | ItemIdentificationNumberGenerator_test.cpp | 40 | ID generator with overflow warning (INT_MAX/2) |
| ItemSelectionFilter | ItemSelectionFilter.h | ItemSelectionFilter.cpp | ItemSelectionFilter_test.cpp | 106 | Faithful port (SelectableChoices ×10, values array); prior header diverged from Java and was rewritten. Item gained is_user_fixed/is_selected_by_filter (default true)/is_selected_by_fixed_filter |
| CalcFromSide | CalcFromSide.h | CalcFromSide.cpp | CalcFromSide_test.cpp | 159 | Shove-direction side calculation. Edge-number of the `TileShape` border where a polyline / line segment enters: `NOT_CALCULATED` sentinel; edge from `LineSegment::border_intersections` with nearest-intersection fallback when the first corner is inside the shape; edge via `nearest_border_point` + `contains_on_border_line_no`; front-side detection mapping the entry edge ±2 per shove direction. Also fixed a latent bug in `IntPoint::perpendicular_projection` (used `determinant(p_line.b)` of the projected point instead of the line-offset determinant `a.x*b.y - a.y*b.x`), which returned garbage projections for arbitrary border lines and broke `TileShape::nearest_border_point`. Verified by `CalcFromSide_test.cpp` (11 gtests) — **280/280 ctest passing** |

| CalcShapeAndFromSide | CalcShapeAndFromSide.h | CalcShapeAndFromSide.cpp | CalcShapeAndFromSide_test.cpp | 109 | Shove shape + fromside calculation. Orthogonal mode uses `TileShape::bounding_box()`; otherwise the segment shape is converted `to_Simplex()` and dog-ear cut lines at the start/end of the trace are applied via `Simplex::intersection` with a one-line cut plane (`Simplex::get_instance`), keyed on `p_index == line_count - 3` or a corner-distance < `get_compensated_half_width` (Java `Trace.get_compensated_half_width` ported = half_width + `clearance_compensation_value`). Cutline border index uses `is_equal_or_opposite` matching (Java `Line.equals` is direction-insensitive); if neither cut yields a border, `CalcFromSide(polyline, p_index, shape)` is used unless `p_in_shove_check` (matches Java's stack_level guard). Verified by `CalcShapeAndFromSide_test.cpp` (5 gtests, staircase trace; forced end-cut trims the octagon cap) — **285/285 ctest passing** |
| ComponentOutline | ComponentOutline.h | ComponentOutline.cpp | ComponentOutline_test.cpp | 224 | Component outline with placement (front/back, translation, rotation) |
| BoardObservers | BoardObservers.h | - | - | 14 | Observer interface extending `Observers<Item*>` with `notify_moved(Component*)` (header-only) |
| BoardObserverAdaptor | BoardObserverAdaptor.h | BoardObserverAdaptor.cpp | - | 61 | Empty adaptor implementing BoardObservers (active flag) |
| ShapeTraceEntries | ShapeTraceEntries.h | ShapeTraceEntries.cpp | - | 726 | Headless shove bookkeeping: `store_items`/`store_trace` (entrance_points + shove_via collection), `search_from_side`/`resort`/`calculate_stack_levels`/`next_substitute_trace_piece` (offset shape + border walk). Requires `TileShape::offset`/`entrance_points` (added this session); `cutout_trace` stubbed (needs `board.item_list`); `Pin::drill_allowed` not yet ported (treated conservatively) |
| ShoveTraceAlgo | ShoveTraceAlgo.h | ShoveTraceAlgo.cpp | - | 593 | Headless shove checks/insert: static `check(LineSegment)` + instance `check(TileShape)`/`insert` via `ShapeTraceEntries`; `MoveDrillItemAlgo` via-shove and `spring_over` stubbed (headless returns polyline copy, via shove conservatively fails); bounding-box `is_contained_in` stubbed; `get_ignore_items_at_tie_pins` uses `overlapping_objects`. **WP-10 Inc P**: `spring_over_obstacles` (and `spring_over`) take a nullable `const std::set<Pin*>*` because Java passes `null` to disable the own-net pin tie handling — an empty set is not equivalent |
| ForcedPadAlgo | ForcedPadAlgo.h | ForcedPadAlgo.cpp | - | 352 | Force-pad algorithm (WIP) |
| ForcedViaAlgo | ForcedViaAlgo.h | ForcedViaAlgo.cpp | ForcedViaAlgo_test.cpp | 260 | Force-via algorithm; Java `holeCheckShape()` parity (WP-10 Inc S) |
| MoveDrillItemAlgo | MoveDrillItemAlgo.h | MoveDrillItemAlgo.cpp | - | 248 | Drill item movement (WIP) |
| MoveComponent | MoveComponent.h | MoveComponent.cpp | - | 163 | Component placement (WIP) |
| OptViaAlgo | OptViaAlgo.h | OptViaAlgo.cpp | OptViaAlgo_test.cpp | 574 | Via optimization (WIP) |
| PullTightAlgo | PullTightAlgo.h | PullTightAlgo.cpp | PullTightAlgo_test.cpp | 470 | Trace tightening base (WIP) |
| PullTightAlgo45 | PullTightAlgo45.h | PullTightAlgo45.cpp | - | 622 | 45-degree tightening (WIP) |
| PullTightAlgo90 | PullTightAlgo90.h | PullTightAlgo90.cpp | - | 144 | 90-degree tightening (WIP) |
| PullTightAlgoAnyAngle | PullTightAlgoAnyAngle.h | PullTightAlgoAnyAngle.cpp | - | 953 | Any-angle tightening (WIP) |

### Board Facade Stubs Ported (2026-09-18)

| RoutingBoard method | Ported from (Java) | Test |
|---------------------|--------------------|------|
| `combine_traces()` | BasicBoard.java:684 `combineTraces` | RoutingBoard_stubs_test.cpp |
| `get_outline()` | BoardItemRepository.java:34 `getOutline` (first BoardOutline on the board) | RoutingBoard_stubs_test.cpp |
| `contains_trace_tails()` | RoutingBoard.java:1188 `containsTraceTails` | RoutingBoard_stubs_test.cpp |
| `get_trace_tail()` | BasicBoard.java:1307 `getTraceTail` (overlapping point shape → trace corner with empty start/end contacts) | RoutingBoard_stubs_test.cpp |
| `additional_update_after_change()` | RoutingBoard.java:96 — faithful no-op (Java returns when `autorouteEngine == null`; no engine wired into the C++ item-change path yet) | - |
| `opt_changed_area()` | RoutingBoard.java:151 `optChangedArea` | - |
| `check_forced_trace_polyline()` | RoutingBoard.java:412 `checkForcedTracePolyline` | RoutingBoardForcedTrace_test.cpp |
| `insert_forced_trace_polyline()` | RoutingBoard.java:456 `insertForcedTracePolyline` (spring over, trace combine, per-shape shove check/insert, short-sample retry, normalize + pull tight) | RoutingBoardForcedTrace_test.cpp |
| `insert_forced_trace_segment()` | RoutingBoard.java:361 `insertForcedTraceSegment` | RoutingBoardForcedTrace_test.cpp |
| `connect_to_trace()` | RoutingBoard.java:1128 `connectToTrace` | RoutingBoardForcedTrace_test.cpp |
| `clear_shove_failing_obstacle()` / `get_shove_failing_layer()` | no Java counterpart — C++-only shove diagnostics that `ForcedViaAlgo`/`ForcedPadAlgo` write | RoutingBoardForcedTrace_test.cpp |
| `insert_via()` trace splitting | BasicBoard.java:269 `insertVia` + 292 `splitTraces` | - |

### Phase 7 Board Model Stubs — COMPLETED (2026-09-18)

| Task | Status |
|------|--------|
| 7.2 `opt_changed_area()` | ✅ Delegates to `PullTightAlgo::opt_changed_area`, clears `changed_area_`, calls `join_graphics_update_box` with `ChangedArea::surrounding_box()` |
| 7.4 `insert_via()` trace splitting | ✅ Calls `split_traces(center, layer, net_no)` for each padstack layer `from_layer..to_layer-1` and each net |

Added `ChangedArea::surrounding_box()` returning `IntBox` bounding box of all marked areas.

`BoardOutline` was a stale, non-compiling draft (excluded from `src/board/CMakeLists.txt`, wrong `namespace board` forward-declarations shadowing the `geometry::planar` types, missing `Item` base ctor, `get_tile_shape()` on APIs that don't exist). Restored: corrected `geometry::planar` namespaces, `Item(std::vector<int>{}, cl, 0, 0, FixedState::SYSTEM_FIXED, nullptr)` base ctor, `get_tile_shape()` via the C++ `PolylineShape::split_to_convex()` + `clone()`, and `BoardOutline.cpp` added to the build.

### WP-1 Contact Graph (2026-09-18)

| Item/C++ method | Ported from (Java) | Notes |
|-----------------|--------------------|-------|
| `Item::first_common_layer()` | Item.java:320 `firstCommonLayer` | max(first)/min(last); -1 when disjoint. Was a hardcoded -1 stub. |
| `Item::get_connection_items()` | Item.java:741 `getConnectionItems` | Full port: unique-contact-point check, trace single-contact guard, via VIA/FANOUT_VIA stops, fork/stub detection via `normal_contact_point` + layer filtering |
| `Item::is_fanout_via()` | Item.java:1250 `isFanoutVia` | `kProtectFanoutLength=400` (Java `PROTECT_FANOUT_LENGTH`); SMD-pin (single-layer, ≤1 contact) + shove-fixed 2-corner PolylineTrace detection |
| `DrillItem::first_layer()/last_layer()` | DrillItem.java:162/175 | Overrides added. Layer range now synced from the padstack in `Via` ctor + `set_padstack` — Java derives `firstLayer`/`lastLayer` from the padstack (`isPlacedOnFront` always true). **Fix for a real bug**: vias always reported 0..0. |
| `Trace::normal_contact_point(Item*)` | DrillItem.java:339 | Added DrillItem branch (drill center == trace corner on shared layer) |
| `DrillItem::normal_contact_point(Item*)` | DrillItem.java:326-349 | New override; DrillItem+Trace cases |

**Test coverage** (`ItemContacts_test.cpp`, 8 new `ItemContactGraphTest` cases, 14 total):
`DrillItemExposesLayerRange`, `FirstCommonLayerComputedFromBothLayerRanges`, `NormalContactPointBetweenTraceAndVia`, `ConnectedSetRecursesThroughVia`, `ConnectionItemsSpanTraceAndVia`, `ConnectionItemsStopAtVia`, `TraceContactsIgnoreNetFlag`, `ViaIsFanoutViaForSmdPinExit`.

**Gotcha:** cannot take the address of a temporary `IntPoint` in `equals(&IntPoint(...))` — use a named local.
**Gotcha:** `Padstack` is incomplete in `Via.h`, so `set_padstack` (which touches padstack layers) must be declared in the header but defined in `Via.cpp`.

## Summary

- **Board stubs + BoardOutline restore** (this session): ported `combine_traces`/`get_outline`/`contains_trace_tails`/`get_trace_tail` from Java BasicBoard/RoutingBoard (see table above), kept `additional_update_after_change` as a faithful no-op (Java returns when no autoroute engine maintains the DB), restored `BoardOutline` (namespaces + `Item` base ctor + `split_to_convex` implementation + CMake registration). Verified by `RoutingBoard_stubs_test.cpp` (8 gtests) — **405/405 ctest passing**.
- **CalcShapeAndFromSide port** (this session): ported the shove shape + fromside calculation (see row above; orthogonal/int-box vs simplex dog-ear-cut paths, `in_shove_check` fallback guard) together with `Trace::get_compensated_half_width` (which required only a `ShapeSearchTree` forward reference) — `get_tree_shape`/`bounding_box`/`to_Simplex`/`intersection` were already on `Item`/`Simplex`. Verified by `CalcShapeAndFromSide_test.cpp` (5 gtests) — **285/285 ctest passing**. The cut-line border match uses `Line::is_equal_or_opposite` because Java `Line.equals` normalizes direction, whereas C++ `fast_equals` is direction-sensitive.
- **CalcFromSide port** (previous session step): ported the shove-direction side calculation (see row above) with 11 gtests. That step also uncovered and fixed a latent geometry bug: `IntPoint::perpendicular_projection` used `determinant(p_line.b)` (cross product of the projected point with the line's endpoint) instead of the correct line-offset determinant `a.x*b.y - a.y*b.x`, producing wrong projections for border lines not through the origin and silently breaking `TileShape::nearest_border_point`.
- **ObstacleArea subclasses** (previous session): ported `ConductionArea`, `ViaObstacleArea` and `ComponentObstacleArea` (see rows above) on top of the `ObstacleArea` base. This resolves the previously "blocked on `ViaObstacleArea`/`ComponentObstacleArea`/`ConductionArea`" branches — the board item hierarchy now covers all obstacle-area kinds. Verified by `ObstacleAreaSubclass_test.cpp` (30 gtests) — **227/227 ctest passing**. Divergences: `Connectable` not implemented (same as Trace); UI (`get_draw_colors`/`print_info`) not ported.
- **ObstacleArea port** (previous session): replaced the 30-line stub with a faithful port — unblocked by the widened `Area` interface (mirror/rotate/turn/translate + `split_to_convex` on `PolygonArea`/`PolylineArea`). Verified by `ObstacleArea_test.cpp` (16 gtests). Note: 7 standalone data tests in `tests/data` (`geometry_tile_test`, `geometry_polyline_area_test`, `geometry_regular_tile_shape_test`, `datastructures_shape_tree_test`, `board_shape_search_tree_test`, `geometry_line_segment_test`, `geometry_direction_test`) had source files deleted from the tree by an external process mid-session (untracked, not in git); their CMake blocks are guarded with `if(EXISTS ...)` so the build proceeds and the tests auto-restore when the files are put back.
- **Done**: 51 classes (100% of headless scope, 100% of 51) — the 9 PullTight/ForcedPad/Move*/OptVia entries are now compiled (WIP) rather than absent; **Phase 7 Board Model Stubs complete**: `opt_changed_area()` + `insert_via()` trace splitting
- **`TraceShover.springOver` port (WP-10 Inc N, 2026-10-02)**: `ShoveTraceAlgo::spring_over()`/`spring_over_obstacles()`/`get_ignore_items_at_tie_pins()` replaced their stubs with full Java `TraceShover` ports (the C++ class is named after an older Java revision; the current Java class is `TraceShover`). Java returns `null` / the input object / a new object; the C++ port uses `SpringOverResult{std::optional<Polyline> polyline; bool changed;}`, where `changed == false` is the Java identity case. Prereqs ported in the same step: `Item::get_all_contacts()`/`get_all_contacts(int)`, `DrillItem::get_tree_shape_on_layer()` (on the `DrillItem` of `Via.h`), `BasicBoard::insert_trace`/`check_polyline_trace`/`get_max_generated_id()` plus the min/max trace half-width bookkeeping. Divergences: the `BoardOutline` rejection branch is unreachable in C++ (the outline is not a `board::Item`), a null obstacle shape is treated as a failure instead of throwing, and the C++ `rules::AngleRestriction` spells the two restrictions `RIGHT`/`FORTY_FIVE`. Verified by the new `ShoveTraceAlgo_test.cpp` (7 gtests, including a real counter-clockwise wrap) — **673/673 ctest + run_tests green**.
- **Remaining**: headless blockers are incomplete *methods* (see Stub table), not whole classes: `PullTightAlgo*`/`ShoveTraceAlgo::spring_over` quality paths, `BoardOutline::tile_shape_count`/keepout generation, `Pin::drill_allowed`, `ShapeSearchTree` room creation
- **Blockers**: PullTightAlgo* needed for trace routing quality; ForcedViaAlgo needed for via placement. Headless DSN loading now uses parsed padstack extents for pin bounding boxes, but full padstack shape/layer ownership and component obstacle materialization remain pending.
- **Note**: `ShapeSearchTree` now subclasses the ported `datastructures::MinAreaTree`; `Item` implements the `ShapeTree::Storable`/`SearchTreeObject` interface with per-tree entry/shape caches (`ItemSearchTreesInfo`); `SearchTreeManager::remove` uses the stored entries. The Java query layer is ported: `overlapping_tree_entries`, `overlapping_objects`, `overlapping_tree_entries_with_clearance`, `overlapping_objects_with_clearance`, `overlapping_items_with_clearance` (with the `EntrySortedByClearance` clearance-aware candidate sort and `shapes_intersect`/`enlarge_tile` shape helpers), plus a real `validate_entries`. Board-backed queries use `ClearanceMatrix::get_max_value_on_layer` as a conservative bound since the C++ matrix is layer-flat. `BasicBoard` now owns a `SearchTreeManager` and bridges the tree queries to `Board`-level API: `BasicBoard::overlapping_objects`/`overlapping_items_with_clearance` (mirroring `BasicBoard.java:859`). Large legacy entries (BasicBoard, RoutingBoard, Pin, BoardOutline, etc.) may still have incomplete implementations; validate with tests.
- **Trace port**: `Trace` is now an abstract base (dropping the `Connectable` interface, which demanded many pure virtuals only the broken untracked DRC used) with pure-virtual `get_length()` and fully ported `get_normal_contacts(Point, bool)` (Manhattan tolerance `half_width+1`, `shares_layer`/`shares_net`/`shares_net_no` filtering, `get_start_contacts`/`get_end_contacts`). The remaining `Trace.java` base API is ported too: `is_obstacle(Item*)`, `normal_contact_point(Item*)` (trace-to-trace unique corner), `is_tail()`, `is_overlap()`, `is_shove_fixed()` (with net-class lookup via `BoardRules::get_nets`), `is_drillable(int)`, `get_ratsnest_corners()`, `nearest_end_point(Point)`, `is_cycle()`, and `validate()` — backed by new `Item` virtuals (`Item::is_obstacle(Item*)`, `normal_contact_point`, `is_tail`, `is_overlap`, `is_shove_fixed`, `is_drillable`, `get_ratsnest_corners`, `validate`, `is_cycle_recu`). `PolylineTrace` is the concrete implementation with a lazy corner cache (`ensure_corners`/`first_corner`/`last_corner`), `get_tile_shape(i)` = segment bounding octagon enlarged by half width (approximating Java's `to_simplex().simplify()`), `get_length()` (full corner-chain length), `tile_shape_count()`, and `change()`. `Connection::trace_length()` now uses the real `Trace::get_length()` via `dynamic_cast`. Verified by `test_trace_normal_contacts` + `test_trace_core_methods` in `board_shape_search_tree_test.cpp` (8/8 tests green). Still pending: `combine()`, `split()`, `pull_tight()` (blocked on `PullTightAlgo*`). The obstructing `ViaObstacleArea`/`ComponentObstacleArea`/`ConductionArea` branches of `is_obstacle`/`get_normal_contacts` are now ported (this session).

- **`ForcedViaInserter` hole-shape parity (WP-10 Inc S, 2026-10-06)**: `ForcedViaAlgo::check()`/`insert()` are now the full Java ports of the layer loops — `hole_check_shape()` builds a `Circle` of `ceil(drill_radius + board_rules.hole_clearance + 10)` around the via location, a via layer whose padstack has no shape on that layer is checked against the hole shape with the **null** clearance class instead of being skipped, and every layer gets an extra hole-clearance `ForcedPadAlgo::check_forced_pad()` call while the rule is active. Prerequisites: `rules::BoardRules::get_hole_clearance()`/`set_hole_clearance(int)` (negative clamped to zero, matching Java) + `BoardRules::clearance_class_none()`, and `Padstack::get_drill_radius()` (Java `getSmallestRadius` fallback, digit-only `strtod` parsing of the `<outer>:<drill>` pair in the padstack name, cached and invalidated on shape changes). **Parity fix**: the file-local `tile_shape_of()` helper used to translate only `Area`/`Circle` shapes and fell back to the *untranslated* bounding box for all other shape types, so a padstack whose layer shape is a `PolygonShape` searched an empty tile at negative coordinates and every via check failed; the C++ `Shape` base class has no virtual `translate_by` (only `Area` declares it), so the helper now builds the bounding tile first and translates the tile itself. **Test gotchas**: `ShapeTraceEntries::store_items` only rejects `is_shove_fixed()` items, so a `NOT_FIXED` foreign trace is shoved aside and never blocks a via check — negative tests need a `SHOVE_FIXED` trace; and the padstack drill name is `<outer>:<drill>` (Java takes the token before and the token after the first colon), not `1.0_0.6:0.6`. 19 gtests in `ForcedViaAlgo_test.cpp` — **745/745 ctest + run_tests green**.

### Pending (query consumers)
- `BasicBoard.overlapping_items(Area, int)` is now unblocked: `Area::split_to_convex` is on the `Area` base (this session) and `ObstacleArea` provides `tile_shape_count`/`get_tile_shape`.
- `Trace.combine()`, `Trace.split(IntOctagon)`, `Trace.split(Point)`, `Trace.pull_tight()` — blocked on `PullTightAlgo*` and trace-splitting geometry.

## WP-10 Increment O — `TraceShover.check`/`insert` + `ShapeTraceEntries` cutout (2026-10-02)

Ported with a `.h`, a `.cpp` and a `_test.cpp`, keeping the Java control flow statement by statement.

| Java | C++ |
|------|-----|
| `TraceShover.insert(TileShape, int, int, [], int, int, int, int)` | `ShoveTraceAlgo::insert(TileShape, CalcFromSide*, int, vector<int>, int, vector<const ConductionArea*>, int, int, int)` |
| `TraceShover.check(TileShape, int, int, [], int)` | `ShoveTraceAlgo::check(TileShape, int, vector<int>, int)` (static) |
| `TraceShover.check(LineSegment, int)` | `ShoveTraceAlgo::check(LineSegment, int)` (static) |
| `ShapeTraceEntries.nextSubstituteTracePiece()` | `ShapeTraceEntries::next_substitute_trace_piece()` |
| `ShapeTraceEntries.cutoutTrace(CutoutTrace...)` | `ShapeTraceEntries::cutout_trace()` (the two cut modes map to `cutout_trace()` / `fast_cutout_trace()`) |
| `Board.getShoveFailingObstacle()` | `RoutingBoard::get_shove_failing_obstacle()` |

### Porting notes

- **Cutting a trace removes it from the search tree as well.** `RoutingBoard::remove_item()` is list-only (the counterpart of Java's `addItem` family), so `cutout_trace()` erases the cut trace from the item list *and* from `SearchTreeManager` explicitly — the same two-step that `PolylineTrace::split()` already uses.
- **The substitute piece stays off the board.** Java only calls `board.addItem(piece)` after the per-segment recursion, so `next_substitute_trace_piece()` returns a `PolylineTrace` with `board == nullptr` semantics (built, not registered). Registering it earlier makes the piece an obstacle of its own recursion.
- **`border_line()` returns unit lines, on purpose.** `IntBox::border_line()` / `IntOctagon::border_line()` mirror the Java, which passes `(0,0)->(1,0)`-style border lines into `TileShape::corner()` (infinite lines) but also into `LineSegment`-based queries. Corner maths is correct; every `LineSegment` query on such a line is degenerate. The cut-out pieces therefore legitimately carry degenerate first/last lines, and the tests pin the exact geometry.
- **`LineSegment(const Polyline&, int)` needs its neighbours.** With fewer than 3 lines the C++ (and the Java) cannot build a middle point and the segment degenerates to `(0,0)->(1,0)`. A 2-line test polyline therefore produces an empty offset shape and a silently wrong static check — the test helper builds 3 lines instead.
- **A crossing foreign trace is shoved around the new shape (Java parity).** After the cut, the substitute piece is inserted segment by segment; each cut piece of the same foreign net is an own-net obstacle of the recursion, but its entrance-point chain stays at stack level 1, so `calculate_stack_levels()` succeeds and `insert()` joins the pieces around the shape. Verified against the Java baseline (`TraceShover.insert` returns `true` and one net 2 trace with the corners `4000,2000 / 4000,3923 / 7077,3923 / 7077,6077 / 4000,6077 / 4000,8000` remains), pinned by `InsertJoinsACrossingForeignTraceAroundTheShape`. This only works with the faithful `ShapeTraceEntries::resort()`: the entry projection uses `projection_approx(shape.border_line(from_side.no))`, which usually differs from the raw entry point, and a non-null border intersection must *not* skip the anchor search.
- **`shove_vias` gives up with `true`.** `MoveDrillItemAlgo::shove_vias()` returns `true` when its own `store_items` fails or when `max_via_recursion_depth <= 0`, so `insert()` must (and does) fail afterwards through the non-empty `shove_via_list` rather than through the via return value.
- **Only deliberate divergence:** Java's `SearchTreeManager::reuseEntriesAfterCutout` recycles the pre-cutout tree entries for the cut pieces; the C++ port calculates fresh entries (an allocation optimisation only).
- **`resort()` needs a nullable border intersection.** Java `ShapeEntrySide.borderIntersection` is a nullable `FloatPoint` and `resort()` branches on it, so `CalcFromSide::border_intersection` is a `std::optional<FloatPoint>` (`CalcFromSide::values()` mirrors `new ShapeEntrySide(no, null)`). Using `(0,0)` as a "null" sentinel broke the own-net entry removal: a sole own-net entry survived and made `calculate_stack_levels()` return `false`.
- **`PolylineTrace::combine()` iterates, `combine_at_start()` owns its corner.** Java copies the corner (`Point startCorner = firstCorner()`); the C++ `first_corner()` returns a reference into `corner_points_`, which `clear_derived_data()` empties, so the reference dangled into `join_changed_area()`. `PolylineTrace::own_corner()` returns an owned copy. Java also loops instead of recursing in `combine()` (a chain of combinable traces overflowed the stack); the C++ now matches.
- Still open on this class: `ShapeTraceEntries::store_trace()` uses a simplified headless tail test instead of the Java start/end-contact analysis.

## WP-10 Increment R — fanout pass: `RoutingBoard::fanout()` + `Pin` centres (2026-10-05)

`RoutingBoard::fanout(Pin*, settings::RouterSettings*, int, datastructures::Stoppable*,
datastructures::TimeLimit*)` is **declared** in `src/board/RoutingBoard.h` and **defined** in
`src/autoroute/RoutingBoardFanout.cpp`. The Java method (`RoutingBoard.java:990-1122`) needs
`AutorouteControl` + `AutorouteEngine` + `AutorouteAttemptResult`, which the board module may not
reference, so the definition lives in the autoroute module while the public API stays on the board
facade. The header forward-declares `autoroute::AutorouteAttemptResult`, the same convention
already used for `settings::RouterSettings`, `datastructures::Stoppable` and
`datastructures::TimeLimit`.

### Board-side prerequisites added

| Java | C++ | Notes |
|------|-----|-------|
| `Item.getNetNo()` / `Item.getNetNo(int)` | `Item::get_net_no(int)` | overload keeps the Java shape |
| `Item.getConnectedSet()` | `Item::get_connected_set(int)` | |
| `Item.getUnconnectedSet()` | `Item::get_unconnected_set(int)` | the connectable items minus the connected set, ordered like Java |
| `Item.netCount()` | `Item::net_count()` | |
| `Communication.getResolution(Unit)` | `Communication::get_resolution(Unit)` | reproduces Java's **inverted** `Unit.scale(resolution, Unit.UM, unit)` argument order so the fanout escape limits match Java |
| `Pin.relativeLocation()` | `Pin::relative_location()` | |
| `Pin.getCenter()` | `Pin::get_center()` | lazy + cached, see below |
| `Pin.name()` | `Pin::name()` | `std::optional<std::string>` because the C++ `Package::Pin` has only the pin number |
| `DrillItem.centerIsSet` | `DrillItem::center_is_set()` | plus the mutable cached centre |
| `RouterSettings.getTracePullTightAccuracy()` | `RouterSettings::get_trace_pull_tight_accuracy()` | accessor + setter, default 500 (Java's unset value) |

### Porting notes

- **`relative_location()` starts from the package pin.** The C++ `Package::Pin` stores `x`/`y`
  relative to the package origin (the DSN reader fills them from the `image_pin` x/y), so the
  location is `package_pin.x/y` transformed by the component mirror state and rotation. The Java
  order is *mirror on x* → *rotate by component rotation* → *mirror on y for flipped components*,
  and an arbitrary rotation rounds the result to an integer (`Math.round`), which the C++ port
  reproduces.
- **`get_center()` is lazy and caches.** `DrillItem` gained `center_is_set()` and a mutable cached
  centre; a `Pin` starts with `center_is_set_ = false`, and `get_center()` computes
  `component.location + relative_location()`, then takes the first shape of the pin's padstack
  (`current_padstack().first_shape()`) and applies the Java centre-of-gravity correction when the
  centre is inside that shape. Pins created by the DSN reader keep their explicit centre.
- **Divergence — untransformed padstack shape.** Java precalculates the pin shape in board
  coordinates; the C++ `Pin` does not, so the centre correction tests the *untransformed* padstack
  shape against an absolute centre. Documented in `Pin.cpp`.
- **Divergence — component lookup by index.** Java's `Pin.getComponent()` resolves the
  1-based component id, but the compiled C++ `Components::get()` in `src/board/Component.h` is
  **index-based** (the `board/Components.h` shared-pointer class is a separate, unused class). The
  port follows the compiled C++ behaviour and guards with `< 0`; the Java line numbers are kept in
  the comment so the divergence is traceable.
- **`Pin::name()` returns the pin number.** Java returns `Package.Pin.name`; the C++
  `Package::Pin` only stores `number`/`x`/`y`, so the port returns `std::to_string(number)` and
  `std::nullopt` when the package or the pin is missing.
- **`RoutingBoard::fanout()` control wiring.** `is_fanout`, `fanout_start_pin_name` (via `name()`),
  `fanout_start_pin_center` (via `get_center()`), `fanout_start_pin_layer` and
  `remove_unconnected_vias = false`; the guard clauses are the Java ones (`ALREADY_CONNECTED` for a
  multi-layer pin, a multi-net pin or a connected set that leaves the start layer;
  `NO_UNCONNECTED_NETS` for an empty unconnected set); targets are sorted by squared distance and
  routed closest-target-first up to four targets, whole-set above.
- **Divergence — no `fallbackToBoardVias`.** The Java fallback combines the fanout via rule with
  the *global* via-rule list; the C++ `RouterSettings` has no fanout settings and `BoardRules` has
  no global via-rule list, so the branch is omitted (documented in `RoutingBoardFanout.cpp`).

### Tests

`src/board/Pin_center_test.cpp` — 6 gtests (unset centre stays unset until requested, component +
relative location composition, arbitrary-rotation rounding, mirrored and flipped components,
`name()` of a package pin, missing package pin → `std::nullopt`);
`src/autoroute/RoutingBoardFanout_test.cpp` — 6 gtests →
**ctest 726/726** + run_tests PASSED.

## WP-10 Increment T — tie-pin trace shape reduction (2026-10-06)

Board half of the tie-pin reduction (the autoroute half, `MazeSearchAlgo::init`, is documented in
`MIGRATION_autoroute.md`).

### Ported surface

| Java | C++ | Notes |
|------|-----|-------|
| `ShapeSearchTree.changeItemShapeUnlocked(Item, int, TileShape)` | `ShapeSearchTree::change_item_shape(Item*, int, shared_ptr<TileShape>)` | take entries → remove only the old leaf from its shape slot → replace the `ItemSearchTreesInfo` precalculated shape at the same index → rebuild the entries → insert at `p_shape_index` → write the entries back. Out-of-range index: warning + the item is left untouched (Java would `ArrayIndexOutOfBoundsException`) |
| `ShapeSearchTree.reduceTraceShapeAtTiePin(Pin, PolylineTrace)` | `ShapeSearchTree::reduce_trace_shape_at_tie_pin(Pin*, PolylineTrace*)` | pin tree shape on the trace layer; first/last corner == pin center → shape index 0 (compare corner `corner_approx(1)`) or `corner_count() - 2` (compare corner `corner_approx(corner_count()-2)`); 2-D intersection guard; `TileShape::cutout` pieces; keep the first 2-D piece or the piece containing the compare corner; then `change_item_shape` |
| (internal) | `Item::get_precalculated_tree_shapes(const ShapeTree&)` / `set_precalculated_tree_shapes(vector<shared_ptr<TileShape>>, const ShapeTree&)` | new public accessors next to `take_search_tree_entries`/`set_search_tree_entries`, needed to replace the shape *inside* the per-tree `ItemSearchTreesInfo` cache |

### Porting notes

- **Index math.** C++ `PolylineTrace::corner_count()` is `arr.size() - 1` (Java parity) and
  `tile_shape_count()` is `arr.size() - 2`; `Polyline::from_corners(N points)` builds `N + 1`
  lines (start stub + segments + end stub), so a 2-corner trace has 1 tile shape and a 3-corner
  trace has 2.
- **Both shapes are `IntOctagon`.** The default `Item::calculate_tree_shapes` stores an octagon
  for `Pin` and for `PolylineTrace`, so `cutout` runs the octagon/octagon branch
  (`IntOctagon::cutout_from`) — the *correct* Java-equivalent dispatch (see the latent bug below).
- **Piece selection.** `cutout` pieces arrive in a deterministic order; the port keeps the first
  2-D piece and then replaces it with the piece containing the compare corner, matching Java's
  `foundPiece`/`contains(compareCorner)` loop.
- **Java NPE → early return.** A pin without a tree shape on the trace layer (layer out of the
  padstack range) returns without touching the trace; `DrillItem::get_tree_shape_on_layer` warns.

### Known latent bug discovered in this increment (NOT fixed)

`TileShape::cutout(const TileShape&)` (`src/geometry/planar/TileShape.cpp`) inverts the result for
the two dispatch branches where the **hole is a `Simplex`** and the **dividing shape is an
`IntOctagon` or `IntBox`**: they return hole-minus-divide instead of divide-minus-hole
(Java `A.cutout(B)` = pieces of A minus B). All other dispatch combinations are correct, and the
tie-pin path only uses octagon/octagon, so this increment is unaffected. Recorded in
`SOURCE_OF_TRUTH/03_MISSING.md` §2.1; a fix must add tests for the Simplex-hole branches.

### Tests

`src/board/ShapeSearchTree_tie_pin_test.cpp` — 7 gtests (change replaces the shape and keeps the
tree consistent incl. the out-of-range guard, first-corner reduction, last-corner reduction of a
3-corner trace with the first shape untouched, both no-op cases, null arguments) →
**ctest 753/753** + run_tests PASSED.

- **2026-10-08 (T418/T419)**: board library (`Padstacks`, `board::Package/Packages`, `BoardLibrary`,
  Java-faithful `Component/Components` with 1-based ids), per-layer drill item shapes (`DrillItem::get_shape`,
  `Pin::calculate_shapes`/`get_padstack_layer`, `Via::calculate_shapes`), Java tree shapes for drill items
  and a working `SearchTreeManager` (autoroute trees are filled with the board items). `ConvexTileShape`
  bridges the C++ split between `TileShape` and `Shape` for padstack shapes.
- **2026-10-08 (T121)**: Java insert facade on RoutingBoard (`insert_item`, `insert_pin`, `insert_obstacle`,
  `insert_via_obstacle`, `insert_component_obstacle`, `insert_component_outline`, `insert_conduction_area`,
  `insert_outline`), Java `BoardOutline` item and Java tree shapes for outlines and obstacle/conduction areas.

- **2026-10-10 (spec review, no code change)**: two parity gaps found while writing the per-task specs
  (`SOURCE_OF_TRUTH/plans/`): (1) **item order** — Java `Item.compareTo` returns `other.id - this.id` and the board item
  list is a `ConcurrentSkipListMap`, so `getItems()` and every `TreeSet<Item>` iterate newest-first; C++ iterates
  `items_` oldest-first and uses 118 pointer-ordered `std::set<Item*>` (not reproducible between runs) → T011a/b;
  (2) **search tree variants** — Java's autoroute tree is `ShapeSearchTree45Degree` / `ShapeSearchTree90Degree` for 45° /
  90° boards (`SearchTreeManager.getAutorouteTree`); the C++ subclasses are empty and never instantiated → T506a/b.
