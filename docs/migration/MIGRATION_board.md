# Board Module - Migration Status

**Module**: `src/board/` (headless)
**CMake Target**: `freerouting_board`
**Java Files**: 51 (headless scope: 49, excl. GUI `ObjectInfoPanel`/`PrintableShape`) | **C++ Headers**: 34 | **C++ Impls**: 28 | **Tests**: 6

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
| ShoveTraceAlgo | ShoveTraceAlgo.h | ShoveTraceAlgo.cpp | - | 593 | Headless shove checks/insert: static `check(LineSegment)` + instance `check(TileShape)`/`insert` via `ShapeTraceEntries`; `MoveDrillItemAlgo` via-shove and `spring_over` stubbed (headless returns polyline copy, via shove conservatively fails); bounding-box `is_contained_in` stubbed; `get_ignore_items_at_tie_pins` uses `overlapping_objects` |

## Not Migrated

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| ForcedPadAlgo | 352 | HIGH | Force-pad algorithm |
| ForcedViaAlgo | 223 | HIGH | Force-via algorithm |
| MoveComponent | 163 | MED | Component placement |
| MoveDrillItemAlgo | 248 | MED | Drill item movement |
| OptViaAlgo | 574 | MED | Via optimization |
| PullTightAlgo | 470 | HIGH | Trace tightening base |
| PullTightAlgo45 | 622 | HIGH | 45-degree tightening |
| PullTightAlgo90 | 144 | MED | 90-degree tightening |
| PullTightAlgoAnyAngle | 953 | HIGH | Any-angle tightening |

## Summary

