# Phase 5 — Board gaps (T501–T506)

Run these before the E2E tasks (M5) so known holes do not show up as E2E divergences. **T506a must be done before
T611** (shakedown): nearly every test board uses the 45° snap angle.

---

## T506a — ShapeSearchTree45Degree (the autoroute tree on 45° boards)  (O) — NEW, high priority

**Finding (2026-10-10).** Java `SearchTreeManager.getAutorouteTree` (board/searchtree/SearchTreeManager.java 140–160)
creates `ShapeSearchTree90Degree` for `NINETY_DEGREE`, `ShapeSearchTree45Degree` for `FORTYFIVE_DEGREE`, and a plain
`ShapeSearchTree(FortyfiveDegreeBoundingDirections, …)` otherwise. In C++ the two subclasses exist only as empty
constructors (`src/board/ShapeSearchTree.h` 324–336, `.cpp` ≈1105–1115) and `SearchTreeManager.cpp` never
instantiates them. Every C++ caller of `complete_shape` (e.g. `Sorted45DegreeRoomNeighbours.cpp:764`,
`SortedRoomNeighbours.cpp:732`) therefore runs the *generic* algorithm where Java runs the 45° one — room shapes,
doors and maze expansion differ on almost every test board.

**Java** (`board/searchtree/ShapeSearchTree45Degree.java`, 667): ctor, `completeShape(...)` 98–111 (public entry:
returns rooms as IntOctagon-based `IncompleteFreeSpaceExpansionRoom`s), `completeShapeUnrestricted…` 112–307,
`divideLargeRoom` 308–323, `restrainShape` 324–507, `calculateTreeShapes(DrillItem)` 508–540 (drill-hole parts already
ported into the base class by T419 — reuse), `calculateTreeShapes(ObstacleArea)` 541–551,
`calculateTreeShapes(BoardOutline)` 552–≈580, and the remaining helpers to 667.

**C++ target.** Implement the overrides in `ShapeSearchTree45Degree` (make the base methods `virtual` where Java
overrides them: `complete_shape`, `calculate_tree_shapes(DrillItem/ObstacleArea/BoardOutline)`); move the angle
branches that T419 put into the base `calculate_tree_shapes(DrillItem)` into the subclasses if that makes the Java
structure clearer (behaviour must stay the same). Update `SearchTreeManager::get_autoroute_tree` to instantiate the
subclass by `rules.get_trace_angle_restriction()` exactly like Java.

**Watch out.** 01_MEMORY gotcha 15: C++ `IntOctagon::is_empty()` differs from Java's — use the local
`java_is_empty()`/`java_dimension()` helpers (see `Sorted45DegreeRoomNeighbours.cpp`) inside the 45° algorithm.
Gotcha 16: rooms inserted into the tree must outlive it.

**Tests.** Port the scenario of each `completeShape` branch: empty tree → board-bounded room; one octagon obstacle →
pieces equal Java's; room larger than the divide threshold → `divideLargeRoom` result. Golden pieces from a throwaway
Java program calling `ShapeSearchTree45Degree.completeShape` on the same tree contents. Re-run all autoroute tests —
expect some expectation changes; update only from Java output.

## T506b — ShapeSearchTree90Degree  (O) — NEW

**Java** (`board/searchtree/ShapeSearchTree90Degree.java`, 523): `completeShape` 41–54, `completeShapeUnrestricted…`
55–216, `restrainShape` 217–453, `calculateTreeShapes(DrillItem)` 454–483 (drill hole: reuse), `(ObstacleArea)`
484–494, `(BoardOutline)` 495–512, `offsetShapes(Polyline, halfWidth, …)` 513–523. The 90° tree works on `IntBox`es
(check its bounding directions in the ctor).

**Tests.** As T506a with IntBox obstacles; a board with `(snap_angle ninety_degree)` routes through the 90° tree
(`SortedOrthogonalRoomNeighbours` only accepts IntBox rooms — 01_MEMORY gotcha 15).

---

## T501 — Drill-hole obstacles: verify, finish  (S)

**Status.** The search-tree part is ported (T419): `drill_hole_obstacle` / `drill_hole_clearance_delta`
(`src/board/ShapeSearchTree.cpp` 745–805, Java ShapeSearchTree.java 1137–1200, `DRILL_HOLE_CLEARANCE_MARGIN = 10`),
used for 45°/90°/any-angle in the base class. `ForcedViaAlgo.cpp` ≈396 uses the hole clearance too.

**Remaining.**
1. After T506a/b, check the 45°/90° `calculateTreeShapes(DrillItem)` overrides call the same helpers (Java
   90Degree 462/471, 45Degree 516/532).
