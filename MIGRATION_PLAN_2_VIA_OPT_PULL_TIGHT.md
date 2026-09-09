# Migration Plan 2: Via Optimization + Trace Tightening (Session 2)

## Objective
Complete the headless router by porting **via optimization** (`OptViaAlgo`) and **trace tightening** (`PullTightAlgo` + variants), enabling `Trace::combine`, `Trace::split`, `Trace::pull_tight` — critical for routing quality.

## Target Classes (5 classes, ~2763 Java lines)

| Priority | Java Class | Lines | Depends On | Notes |
|----------|------------|-------|------------|-------|
| MED | `OptViaAlgo.java` | 574 | `ForcedPadAlgo`, `ShoveTraceAlgo`, `MoveDrillItemAlgo` (Plan 1) | Via location optimization |
| HIGH | `PullTightAlgo.java` | 470 | `ShoveTraceAlgo`, `CalcShapeAndFromSide`, `Polyline` | Base tightening (abstract) |
| HIGH | `PullTightAlgo45.java` | 622 | `PullTightAlgo` | 45° tightening |
| MED | `PullTightAlgo90.java` | 144 | `PullTightAlgo` | 90° tightening |
| HIGH | `PullTightAlgoAnyAngle.java` | 953 | `PullTightAlgo` | Any-angle tightening |

**Total**: ~2763 lines

## Prerequisites (Plan 1 Must Complete ✅)
- `ForcedPadAlgo` / `ForcedViaAlgo` / `MoveDrillItemAlgo` / `MoveComponent`
- `ShoveTraceAlgo` (check/insert stubs)
- `ShapeTraceEntries` / `CalcShapeAndFromSide` / `CalcFromSide`
- `Polyline` (offset_shape, projection_line, shorten, corner_approx)
- `TileShape` (offset, entrance_points, is_IntBox, is_IntOctagon)

## Implementation Order

### 1. OptViaAlgo (574 lines) — Via Optimization
**Key methods:**
- `opt_via_location(RoutingBoard, Via, ExpansionCostFactor[], int, int)` — optimizes via connected to ≤2 traces by trace costs; uses `ForcedPadAlgo` + `MoveDrillItemAlgo.check`
- `private` helpers for cost calculation, via shove tries

**Dependencies from Plan 1:** `ForcedPadAlgo.check_forced_pad`, `MoveDrillItemAlgo.check`

**Board APIs needed (stub if missing):**
- `RoutingBoard::get_trace_cost` / `set_trace_cost` (or `AutorouteControl.ExpansionCostFactor`)
- `Via::attach_allowed` (exists)
- `RoutingBoard::clearance_value` (exists via `BoardRules`)

### 2. PullTightAlgo (470 lines) — Abstract Base
**Key methods:**
- `Polyline pull_tight(Polyline)` — iterative: `avoid_acid_traps` → `try_skip_second_corner` → `try_skip_corners` → `reposition_lines` loop
- `Polyline avoid_acid_traps(Polyline)` — removes 45° segments that create acid traps
- `Polyline try_skip_second_corner(Polyline)` — skips corner 1 if clearance ok (uses `board.check_trace_shape`)
- `Polyline try_skip_corners(Polyline)` — main corner reduction loop (checks 4-line windows)
- `Polyline reposition_lines(Polyline)` — moves lines to grid/angle positions
- `Polyline smoothen_start/end_corner_at_trace(PolylineTrace)` — returns null (base)
- Protected fields: `curr_layer`, `curr_half_width`, `curr_net_no_arr`, `curr_cl_type`, `curr_clip_shape`, `contact_pins`, `min_translate_dist`

**Polyline methods used (already ported):**
- `offset_shape(int half_width, int index)` → `TileShape`
- `corner_approx(int)` → `FloatPoint`
- `arr` (Line[])
- `shorten(int, double)` 
- `remove_consecutive_parallel_lines` / `remove_overlaps`

### 3. PullTightAlgo45 (622 lines) — 45° Tightening
**Overrides `pull_tight`:**
- `try_skip_second_corner` — 4-line window check with `curr_clip_shape` containment + `board.check_trace_shape` on `offset_shape(half_width, i)` for i=0,1
- `try_skip_corners` — sliding 4-line window (indices `new_line_index-1`, `new_line_index`, `i-1`, `i`), checks `offset_shape` at indices 0 and 1, joins `changed_area` on skipped corners
- `smoothen_start/end_corner_at_trace` — returns null

**Specifics:** Uses `IntOctagon`/`IntBox` `offset_shape`; `board.changed_area.join` for corner updates.

