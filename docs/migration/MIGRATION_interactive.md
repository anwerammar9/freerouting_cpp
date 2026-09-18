# Interactive Module - Migration Status

**Module**: `src/interactive/`
**CMake Target**: `freerouting_interactive`
**Java Files**: 9 | **C++ Headers**: 6 | **C++ Impls**: 5 | **Tests**: 2 (NetIncompletes_test, RatsNest_test)

---

## Completed (Fully Migrated)

| Java Class | C++ Header | C++ Impl | Test | Java Lines | Notes |
|------------|-----------|----------|------|-----------|-------|
| HeadlessBoardManager | HeadlessBoardManager.h | HeadlessBoardManager.cpp | - | 207 | Headless board management |
| InteractiveSettings | InteractiveSettings.h | InteractiveSettings.cpp | - | 458 | Interactive settings |
| RatsNestItemInfo (+ RatsNestItemType) | RatsNestItemInfo.h | RatsNestItemInfo.cpp | (in RatsNest_test) | 9 | Rats nest item info |
| NetIncompletes | NetIncompletes.h | NetIncompletes.cpp | NetIncompletes_test.cpp | 277 + AirLine(55) | Incomplete net computation (WP-2, 2026-09-18) |
| RatsNest | RatsNest.h | RatsNest.cpp | RatsNest_test.cpp | 236 | Rats nest computation (WP-3) |

## Partially Migrated (Header Only)

| Java Class | C++ Header | Java Lines | What's Missing |
|------------|-----------|-----------|----------------|
| BoardManager | BoardManager.h | 32 | Implementation (interface) |

## Not Migrated

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| Route | 624 | HIGH | Interactive routing |
| ThreadActionListener | N/A | LOW | GUI thread listener |

## WP-2 Porting Notes (2026-09-18)

- `NetIncompletes.{h,cpp}` was already a near-faithful port (per-item filter, connected-set analysis, Delaunay triangulation, `Edge::Less` coordinate tie-breakers, airline creation, `calc_length_violation`). Faithfulness verified against `java/app/freerouting/drc/NetIncompletes.java` (398 lines).
- **Added `connected_group_count_` / `get_connected_group_count()`** — the Java API (`NetIncompletes.java:46,141,155`) exposes it, the C++ never tracked it. Counts distinct connected_set objects (identity) via a `std::set<const std::set<Item*>*>`; overridden to the net-item count when ≤ 1.
- **Fixed aliasing bug in `join_connected_sets()`**: took `const shared_ptr<std::set<Item*>>&` bound to `curr_edge.from_item->connected_set` / `->to_item->connected_set` — members the join itself reassigns (`curr_net_item->connected_set = p_to_connected_set`). Once the from-item was repointed mid-loop, `p_from` silently read the new to-set, so the remaining from-group items were never repointed → extra airlines (3 isolated vias gave 3 airlines, expected 2 MST edges). Java is immune: reassigning a field never changes the already-captured set reference. **Fix: pass shared_ptrs by value.**
- Guard against drawing debug: none left in source.

## Summary

- **Done**: 5 classes (67% of Java files)
- **Partial**: 1 class (11%)
- **Remaining**: 2 classes (22%)
- **Blockers**: Route (624 lines) is needed for interactive routing. HeadlessBoardManager + NetIncompletes/RatsNest already migrated, enabling headless incomplete-net computation.