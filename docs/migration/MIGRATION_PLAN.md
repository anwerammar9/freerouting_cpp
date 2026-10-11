# Freerouting C++ Migration — Agent Work Plan

**Purpose**: Handoff document for parallel migration work. Multiple agents (fresh sessions) pick a work package (WP), implement it, verify, and check it off. Work packages are ordered by dependency; independent WPs can run concurrently.

**Source of truth**: Java reference lives in `java/app/freerouting/**` (Freerouting 2.1.0). Migrate faithfully: same classes, names (Java `camelCase`), fields, algorithm logic. Prefer existing C++ idioms already established in this repo (see `src/board/`, `src/geometry/planar/`).

---

## 0. Session Setup / Ground Rules

Verify with these first — do NOT trust stale memory.

```bash
# Configure & build (conan toolchain)
conan install . --build=missing -of build
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=<repo>/build/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DCMAKE_PREFIX_PATH=<repo>/build/build/Release/generators

# Build everything (build tagged test runner first; it is EXCLUDE_FROM_ALL)
cmake --build build --target freerouting_board -j$(nproc)    # quick sanity
cmake --build build --target run_tests -j$(nproc)            # assert-based tests
ctest --test-dir build -N                                    # list registered tests (~377)
ctest --test-dir build --output-on-failure                   # run all
```

- Tests: `tests/CMakeLists.txt` registers tests **manually** (see existing entries; many use
  `$<LINK_GROUP:RESCAN,freerouting_autoroute,freerouting_board,freerouting_core,freerouting_datastructures,freerouting_geometry,freerouting_rules,freerouting_logger,freerouting_constants>`).
  New tests go in `src/<module>/X_test.cpp` and must be added to `tests/CMakeLists.txt` (not auto-discovered).
- GTest is available. `src/test_main.cpp` is a separate minimal `assert`-style smoke binary.
- Do NOT commit unless asked. Update `docs/migration/INDEX.md` + per-module `MIGRATION_*.md` when a WP lands.
- No JSON serializer exists. BoardStatistics needs a **manual JSON string builder** matching Java `@SerializedName` keys.

### Layering constraint (CRITICAL, verified)
- `freerouting_core` = leaf static lib (no `target_link_libraries`) — must stay leaf-ish.
- `freerouting_interactive` links `freerouting_core` + `freerouting_board`.
- Java `core.scoring.BoardStatistics` imports `interactive.RatsNest`. **Do NOT put BoardStatistics in freerouting_core**
  (would force core → interactive, a cycle). Create a new lib (e.g. `freerouting_scoring` in
  `src/core/scoring/CMakeLists.txt`) that links `board + interactive + settings + datastructures + management + core`.
- `freerouting_board` links `freerouting_rules`, `freerouting_geometry`, `freerouting_logger`, `datastructures`;
  `management` has `TextManager` (statics used by BoardStatistics).

---

## 1. Package Dependency Graph (what to pick next)

```
DONE or near-done: geometry, rules, logger, constants, datastructures(core set),
                   board (84% headless), autoroute (60%), core enums/events, settings/RouterScoringSettings
                        │
                        ▼
   [WP-1] contact graph           [WP-2] NetIncompletes (full)         [WP-3] RatsNest (full)
   (Item get_connected_set        (real Delaunay algorithm,            (headless; builds on WP-1/2)
    + DrillItem contacts            replaces the stub)
    + Trace::get_normal_contacts)         │
                        └────────────┬────┘
                                     ▼
   [WP-4] BoardStatistics + 11 substructs + RouterScoringSettings wiring + manual JSON
                                     ▼
   [WP-5] core/enums+events tests, BoardStatistics/RatsNest gtests, CMake registration, docs
                                     ▼
   [WP-6] RoutingJob / Session / BoardFileDetails / RoutingStage / StopRequestState  (full)
                                     ▼
   [WP-7 ..] remaining modules (independent, any order):
     WP-7 datastructures remainder   WP-8 management remainder
     WP-9 settings remainder         WP-10 drc remainder
     WP-11 designforms/specctra      WP-12 boardgraphics
     WP-13 autoroute remainder       WP-14 interactive remainder (HeadlessBoardManager, Route)
     WP-15 board remainder (drill pages, move algos)
```