### 4. PullTightAlgo90 (144 lines) — 90° Tightening
**Overrides `pull_tight`:** Similar to 45° but orthogonal logic (simpler corner skipping).
**Overrides `smoothen_start/end_corner_at_trace`** — returns null.

### 5. PullTightAlgoAnyAngle (953 lines) — Any-Angle Tightening
**Largest class. Overrides:**
- `pull_tight` — complex iterative with acid trap avoidance + corner skipping + line repositioning
- `smoothen_start/end_corner_at_trace` — actual implementation for smooth corners
- Uses `Side`/`FloatPoint` geometry heavily; `Polyline::projection_line`, `Line::intersection_approx`
- Acid trap detection via angle checks (`c_max_cos_angle = 0.999`)

## CMake Changes
Add to `src/board/CMakeLists.txt`:
```cmake
OptViaAlgo.cpp
PullTightAlgo.cpp
PullTightAlgo45.cpp
PullTightAlgo90.cpp
PullTightAlgoAnyAngle.cpp
```

## Trace Integration (Enable combine/split/pull_tight)
After `PullTightAlgo*` ported, implement in `Trace.cpp` / `PolylineTrace.cpp`:

```cpp
// Trace.h additions
virtual std::vector<std::shared_ptr<Trace>> combine() const;
virtual std::vector<std::shared_ptr<Trace>> split(const geometry::planar::IntOctagon& p_shape) const;
virtual std::vector<std::shared_ptr<Trace>> split(const geometry::planar::Point& p_point) const;
virtual void pull_tight(int p_accuracy, int p_max_recursion_depth, int p_max_via_recursion_depth);

// PolylineTrace.cpp implementations:
void PolylineTrace::pull_tight(...) {
    auto* pull_tight = create_pull_tight_algo(); // based on angle restriction
    auto new_poly = pull_tight->pull_tight(polyline());
    change(std::make_shared<Polyline>(new_poly));
}
```

**Angle restriction → PullTightAlgo mapping:**
- `NINETY_DEGREE` → `PullTightAlgo90`
- `FORTYFIVE_DEGREE` → `PullTightAlgo45`
- `ANY_ANGLE` → `PullTightAlgoAnyAngle`

## Validation Checklist
- [ ] `cmake --build --preset conan-release -j4` succeeds
- [ ] `ctest --preset conan-release -E AllTests` → 322/322 pass (add new tests if any)
- [ ] Headless test: `freerouting -de tests/data/Issue029-hw48na.dsn -do /tmp/test.ses -mp 5` → higher completion % (>19%)
- [ ] Verify `OptViaAlgo::opt_via_location` compiles
- [ ] Verify `PullTightAlgo*` hierarchy compiles (abstract + 3 concrete)
- [ ] Verify `Trace::pull_tight` compiles and runs without crash
- [ ] Verify `Trace::combine` / `split` compile (may need additional board APIs)

## Estimated Effort
- **OptViaAlgo**: ~3 hours (moderate, depends on Plan 1)
- **PullTightAlgo (base)**: ~3 hours (abstract + core loops)
- **PullTightAlgo45**: ~4 hours (largest concrete, complex corner logic)
- **PullTightAlgo90**: ~1 hour (simpler)
- **PullTightAlgoAnyAngle**: ~5 hours (most complex, acid traps, smoothen)
- **Trace integration**: ~2 hours
- **Total**: ~18 hours

## Quick Reference: Remaining Board Stubs Needed

| Method | Location | Needed By |
|--------|----------|-----------|
| `RoutingBoard::check_trace_shape` | `BasicBoard.cpp` | All `PullTightAlgo*` |
| `RoutingBoard::get_trace_cost` | `RoutingBoard.cpp` | `OptViaAlgo` |
| `RoutingBoard::set_trace_cost` | `RoutingBoard.cpp` | `OptViaAlgo` |
| `RoutingBoard::clearance_value` | `BasicBoard.cpp` | Exists? |
| `RoutingBoard::changed_area` | `BasicBoard.cpp` | Exists |
| `Trace::normalize` | `Trace.cpp` | `PullTightAlgo` (after change) |
| `PolylineTrace::change` | `PolylineTrace.cpp` | Exists |

---

## Sequential Dependency
**Plan 1 MUST complete first** — `OptViaAlgo` and `PullTightAlgo*` call `ForcedPadAlgo.check_forced_pad` / `ForcedPadAlgo.forced_pad` and `MoveDrillItemAlgo.check` / `ShoveTraceAlgo.check`.

---

**Start with OptViaAlgo** — it's independent of `PullTightAlgo*` and uses the shove chain from Plan 1. Then do `PullTightAlgo` base, then the three concrete variants in order of complexity (90° → 45° → AnyAngle).