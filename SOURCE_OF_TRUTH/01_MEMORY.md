# Memory: Architecture, Conventions, and Patterns

## Project Structure

```
freerouting_cpp/
├── src/                          # C++20 migration (no Java here)
│   ├── main.cpp                  # Entry point
│   ├── Freerouting.cpp/.h        # Main application class
│   ├── autoroute/                # Auto-routing algorithms
│   ├── board/                    # Board model
│   ├── constants/                # Constants
│   ├── core/                     # Core data model
│   ├── datastructures/           # Generic data structures
│   ├── designforms/specctra/     # Specctra DSN/SES I/O
│   ├── drc/                      # Design rules checking
│   ├── geometry/planar/          # 2D geometry primitives
│   ├── interactive/              # Interactive/headless
│   ├── logger/                   # Logging
│   ├── management/               # Session/job management
│   ├── rules/                    # Design rules
│   └── settings/                 # Configuration
├── java/                         # Java baseline (Freerouting 2.1.0), source of truth
│   └── app/freerouting/          # package root (incl. gui/, api/, analytics/)
├── tests/                        # Integration tests + test data
├── docs/migration/               # Per-module migration status
├── templates/                    # Migration templates
├── SOURCE_OF_TRUTH/              # This folder — compiled knowledge
└── build/                        # Build output
```

The Java reference files are no longer co-located in `src/`; read them from
`java/app/freerouting/<module>/ClassName.java`.

## Build System

- **CMake** 3.20+, **C++20**, **Conan** for deps
- Dependencies: Boost (multiprecision), GoogleTest 1.15, Threads
- Static libraries per module, linked into `freerouting` executable
- Tests: `ctest --test-dir build` (~377 tests registered)

### Build Commands
```bash
conan install . --build=missing -of build
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DCMAKE_PREFIX_PATH=build/build/Release/generators
cmake --build build --target freerouting_board -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Layering Constraints (CRITICAL)

- `freerouting_core` = leaf static lib (no target_link_libraries)
- `freerouting_interactive` links core + board
- `freerouting_board` links rules, geometry, logger, datastructures
- `freerouting_autoroute` links core + board
- **BoardStatistics** must go in a separate `freerouting_scoring` lib (core→interactive would be a cycle)

## C++20 Conventions

- Smart pointers (`unique_ptr`/`shared_ptr`), RAII, `std::optional`
- Naming: `PascalCase` classes, `snake_case` functions/variables, `kPascalCase` constants, trailing `_` for private members
- Each migrated class produces 3 files: `.h`, `.cpp`, `_test.cpp` (Java reference in `java/`)
- `#pragma once` headers
- Member initializer lists for constructors

## Key Gotchas (Verified, Do Not Rediscover)