---

## 2. Work Packages

### WP-1 — Contact graph for ratsnest (PREREQUISITE for WP-2)

**Java refs**: `board/Item.java:546` (`get_connected_set`), `board/Trace.java:172`
(`get_normal_contacts(Point,boolean)`), `board/DrillItem.java:264` (`get_normal_contacts`),
`board/DrillItem.java:342` (`get_ratsnest_corners`), `board/ConductionArea.java`.

**C++ files**: `src/board/Item.{h,cpp}`, `src/board/Trace.{h,cpp}`, `src/board/Via.h` (canonical `DrillItem`), `src/board/ConductionArea.{h,cpp}`.

**Tasks**:
1. `Item::get_connected_set(int p_net_no, bool p_stop_at_plane)` — **signature already added to `src/board/Item.h`**.
   Implement `get_connected_set_recu` faithfully (recursive over `get_normal_contacts()`, skip `ConductionArea`
   with `get_component_no() <= 0` when stop_at_plane, filter by `contains_net`).
2. `DrillItem::get_ratsnest_corners() const` — returns `std::vector<std::shared_ptr<geometry::planar::Point>>`
   with the single center point (Java `DrillItem.java:342`). `get_center()` exists.
3. `DrillItem::get_normal_contacts() const` — search `board.overlapping_objects(search_shape, layer)` with
   tolerance `min_width()/2 + 1` over padstack layers; collect `Trace`/`DrillItem`/`ConductionArea` (`DrillItem.java:264`).
   `DrillItem::min_width()` exists in `Via.cpp`. `BasicBoard::overlapping_objects` exists.
4. Extend `Trace::get_normal_contacts(Point, bool)` (currently in `Trace.cpp:63`) to also detect
   `DrillItem` and `ConductionArea` within tolerance `half_width + 1` (Java `Trace.java:172`).
5. **Verify**: `cmake --build build --target freerouting_board`; no new layout breaking. Tests: existing board
   shape-search tests still pass via ctest. If you add gtests, register them.

### WP-2 — NetIncompletes full port (PREREQUISITE for WP-3)

**Java refs**: `interactive/NetIncompletes.java` (~450 lines).

**C++ files**: `src/interactive/NetIncompletes.{h,cpp}` — **current `NetIncompletes.h` is a STUB and the .cpp
does not exist; replace both with a full port.**

**Key Java logic to replicate**:
- Ctor: `draw_marker_radius = p_board.rules.get_min_trace_half_width() * 2`; `net = board.rules.nets.get(net_no)`.
- `calculate_net_items()`: per-item filter — skip `is_tail()`; skip items with no normal contacts UNLESS
  `DrillItem`/`ConductionArea`; build per-item `get_connected_set(net_no)` (needs WP-1).
- Delaunay triangulation over net item corners via `PlanarDelaunayTriangulation` (already ported —
  `src/datastructures/PlanarDelaunayTriangulation.{h,cpp}`). C++ API:
  `class Storable { virtual std::vector<std::shared_ptr<Point>> get_triangulation_corners() const = 0; }`,
  `struct ResultEdge { shared_ptr<Point> start_point; shared_ptr<Storable> start_object; ... }`,
  ctor takes `vector<shared_ptr<Storable>>`, has `get_edge_lines()`, `validate()`.
- Sort edges by length; build AirLines joining disjoint connected sets; join connected sets greedily.
- `calc_length_violation()`: sum `Trace::get_length()` over `board.get_connectable_items(net_no)`
  (Java `Net.get_trace_length()`), compare to net-class min/max trace length (Java `NetClass`).
