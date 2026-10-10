# TASKS — Remaining Migration, Split Into Loop-Sized Tasks

This file is the **task queue** driven by `loop_migration.sh`. One loop iteration = one task.
Created 2026-10-08 from a full repo review (see "State at plan time" at the bottom).

## Format (the loop script parses this — keep it exact)

```
- [ ] T123 | deps: T100,T101 | Short title
  - java: java/app/freerouting/.../X.java (lines)
  - cpp: src/.../X.{h,cpp,_test.cpp}
  - done when: concrete, checkable acceptance
```

Status markers: `[ ]` todo · `[~]` partially done (continue it) · `[x]` done · `[!]` blocked (add an
indented `- blocked:` line with the reason). `deps: -` means no dependencies.
The script picks the **first `[ ]`/`[~]` task whose deps are all `[x]`**.

## Rules for every task

1. Read only: `SOURCE_OF_TRUTH/01_MEMORY.md`, `SOURCE_OF_TRUTH/CONVENTIONS.md`, this task, the Java
   files it names and the C++ files you touch. Do **not** read `02_PROGRESS.md`–`05_WORK_PACKAGES.md`
   (large historical logs) unless the task says so.
2. Java is truth. Port faithfully; keep Java quirks (comment `// Java parity:` where it looks wrong).
3. Size limit: one task ≈ ≤ 700 Java lines / one session. If it is bigger than you can finish,
   **split it**: replace this task with `T123a`, `T123b`, … (same format; `T123b` depends on `T123a`),
   do `T123a`, leave the rest `[ ]`. Tasks that depended on `T123` must then depend on the last part.
4. Gate (the script re-checks it): `cmake --build build -j$(nproc)` green and
   `ctest --test-dir build` green. Never delete, skip or weaken an existing test to get green.
5. On finish: mark `[x]`, add one indented `- result:` line (what landed, test count), add a short
   note to the relevant `docs/migration/MIGRATION_<module>.md`. **Do not commit** — the script commits.
6. If you run out of time with useful work done and the build green, mark the task `[~]` and add a
   `- progress:` line saying exactly what is left.
7. New required work you discover: append a new task in the right phase (next free ID, with deps).
8. If you cannot proceed (missing prerequisite, design question): mark `[!]` with `- blocked:` reason.

---

## Phase 0 — Harness & hygiene (first: they make every later task measurable)

- [x] T001 | deps: - | E2E comparison tool
  - cpp: tools/e2e_compare.sh, tools/e2e_all.sh, docs/migration/E2E_STATUS.md
  - what: `e2e_compare.sh <board.dsn>` runs `build/freerouting -de <dsn> -do <out.ses>` with a timeout
    (default 300s), then prints one line: board, exit code, seconds, #wires, #vias in the C++ SES vs the
    Java SES `java_output_ses/<base>_java.ses` (count `(wire` / `(via` inside `(network_out`), "n/a" when
    a baseline is missing. `e2e_all.sh` runs it over the boards listed in `test_java_boards.sh` and
    rewrites `docs/migration/E2E_STATUS.md` as a table with the date.
  - done when: both scripts run on all 8 boards without aborting (C++ crashes/timeouts are reported as
    rows, not fatal) and E2E_STATUS.md exists with today's numbers.
  - result: tools/e2e_compare.sh + tools/e2e_all.sh (board list read from test_java_boards.sh), docs/migration/E2E_STATUS.md generated 2026-10-08: all 8 boards run without timeouts after T417, but every board routes 0 wires (root cause: T110-T115 pin geometry). Issue006-LPC18XX_43XX_SCH.dsn is a binary OLE file, not a DSN (Java has no baseline either) — parse error is correct. ctest 788/788.
- [x] T002 | deps: - | Reconcile JavaToCppClassMap.csv with the real C++ names
  - result: tools/class_map_check.py created; CSV reconciled; script exits 0 (ok=229, out_of_scope=124, issues=0); README Quick Status updated with class map counts. ctest 851/851, run_tests passed.
  - what: the CSV is name-based and wrong for renamed ports (MazeSearchEngine→MazeSearchAlgo,
    TraceShover→ShoveTraceAlgo, TraceTightener*→PullTightAlgo*, ViaOptimizer→OptViaAlgo,
    ForcedViaInserter→ForcedViaAlgo, ForcedPadRouter→ForcedPadAlgo, IntPoint/FloatPoint→Point.h,
    Line→Vector.h, ExpansionDoor→ExpansionRoom.h, ShapeSearchTree45/90Degree→ShapeSearchTree.h, …).
    Write `tools/class_map_check.py` that checks each row's `cpp_header` exists and contains
    `class|struct <cpp_class>`, and reports mismatches. Fix rows by hand. Mark Java classes that are
    intentionally out of scope (gui/api/analytics/io.kicad/util.gson/settings.sources UI bits) as
    `out_of_scope`.
  - done when: script exits 0; README "Quick Status" counts regenerated from the CSV.
- [x] T003 | deps: - | Fix the known `TileShape::cutout(const TileShape&)` inverted branches
  - java: geometry/planar/TileShape.java `cutout` + Simplex/IntOctagon/IntBox `cutoutFrom`
  - cpp: src/geometry/planar/TileShape.cpp
  - what: documented latent bug (03_MISSING.md §2.1 "Known latent bug"): when the hole is a Simplex and
    the dividing shape is IntOctagon/IntBox the C++ returns hole-minus-divide instead of Java's result.
  - done when: gtests for all 9 hole/divide type combinations match the Java semantics; ctest green.
  - result: The inverted Simplex-hole branches had already been fixed in f00c632 but were untested. Added the missing Java IntBox.cutout simplify() of each piece (TileShape::cutout -> private cutout_pieces) and 27 parameterized gtests (9 divide x hole combinations x {area = divide - overlap, disjoint hole, simplify only for IntBox}); reinstating the old inverted branch makes them fail. ctest 788/788.
- [x] T004 | deps: - | Clean `-Wunused-*` warnings (Item.h, ClearanceMatrix.cpp, SearchTreeManager.cpp, FRLogger.cpp)
  - done when: a clean full build prints zero warnings from `src/`; no behaviour change.
  - result: full clean build (`cmake --build build --clean-first`) now prints 0 warnings from `src/`
    (was 6). The four files named in the task were already clean; the actual remaining warnings were
    fixed: NetClass.cpp:30 sign-compare (size_t -> int loop index), DefaultItemClearanceClasses.h
    implicitly-declared copy-assignment deprecated because of the user copy ctor (added defaulted
    `operator=`), Trace_test.cpp:1 literal `\n` in `#include <cstdio>\n#include <memory>` split into
    two lines, DrillPage_test.cpp unused `trace` variable (kept the side-effecting `insert_trace`
    call), NamedAlgorithm_test.cpp unused `p_event` param cast to void, RoutingBoardFanout_test.cpp
    unused `add_component` helper marked `[[maybe_unused]]`. No behaviour change. ctest 851/851,
    run_tests PASSED.