1. **`Components` duplicate**: canonical is `board::Components` in `src/board/Component.h` (method `component_count()`). `src/board/Components.h/.cpp` is DEAD code.
2. **Java nets are 1-based** for RatsNest indexing (`get_net_no(i) - 1`)
3. **`Item::get_normal_contacts()`** default returns `{}`. Trace overrides with real implementation.
4. **`RoutingBoard::add_item`** only appends to `items_`; search-tree registration via `BasicBoard::insert_trace_without_cleaning`
5. **`DrillItem` canonical = `Via.h`** (DrillItem base, Via, Pin : DrillItem)
6. **`Trace::get_length()`** is pure virtual; PolylineTrace overrides
7. **`PlanarDelaunayTriangulation`**: turn Items into `shared_ptr<Storable>` adapters
8. **JSON**: hand-rolled; match Java `@SerializedName` exactly
9. **No JSON serializer** — BoardStatistics needs manual JSON string builder
10. **`run_tests`** (assert suite) is separate from ctest; both must pass
11. **DrillItem layer range comes from the padstack** (Java `DrillItem.firstLayer/lastLayer`) — `Via` ctor + `set_padstack` sync `from_layer_`/`to_layer_` from the padstack. `Padstack` is incomplete in `Via.h`, so `set_padstack` must be defined in `Via.cpp`.
12. **shared_ptr aliasing vs Java reference semantics (NetIncompletes.join_connected_sets)**: Java captures a set reference in a local before a loop that reassigns fields — the captured reference stays valid. Porting this to `const shared_ptr<...>&` params that alias **members the same loop reassigns** breaks: once the from-item is repointed mid-loop, `p_from` reads the new to-set and the rest of the from-group is skipped → too many airlines. **Pass shared_ptrs by value** for such join/union helpers. (3-island MST test would catch: 3 isolated vias must produce 2 airlines, gave 3.)
13. **dynamic_cast branch order matters (BoardStatistics item counting)**: `Pin` derives from `DrillItem`, so a `dynamic_cast<DrillItem*>` branch placed BEFORE the `Pin` branch silently swallows all pins (`pinCount` stays 0, which cascades into difficulty/fanout stats). Check `is-a` relationships before trusting an if/else-if chain — test classes before registering them. (Also: mirror Java's `RatsNest.max_connections = total_item_count - max_net_no`, which goes NEGATIVE on boards with empty registered nets — do not "fix" it, and make sure normalisation guards `maxScore <= 0`.)
14. **Same class name in two headers = landmine (RouterScoringSettings)**: `settings/RouterSettings.h` historically defined its OWN ad-hoc `RouterScoringSettings` (via_cost/corner_cost/length_cost) while `settings/RouterScoringSettings.h` defines the real Java-mirror struct — both in `freerouting::settings`. They only collide when a single TU includes both (e.g. a file using RoutingJob + BoardStatistics). Fixed by deleting the legacy struct and including the real header (legacy fields were never read). When migrating, always grep that a name isn't already defined elsewhere before reusing it. **Layering:** BoardFileDetails (needs scoring+board+RoutingJob) CANNOT live in the leaf `freerouting_core` — it lives in `freerouting_scoring`. `freerouting_core` stays dependency-free.

15. **C++ `IntOctagon::is_empty()` != Java `IntOctagon.isEmpty()` (45° port gotcha)**: Java tests identity against the `IntOctagon.EMPTY` singleton, while the C++ port treats every *degenerate* octagon (1-D segment or point, i.e. `right <= left` / `top <= bottom` after `normalize()`) as empty. So C++ `dimension()` returns -1 where Java returns 0/1, and any Java branch on `dimension` (door creation, skip of empty intersections) diverges. `Sorted45DegreeRoomNeighbours` contains this with local `java_is_empty()` / `java_dimension()` helpers instead of changing the shared geometry class — don't "fix" `IntOctagon` under a parity port. **Verified in the 90-degree port (WP-10 Inc G):** `SortedOrthogonalRoomNeighbours` needs NO such helpers, because that Java variant only ever handles `IntBox` shapes (a non-box room shape or a non-box neighbour tree shape aborts the calculation exactly like in Java) — apply the `java_is_empty`/`java_dimension` workaround only to variants that really walk octagon geometry.
16. **Search-tree room lifetime**: `ShapeSearchTree::insert(SearchTreeObject&)` stores raw pointers to the object, so a room created as a temporary (or a `unique_ptr` that dies first) leaves dangling leaves → `MinAreaTree::overlaps()` segfaults, far from the real cause. In tests, hand the tree rooms that outlive it (e.g. `make_unique` results held in the test body, declared *after* the tree); in production code the engine owns its rooms. Also declare the room after the tree so destruction order never outlives it either.
17. **`ExpansionDoor::other_room()` overload trap**: `ExpansionDoor` has two overloads — `other_room(ExpansionRoom*)` and `other_room(CompleteExpansionRoom*)`. The second one **nulls the result when the other room is not complete** (a faithful port of Java `ExpansionDoor.otherRoom(CompleteExpansionRoom)`, which does the same `instanceof CompleteExpansionRoom` check). So `door->other_room(complete_room)` returns `nullptr` for a door to an `IncompleteFreeSpaceExpansionRoom` and the code silently does nothing. When walking the doors of a complete room, always go through a base `ExpansionRoom*` (e.g. `static_cast<ExpansionRoom*>(p_room)`), mirroring the explicit `(ExpansionRoom) room` cast in Java `AutorouteEngine.completeNeighbourRooms()`.
19. **`java.util.Random` LCG parity (maze ripup randomization)**: the JDK `next(bits)` returns the top `bits` of the state **after** the `seed = seed * 0x5DEECE66D + 0xB` step (verified: `new Random(1000).next(32) == -1244746321`), and `nextDouble()` is `((next(26) << 27) + next(27)) * 2^-53`. `MazeRandom` in `src/autoroute/MazeRandom.h` reproduces this bit for bit — do not "simplify" it to `std::mt19937`/`std::uniform_real_distribution`, the ripup costs of pass >= 4 would stop matching Java. Verify new ports against the workspace JDK (`/home/anwer/.local/jdk-25.0.4.1+1`) with a throwaway `java.util.Random` program, not against hand-computed constants.
20. **C++ contacts only at trace corners**: the ported `Trace::get_normal_contacts(point, ignore_net)` finds a contact item only if the point is within tolerance of the contact's **first or last corner** (or a drill center / a conduction area). A T-contact in the middle of a segment is not detected, so a test for `get_start_contacts()` must place the *other* trace's end corner on the point.
21. **Java gate ≠ C++ gate for the same removal (AutorouteEngine)**: Java `removeCompleteExpansionRoom()` warns only when the `completeExpansionRooms` collection is `null`, then calls `remove()` (a no-op for an unknown room). The C++ engine always has the `complete_expansion_rooms_` vector, so warn when the room is *not found* in it (the old port did an unguarded `erase(find(...))`, i.e. UB on `end()`) and otherwise erase.

22. **Java `Polyline` is a closed 3+-line object, not a corner list**: `new Polyline(Point[] points)` creates `points.length + 1` lines (the two corner-to-corner segments plus a 45-degree stub line at each end), and `cornerCount() == lines.length - 1`. `new Polyline(fromCorner, toCorner)` always creates exactly **3** lines and returns an **empty** polyline (`lines.length == 0`) when the corners are equal. Because `isEmpty()` is `lines.length < 3` and `RoutingBoard.insertForcedTracePolyline` bails out on `lines.length < 3`, getting this wrong makes every single-segment forced-trace insert silently return its from corner instead of inserting. In C++: Java `new Polyline(Point[])` → `Polyline::from_corners`, and `Polyline(const std::vector<IntPoint>&)` is a **C++-only** lines-between-points ctor that has no Java counterpart — never substitute it for `from_corners`.
23. **Test fixtures must anchor connection ends with a pin**: `FoundConnectionInserter::insert_trace` finishes with a stub-cleanup loop that removes every contact-free trace whose first/last corner sits on an intermediate connection corner (Java does the same). A synthetic route that starts in empty space therefore deletes its own freshly inserted trace and `get_instance` looks like it failed. Put a pin (or a pin-anchored trace) on the connection's end points in gtests.
24. **`ShapeTraceEntries::store_items` blocks `FixedState::SHOVE_FIXED`, not `USER_FIXED`**: a user-fixed trace is *not* by itself a blocker for the shove check (Java `currentItem.isShoveFixed()`). Do not write "blocked by user fixed" tests.
25. **`RoutingBoard::insert_forced_trace_polyline` passes a negative `CalcFromSide` index** (`corner_count() - trace_shapes.size() - 1 + i` = `i - 2` when no trace was picked for combining). Java computes the same expression; `ShapeEntrySide`/`CalcFromSide` silently fall back to the "first corner inside shape" branch for a negative index. Consequence: the in-loop shove check is weaker than `check_forced_trace_polyline`, which uses `i + 1` (Java parity, reproduce it, do not "fix" it).
26. **Java nullable floats must stay nullable in C++ ports**: `ShapeEntrySide.borderIntersection` is a nullable `FloatPoint`, and `ShapeTraceEntries.resort()` branches on `null` (own-net entry removal). The first C++ port used `(0,0)` as the sentinel, which collided with a legal intersection and left a sole own-net entry in the list, so `calculate_stack_levels()` returned `false` and every multi-segment forced-trace insert failed. Use `std::optional<FloatPoint>` + a `values()` factory mirroring `new ShapeEntrySide(no, null)`; `ShapeTraceEntries::from_side_` is likewise `std::optional<CalcFromSide>`.
27. **`clearance_safety_margin` is ADDED for safety checks, and `BasicBoard.clearanceValue()` always adds it**: `ClearanceMatrix::get_value(c1, c2, layer, /*p_for_safety_check=*/true)` is `value + clearance_safety_margin` (16), **not** `value - margin` — the port had the sign inverted, so every shove offset was 16 too small. `RoutingBoard::clearance_value()` is the port of Java `BasicBoard.clearanceValue()`, which calls the 4-arg overload, so it must request the safety-check value (Java `ShapeSearchTree` calls it with `false` at its `clearanceValue`/`drillRadius` sites and with `true` at line 568).
28. **Java trace corners must be copied, not referenced**: `PolylineTrace::first_corner()`/`last_corner()` return a `const Point&` into the `corner_points_` cache, and `combine_at_start()`/`combine_at_end()` call `clear_derived_data()` (which empties it) after taking the corner — so the reference dangles into `join_changed_area()` and segfaults. Java writes `Point startCorner = firstCorner()` (a copy). `PolylineTrace::own_corner(index)` (`-1` = last) returns an owned `IntPoint`. Java `PolylineTrace.combine()` also **loops** (`while (isOnTheBoard() && (combineAtStart(true) || combineAtEnd(true)))`) instead of recursing; keep the C++ version iterative.
29. **A C++ trace that should "always fail" is often a mis-ported Java branch**: `ShoveTraceAlgo::insert()` returning `false` for a crossing foreign trace was blamed on `calculate_stack_levels()` requiring end level 1; the real cause was the abbreviated `ShapeTraceEntries::resort()` (an early `return` for lists of fewer than two entries). Verify suspicious "documented Java parity quirks" against the actual Java before pinning them in a test — and drive the real Java with a throwaway harness (compile the whole `java/app/freerouting` tree with `javac` against `/home/anwer/.cache/freerouting-gradle-build/libs/freerouting-2.1.0-executable.jar`; set `board.changedArea = new ChangedArea(board.getLayerCount())`, otherwise the trailing `normalize()` NPEs into the log and the harness reports nothing).

30. **`RoutingBoard::fanout()` cannot live in the board module**: the Java method (`RoutingBoard.java:990-1122`) needs `AutorouteControl` + `AutorouteEngine` + `AutorouteAttemptResult`, which `freerouting_board` may not reference. Declare it in `src/board/RoutingBoard.h` (forward-declare `autoroute::AutorouteAttemptResult`, as is already done for `settings::RouterSettings` / `datastructures::Stoppable` / `datastructures::TimeLimit`) and define it in `src/autoroute/RoutingBoardFanout.cpp`.
31. **Java `Communication.getResolution(Unit)` scales with INVERTED arguments** (`Unit.scale(resolution, Unit.UM, unit)` — the target unit comes first). Reproduce it verbatim so the fanout escape limits match Java; do not "fix" the order.
32. **The compiled `board::Components` is index-based, Java's is id-based**: `src/board/Component.h` holds the `Components` used by `BasicBoard` (`get(int index)`), while `src/board/Components.h` is an unrelated shared-pointer class that is not compiled into anything. `Pin::get_component()` must follow the compiled C++ behaviour (index, `< 0` guard) and note the Java `Pin.getComponent()` 1-based id lookup as a divergence.
33. **`DrillItem`'s canonical C++ declaration is `src/board/Via.h`** (`board/DrillItem.h` is a stale uncompiled duplicate), and `center_is_set_`/`center` are mutable because Java's `Pin.getCenter()` fills a cached centre on demand (`Item.drillIsSet`); `Pin::get_center()` computes `component.location + relative_location()` and corrects against the **untransformed** padstack shape, since the C++ `Pin` has no precalculated board-coordinate shape like Java's.
34. **`MazeFanoutDiagnostics` holds an `AutorouteControl` reference**, so `MazeSearchAlgo::set_control()` must destroy and recreate it — a stale control reference otherwise breaks as soon as the control is replaced. Its `enabled()` gate is a `fanout_start_pin_name` prefix (`U27-`), so fanout traces only appear for that naming pattern (Java parity, not a bug).

## Dependency Graph (What to Migrate Next)

```
DONE: geometry, rules, logger, constants, datastructures(core), board(100% headless), autoroute(83%)
                          │
                          ▼
   WP-1: Contact graph (Item contacts + DrillItem contacts)
   WP-2: NetIncompletes (full port)
   WP-3: RatsNest (full port)
                          │
                          ▼
   WP-4: BoardStatistics + scoring
                          │
                          ▼
   WP-5: Tests + CMake + docs
                          │
                          ▼
   WP-6: RoutingJob / Session / BoardFileDetails
                          │
                          ▼
   WP-7..15: Remaining modules (parallel)
```

## Critical Path for Headless Routing

1. `geometry/planar` — ✅ Done
2. `datastructures` — ✅ Core done
3. `rules` — ✅ Done
4. `board` — 84% headless (41/49)
5. `autoroute` — 83% (30/36 compiled)
6. `designforms/specctra` — 32% (DSN load works, SES write partial)
7. `drc` — 29%
8. `settings` — 29%
9. `core` — 19%
10. `management` — 11%