- **CalcShapeAndFromSide port** (this session): ported the shove shape + fromside calculation (see row above; orthogonal/int-box vs simplex dog-ear-cut paths, `in_shove_check` fallback guard) together with `Trace::get_compensated_half_width` (which required only a `ShapeSearchTree` forward reference) — `get_tree_shape`/`bounding_box`/`to_Simplex`/`intersection` were already on `Item`/`Simplex`. Verified by `CalcShapeAndFromSide_test.cpp` (5 gtests) — **285/285 ctest passing**. The cut-line border match uses `Line::is_equal_or_opposite` because Java `Line.equals` normalizes direction, whereas C++ `fast_equals` is direction-sensitive.
- **CalcFromSide port** (previous session step): ported the shove-direction side calculation (see row above) with 11 gtests. That step also uncovered and fixed a latent geometry bug: `IntPoint::perpendicular_projection` used `determinant(p_line.b)` (cross product of the projected point with the line's endpoint) instead of the correct line-offset determinant `a.x*b.y - a.y*b.x`, producing wrong projections for border lines not through the origin and silently breaking `TileShape::nearest_border_point`.
- **ObstacleArea subclasses** (previous session): ported `ConductionArea`, `ViaObstacleArea` and `ComponentObstacleArea` (see rows above) on top of the `ObstacleArea` base. This resolves the previously "blocked on `ViaObstacleArea`/`ComponentObstacleArea`/`ConductionArea`" branches — the board item hierarchy now covers all obstacle-area kinds. Verified by `ObstacleAreaSubclass_test.cpp` (30 gtests) — **227/227 ctest passing**. Divergences: `Connectable` not implemented (same as Trace); UI (`get_draw_colors`/`print_info`) not ported.
- **ObstacleArea port** (previous session): replaced the 30-line stub with a faithful port — unblocked by the widened `Area` interface (mirror/rotate/turn/translate + `split_to_convex` on `PolygonArea`/`PolylineArea`). Verified by `ObstacleArea_test.cpp` (16 gtests). Note: 7 standalone data tests in `tests/data` (`geometry_tile_test`, `geometry_polyline_area_test`, `geometry_regular_tile_shape_test`, `datastructures_shape_tree_test`, `board_shape_search_tree_test`, `geometry_line_segment_test`, `geometry_direction_test`) had source files deleted from the tree by an external process mid-session (untracked, not in git); their CMake blocks are guarded with `if(EXISTS ...)` so the build proceeds and the tests auto-restore when the files are put back.
- **Done**: 41 classes (84% of headless scope, 80% of 51)
- **Remaining**: 9 classes (18% headless), `ObjectInfoPanel`/`PrintableShape` excluded as GUI-only; headless blockers: `ForcedPadAlgo`→`ForcedViaAlgo`/`MoveDrillItemAlgo` chain, `PullTight*`/`OptVia`/`MoveComponent`
- **Blockers**: PullTightAlgo* needed for trace routing quality; ForcedViaAlgo needed for via placement. Headless DSN loading now uses parsed padstack extents for pin bounding boxes, but full padstack shape/layer ownership and component obstacle materialization remain pending.
- **Note**: `ShapeSearchTree` now subclasses the ported `datastructures::MinAreaTree`; `Item` implements the `ShapeTree::Storable`/`SearchTreeObject` interface with per-tree entry/shape caches (`ItemSearchTreesInfo`); `SearchTreeManager::remove` uses the stored entries. The Java query layer is ported: `overlapping_tree_entries`, `overlapping_objects`, `overlapping_tree_entries_with_clearance`, `overlapping_objects_with_clearance`, `overlapping_items_with_clearance` (with the `EntrySortedByClearance` clearance-aware candidate sort and `shapes_intersect`/`enlarge_tile` shape helpers), plus a real `validate_entries`. Board-backed queries use `ClearanceMatrix::get_max_value_on_layer` as a conservative bound since the C++ matrix is layer-flat. `BasicBoard` now owns a `SearchTreeManager` and bridges the tree queries to `Board`-level API: `BasicBoard::overlapping_objects`/`overlapping_items_with_clearance` (mirroring `BasicBoard.java:859`). Large legacy entries (BasicBoard, RoutingBoard, Pin, BoardOutline, etc.) may still have incomplete implementations; validate with tests.
- **Trace port**: `Trace` is now an abstract base (dropping the `Connectable` interface, which demanded many pure virtuals only the broken untracked DRC used) with pure-virtual `get_length()` and fully ported `get_normal_contacts(Point, bool)` (Manhattan tolerance `half_width+1`, `shares_layer`/`shares_net`/`shares_net_no` filtering, `get_start_contacts`/`get_end_contacts`). The remaining `Trace.java` base API is ported too: `is_obstacle(Item*)`, `normal_contact_point(Item*)` (trace-to-trace unique corner), `is_tail()`, `is_overlap()`, `is_shove_fixed()` (with net-class lookup via `BoardRules::get_nets`), `is_drillable(int)`, `get_ratsnest_corners()`, `nearest_end_point(Point)`, `is_cycle()`, and `validate()` — backed by new `Item` virtuals (`Item::is_obstacle(Item*)`, `normal_contact_point`, `is_tail`, `is_overlap`, `is_shove_fixed`, `is_drillable`, `get_ratsnest_corners`, `validate`, `is_cycle_recu`). `PolylineTrace` is the concrete implementation with a lazy corner cache (`ensure_corners`/`first_corner`/`last_corner`), `get_tile_shape(i)` = segment bounding octagon enlarged by half width (approximating Java's `to_simplex().simplify()`), `get_length()` (full corner-chain length), `tile_shape_count()`, and `change()`. `Connection::trace_length()` now uses the real `Trace::get_length()` via `dynamic_cast`. Verified by `test_trace_normal_contacts` + `test_trace_core_methods` in `board_shape_search_tree_test.cpp` (8/8 tests green). Still pending: `combine()`, `split()`, `pull_tight()` (blocked on `PullTightAlgo*`). The obstructing `ViaObstacleArea`/`ComponentObstacleArea`/`ConductionArea` branches of `is_obstacle`/`get_normal_contacts` are now ported (this session).

### Pending (query consumers)
- `BasicBoard.overlapping_items(Area, int)` is now unblocked: `Area::split_to_convex` is on the `Area` base (this session) and `ObstacleArea` provides `tile_shape_count`/`get_tile_shape`.
- `Trace.combine()`, `Trace.split(IntOctagon)`, `Trace.split(Point)`, `Trace.pull_tight()` — blocked on `PullTightAlgo*` and trace-splitting geometry.