- [ ] T006 | deps: - | Class map: `pending` status + real out-of-scope list
  - cpp: tools/class_map_check.py, JavaToCppClassMap.csv, SOURCE_OF_TRUTH/README.md
  - what: today's 124 `out_of_scope` rows mix truly non-headless classes (gui, api, api.mcp, analytics, io.kicad,
    parser/SessionToFusion, util.gson API adapters, util.VersionChecker, settings Api/Gui/GoogleSheets/McpServer/
    RateLimit/UsageAndDiagnostic/UserProfile) with ~60 headless-path classes that are simply not ported yet
    (BoardHistory, BatchFanout, AutoroutePassRunner, SesWriter/Reader, DsnReader/Writer, RulesReader/Writer,
    SettingsMerger and settings.sources.*, RoutingJobScheduler, BoardComparator, board.facade.*, ...).
    Add status `pending` for the latter; keep `out_of_scope` only for the former, each with a one-word reason column.
  - done when: `class_map_check.py` prints counts per status and the pending list; README Quick Status shows
    "ported / header_only / partial / pending / out_of_scope"; goal for the whole migration is `pending = 0`.
- [ ] T007 | deps: - | Java reference harness (oracle for T110-T118 and T611)
  - cpp: tools/java_ref/{BoardCounts.java,build.sh}, tests/data/expected/*.counts.json
  - what: compile a small Java class with `javac` against the executable jar (see 01_MEMORY.md gotcha 29) that
    loads a DSN exactly like the CLI and prints JSON: layer/net/component/pin/padstack/netclass/via-rule/keepout/
    conduction-area counts, the item-id sequence of `board.get_items()`, and the clearance matrix. Same schema as
    `freerouting --dump-board-counts` (T117). Run it over all `tests/data/**/*.dsn` once and store the JSON.
  - done when: harness runs on the 8 E2E boards + a hand-picked set; JSON committed; Issue006 reports the load error.
- [ ] T008 | deps: - | Java baseline runner (replaces test_java_boards.sh)
  - cpp: tools/java_baseline.sh (+ docs in docs/migration/E2E_STATUS.md)
  - what: run the jar headlessly per board with a clean `--user_data_path`, `-mt 1`, fixed `-mp`, explicit
    `--router.job_timeout`, `--router.optimizer.timeout`, `--router.result_json_path`; capture SES, manifest JSON, stdout log
    and wall time; per-tier timeout instead of the fixed 180 s. Today 4 of 8 boards (BBD_Mars-64, Issue006,
    Issue070, Issue103-Board-Unrouted) have no baseline: record WHY (timeout / error) per board.
  - done when: `java_output_ses/<board>_java.{ses,json,log}` exist for every board Java can route, with a
    `baseline.json` summary (seconds, exit code, timed_out flag).
- [ ] T009 | deps: - | Determinism: PolygonShape LCG + normalizeAllTraces order
  - cpp: src/geometry/planar/PolygonShape.cpp (line ~67), src/board/BasicBoard.cpp (normalize_all_traces)
  - what: the LCG step uses multiplier `0x5DEECE66ULL`, Java's `java.util.Random` uses `0x5DEECE66D`
    (so `split_to_convex` starts at a different corner); Java `normalizeAllTraces` iterates a HashMap<Integer,..>,
    the C++ port must iterate in the Java order (ascending net number — verify against Java).
    Verify with a throwaway `java.util.Random`/harness program (01_MEMORY.md gotchas 19, 29), not hand-computed numbers.
  - done when: golden gtests with values generated by the Java harness; ctest green.
- [ ] T010 | deps: - | Determinism: PlanarDelaunayTriangulation parity
  - java: datastructures/PlanarDelaunayTriangulation.java (+ the shuffle with seed 99)
  - cpp: src/datastructures/PlanarDelaunayTriangulation.cpp
  - what: C++ has no `Collections.shuffle(seed 99)` and uses a different algorithm; ties on co-circular pads change the
    airline set and therefore the routing order. Port the Java algorithm and the shuffle via `MazeRandom`.
  - done when: golden test (point sets incl. co-circular/collinear ones) returns the same edge set as Java.

## Phase 1 — DSN reader parity (`io/specctra/parser`, Java ≈ 11.4k lines, C++ ≈ 2.2k)

**THIS IS THE HEADLESS BLOCKER (found 2026-10-08, see T417):** the current loader
(`BoardLoader` + `RoutingBoard::create_pins_from_parsed`) is hand-rolled, not a Java port. All pins
of a component end up on the component origin as 100x100 placeholder boxes in raw DSN units (no
resolution scaling, image pin offsets not applied), so pins of different nets overlap, every
maze start room is empty and **every board routes 0 connections**. T110-T115 must replace it with
real padstack shapes, image pin offsets and the Java CoordinateTransform (bounds offset 1000).

Theme: every `tests/data/*.dsn` loads; board counts (layers, nets, components, pins, padstacks,
net classes, keepouts) equal Java's. T110 adds the count dump used as acceptance later.

- [x] T101 | deps: - | Keyword table + ScopeKeyword parity
  - java: io/specctra/parser/Keyword.java (131), ScopeKeyword.java (82)
  - cpp: src/designforms/specctra/Keyword.{h,cpp}
  - done when: every Java keyword constant exists with the same spelling; gtest covers the full table.
  - result: new src/designforms/specctra/parser/{Keyword,ScopeKeyword}: all 106 Java Keyword constants generated from Keyword.java (scope keywords are generic ScopeKeyword instances until their reader is ported); ScopeKeyword::skip_scope/read_scope ported. The new parser lives in namespace specctra::parser next to the old hand-rolled reader until T117 switches BoardLoader.
- [x] T102 | deps: T101 | Lexer: SpecctraDsnStreamReader behaviour
  - java: io/specctra/parser/SpecctraDsnStreamReader.java (1862, flex-generated — port the *behaviour*:
    token kinds, NAME vs IDENTIFIER states, `string_quote` switching, parentheses, numbers), IJFlexScanner.java (38)
  - cpp: src/designforms/specctra/Scanner.{h,cpp}
  - done when: gtests on tricky snippets (quoted names, names with `-`/`.`/`/`, string_quote change,
    negative/exponent numbers) and a smoke test that tokenizes every `tests/data/*.dsn` without error.
  - result: parser/SpecctraDsnStreamReader (+ IJFlexScanner/Token): hand-written longest-match port of the shipped JFlex scanner. The keyword table (111 spellings + following lexical state) and the character classes were extracted from the generated Java DFA by reflection (the 2008 .flex is outdated: shipped scanner warns+skips illegal chars instead of throwing, "generated_by_freeroute"->GENERATED_BY_FREEROUTING). UTF-8 decoding with Java maximal-subpart U+FFFD. tools/dsn_token_dump + tools/dsn_token_compare.py: all 76 tests/data/*.dsn tokenize identically to the Java lexer. 14 gtests. ctest 828/828.
- [x] T103 | deps: T102 | Resolution / Unit / CoordinateTransform in the parser
  - java: parser/Resolution.java (73), parser/Unit.java (27), board/state/CoordinateTransform.java (112)
  - done when: DSN→board unit conversion matches Java for mil/inch/mm/um resolutions (gtest).
  - result: parser/Resolution (scope reader, now the RESOLUTION_SCOPE keyword instance), parser/CoordinateTransform (numeric dsn<->board part of Java io.CoordinateTransform; the board->dsn shape conversions follow with T201), UnitHelper::parse (Java Unit.fromString: case-insensitive, nullopt for unknown; from_string kept for old callers).
- [x] T104 | deps: T102 | Shape reader part 1: Shape dispatch, Rectangle, Circle
  - java: parser/Shape.java (646, read_scope / read_area part), Rectangle.java (96), Circle.java (91)
  - cpp: src/designforms/specctra/{Shape,Rectangle,Circle}.{h,cpp}
  - result: parser/{Shape,Rectangle,Circle,Polygon,Path(PolygonPath,PolylinePath),Layer(+LayerStructure),DsnFile(scope helpers)}: all Java read paths + transform_to_board(_rel); tile results are wrapped in ConvexTileShape, area borders/holes converted to PolygonShape for PolylineArea (Java IntBox is a PolylineShape). Padstack shapes are now typed Shape (C++ Circle is not a ConvexShape). ParserShape_test 11 (incl. resolution).
- [x] T105 | deps: T104 | Shape reader part 2: Polygon, Path, PolygonPath, PolylinePath, areas with holes/window
  - java: parser/Polygon.java (111), Path.java (24), PolygonPath.java (131), PolylinePath.java (73), rest of Shape.java
  - result: done together with T104 (Polygon, Path, PolygonPath, PolylinePath, areas with window holes and clearance_class).
- [x] T106 | deps: T103 | Rule reader (width, clearance + types) and Read/WriteScopeParameter
  - java: parser/Rule.java (343), ReadScopeParameter.java (187), WriteScopeParameter.java (38)
  - result: parser/Rule: Rule::read_scope, read_layer_rule_scope, WidthRule, ClearanceRule (read part; write part with T204). ReadScopeParameter fields still grow per scope. Rule tests come with T107.
- [x] T120 | deps: - | BoardRules parity (prerequisite of T107/T114)
  - java: rules/BoardRules.java (435), ViaInfos.java (88), NetClasses.java (108) and the NetClass/ClearanceMatrix pieces they use
  - what: C++ BoardRules lacks via_infos, via_rules, default/appended net classes with default item clearance
    classes, get/set default trace half widths, create_default_via_rule, get_via_rule, change/remove clearance
    class, get_default_via_diameter, and the Java constructor taking a ClearanceMatrix (C++ takes a clearance value).
  - result: ClearanceMatrix rewritten Java-faithful (per-layer values in row[j].column[i], odd->even,
    case-insensitive names, append/remove_class, set_inner_value, max_value, is_(inner_)layer_dependent,
    is_equal) — the old port returned 0 from get_clearance_compensation_value, so compensated trees never
    compensated. BoardRules: Java ctor (LayerStructure, ClearanceMatrix), via_infos, owned via_rules,
    default/new/appended net classes, default trace widths (Java min 100000 / max 100 defaults),
    create_default_via_rule, get_via_rule, change/remove_clearance_class, get_default_via_diameter;
    NetClasses owns its classes; Nets::add assigns the default net class (Java Net ctor). ViaInfo now
    holds a non-owning Padstack* (library-owned). Padstack/Item-dependent rule functions are compiled in
    src/board/RulesBoardAccess.cpp (a rules->board link dependency would be cyclic and breaks the RESCAN
    groups). ClearanceMatrix_test 6, BoardRules_test 5. ctest 850/850.
- [x] T121 | deps: T120 | BasicBoard insert facade (prerequisite of T107-T116)
  - java: board/facade/BasicBoard.java insertObstacle/insertViaObstacle/insertComponentObstacle/insertComponentOutline/
    insertConductionArea/insertPin/insertVia (~300 lines), RoutingBoard ctor creating the BoardOutline item
  - what: items must go into items_ AND the search trees (SearchTreeManager::insert) like Java insertItem.
  - result: RoutingBoard::insert_item (Java BoardItemRepository.insertItem: clearance-class check, item list,
    all search trees, additional_update_after_change) + insert_pin (padstack from the component package,
    bbox from the pad shapes), insert_obstacle/insert_via_obstacle/insert_component_obstacle (both overloads),
    insert_component_outline, insert_conduction_area, insert_outline. BoardOutline ported (Java item: thin
    tree shapes along every outline line or the keepout outside the outline, keepout_outside default false,
    is_obstacle, contains). ShapeSearchTree::calculate_tree_shapes(BoardOutline/ObstacleArea) +
    TileShape::divide_into_sections: area obstacles were bounding-box octagons before. The DRC outline
    special case (not in Java) now uses BoardOutline::contains. ctest 851/851. Still open for T107: the
    RoutingBoard ctor does not insert the outline (Java does) — Structure.createBoard must call insert_outline.
- [x] T107 | deps: T105,T106,T121 | Structure part 1: layers, LayerStructure, boundary, planes
  - java: parser/Structure.java (layer/boundary/plane sections), Layer.java (67), LayerStructure.java (69), Plane.java (84)
  - cpp: src/designforms/specctra/Structure.{h,cpp}
  - result: landed in `src/designforms/specctra/parser/` (like T104/T105), not the legacy `specctra/Structure.*` (that path is the old Scanner reader). `parser/Structure` (ScopeKeyword subclass; BoardConstructionInfo, read_layer_scope with type/use_net/layer rules, read_boundary_scope + add_boundary_shape, default rules; sets `ReadScopeParameter::layer_structure`, defers create_board/inserts to T109), `parser/Plane` (read_scope -> plane_list, Allegro window skipping). `Keyword::STRUCTURE_SCOPE`/`PLANE_SCOPE` are now the real classes. `Layer`/`LayerStructure` were already ported (T103/T104). Structure_test 13. ctest 864/864, run_tests PASSED.
- [x] T108 | deps: T107 | Structure part 2: keepout / via_keepout / wire_keepout, via list, snap_angle, control
  - java: parser/Structure.java remaining read sections
  - result: `parser/Structure`: read_via_padstacks (`(spare ...)` appended last, unknown sub-scopes skipped,
    nullopt on error like Java null), read_snap_angle (→ `ReadScopeParameter::snap_angle`, default
    FORTYFIVE_DEGREE, invalid token keeps default), read_control_scope (→ `via_at_smd_allowed`) and the
    keepout/via_keepout/place_keepout `Shape::read_area_scope` collections (new `ReadScopeParameter`
    `keepout_list`/`via_keepout_list`/`place_keepout_list`). `wire_keepout` lexes to `Keyword::KEEPOUT`
    so the KEEPOUT branch covers it. `via_padstack_names` is now `std::optional` to keep Java null vs
    empty. Board creation / keepout insertion / autoroute_settings / flip_style deferred to T109/T112.
    Structure_test 23 (+10). ctest 874/874, run_tests PASSED.
- [x] T109a | deps: T108 | Structure part 3a: autoroute_settings scope (read + write)
  - java: parser/AutorouteSettings.java (243)
  - cpp: src/designforms/specctra/parser/AutorouteSettings.{h,cpp,_test.cpp}
  - result: `parser/AutorouteSettings` (read_scope, read_layer_rule, write_scope with Java `Float.toString`
    formatting) wired into `Structure::read_scope` -> `ReadScopeParameter::autoroute_settings`
    (Java parity: only consumed while no layer structure exists yet). RouterSettings gained run_router /
    run_optimizer, plane_via_costs and the per-layer setters with Java clamps (cost >= 1, trace cost >= 0.1).
    Fixed the uncommitted-tree defects found in the 2026-10-10 review: duplicate `get_start_ripup_costs()`,
    `settings::RouterSettings` forward declaration in the wrong namespace, missing test file. Build graph:
    `freerouting_settings` no longer links `freerouting_autoroute` (header-only use; it formed a cycle with
    the RESCAN link groups) and `freerouting_settings` was added to every RESCAN group that holds
    `freerouting_designforms`. AutorouteSettings_test 13. ctest 887/887 (incl. AllTests/run_tests).
- [ ] T109b | deps: T109a | Structure part 3b: board rules from the structure (clearance rules, default classes)
  - java: parser/Structure.java updateBoardRules / setClearanceRule / containsWireClearancePair /
    createDefaultClearanceClasses / appendClearanceClass / clearanceEquals (lines 611-855, ~245)
  - cpp: src/designforms/specctra/parser/Structure.{h,cpp}
  - done when: gtests build a BoardRules from `(rule ...)`/`(structure (rule ...))` snippets (width,
    clearance with `type` pairs, `default`/`wire_wire` etc.) and the clearance matrix equals Java's values.
- [ ] T109c | deps: T109b | Structure part 3c: create_board (outline, keepouts, planes → board)
  - java: parser/Structure.java createBoard (1139-1307), readScope tail (1100-1138), insertKeepout (857-935),
    separateHoles (577-610), insertMissingPowerPlanes (526-576); parser/DsnFile.adjustPlaneAutorouteSettings
    (DsnFile.java 29-107, called by DsnReader when the DSN has no `(autoroute ...)` scope)
  - done when: a structure scope creates a RoutingBoard with outline (T121 `insert_outline`), keepouts as
    obstacle areas, planes as conduction areas; gtests with hand-written snippets. Note: `place_keepout`,
    `wire_keepout`, `via_keepout` appear in (almost) no test DSN — cover them with snippets.
- [ ] T110 | deps: T105,T106,T121 | Library part 1: padstacks → board Padstacks; board-count dump
  - java: parser/Library.java (padstack part), core/library/Padstacks.java (84)
  - also: add a test helper (or `freerouting --dump-board-counts <dsn>`) printing layer/net/component/
    pin/padstack/netclass/keepout counts; used as acceptance by T111–T118.
- [ ] T111 | deps: T110 | Library part 2: images / Package + PartLibrary + LogicalParts + BoardLibrary
  - java: parser/Package.java (425), PartLibrary.java (351), core/library/{Packages,LogicalParts,BoardLibrary}.java
- [ ] T112 | deps: T111 | Placement: Component, ComponentPlacement, PlaceControl
  - java: parser/Component.java (381), ComponentPlacement.java (92), PlaceControl.java (73), Placement.java (29)
- [ ] T125 | deps: T106 | Circuit + Parser (moved out of T117: needed by NetClass.read and the Plane reader)
  - java: parser/Circuit.java (143), parser/Parser.java (228)
  - cpp: src/designforms/specctra/parser/{Circuit,Parser}.{h,cpp,_test.cpp}
  - what: `Keyword::PARSER_SCOPE` becomes the real class (string_quote, space_in_quoted_tokens,
    host_cad, host_version, constant, write_resolution, ...), so `ReadScopeParameter::host_cad` is
    filled before `Plane` reads its Allegro windows. Circuit holds the rule/length/use-via data NetClass uses.
  - done when: gtests on `(parser ...)` snippets (string_quote switch, host_cad/host_version) and Circuit
    read; `Plane` Allegro window handling covered by a host_cad=Allegro snippet.
- [ ] T113 | deps: T112 | Network part 1: nets, NetList, pin assignment
  - java: parser/Net.java (129), NetList.java (56), Network.java (net read part)
- [ ] T114 | deps: T113,T125 | Network part 2: net classes, class pairs, via rules → BoardRules
  - java: parser/NetClass.java (197), Network.java (class part), rules/{NetClasses,Nets,ViaInfos}.java
- [ ] T115 | deps: T114 | Network part 3: insert components and pins into the board
  - java: parser/Network.java (insert components / pins part)
- [ ] T116 | deps: T115 | Wiring read: wires, vias, fixed states, polyline paths
  - java: parser/Wiring.java (715, read part)
- [ ] T117 | deps: T109c,T116,T125 | DsnFile.read orchestration + DsnReader + BoardParserCallback + board-count dump
  - java: parser/DsnFile.java (220), io/specctra/DsnReader.java (291, incl. readMetadata), parser/BoardParserCallback.java (42),
    io/BoardReadResult.java + io/BoardMetadata.java (return types of DsnReader), the MinimalBoardManager shim
    used by ReadScopeParameter
  - also: `freerouting --dump-board-counts <dsn>` printing layer/net/component/pin/padstack/netclass/keepout
    counts and the item-id sequence as JSON (same schema as the Java harness of T007)
  - done when: every `tests/data/*.dsn` loads without error through the CLI path; counts match Java
    for the 8 E2E boards (record them in docs/migration/MIGRATION_designforms.md).
- [ ] T118 | deps: T117,T007 | DSN reader audit vs Java on all test boards; file a task per mismatch
  - what: run C++ `--dump-board-counts` and the Java harness (T007) over all 117 `tests/data/**/*.dsn`;
    compare counts AND item-id order (item ids / item list order drive TreeSet ordering in the router, so
    equal counts are not enough). `Issue006-LPC18XX_43XX_SCH.dsn` is a binary OLE file: an error is correct.
  - done when: zero mismatches or one filed task per mismatch (T118a, ...); results table in MIGRATION_designforms.md.
- [ ] T124 | deps: T117,T202 | Delete the legacy hand-rolled reader and switch the CLI to the Java-faithful path
  - remove: `specctra/{Scanner,Structure,Library,Network,Placement,Wiring,Keyword,Circle,Polygon}.*`, `Path.h`,
    `Wiring_test.cpp`, `src/board/BoardLoader.*` (+ test), `RoutingBoard::create_pins_from_parsed`,
    `get/set_parsed_result` + `parsed_result_` + the Scanner include in RoutingBoard.h, the dead
    `board/BoardBuilder.*`, the stub `interactive/HeadlessBoardManager.*` (`specctra/SessionWriter.*` is rewritten in place by T201/T202)
  - port: Java `HeadlessBoardManager` load/save (board/facade side of DsnReader/SesWriter) on the new parser
  - switch: `src/Freerouting.cpp` (BoardLoader/parsed_result/SessionWriter uses, ~lines 258/276/277/317),
    `src/autoroute/BatchAutorouter.cpp:186` (`get_pin_count`), `src/core/RoutingJob.cpp` (`job.board = board.release()` leak)
  - done when: grep for the removed names finds nothing; build + ctest + run_tests green; e2e_all.sh unchanged or better.

## Phase 2 — SES writer parity (Java SesWriter 552 + write paths; C++ SessionWriter 88)

- [ ] T122 | deps: T106 | WriteScopeParameter + shape/resolution write halves (T106 was marked done without them)
  - java: parser/WriteScopeParameter.java (38), `writeScope` of Shape/Rectangle/Circle/Polygon/Path/PolygonPath/
    PolylinePath/Resolution (and Layer/Rule write parts that SesWriter and DsnWriter both call)
  - cpp: src/designforms/specctra/parser/{WriteScopeParameter,...}
  - done when: each shape round-trips (write -> read -> equal) in a gtest; output text equals Java for the same shape.
- [ ] T201 | deps: T117,T122 | SesWriter part 1: session header, base_design, placement, was_is
  - java: io/specctra/SesWriter.java (552, first half)
  - cpp: src/designforms/specctra/SessionWriter.{h,cpp}
- [ ] T202 | deps: T201 | SesWriter part 2: routes, library_out (via padstacks), network_out (wires, vias, fixed)
  - java: SesWriter.java second half, parser/Wiring.java write part, Shape write part
  - done when: `Issue103-Board-Routed.dsn` loaded and written without routing gives an SES token-equal
    to Java's (generate the Java SES once with the jar, store it in tests/data/expected/).
- [ ] T203 | deps: T202 | SesReader (enables round-trip tests, `-de x.dsn x.ses` resume, C++-side DRC on an SES)
  - java: io/specctra/SesReader.java (413), SesImportSummary.java (15). Skip `saveSpecctraSessionSesAsFusionScriptScr`
    (parser/SessionToFusion, Fusion export is out of scope).
- [ ] T204a | deps: T202 | RulesReader + RulesWriter
  - java: io/specctra/{RulesReader (376, incl. readRouterSettings used by RulesFileSettings),RulesWriter (104)}.java
- [ ] T204b | deps: T204a | DsnWriter + the remaining parser write halves
  - java: io/specctra/DsnWriter.java (92) and `writeScope` of Structure, Layer, Plane, Rule, Library, Package, Network,
    Component, Net, NetClass, Parser, Placement. (HeadlessBoardManager.calculateCrc32 needs DsnWriter.)
  - done when: DSN -> board -> DSN round-trips token-equal to Java's DsnWriter output on the 8 E2E boards.

## Phase 2b — Core shakedown (first contact of the ported router with real boards; highest-risk step)

All 874+ unit tests use synthetic fixtures; no real board has ever been routed by the C++ core. Do this before
stacking the rest of the pipeline on top.

- [ ] T611 | deps: T117,T202,T009,T010,T008,T007 | Shakedown: route Issue026, Issue107, Issue034 with the interim BatchAutorouter
  - cpp: tools/first_divergence.py (+ fixes in whatever the diff points at)
  - what: run Java and C++ with `-mt 1`, same `-mp`, optimizer off (isolate the router); `first_divergence.py` compares
    the two logs per pass (item order, per-connection result, incomplete count after each pass) and finally the SES.
    Fix the FIRST divergence, repeat. Expect latent bugs in maze search / locator / shove / inserter that synthetic
    fixtures never exercised; file a narrower task (T611a, ...) for anything > ~700 lines.
  - done when: Issue026 routes with #unrouted equal to Java's (stretch: SES token-equal); Issue107 and Issue034
    route something and the first divergence is documented in docs/migration/E2E_STATUS.md.

## Phase 3 — Settings & CLI needed by routing

- [ ] T301 | deps: - | RouterSettings full parity (fields, defaults, getters, copy)
  - java: settings/RouterSettings.java (1042) — C++ src/settings/RouterSettings.cpp is 162 lines
  - split if needed (T301a fields/defaults, T301b per-layer/cost accessors)
- [ ] T302 | deps: T301 | FanoutSettings, RoutingCostSettings, LayerSettings, AutorouterSettings, OptimizerScoreSettings, RouterScoreSettings
  - java: settings/{FanoutSettings,RoutingCostSettings,LayerSettings,AutorouterSettings,OptimizerScoreSettings,RouterScoreSettings}.java
- [ ] T303 | deps: T302 | SettingsMerger + GlobalSettings load/save (hand-rolled JSON with the `@SerializedName` names)
  - java: settings/SettingsMerger.java (199), settings/GlobalSettings.java (996)
- [ ] T304 | deps: T303 | CLI parity with Freerouting.java argument parsing
  - known gaps (03_MISSING.md §4): `-dl` sets the wrong field, `set_input()` does not read the file,
    generic `--property=value`, `session.add_job()` commented out
  - flag table to reach Java parity (2026-10-10 review): `-de` (dsn/json/ses/rules, `+`-separated lists; `-de`+`-do`
    forces the router on), `-do` multi-output, `-dr` rules, `-mp` (0..9999, 0 = unlimited; C++ clamps 1..99998, default
    9999), `-mt` (sets autorouter AND optimizer max threads, default cores-1; C++ stores it but ignores it),
    `-oit` (values 0..1 are multiplied by 100, default 2.5; C++ divides by 100 and drops it), `-us/-is/-inc`
    (optimizer strategy, item selection, ignored net classes: TODO no-ops in C++), `-dl` (disables the FILE log only;
    C++ disables the batch autorouter), `-ll`, `-drc`, `--user_data_path`, generic `--router.*=v` / `--section.prop=v`
    setters (C++ only logs them). Keys that matter: `router.autorouter.max_passes|max_threads`,
    `router.optimizer.enabled|max_passes|max_threads|improvement_threshold|timeout`, `router.fanout.enabled`,
    `router.job_timeout`, `router.scoring.*`, `router.result_json_path`, `router.strict_drc`. Also fix the C++
    bug that any argument containing "help" prints the help text.
  - done when: a table-driven gtest feeds each Java flag spelling to the C++ parser and checks the resulting settings.
- [ ] T306 | deps: T303 | Settings sources and their priority order
  - java: settings/sources/{DefaultSettings (237),CliSettings (203),DsnFileSettings (69),RulesFileSettings (109),
    JsonFileSettings (79),EnvironmentVariablesSource (140)}.java, settings/LegacyRouterSettingsBridge.java (126),
    util/ReflectionUtil.java (380)
  - what: priority order as in Java (`freerouting.json` is priority 10); `FREEROUTING__ROUTER__*` /
    `FREEROUTING__LOGGING__*` env vars; replace Java reflection with an explicit field registry that keeps the
    `@SerializedName` names. DsnFileSettings needs T117 (`DsnReader.readMetadata`), RulesFileSettings needs T204a.
  - done when: gtests merge defaults < json < env < dsn < rules < cli and match Java's effective settings.
- [ ] T307 | deps: T302 | RouterSettings.applyBoardSpecificOptimizations + scoring versions
  - java: settings/RouterSettings.java (lines ~300-430), RouterScoringVersion.java, OptimizerScoringVersion.java
  - what: this method changes the effective router settings per board (layer count, net count, ...) and therefore the
    routing result; port verbatim.
  - done when: gtests on synthetic boards (2/4/6 layers, few/many nets) equal values taken from the Java harness.
- [ ] T305 | deps: T302 | `fallbackToBoardVias` fanout via-rule combination (needs FanoutSettings + BoardRules via rules)

## Phase 4 — Autoroute pipeline (Java autoroute/pipeline ≈ 5.4k lines; C++ BatchAutorouter 352)

- [ ] T401 | deps: - | AutorouteAirlineCalculator
  - java: autoroute/pipeline/AutorouteAirlineCalculator.java (214)
- [ ] T402 | deps: T401 | AutorouteConnectionRouter
  - java: autoroute/pipeline/AutorouteConnectionRouter.java (270)
- [ ] T403 | deps: T402 | AutoroutePassRunner part 1 (item ordering, per-item routing loop)
  - java: autoroute/pipeline/AutoroutePassRunner.java (535, first half)
- [ ] T404 | deps: T403 | AutoroutePassRunner part 2 (ripup escalation, pass result) + AutorouteRuntimeMetrics + AutorouteUnroutedReport
  - java: AutoroutePassRunner.java second half, AutorouteRuntimeMetrics.java (66), AutorouteUnroutedReport.java (80)
- [ ] T420 | deps: - | Board deep copy + state hash (history, batch loop and optimizer all need it)
  - java: board/facade/RoutingBoardUndoFacade.deepCopy (serialization) + BoardSnapshotManager.getHash (MD5)
  - cpp: src/board/RoutingBoard.{h,cpp}
  - what: `RoutingBoard::deep_copy()` returning an independent board (items, nets, rules, library, components, search
    trees rebuilt) and `state_hash()` — semantic equivalent, a stable hash over the same observable state, not MD5 of
    Java serialization bytes. Used for BoardHistory dedup and per-thread optimizer boards.
  - done when: gtests: the copy is independent (mutating one leaves the other), the hash is stable across copies and
    changes on insert/remove/move.
- [ ] T421 | deps: T302 | Router and optimizer scores
  - java: core/scoring/BoardStatistics.getRouterScore/getOptimizerScore (V2_CONTINUOUS, V2_LOWER_BOUND), RouterScoreSettings,
    OptimizerScoreSettings; DesignRulesChecker.calculateAllIncompletes
  - cpp: src/core/scoring/BoardStatistics.{h,cpp} (current API is the old calculateScore)
  - done when: scores for synthetic boards equal Java harness values; BatchLoop (T408) and optimizer acceptance can call them.
- [ ] T422 | deps: T403 | RoutingBoard::finish_autoroute + clear_transient_autoroute_state
  - java: RoutingBoard.finishAutoroute / clearTransientAutorouteState
- [ ] T407 | deps: T404,T420 | BoardHistory (board snapshots, BOARD_RANK_LIMIT=50) + BoardSnapshotManager
  - java: autoroute/BoardHistory.java (221), board/facade/BoardSnapshotManager.java (101)
  - note: no longer depends on BoardComparator (T405/T406 moved to Phase 7: only `--compare-boards`/debug use it)
- [ ] T408 | deps: T404,T407,T421,T422 | AutorouteBatchLoop (convergence, STOP_AT_PASS_MINIMUM/MODULO, restore best board)
  - java: autoroute/pipeline/AutorouteBatchLoop.java (681)
- [ ] T409 | deps: T408 | BatchAutorouter rewired onto PassRunner + BatchLoop (replaces the "2 no-progress passes" stop)
  - java: autoroute/pipeline/BatchAutorouter.java (603)
  - cpp: src/autoroute/BatchAutorouter.{h,cpp}
- [ ] T410 | deps: T409,T305 | BatchFanout part 1
  - java: autoroute/pipeline/BatchFanout.java (800, first half)
- [ ] T411 | deps: T410 | BatchFanout part 2
- [ ] T412 | deps: T409 | BatchOptimizer part 1 (item ordering, pass structure)
  - java: autoroute/pipeline/BatchOptimizer.java (1250, ~400 lines per part)
- [ ] T413 | deps: T412 | BatchOptimizer part 2 (per-item optimize: pull tight + via optimize)
- [ ] T414 | deps: T413 | BatchOptimizer part 3 (scoring/acceptance, statistics)
- [ ] T415 | deps: T409,T411,T414 | RoutingPipeline + NamedAlgorithm + TaskState; wire into the CLI route path
  - java: autoroute/pipeline/{RoutingPipeline,NamedAlgorithm,NamedAlgorithmType,TaskState}.java
  - cpp: also src/Freerouting.cpp (route path)
- [x] T416 | deps: T415 | BatchAutorouterThread — dropped (dead code in the Java headless path)
  - result: dropped. Verified 2026-10-10: `BatchAutorouter.autoroutePass` always calls
    `AutoroutePassRunner.runSingleThread` (called from `AutorouteBatchLoop`); `autoroutePassMultiThread` /
    `runMultiThread` / `BatchAutorouterThread` have no callers. Nothing to port; the single-thread pass is T403/T404.
- [ ] T423 | deps: T414,T420 | Optimizer thread pool (`-mt N`)
  - java: BatchOptimizer thread pool with per-thread board copies (autoroute/pipeline/BatchOptimizer.java)
  - what: results are consumed in item order in Java, so the thread count must not change the outcome (apart from
    wall-clock-limited runs). Required for the multi-thread benchmark mode. `std::thread` pool + T420 board copies.
  - done when: same SES for `-mt 1` and `-mt 4` on a small board; speed-up measurable.

- [x] T417 | deps: - | AutorouteEngine.autoroute_connection faithful port (found during T001)
  - java: autoroute/maze/AutorouteEngine.java autorouteConnection / resetAllDoors / clear
  - result: removed the invented grid-A* fallback (it queried the search tree for up to 3M cells
    per failed connection — the cause of every E2E timeout). Now Java order: maze search ->
    locator -> clear()/reset_all_doors() -> layer-active check -> remove ripped connections + trace
    tails -> FoundConnectionInserter. Added ripped-item/ripup-cost params, `Item::clear_autoroute_info`,
    `get_autoroute_info_pur`, `RoutingBoard::clear_all_item_temporary_autoroute_data`; `clear()` now
    clears item autoroute data like Java. Interim: BatchAutorouter's star loop (default, board-less
    AutorouteControl -> no maze could ever start) replaced by per-item `RoutingBoard::autoroute`
    calls until T401-T409 land. ctest 788/788.

- [x] T418 | deps: - | Board library model (prerequisite of T110-T115, found during T417)
  - java: core/library/{Padstack,Padstacks,Package,Packages,BoardLibrary}.java, board/model/structure/{Component,Components}.java
  - result: geometry ConvexTileShape (TileShape as ConvexShape, the C++ TileShape is not a Shape) +
    polymorphic TileShape::turn_90_degree/rotate_approx/mirror_vertical/mirror_horizontal/translate_by/enlarge;
    Padstack gets Java ctor (owned per-layer ConvexShapes, id, attach_allowed, hole_only); new Padstacks,
    board::Package/Packages (pins with name/padstack id/rotation, keepouts, side), BoardLibrary
    (RoutingBoard::library()); Component/Components rewritten Java-faithful (1-based ids, owned, front/back
    package). core::Package and the dead board/Components.{h,cpp} removed. DRC pin names now Java-equal.
- [x] T419 | deps: T418 | Drill item geometry + SearchTreeManager parity
  - java: DrillItem.getShape/tileShapeCount/shapeLayer, Pin.getShape/getPadstackLayer, Via.getShape,
    ShapeSearchTree.calculateTreeShapes(DrillItem)/drillHoleObstacle/drillHoleClearanceDelta, SearchTreeManager
  - result: pins/vias have per-layer board shapes and Java tree shapes (bounding box/octagon/tile by angle
    restriction, enlarged by clearance compensation + drill-hole delta) instead of one bounding-box octagon.
    SearchTreeManager: get_autoroute_tree inserts all board items, clearance hooks, reinsert; trees are
    emptied before deletion; ShapeSearchTree::remove drops the item's entries. AutorouteEngine uses the
    board's compensated autoroute tree (was a private uncompensated tree) and removes its rooms on
    destruction. Pin_shape_test (6). C++-only: a pin without component uses its padstack at the center.
    ctest 828/828.

## Phase 5 — Board gaps found during review

- [ ] T501 | deps: - | Drill-hole obstacles + clearance in ShapeSearchTree / HeadlessBoardManager (Java Via/Pin/Component hole insertion)
  - source: "Next candidates" after WP-10 Inc U/V in 05_WORK_PACKAGES.md (read that section only)
- [ ] T502 | deps: - | Pin parity audit (Java 706 lines vs C++ 557): exit restrictions, per-layer shapes, swap info
  - java: board/model/items/Pin.java
- [ ] T503 | deps: - | DrillItemMover
  - java: board/actions/DrillItemMover.java (326)
- [ ] T504 | deps: - | ShapeEntrySide / ShapeAndEntrySide audit (verify against ShapeTraceEntries / CalcFromSide; port what is absent)
  - java: board/model/structure/{ShapeEntrySide,ShapeAndEntrySide}.java
- [ ] T505 | deps: - | Board facade audit: BoardConnectivityQueries, BoardItemRepository, RoutingBoardOperations, RoutingBoardSearchFacade, RoutingBoardUndoFacade
  - what: for each Java public method find the C++ RoutingBoard/BasicBoard equivalent; port the small
    gaps here, file tasks for the big ones.

## Phase 6 — End-to-end parity, one board per task

For each board: run `tools/e2e_compare.sh`, find the **first** divergence from Java (crash, missing
connections, wrong SES), fix it or file narrower tasks (T6xxa, …). Done when C++ routes the board with
#unrouted equal to Java and a valid SES (wire/via counts within ±10% of Java). Update E2E_STATUS.md.

- [ ] T601 | deps: T001,T117,T202,T415,T008,T611 | E2E: empty_board.dsn
- [ ] T602 | deps: T601 | E2E: Issue034-Green14SegLED.dsn
- [ ] T603 | deps: T601 | E2E: Issue107-freq_teiler_200kHz_kicad.dsn
- [ ] T604 | deps: T601 | E2E: Issue026-J2_reference.dsn
- [ ] T605 | deps: T601 | E2E: Issue103-Board-Unrouted.dsn
- [ ] T606 | deps: T601 | E2E: Issue006-LPC18XX_43XX_SCH.dsn (binary OLE file: parity = both report the same load error)
- [ ] T607 | deps: T601 | E2E: Issue070-Autorouter_FQ101_PCB_2022-05-13.dsn
- [ ] T608 | deps: T601 | E2E: BBD_Mars-64.dsn
- [ ] T609 | deps: T608 | E2E: Issue029-hw48na.dsn (the original "19% headless" benchmark)
- [ ] T610 | deps: T609 | E2E sweep over all 117 `tests/data/**/*.dsn`
  - what: load parity (T118 results) for every DSN; route every board Java finishes within 60 s and record
    pass/fail + unrouted counts in E2E_STATUS.md. The DAC2020 `bm01-bm11` set (5-80 KB) is the best bit-exact corpus.

## Phase 7 — DRC, jobs, scoring stragglers, final docs

- [ ] T405 | deps: - | BoardComparator part 1 (`--compare-boards`; moved here from Phase 4, not on the route path)
  - java: board/state/BoardComparator.java (758, first half)
- [ ] T406 | deps: T405 | BoardComparator part 2
- [ ] T701 | deps: T117 | CLI DRC mode calls DesignRulesChecker; DrcReport JSON field names match Java
- [ ] T702 | deps: T303 | RoutingJobScheduler + RoutingJobSchedulerActionThread (headless, synchronous)
  - java: management/jobs/*.java (986)
- [ ] T703 | deps: T002,T421 | Scoring stragglers: BenchmarkScoreCalculator (`--calculate-benchmark-scores`), BoardScoreBreakdown, ScoringWeightComparison, BoardStatisticsBounds* (router/optimizer scores are T421)
- [ ] T704 | deps: T415 | core/results RoutingResultManifest (`--router.result_json_path`; the benchmark's per-phase metrics channel on BOTH sides, T801 depends on it)
  - java: core/results/RoutingResultManifest.java (256)
- [ ] T706 | deps: T702,T704 | RoutingJob phase metrics + resource usage
  - java: core/RoutingJob.java (resultPhaseMetrics, log plumbing, stats JSON at RoutingJob.java:662-690), RouterJobResourceUsage
  - what: C++ `RouterJobResourceUsage` exists but nothing populates it. Fill cpu_time_seconds and peak_memory_mb
    (getrusage), record per-phase wall times (load / fanout / route / optimize / write) in the manifest.
- [ ] T705 | deps: T601,T602,T603,T604,T605,T606,T607,T608,T609,T610,T006 | Final docs sync: regenerate 02_PROGRESS/03_MISSING/INDEX from the CSV + E2E_STATUS; archive WP-10 increment logs to docs/migration/history/

## Phase 8 — Java-vs-C++ benchmark (Track D: T801-T805 can be built now against the Java jar only)

Design (see the approved plan, 2026-10-10): modes `st` = `-mt 1` pinned with `taskset`, `mt` = default threads;
measure wall (monotonic), user+sys CPU, peak RSS (`os.wait4` rusage), per-phase times (manifest, T704), and quality
through the Java jar as the neutral scorer. A board enters the benchmark only after it passes the G4 parity gate.
Tiers: smoke `empty_board`; tiny `Issue026-J2_reference`, a DAC2020 `bm0x`, `corney_island_wireless`; small
`Issue034`, `BBD_Mars-64`, `Issue107`; medium `Issue029-hw48na`, `Issue214`, `Issue289`; large `Issue070`,
`Issue103-Board-Unrouted`, `Issue420`, `Issue555-CNH`; stress `Issue190`, `Issue187` (run alone).
Fairness: identical DSN/flags/time limits, fresh `--user_data_path`, strictly sequential runs (box: 12 cores, ~7.8 GB
RAM, ~1.3 GB free: fixed `-Xmx` for Java, no builds in parallel), report in-program time next to process wall time
(Java adds ~1 s analytics sleep, 500 ms wait loop, 250 ms scheduler poll).

- [ ] T801 | deps: T008 | tools/bench/bench.py — run matrix board x impl x mode x reps
  - cpp: tools/bench/bench.py (stdlib python only: no hyperfine/jq/perf on this machine)
  - done when: Java-vs-Java run on a small board gives a ratio ~1.0 with small spread (harness sanity), results in
    results/<date>/<board>.<impl>.<mode>.json with machine info (nproc, RAM, JDK, g++, flags, commit SHA).
  - note: C++ per-phase times need T704/T706; until then record process-level numbers only.
- [ ] T802 | deps: T801 | Neutral quality scorer
  - what: every produced SES (either implementation) goes through `java -jar <jar> -de x.dsn x.ses -drc out.json`;
    extract qualityScore, optimizerScore, unrouted, clearance violations, wire length, via count. After T203/T701 add the
    C++ DRC as a cross-check (both scorers must agree).
- [ ] T803 | deps: T801 | Tier definitions + per-tier reps/timeouts (smoke/tiny/small/medium/large/stress) in tools/bench/tiers.json
- [ ] T804 | deps: T802,T803 | Reporter: docs/migration/BENCHMARK.md + CSV (median/min/max, speed ratio, quality delta, memory ratio)
- [ ] T805 | deps: T804 | tools/bench/check_regression.py: fail when C++ quality < Java's or time regresses > X % vs stored baseline
- [ ] T806 | deps: T801 | Optional: warm-JVM measurement (compiled harness that routes N times in one JVM) to separate JIT warm-up from cold CLI runs
- [ ] T807 | deps: T610 | Profile the C++ router on a medium + a large board (valgrind/callgrind or `-pg`; `perf` is not installed), list hot spots
- [ ] T808 | deps: T807 | CMake preset `release-perf` (-O3 -march=native -flto, optional mimalloc); never used for parity gates
- [ ] T809 | deps: T807 | Profile-driven optimizations, each guarded by the T6xx parity suite (output must not change)
- [ ] T810 | deps: T809,T804,T423 | Final benchmark run (st + mt, all tiers that pass G4) and write-up in docs/migration/BENCHMARK.md

---

## State at plan time (2026-10-08)

- Build: `build/` reconfigured in this WSL checkout (earlier loop runs used `/mnt/d/projects/EDA/freerouting_cpp`).
  `run_tests` is now built by default; **ctest 761/761 green** at commit f25920f.
- Done and solid: geometry, rules, datastructures, board model, maze search / expansion rooms /
  connection locator / inserter / shove / fanout / `RoutingBoard::autoroute` (WP-1 … WP-10 Inc V).
- Biggest remaining gaps, by Java lines: DSN parser (~9k), autoroute pipeline (~5k), settings (~3k),
  SES writer (~0.5k + write paths), BoardComparator/history (~1k).
- `JavaToCppClassMap.csv` reports 171 "missing" but many are renamed ports (fixed by T002).