- Rewrite the `AirLine` struct to carry `net*`, `from_item`, `from_corner`, `to_item`, `to_corner`
  (FloatPoint corners). Existing header's flat `AirLine` is a placeholder — real port needs pointers.

**Verify**: `cmake --build build --target freerouting_interactive freerouting_board run_tests`;
add `interactive/NetIncompletes_test.cpp` with a tiny 2-net board (pins + one trace) → assert airline count/`calc_length_violation`;

### WP-3 — RatsNest full port (headless; PREREQUISITE for WP-4)

**Java refs**: `interactive/RatsNest.java`.

**C++ files**: `src/interactive/RatsNest.{h,cpp}` (new).

**Key Java logic**:
- Ctor `RatsNest(RoutingBoard&)`: build per-net `net_item_lists` indexed `net_no - 1` (Java nets are 1-based);
  iterate `p_board.get_item_count()/get_item(i)`, only `Connectable` (C++: `is_trace()||is_via()||is_pin()||DynamicCast<ConductionArea>` —
  verify `RatsNestItemType` mapping), skip `is_tail()` when required;
  compute `max_connections`; per-net `new NetIncompletes(net_no, list, board)`.
- `incomplete_count()`, `length_violation_count()`, `get_length_violation(net_no)`, `get_airlines()/get_airlines(net_no)`,
  `recalculate(net_no)`, `filter(ItemSelectionFilter)`, `set_hidden(net_no,bool)`, `get_net_number(airline)`.
- Uses `board::Nets::max_net_no()` and `board.nets.get(net_no)` (verify accessor names in `RoutingBoard`).

**Verify**: new `interactive/RatsNest_test.cpp`: build small board (2 disconnected pins + 1 trace net), assert
`incomplete_count() == 1` and airlines == 1; then connect and assert 0.

### WP-4 — BoardStatistics + 11 substructs

**Java refs**: `core/scoring/BoardStatistics.java` (~16.5K) + `BoardStatistics{Board,Bends,ClearanceViolations,Components,Connections,Items,Layers,Nets,Pads,Traces,Vias}.java`.

**C++ files**: `src/core/scoring/BoardStatistics.{h,cpp}` (new) + nested structs (single .h or one-per like Java).
`src/settings/RouterScoringSettings.h` is **already ported** (header-only). Also add `src/core/scoring/CMakeLists.txt`
creating `freerouting_scoring` (see layering note) and register in root CMake + main exe link list.

**Key Java logic**:
- 11 getters: `board()`, `pads()`, `vias()`, `traces()`, `connections()`, `items()`, `components()`, `nets()`,
  `layers()`, `clearanceViolations()`, `bends()`.
- Board-based ctor: `fileName`, `fileDateTime`, `unit`, `host`, `routerVersion` from `Communication`
  (C++ `RoutingBoard::communication()` — returns `Communication*`; use `get_parser_info().host_cad`, etc.).
  `host = TextManager.unescapeUnicode(...)` — `TextManager::unescape_unicode` already ported.
- byte[] ctor parsing SES-style stats — only port if cheap; primary target is board-based ctor + scoring.
- `calculateScore()` / `getMaximumScore()` / `getNormalizedScore()`:
  maxScore = `connections.count * (net.viaChest * viaCosts + defaultTraceCost * unroutedNetPenalty + clearanceViolationPenalty*bends + ...)`
  — READ Java exactly; wire via `RouterScoringSettings`.
- `toString()`: manual JSON matching Java `@SerializedName` (`"board"`, `"pads"`, `"vias"`, ...). Java uses
  GsonProvider; C++ has none — build a small recursive JSON emitter (bounded: numbers + nested objects + arrays of numbers/long).

**Verify**: `board_statistics_test.cpp`: build board via `insert_trace_without_cleaning` (auto-registers into
search tree + item list), build `RatsNest`, construct BoardStatistics, assert field values + score>0 + JSON contains keys.

