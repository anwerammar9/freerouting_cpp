# Migration Plan 1: Core Shove Chain (Session 1)

## Objective
Port the **core shove chain** that enables forced pad/via placement and drill item movement — the critical path for headless routing quality improvement beyond the current 19%.

## Target Classes (4 classes, ~986 Java lines)

| Priority | Java Class | Lines | Depends On | Notes |
|----------|------------|-------|------------|-------|
| HIGH | `ForcedPadAlgo.java` | 352 | `ShapeTraceEntries`, `CalcFromSide`, `ShoveTraceAlgo` | Entry point for forced pad/via; calls `check_forced_pad` / `forced_pad` |
| HIGH | `ForcedViaAlgo.java` | 223 | `ForcedPadAlgo`, `ShapeTraceEntries` | Static `check` / `insert` using `ForcedPadAlgo` |
| MED | `MoveDrillItemAlgo.java` | 248 | `ForcedPadAlgo`, `ShapeTraceEntries` | Static `check`/`insert`/`shove_vias`/`try_shove_via_points` |
| MED | `MoveComponent.java` | 163 | `MoveDrillItemAlgo`, `RoutingBoard` | Component group move (DrillItem + ObstacleArea + ComponentOutline) |

## Prerequisites (Already Done ✅)
- `TileShape::offset` / `entrance_points` (geometry)
- `ShapeTraceEntries` (store_items, substitute traces, stack levels)
- `ShoveTraceAlgo` (static check + instance check/insert stubs)
- `CalcFromSide` / `CalcShapeAndFromSide`
- `RoutingBoard` / `BasicBoard` / `SearchTreeManager` / `ShapeSearchTree`
- `Item` / `DrillItem` / `Via` / `Pin` / `Component` / `ObstacleArea` subclasses

## Implementation Order

### 1. ForcedPadAlgo (352 lines) — Core Shove Pad
**Key methods to port:**
- `CheckDrillResult check_forced_pad(...)` — main entry; creates `ShapeTraceEntries`, calls `store_items`, checks vias via `MoveDrillItemAlgo.check`, iterates substitute traces with `ShoveTraceAlgo.check`
- `boolean forced_pad(...)` — same flow but calls `ShoveTraceAlgo.insert` and mutates board (`cutout_traces`, `insert_item`, `normalize`, `combine_traces`)
- `CalcFromSide calc_from_side(TileShape, Point, int, int, int)` — finds legal from-side via `board.check_trace_shape`
- `private static TileShape calc_check_shape_for_from_side(...)` — helper
- `private static boolean in_front_of_pad(...)` — octagon-specific clearance test (8 cases)

**C++ stubs needed:**
- `RoutingBoard::check_trace_shape` (stub: return true if no exact overlap)
- `RoutingBoard::insert_item` / `remove_item` / `join_changed_area` (already exist)
- `Trace::normalize` (stub)
- `RoutingBoard::combine_traces` (stub)
- `RoutingBoard::get_trace_tail` / `get_connection_items` (stub)
- `Item::StopConnectionOption` enum

### 2. ForcedViaAlgo (223 lines) — Thin Wrapper
**Key methods:**
- `static CheckDrillResult check_layer(...)` — creates `Circle` via shape, calls `ForcedPadAlgo.check_forced_pad`
- `static boolean check(ViaInfo, ...)` — iterates padstack layers, calls `forced_pad_algo.check_forced_pad`
- `static boolean insert(ViaInfo, ...)` — same but `forced_pad_algo.forced_pad` + `board.insert_via`
- `private static CalcFromSide calculate_from_side(...)` — geometric from-side calc (no board)

**Dependencies:** `ViaInfo`, `Padstack`, `Circle` (already in `Shape.h`)

### 3. MoveDrillItemAlgo (248 lines) — Drill Item Shove
**Key methods:**
- `static boolean check(DrillItem, Vector, ...)` — per-layer `ForcedPadAlgo.check_forced_pad` with `CalcFromSide(p_drill_item.get_center(), curr_tile_shape)`
- `static boolean insert(DrillItem, Vector, ...)` — per-layer `ForcedPadAlgo.forced_pad` + `p_drill_item.move_by`
- `static boolean shove_vias(TileShape, CalcFromSide, ...)` — uses `ShapeTraceEntries` + `try_shove_via_points` + recursive `check`/`insert`
- `static IntPoint[] try_shove_via_points(...)` — computes candidate via centers (box/octagon/shape offset + `nearest_border_projections` / `nearest_relative_outside_locations`)

**Dependencies:** `IntBox::nearest_border_projections`, `IntOctagon::nearest_border_projections`, `TileShape::enlarge`, `TileShape::nearest_relative_outside_locations` (already ported)

### 4. MoveComponent (163 lines) — Component Group Move
**Key methods:**
- Constructor: builds item group from component_no or single item; sorts by projection along translate vector
- `boolean check()` — iterates group, calls `MoveDrillItemAlgo.check` for DrillItems, `board.check_move_item` for others
- `boolean insert(int p_tidy_width, int p_pull_tight_accuracy)` — moves component first, then each item; calls `MoveDrillItemAlgo.insert` or `item.move_by`

**Dependencies:** `Components::move` (stub), `Communication::observers.notify_moved` (exists via `BoardObservers`)

## CMake Changes
Add to `src/board/CMakeLists.txt`:
```cmake
ForcedPadAlgo.cpp
ForcedViaAlgo.cpp
MoveDrillItemAlgo.cpp
MoveComponent.cpp
```

## Validation Checklist
- [ ] `cmake --build --preset conan-release -j4` succeeds
- [ ] `ctest --preset conan-release -E AllTests` → 322/322 pass
- [ ] Headless test: `freerouting -de tests/data/Issue029-hw48na.dsn -do /tmp/test.ses -mp 1` runs without crash
- [ ] Verify `ForcedPadAlgo` can be instantiated and `check_forced_pad` returns `DRILLABLE` for simple case
- [ ] Verify `ForcedViaAlgo::check_layer` compiles
- [ ] Verify `MoveDrillItemAlgo::check` compiles
- [ ] Verify `MoveComponent::check` compiles

## Estimated Effort
- **ForcedPadAlgo**: ~4 hours (complex, many board interactions)
- **ForcedViaAlgo**: ~1 hour (thin wrapper)
- **MoveDrillItemAlgo**: ~3 hours (via shove geometry)
- **MoveComponent**: ~1 hour (simple orchestration)
- **Total**: ~9 hours

## Next Session (Plan 2)
After Plan 1, the shove chain is complete. Plan 2 covers:
- `OptViaAlgo` (via optimization)
- `PullTightAlgo` + variants (trace tightening for `combine`/`split`/`pull_tight`)

---

## Quick Reference: Key Board Stubs Needed

| Method | Location | Status |
|--------|----------|--------|
| `RoutingBoard::check_trace_shape` | `BasicBoard.cpp` | STUB NEEDED |
| `RoutingBoard::insert_via` | `RoutingBoard.cpp` | STUB NEEDED |
| `Trace::normalize` | `Trace.cpp` | STUB NEEDED |
| `RoutingBoard::combine_traces` | `RoutingBoard.cpp` | STUB NEEDED |
| `RoutingBoard::get_trace_tail` | `RoutingBoard.cpp` | STUB NEEDED |
| `Item::get_connection_items` | `Item.cpp` | STUB NEEDED |
| `Components::move` | `Components.cpp` | STUB NEEDED |

---

**Start with ForcedPadAlgo** — it's the central piece that `ForcedViaAlgo`, `MoveDrillItemAlgo`, and eventually `ShoveTraceAlgo.insert` all depend on.