2. The hole-clearance *setting* path is `HeadlessBoardManager.applyHoleClearanceOverride` (T124b) — nothing to do here
   beyond confirming `BoardRules::set_hole_clearance` is what it calls.
3. DRC: Java `DesignRulesChecker` 563–570 classifies `holeClearance` violations — confirm the C++ DRC emits
   `hole_clearance` (src/drc/DrcViolation.h:39) in the same situations (unit test with a via near a foreign trace and
   `hole_clearance > 0`).

**Done when.** The three checks have tests or are documented as already covered; then mark T501 `[x]`.

---

## T502 — Pin parity audit  (S)

**Java** `board/model/items/Pin.java` (706) public methods with line numbers: `relativeLocation` 65, `getCenter` 92,
`getPadstack` 123, `copy` 134, `name` 150, `getPinIndex` 160, `getShape` 165–264, `getTraceExitRestrictions` 265–333,
`hasTraceExitRestrictions` 334, `drillAllowed` 348, `isObstacle` 353–373, `rotateApprox` 374, `changePlacementSide`
380, `clearDerivedData` 386, `getSwappablePins` 392–429, `swap` 438–465, `getChangedTo` 466, `isPlacedOnFront` 482,
`getMinWidth` 492–510, `getTraceNeckdownHalfwidth` 511, `getMaxWidth` 517–532, `calcNearestExitRestrictionDirection`
569–637, `nearestTraceExitCorner` 638–675.

**C++ today** (`src/board/Pin.h`): names not found by a quick grep — `get_padstack`, `copy`, `get_pin_index`
(C++ has `get_pin_no`), `get_shape`, `is_obstacle`, `rotate_approx`, `change_placement_side`, `clear_derived_data`,
`get_swappable_pins`, `swap`, `is_placed_on_front`, `get_min_width` (C++ has `min_width`?),
`calc_nearest_exit_restriction_direction`. Some may live in the `DrillItem` base (`src/board/Via.h`).

**Steps.** For each Java method: find the C++ equivalent (Pin or DrillItem); if missing, port it; if present, diff the
logic. `printInfo`/`getHoverInfo`/`isSelectedByFilter`/`write` are GUI/serialisation — skip. `copy` is needed by T420.
`getSwappablePins`/`swap` need `LogicalPart` (T115) — port anyway, test with a hand-built logical part.

**Tests.** One test per ported method; `calc_nearest_exit_restriction_direction` and `nearest_trace_exit_corner` with
golden values from Java for a rectangular SMD pad.

---

## T503 — DrillItemMover vs MoveDrillItemAlgo  (S)

**Java** `board/actions/DrillItemMover.java` (326), used by `board/optimize/TraceShover.java` 119/128/323/335
(`tryShoveViaPoints`, `check`). **C++** `src/board/MoveDrillItemAlgo.{h,cpp}` already has `check` (h:32) and
`try_shove_via_points` (h:71). Diff all methods of `DrillItemMover` against `MoveDrillItemAlgo`; port missing ones
(`insert`, helpers); add `// Port of Java DrillItemMover.<method>` comments; update `JavaToCppClassMap.csv`
(`DrillItemMover` → `MoveDrillItemAlgo.h`).

**Tests.** Via shoved out of a trace's way: new via centre equals Java's for one fixture.

---

## T504 — ShapeEntrySide / ShapeAndEntrySide audit  (S)

**Java** `board/model/structure/ShapeEntrySide.java`, `ShapeAndEntrySide.java`. **C++** `src/board/CalcFromSide.*`,
`ShapeTraceEntries.*` (01_MEMORY gotchas 25/26 mention `ShapeEntrySide` already). Find where each Java class is used,
find the C++ equivalent, port what is missing, record the mapping in `JavaToCppClassMap.csv`.

---

## T505 — Board facade audit  (S)

**Java** `board/facade/`: `BoardConnectivityQueries` (125), `BoardItemRepository` (288), `RoutingBoardOperations` (121),
`RoutingBoardSearchFacade` (218), `RoutingBoardUndoFacade` (105, `deepCopy` → T420), `BoardSnapshotManager` (101 → T420).

**Steps.** For each public/package method, find the C++ `RoutingBoard`/`BasicBoard` method (they were merged into the
board classes in C++). Produce a table in `docs/migration/MIGRATION_board.md` (Java method → C++ method or MISSING).
Port missing methods ≤ ~50 lines each here; file a task for anything bigger. Undo/redo (`undo`, `redo`,
`applyUndoRedoSideEffects`) is used by the optimizer worker boards only through snapshots — check before porting.

**Done when.** The table has no unexplained MISSING rows.