### WP-5 — Core vertical slice tests + CMake + docs

- Register gtests for WP-1..4 + `events` (6 header classes: spot-check constructors/getters) + orphaned
  `src/core/{FileFormat,RoutingStage,StopRequestState}_test.cpp` (currently NOT registered).
- `ctest -N` count grows; run `run_tests` (EXCLUDE_FROM_ALL) + ctest; all green.
- Update `docs/migration/INDEX.md` (Core %, Interactive %, `ctest -N` count) + `MIGRATION_core.md`/`MIGRATION_interactive.md`.
- This WP unblocks full `RoutingJob` (`RoutingJob.cpp/h` exists; needs `BoardFileDetails`).

### WP-6 — RoutingJob / Session / BoardFileDetails full port

**Java refs**: `core/RoutingJob.java`, `core/Session.java`, `core/BoardFileDetails.java`, `core/BoardLibrary.java`, `core/Package*.java`, `core/StoppableThread.java`.
**C++ present (partial)**: `RoutingJob.{h,cpp}`, `Session.{h,cpp}`, `Package.{h,cpp}`, `FileFormat`, `RouterCounters`, `RouterJobResourceUsage`.

Wire the 6 events (`src/core/events/*.h` done) into `RoutingJob`. Use `TextManager` (`management`), logger.
**Verify**: `freerouting_core` builds; `core/RoutingJob_test.cpp` covering state transitions declare/log events.

### WP-7..WP-15 — Remaining modules (parallel, independent of core slice)

Each: port Java → C++, add to module CMake, register gtests, update `docs/migration/INDEX.md` percentages.

| WP | Module | Remaining (Java → C++) | Notes |
|----|--------|------------------------|-------|
| WP-7 | datastructures | `TimeLimit`, `UndoableObjects` (headers exist?), `Stoppable`, `BigIntAux` extras | many done; confirm `ArrayStack`, `Observers` green |
| WP-8 | management | `ReflectionUtil`, `RoutingJobScheduler`, `RoutingJobSchedulerActionThread`, `VersionChecker`, `TextManager` remainder | `TextManager` partially done |
| WP-9 | settings | `GlobalSettings` (save_as_json stub!), `FeatureFlagsSettings`, `RouterOptimizerSettings`, `SettingsMerger`, `StatisticsSettings`, `UserProfileSettings`, `UsageAndDiagnosticDataSettings`, `DesignRulesCheckerSettings` | |
| WP-10 | drc | `DesignRulesChecker`, `DrcReport`, `UnconnectedItems` (needs RatsNest) | depends on WP-1..3 |
| WP-11 | designforms/specctra | `SpecctraDsnStreamReader` (**top blocker**), `SpecctraSesFileWriter`, `P*` parser support classes, `SessionWriter` uses existing `Session*` | `Scanner.cpp`/`read_dsn` works; biggest impact (DSN→route→SES) |
| WP-12 | boardgraphics | all 8 (GUI) | low priority until GUI target exists |
| WP-13 | autoroute | `BatchOptimizer{,MultiThreaded}`, `BoardHistory` (entry ported), `CompleteExpansionRoom`, `FreeSpaceExpansionRoom`, `ObstacleExpansionRoom`, `MazeShoveTraceAlgo`, `Sorted*RoomNeighbours`, `RouterOptimizerSettings` dep | DrillPage/DrillPageArray/ExpansionDrill done (WP-9) |
| WP-14 | interactive | `HeadlessBoardManager`, `Route`, `ThreadActionListener` | depends on routing stack |
| WP-15 | board | `ForcedViaAlgo`, `MoveDrillItemAlgo`, `OptViaAlgo`, `PullTight*` remainder | headless blockers listed in `MIGRATION_board.md` |

---

## 3. Conventions & Gotchas (verified, do not rediscover)

- **`Components` duplicate**: canonical `board::Components` is in `src/board/Component.h` (method `component_count()`,
  NOT `count()`). `src/board/Components.h/.cpp` is DEAD code, not in CMake, never include it (redefinition error).
- **Java nets are 1-based** for RatsNest indexing (`get_net_no(i) - 1`).
- **`Item::get_normal_contacts()` default is a stub** returning `{}` (Item.cpp:42). Traces/ConductionArea override;
  DrillItem does not yet (WP-1 adds it). RatsNest correctness requires WP-1.
- **`RoutingBoard::add_item` only appends to `items_`**; search-tree registration happens via
  `BasicBoard::insert_trace_without_cleaning` → `search_tree_manager_->insert(item)`. For hand-built test boards,
  call `get_search_tree_manager()->insert(item)` (via `BasicBoard`, e.g. `board.search_tree_manager()`).
- **`RoutingBoard` API added** (don't re-add): `get_connectable_items(int net_no)`, `get_pins()`, `get_traces()`,
  `get_vias()`, `communication()`/`set_communication`, `components()`. `Communication.h` holds
  `get_unit()/set_unit()`, `get_resolution()/set_resolution()`, `get_parser_info()` (`SpecctraParserInfo{string_quote,host_cad,host_version,dsn_file_generated_by_host}`),
  `host_cad_is_eagle()`, `host_is_old_kicad()`. `BoardOutline::clearance_violation_count()`,
  `BoardRules::default_clearance_class()` (static, →1) exist.
- **`DrillItem` canonical = `Via.h`** (`DrillItem` base, `Via`, and `Pin : DrillItem` with `is_pin()/get_pin_no()`).
- **`Trace::get_length()` is pure virtual**; `PolylineTrace` overrides. `Trace::get_half_width()` exists.
- **`PlanarDelaunayTriangulation`**: turn `Item*`s into `shared_ptr<Storable>` adapters returning the item's
  `get_ratsnest_corners()`; `ResultEdge.start_object/end_object` are `shared_ptr<Storable>`.
- **JSON**: hand-rolled; match Java `@SerializedName` exactly so output is diffable.
- **`CTest` subset run**: `ctest --test-dir build -R <regex> --output-on-failure`.
- **Class map (T006)**: `JavaToCppClassMap.csv` has 7 columns (`java_package,java_class,java_file,cpp_class,cpp_header,status,reason`);
  `status` ∈ `ported / header_only / partial / pending / out_of_scope`. `out_of_scope` is defined by the explicit
  `OUT_OF_SCOPE` table in `tools/class_map_check.py` (dir prefixes `gui/`, `api/mcp/`, `api/`, `analytics/`, `io/kicad/`,
  `util/gson/` plus exact files including `io/specctra/parser/SessionToFusion.java`, `io/KiCadNetClassNames.java`,
  `util/VersionChecker.java`, `util/SafeObjectInputStream.java` and 11 GUI/API-only settings classes), each with a one-word
  `reason`. Everything headless but not yet ported is `pending` (goal: `pending = 0`). Run `python3 tools/class_map_check.py`
  (checks CSV↔disk coverage, header/class existence, reasons; exits non-zero only on structural errors) and
  `python3 tools/test_class_map_check.py`. The legacy `specctra/Library|Network|Wiring|Placement` files are NOT Java ports —
  the real parser classes live in `src/designforms/specctra/parser/`.

## 4. Definition of Done (per WP)

1. Port complete & faithful (Java camelCase → C++ naming consistent with repo).
2. `cmake --build build --target <module>` green; `-Werror`-level warnings fixed (repo uses `-Wreorder` etc.).
3. gtest added + registered in `tests/CMakeLists.txt`, passes with `ctest --test-dir build -R <test> --output-on-failure`.
4. `run_tests` (assert suite) still passes, full ctest green (no regressions).
5. `docs/migration/INDEX.md` percentages updated + `MIGRATION_<module>.md` notes.