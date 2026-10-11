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

1. Read only: `SOURCE_OF_TRUTH/01_MEMORY.md`, `SOURCE_OF_TRUTH/CONVENTIONS.md`, this task, **its
   `- spec:` section in `SOURCE_OF_TRUTH/plans/`** (plus `plans/README.md` once), the Java files they name
   and the C++ files you touch. Do **not** read `02_PROGRESS.md`–`05_WORK_PACKAGES.md` (large historical
   logs) unless the task says so. If the spec and the Java disagree, the Java wins — fix the spec in the
   same task.
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

- [x] T006 | deps: - | Class map: `pending` status + real out-of-scope list
  - spec: SOURCE_OF_TRUTH/plans/M0_oracles.md (section T006)
  - cpp: tools/class_map_check.py, JavaToCppClassMap.csv, SOURCE_OF_TRUTH/README.md
  - what: today's 124 `out_of_scope` rows mix truly non-headless classes (gui, api, api.mcp, analytics, io.kicad,
    parser/SessionToFusion, util.gson API adapters, util.VersionChecker, settings Api/Gui/GoogleSheets/McpServer/
    RateLimit/UsageAndDiagnostic/UserProfile) with ~60 headless-path classes that are simply not ported yet
    (BoardHistory, BatchFanout, AutoroutePassRunner, SesWriter/Reader, DsnReader/Writer, RulesReader/Writer,
    SettingsMerger and settings.sources.*, RoutingJobScheduler, BoardComparator, board.facade.*, ...).
    Add status `pending` for the latter; keep `out_of_scope` only for the former, each with a one-word reason column.
  - done when: `class_map_check.py` prints counts per status and the pending list; README Quick Status shows
    "ported / header_only / partial / pending / out_of_scope"; goal for the whole migration is `pending = 0`.
  - result: CSV rebuilt (375 rows, new `reason` column) with the heuristic `is_out_of_scope()` dropped for an explicit
    OUT_OF_SCOPE table (dir rules gui/api/mcp/analytics/io.kicad/util.gson + exact files SessionToFusion, io.KiCadNetClassNames,
    util.VersionChecker, util.SafeObjectInputStream, 11 settings classes). Counts: ported 203, header_only 32, partial 2,
    pending 90, out_of_scope 48. Real mappings fixed along the way: deprecated legacy specctra/Library|Network|Wiring|Placement
    rows (were wrongly `ported`) now `pending`; parser classes re-pointed to live `src/designforms/specctra/parser/*.h`;
    io/parser/{Library,Network,Wiring,Placement} pending; geometry ShapeEntrySide->CalcFromSide, ShapeAndEntrySide->
    CalcShapeAndFromSide, LayerStructure->board/Layer.h, Side->enum class in Point.h, MazeExpansionEngine->MazeExpansionEngine.h
    (was pointing at a forward decl), Constants and FreeSpaceExpansionRoom -> partial, MultiOutputGenerator -> pending
    (dispatches to the real SesWriter, per plan conditional). Added tools/test_class_map_check.py (stdlib unittest, not ctest).
    `python3 tools/class_map_check.py` prints counts + pending grouped by package, exits 0 (errors only on structural issues).
- [x] T007 | deps: - | Java reference harness (oracle for T110-T118 and T611)
  - spec: SOURCE_OF_TRUTH/plans/M0_oracles.md (section T007)
  - cpp: tools/java_ref/{BoardCounts.java,build.sh}, tests/data/expected/*.counts.json
  - what: compile a small Java class with `javac` against the executable jar (see 01_MEMORY.md gotcha 29) that
    loads a DSN exactly like the CLI and prints JSON: layer/net/component/pin/padstack/netclass/via-rule/keepout/
    conduction-area counts, the item-id sequence of `board.get_items()`, and the clearance matrix. Same schema as
    `freerouting --dump-board-counts` (T117). Run it over all `tests/data/**/*.dsn` once and store the JSON.
  - done when: harness runs on the 8 E2E boards + a hand-picked set; JSON committed; Issue006 reports the load error.
  - result: tools/java_ref/BoardCounts.java (loads via DsnReader.readBoard exactly like Freerouting.java:1016; prints
    schema layers/counts/clearance_classes/clearance_matrix/items/warnings to a file arg or after a ===JSON=== marker),
    build.sh + run_all.sh; ran over all 117 tests/data/**/*.dsn -> tests/data/expected/<rel>.counts.json, 117/117 written,
    0 failures; 116 success + Issue006 parse_error (binary OLE, "(pcb" header check as expected). Oracle set confirmed
    deterministic (bit-identical re-run), item-id descending order OK. 1 file (Issue289 FHT-VGA) > 2 MB -> items_omitted.
    Gotcha: the jar is Java-25 classfiles but env JAVA_HOME=JDK21, so build.sh/prefer JAVA25_HOME (JDK 25 javac). ctest 887/887, jar run_all green.
- [x] T008 | deps: - | Java baseline runner (replaces test_java_boards.sh)
  - result: tools/java_baseline.sh (tiers smoke/tiny/small/medium/large/stress/e2e/all, st/mt mode, fresh --user_data_path,
    -Xmx2g, --router.result_json manifest, `time -v`, reason classification, new `hit_job_timeout` flag) writes
    java_output_ses/<board>_java.{ses,json,log,time}, docs/migration/baseline.json and a marked section in E2E_STATUS.md
    (tools/e2e_all.sh preserves it). e2e tier, st: 7/8 boards routed (BBD_Mars-64 170 s, Issue070 318 s, Issue103
    1806 s and cut by the 30 min job limit — wall-clock limited); Issue006 = parse error (binary OLE file, correct).
    The 4 previously missing baselines were the old 180 s timeout + Issue006. No C++ change; ctest unaffected.
  - spec: SOURCE_OF_TRUTH/plans/M0_oracles.md (section T008)
  - cpp: tools/java_baseline.sh (+ docs in docs/migration/E2E_STATUS.md)
  - what: run the jar headlessly per board with a clean `--user_data_path`, `-mt 1`, fixed `-mp`, explicit
    `--router.job_timeout`, `--router.optimizer.timeout`, `--router.result_json`; capture SES, manifest JSON, stdout log
    and wall time; per-tier timeout instead of the fixed 180 s. Today 4 of 8 boards (BBD_Mars-64, Issue006,
    Issue070, Issue103-Board-Unrouted) have no baseline: record WHY (timeout / error) per board.
  - done when: `java_output_ses/<board>_java.{ses,json,log}` exist for every board Java can route, with a
    `baseline.json` summary (seconds, exit code, timed_out flag).
- [x] T009 | deps: - | Determinism: one `java.util.Random` port (JavaRandom.h); fix the PolygonShape LCG multiplier
  - result: header-only `src/datastructures/JavaRandom.h` (setSeed/next/nextInt/nextInt(bound)/nextDouble + `java_shuffle`)
    verified against JDK 25 output (JavaRandom_test 8); PolygonShape uses it (thread_local, re-seeded per split like Java)
    instead of the private LCG with the wrong multiplier. PolygonShapeSplitTest 3 golden splits from the Java jar
    (comb/L/arrow); the comb test fails with the old 0x5DEECE66 multiplier. MazeRandom left unchanged. ctest 898/898.
  - spec: SOURCE_OF_TRUTH/plans/M0_oracles.md (section T009)
  - cpp: src/datastructures/JavaRandom.h (+ _test), src/geometry/planar/PolygonShape.cpp (~line 67)
  - what: `PolygonShape.cpp` multiplies by `0x5DEECE66ULL`, Java uses `0x5DEECE66D`, so `split_to_convex` starts at
    a different corner than Java. One shared header-only `JavaRandom` (+ `java_shuffle`) replaces the private copy.
    (The `normalizeAllTraces` HashMap order moved to T116, where that method is ported.)
  - done when: golden values from a throwaway Java program (`new Random(99)`, `nextInt`, `nextDouble`, `Collections.shuffle`) match; ctest green.
- [x] T010a | deps: T009 | Determinism: PlanarDelaunayTriangulation part 1 (Corner/Edge/Triangle/TriangleGraph, corner insertion, seeded shuffle)
  - spec: SOURCE_OF_TRUTH/plans/M0_oracles.md (section T010a)
  - java: datastructures/PlanarDelaunayTriangulation.java (≈ lines 1-500 of 978)
  - cpp: src/datastructures/PlanarDelaunayTriangulation.{h,cpp,_test.cpp} (C++ today: 191 lines, different algorithm, no shuffle)
- [x] T010b | deps: T010a | Determinism: PlanarDelaunayTriangulation part 2 (legalisation/flips, degenerate edges, result edges)
  - result: T010a+T010b done together (the Edge/Triangle/DAG classes are mutually dependent; a half port would not
    compile). Full Java-faithful port (de Berg incremental insertion, search DAG, flips, border splits, degenerate
    edges, Java's unused-edge id quirk in splitAtInnerPoint, TreeSet-by-id result order, insideCircle tolerance) with
    the seeded `java_shuffle` from T009. Public API unchanged (NetIncompletes untouched). 7 golden tests from the jar
    (three, square — Java returns 4 edges —, collinear, duplicate corners, co-circular, 20 random points, validate);
    the old C++-specific expectations were replaced. ctest 902/902.
  - spec: SOURCE_OF_TRUTH/plans/M0_oracles.md (section T010b)
  - java: datastructures/PlanarDelaunayTriangulation.java (≈ lines 500-978)
  - done when: golden edge sets from Java (co-circular, collinear, duplicate points) match; NetIncompletes tests green.
- [x] T011a | deps: - | Item ordering parity part 1: ItemSet (descending id), RoutingBoard::get_items() order, src/board/ sets
  - spec: SOURCE_OF_TRUTH/plans/M0_oracles.md (section T011a)
  - what: Java `Item.compareTo` sorts by descending id and the board item list (ConcurrentSkipListMap) iterates newest
    first; Java uses `TreeSet<Item>` in ~45 places. C++ uses pointer-ordered `std::set<Item*>` (118 occurrences — not
    even reproducible between C++ runs) and an insertion-ordered `items_` vector.
  - done when: Item_order_test green; all sets in src/board/ converted or documented; ctest green.
- [x] T011b | deps: T011a | Item ordering parity part 2: autoroute/, interactive/NetIncompletes, drc/, core/scoring sets
  - result: T011a+T011b done together (the set type appears in signatures shared by board/, autoroute/, interactive/,
    drc/, scoring/, so a partial change does not compile). `board/ItemSet.h`: `ItemIdDescending` (null-safe) +
    `ItemSet`/`ConstItemSet`; all 149 `std::set<Item*>` / `std::set<board::Item*>` uses in src/ and tests/ converted
    (the 13 `TreeStorableLess` sets already used Item::compare_to = Java order). `RoutingBoard::items_` is kept sorted by
    ascending id (append for new items, binary insert/remove otherwise); `get_items()`, `get_autoroute_items()`, the
    trace/pin/via getters, the combine/normalize snapshots and `get_item(i)` (index 0 = newest) follow Java's
    newest-first order. Only exception: `DesignRulesChecker.cpp` `unordered_set processed_items` (membership only).
    Test updates: ItemSelectionFilter fixture items get distinct ids (equal ids collapse, as in a Java TreeSet); two
    DRC tests now expect Java's newest-first item order. New Item_order_test (4). ctest 906/906.
  - spec: SOURCE_OF_TRUTH/plans/M0_oracles.md (section T011b)
  - done when: grep for pointer-ordered item sets in src/ finds only documented exceptions; ctest green.

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
- [x] T109b | deps: T109a | Structure part 3b: board rules from the structure (clearance rules, default classes)
  - result: parser `Structure::update_board_rules`, `set_clearance_rule`, `contains_wire_clearance_pair`,
    `create_default_clearance_classes`, `append_clearance_class`, `clearance_equals` (Java Structure 611-855) with Java
    quirks (double `_` strip in 2-element pairs, `split(…,2)` keeps trailing empty, `smd_via_same_net` creates classes
    `smd`/`via_same_net`, `(int) Math.round`). 8 golden tests from the jar (Java `setClearanceRule` output incl. odd→even
    clearance) + 1 update_board_rules test. Structure_test 32. ctest 915/915.
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T109b)
  - java: parser/Structure.java updateBoardRules / setClearanceRule / containsWireClearancePair /
    createDefaultClearanceClasses / appendClearanceClass / clearanceEquals (lines 611-855, ~245)
  - cpp: src/designforms/specctra/parser/Structure.{h,cpp}
  - done when: gtests build a BoardRules from `(rule ...)`/`(structure (rule ...))` snippets (width,
    clearance with `type` pairs, `default`/`wire_wire` etc.) and the clearance matrix equals Java's values.
- [x] T126 | deps: - | Parser Net, Net::Id, Net::Pin, NetList (moved forward from T113: Structure needs the netlist for planes)
  - result: parser `Net` (header-only: `Id`, `Pin` with Java compareTo order via defaulted `<=>`, optional pin set =
    Java null until setPins) and `NetList` (`std::map<Net::Id, unique_ptr<Net>>`, add_net returns nullptr for an
    existing id, get_nets in id order); `ReadScopeParameter::netlist`. NetList_test 4. ctest 919/919.
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T126)
  - java: parser/NetList.java (56), parser/Net.java (129, read part; write part is T204c)
  - cpp: src/designforms/specctra/parser/{Net,NetList}.{h,cpp,_test.cpp}; `ReadScopeParameter::netlist`
- [x] T109c | deps: T109b,T126 | Structure part 3c: create_board (outline, keepouts, planes → board) + BoardParserCallback/MinimalBoardManager + Communication fields
  - result: `parser::BoardParserCallback` + `MinimalBoardManager` (creates the RoutingBoard, adopts the Communication, inserts
    the outline first like Java's ctor); `Structure::create_board` / `separate_holes` / `insert_keepout` /
    `insert_missing_power_planes` + read_scope tail (keepout → via_keepout → place_keepout, planes, missing power planes);
    `DsnFile::adjust_plane_autoroute_settings`; `board::Communication` extended (parser info incl. constants and
    write resolution, transform as scale/base numbers, Java hostIsOldKicad/hostCadIsFusion); parser helpers
    `to_polyline_shape` / `to_board_area`. Found and fixed a geometry bug on the way: C++ `Polygon` removed collinear
    corners cyclically and all at once, Java removes interior ones only, one per pass (a closed outline path came out
    as a triangle); golden tests from the jar. Structure board test reproduces the Java harness item list exactly
    (ids 1-10). Structure tests that read boundary-less snippets now include a boundary (Java fails without one).
    flip_style stays default until T112; DSN autoroute settings → routing job waits for T124a (no job in Minimal).
    Structure_test 37, ctest 928/928.
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T109c)
  - java: parser/Structure.java createBoard (1139-1307), readScope tail (1100-1138), insertKeepout (857-935),
    separateHoles (577-610), insertMissingPowerPlanes (526-576); parser/DsnFile.adjustPlaneAutorouteSettings
    (DsnFile.java 29-107, called by DsnReader when the DSN has no `(autoroute ...)` scope)
  - done when: a structure scope creates a RoutingBoard with outline (T121 `insert_outline`), keepouts as
    obstacle areas, planes as conduction areas; gtests with hand-written snippets. Note: `place_keepout`,
    `wire_keepout`, `via_keepout` appear in (almost) no test DSN — cover them with snippets.
- [x] T110 | deps: T109c | Library part 1: padstacks → board Padstacks
  - result: `parser::Library` (Keyword LIBRARY_SCOPE is now the real class) with `read_padstack_scope` (name `.\d+`
    suffix removed, shapes per layer, attach/absolute, convex hull + single piece + Simplex.simplify for non-convex
    pads, offset(1) workaround for non-area pads, duplicate names ignored) and `read_scope` (fresh Padstacks; images
    skipped until T111). Golden padstack dump from the jar matches (Library_test 2). Padstack counts equal the Java
    harness on BBD_Mars-64, Issue026, Issue034, Issue070, Issue103-Unrouted, Issue107 (read through PCB_SCOPE);
    empty_board creates no board yet because its `(string_quote ")` parser scope needs T125. The board-count dump moved
    to T117 (`--dump-board-counts`). ctest 930/930.
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T110)
  - java: parser/Library.java (padstack part), core/library/Padstacks.java (84)
  - also: add a test helper (or `freerouting --dump-board-counts <dsn>`) printing layer/net/component/
    pin/padstack/netclass/keepout counts; used as acceptance by T111–T118.
- [x] T111 | deps: T110 | Library part 2: images / Package + PartLibrary + LogicalParts + BoardLibrary
  - result: parser `Package` (image read: pins incl. rotation before/after the pin name, side, outlines, keepout /
    via_keepout / place_keepout areas) and `PartLibrary` (logical part mappings and parts into ReadScopeParameter; board
    insertion is T115); `Library::create_board_packages` (cleaned padstack lookup, rounded relative pin locations,
    outline widths/closed flags, generated keepout names, `::<n>` de-duplication with Java's identical-pins check).
    New geometry `CircleArea` adapter (Java Circle is an Area; C++ Circle is not) used by `to_board_area` for circular
    keepouts/planes. Golden package dump from the jar matches. Library_test 4, CircleArea_test 3. ctest 935/935.
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T111)
  - java: parser/Package.java (425), PartLibrary.java (351), core/library/{Packages,LogicalParts,BoardLibrary}.java
- [x] T112 | deps: T111 | Placement: Component, ComponentPlacement, PlaceControl
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T112)
  - java: parser/Component.java (381), ComponentPlacement.java (92), PlaceControl.java (73), Placement.java (29)
  - result: T111 had already over-delivered the placement readers but left HEAD broken: the parser
    `Component_test` collided with the board `Component_test` target (cmake configure failed) and the test
    called a non-existent `is_flip_style_rotate_first()`. Fixed both (test renamed `ParserComponent_test.cpp`,
    suite `ParserComponentTest`, getter corrected) and split `PlaceControl` out of `Component.{h,cpp}` into
    its own `PlaceControl.{h,cpp}` per the spec/conventions (Keyword.cpp + Structure.cpp now include it).
    Verified faithful: `Component::read_component_scope`/`read_place_scope`/`read_item_clearance_info`/
    `read_lock_type`, `ComponentPlacement` (TreeMap-order maps, null coor = unplaced) and
    `PlaceControl::read_scope`/`read_flip_style_rotate_first`; `PLACEMENT_SCOPE` stays a plain ScopeKeyword
    (Java `Placement` has no readScope; writeScope is T204b). ParserComponent_test 3. ctest 938/938,
    run_tests PASSED.
- [x] T125 | deps: T106 | Circuit + Parser (moved out of T117: needed by NetClass.read and the Plane reader)
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T125)
  - java: parser/Circuit.java (143), parser/Parser.java (228)
  - cpp: src/designforms/specctra/parser/{Circuit,Parser}.{h,cpp,_test.cpp}
  - what: `Keyword::PARSER_SCOPE` becomes the real class (string_quote, space_in_quoted_tokens,
    host_cad, host_version, constant, write_resolution, ...), so `ReadScopeParameter::host_cad` is
    filled before `Plane` reads its Allegro windows. Circuit holds the rule/length/use-via data NetClass uses.
  - done when: gtests on `(parser ...)` snippets (string_quote switch, host_cad/host_version) and Circuit
    read; `Plane` Allegro window handling covered by a host_cad=Allegro snippet.
  - result: Parser.{h,cpp} (read_scope, read_write_solution, read_constant, write_scope, read_quote_char)
    and Circuit.{h,cpp} (read_scope, read_length_scope) ported with Java parity (host_cad/host_version
    fall back to "" on missing scope since C++ read_string_scope returns nullopt vs Java null;
    unused read_ok kept; lengthArr[0]=max, lengthArr[1]=min; failed read_length_scope keeps zeros).
    Parser.cpp sets string_quote; Plane.java-style Allegro window skip handled via existing
    equals_ignore_case(host_cad, "allegro") — verified by gtests (incl. non-Allegro keeps windows).
    write_scope golden strings captured from the Java executable jar via throwaway harness
    (/tmp/opencode/ParserGolden.java, reservedChars {(), ;-_}), non-ASCII quote escape preserved.
    Implemented read-only (no public mutators); net class sees Circuit data via ReadScopeParameter.plane_list.
    Keyword.cpp: k_PARSER_SCOPE is now the real Parser instance. Registered in
    src/designforms/CMakeLists.txt + tests/CMakeLists.txt; 15 new gtests; ctest 953/953 green.
- [x] T113 | deps: T112 | Network part 1: net scopes (readNetScope, readNetPins, ordered subnets) + Network::read_scope skeleton
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T113)
  - java: parser/Net.java (129), NetList.java (56), Network.java (net read part)
  - result: The T113 code landed in 311ba5c but its test was never registered, so the task stayed open. Finished it:
    `parser::Network` (`read_scope` loop with via/via-rule/class/class-class branches still `skip_scope` stubs for
    T114a/T114b, `read_net_scope`, `read_net_pins` → `optional<vector<Net::Pin>>` for Java's boolean+out-param,
    `create_ordered_subnets`), `Keyword::NETWORK_SCOPE` real instance, `Network.cpp` in `src/designforms/CMakeLists.txt`.
    Added `Network_test.cpp` to `tests/CMakeLists.txt` (DESIGNFORMS_TEST_SOURCES — it was missing, so the 9 tests never ran).
    Parity verified incl. `+5V` kept raw by `nextString()` (raw-buffer read, unlike the illegal-char-skipping identifier
    tokenizer of gotcha 37). Network_test 9, ctest 962/962, run_tests PASSED.
- [x] T114a | deps: T113,T125 | Network part 2a: via infos, via rules, via padstacks
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T114a)
  - java: parser/Network.java readViaInfo/readViaRule/insertViaInfos/createDefaultViaInfos/insertViaRules/addViaRule (255-422) + readScope via-padstack block (1273-1318)
  - result: Ported read_via_info (three NAME reads, padstack lookup falls back to padstacks + add_via_padstack, ATTACH handling), read_via_rule, insert_via_infos, create_default_via_infos (name = padstack or padstack+'-'+class for non-default classes), insert_via_rules + add_via_rule (replaces an existing same-named rule) into parser/Network.{h,cpp}. Ported the readScope via-padstack block: names cleaned with `\.\d+`, missing padstacks dropped with the Java warning, `setViaPadstacks`. Added `BoardRules::remove_via_rule` (Java viaRules.remove, pointer identity). The net-class `use_via` → viaPadstackNames loop is deferred to T114b (NetClass reader not ported yet), marked with a T114b comment. 4 new NetworkTest cases (via info+rule, default via infos from padstacks, missing padstack dropped, via rule without via infos falls back). Network_test 13, ctest 966/966, run_tests PASSED.
 - [x] T114b | deps: T114a | Network part 2b: net classes and class pairs → BoardRules
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T114b)
  - java: parser/NetClass.java (197), parser/Network.java insertNetClasses … getClearanceClass (423-831)
  - result: Ported parser.NetClass (readScope, readClassClassScope, ClassClass) as NetClass.{h,cpp}; wired the CLASS/CLASS_CLASS branches in Network::read_scope; ported insertNetClasses/insertNetClass, insertClassPairs/insertClassPairInfo, addMixedClearanceRule, addClearanceRule, createDefaultClearanceClasses, getClearanceClass, createViaRule, createActiveTraceLayers and the use_via → via_padstack_names append (incl. the Java aliasing quirk). KiCad default-name helpers live in Network.cpp. 7 NetClassTest + 6 NetworkTest cases. ctest 979/979, run_tests PASSED.
- [ ] T115 | deps: T114b | Network part 3: insert components, pins, package keepouts, outlines, logical parts
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T115)
  - java: parser/Network.java (insert components / pins part)
- [ ] T116 | deps: T115 | Wiring read: wires, vias, fixed states, polyline paths + BasicBoard.normalizeAllTraces (Java HashMap order)
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T116)
  - java: parser/Wiring.java (715, read part)
- [ ] T117 | deps: T109c,T116,T125 | DsnFile.read orchestration + DsnReader + BoardParserCallback + board-count dump
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T117)
  - java: parser/DsnFile.java (220), io/specctra/DsnReader.java (291, incl. readMetadata), parser/BoardParserCallback.java (42),
    io/BoardReadResult.java + io/BoardMetadata.java (return types of DsnReader), the MinimalBoardManager shim
    used by ReadScopeParameter
  - also: `freerouting --dump-board-counts <dsn>` printing layer/net/component/pin/padstack/netclass/keepout
    counts and the item-id sequence as JSON (same schema as the Java harness of T007)
  - done when: every `tests/data/*.dsn` loads without error through the CLI path; counts match Java
    for the 8 E2E boards (record them in docs/migration/MIGRATION_designforms.md).
- [ ] T118 | deps: T117,T007 | DSN reader audit vs Java on all test boards; file a task per mismatch
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T118)
  - what: run C++ `--dump-board-counts` and the Java harness (T007) over all 117 `tests/data/**/*.dsn`;
    compare counts AND item-id order (item ids / item list order drive TreeSet ordering in the router, so
    equal counts are not enough). `Issue006-LPC18XX_43XX_SCH.dsn` is a binary OLE file: an error is correct.
  - done when: zero mismatches or one filed task per mismatch (T118a, ...); results table in MIGRATION_designforms.md.
- [ ] T124a | deps: T117,T202 | HeadlessBoardManager core + delete the legacy hand-rolled reader, switch the CLI
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T124a)
  - java: management/HeadlessBoardManager.java ctor/getRoutingBoard/replace/createBoard (no overrides)/loadFromSpecctraDsn/applyParsedBoardResult/saveAsSpecctraSessionSes
  - remove: legacy `specctra/{Scanner,Structure,Library,Network,Placement,Wiring,Keyword,Circle,Polygon}.*`, `Path.h`, `DsnFile.h`,
    `Wiring_test.cpp`, `board/BoardLoader.*`, `board/BoardBuilder.*`, `RoutingBoard::create_pins_from_parsed`, `parsed_result_`
  - done when: grep for the removed names finds nothing; build + ctest green; e2e_all.sh unchanged or better.
- [ ] T124b | deps: T124a,T302,T307 | HeadlessBoardManager settings overrides after load (hole clearance, copper-to-edge, plane nets, plane as obstacle)
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T124b)
  - java: HeadlessBoardManager.java applyRouterSettingsForLoadedBoard (784-796), apply*Override (352-614)
- [ ] T124c | deps: T124b | HeadlessBoardManager post-load processing (reduceNetsOfRouteItems, validatePowerPlanes, validateBoardDesignErrors, pre-existing violations)
  - spec: SOURCE_OF_TRUTH/plans/M1_dsn_reader.md (section T124c)
  - java: HeadlessBoardManager.java 798-844, 962-1198; board/facade/RoutingBoard.java reduceNetsOfRouteItems (~1310)

## Phase 2 — SES writer parity (Java SesWriter 552 + write paths; C++ SessionWriter 88)

- [x] T122 | deps: T106 | WriteScopeParameter + shape/resolution write halves (T106 was marked done without them)
  - spec: SOURCE_OF_TRUTH/plans/M2_ses_io.md (section T122)
  - java: parser/WriteScopeParameter.java (38), `writeScope` of Shape/Rectangle/Circle/Polygon/Path/PolygonPath/
    PolylinePath/Resolution (and Layer/Rule write parts that SesWriter and DsnWriter both call)
  - cpp: src/designforms/specctra/parser/{WriteScopeParameter,...}
  - done when: each shape round-trips (write -> read -> equal) in a gtest; output text equals Java for the same shape.
  - result: Ported WriteScopeParameter, Shape/Rectangle/Circle/Polygon/Path/PolygonPath/PolylinePath
    writeScope+writeScopeInt+writeHoleScope, Resolution.writeScope and the board<->DSN
    CoordinateTransform value/shape conversions (new CoordinateTransform.cpp). Added
    datastructures/JavaFormat (Java Double/Float.toString). 28 gtests pass (golden strings
    captured from Java JDK 25 + write->read->equal round-trips); ctest 1005/1005, run_tests green.
    Layer/Rule write parts deferred to T201/T202 (SesWriter/DsnWriter callers).
- [ ] T201 | deps: T117,T122,T125 | SesWriter part 1: session header, base_design, placement, was_is
  - spec: SOURCE_OF_TRUTH/plans/M2_ses_io.md (section T201)
  - java: io/specctra/SesWriter.java (552, first half)
  - cpp: src/designforms/specctra/SessionWriter.{h,cpp}
- [ ] T202 | deps: T201 | SesWriter part 2: routes, library_out (via padstacks), network_out (wires, vias, fixed)
  - spec: SOURCE_OF_TRUTH/plans/M2_ses_io.md (section T202)
  - java: SesWriter.java second half, parser/Wiring.java write part, Shape write part
  - done when: `Issue103-Board-Routed.dsn` loaded and written without routing gives an SES token-equal
    to Java's (generate the Java SES once with the jar, store it in tests/data/expected/).
- [ ] T203 | deps: T202 | SesReader (enables round-trip tests, `-de x.dsn x.ses` resume, C++-side DRC on an SES)
  - spec: SOURCE_OF_TRUTH/plans/M2_ses_io.md (section T203)
  - java: io/specctra/SesReader.java (413), SesImportSummary.java (15). Skip `saveSpecctraSessionSesAsFusionScriptScr`
    (parser/SessionToFusion, Fusion export is out of scope).
- [ ] T204a | deps: T202,T114b | RulesReader (+ readRouterSettings for RulesFileSettings)
  - spec: SOURCE_OF_TRUTH/plans/M2_ses_io.md (section T204a)
  - java: io/specctra/RulesReader.java (376)
- [ ] T204b | deps: T204a | RulesWriter + the write halves it needs (Rule, Library.writePadstackScope, Network via/net-class writes, snap angle)
  - spec: SOURCE_OF_TRUTH/plans/M2_ses_io.md (section T204b)
  - java: io/specctra/RulesWriter.java (104), parser/Rule.java 119-343, Library.java 54-100, Network.java 58-192, Structure.java 510-524
- [ ] T204c | deps: T204b | DsnWriter + the remaining parser write halves (Structure, Layer, Library, Package, PartLibrary, Placement, Component, Network, Net, Wiring)
  - spec: SOURCE_OF_TRUTH/plans/M2_ses_io.md (section T204c)
  - java: io/specctra/DsnWriter.java (92) and the writeScope methods listed in plans/M2_ses_io.md
  - done when: DSN → board → DSN round-trips token-equal to Java's DsnWriter output on the 8 E2E boards.

## Phase 2b — Core shakedown (first contact of the ported router with real boards; highest-risk step)

All 874+ unit tests use synthetic fixtures; no real board has ever been routed by the C++ core. Do this before
stacking the rest of the pipeline on top.

- [ ] T611 | deps: T117,T202,T009,T010b,T011b,T506a,T008,T007 | Shakedown: route Issue026, Issue107, Issue034 with the interim BatchAutorouter
  - spec: SOURCE_OF_TRUTH/plans/M3_shakedown.md (section T611)
  - cpp: tools/first_divergence.py (+ fixes in whatever the diff points at)
  - what: run Java and C++ with `-mt 1`, same `-mp`, optimizer off (isolate the router); `first_divergence.py` compares
    the two logs per pass (item order, per-connection result, incomplete count after each pass) and finally the SES.
    Fix the FIRST divergence, repeat. Expect latent bugs in maze search / locator / shove / inserter that synthetic
    fixtures never exercised; file a narrower task (T611a, ...) for anything > ~700 lines.
  - done when: Issue026 routes with #unrouted equal to Java's (stretch: SES token-equal); Issue107 and Issue034
    route something and the first divergence is documented in docs/migration/E2E_STATUS.md.

## Phase 3 — Settings & CLI needed by routing

- [ ] T301 | deps: - | Settings data model: all settings structs (nullable fields), copy_fields, clone, validate; remove the invented classes in OptimizerSettings.h
  - spec: SOURCE_OF_TRUTH/plans/TrackB_settings_cli.md (section T301)
  - java: settings/RouterSettings.java (1042) — C++ src/settings/RouterSettings.cpp is 162 lines
  - split if needed (T301a fields/defaults, T301b per-layer/cost accessors)
- [ ] T302 | deps: T301 | RouterSettings behaviour: getters/setters with Java null fallbacks, per-layer accessors
  - spec: SOURCE_OF_TRUTH/plans/TrackB_settings_cli.md (section T302)
  - java: settings/{FanoutSettings,RoutingCostSettings,LayerSettings,AutorouterSettings,OptimizerScoreSettings,RouterScoreSettings}.java
- [ ] T303 | deps: T302 | SettingsMerger + GlobalSettings load/save (hand-rolled JSON with the `@SerializedName` names)
  - spec: SOURCE_OF_TRUTH/plans/TrackB_settings_cli.md (section T303)
  - java: settings/SettingsMerger.java (199), settings/GlobalSettings.java (996)
- [ ] T304 | deps: T303 | CLI parity with Freerouting.java argument parsing
  - spec: SOURCE_OF_TRUTH/plans/TrackB_settings_cli.md (section T304)
  - known gaps (03_MISSING.md §4): `-dl` sets the wrong field, `set_input()` does not read the file,
    generic `--property=value`, `session.add_job()` commented out
  - flag table to reach Java parity (2026-10-10 review): `-de` (dsn/json/ses/rules, `+`-separated lists; `-de`+`-do`
    forces the router on), `-do` multi-output, `-dr` rules, `-mp` (0..9999, 0 = unlimited; C++ clamps 1..99998, default
    9999), `-mt` (sets **`optimizer.max_threads` only**, clamped 0..1024 — verified GlobalSettings.java 759-770; C++ stores it but ignores it),
    `-oit` (values 0..1 are multiplied by 100, default 2.5; C++ divides by 100 and drops it), `-us/-is/-inc`
    (optimizer strategy, item selection, ignored net classes: TODO no-ops in C++), `-dl` (disables the FILE log only;
    C++ disables the batch autorouter), `-ll`, `-drc`, `--user_data_path`, generic `--router.*=v` / `--section.prop=v`
    setters (C++ only logs them). Keys that matter: `router.autorouter.max_passes|max_threads`,
    `router.optimizer.enabled|max_passes|max_threads|improvement_threshold|timeout`, `router.fanout.enabled`,
    `router.job_timeout`, `router.scoring.*`, `router.result_json`, `router.strict_drc`. Also fix the C++
    bug that any argument containing "help" prints the help text.
  - done when: a table-driven gtest feeds each Java flag spelling to the C++ parser and checks the resulting settings.
- [ ] T306 | deps: T303 | Settings sources and their priority order
  - spec: SOURCE_OF_TRUTH/plans/TrackB_settings_cli.md (section T306)
  - java: settings/sources/{DefaultSettings (237),CliSettings (203),DsnFileSettings (69),RulesFileSettings (109),
    JsonFileSettings (79),EnvironmentVariablesSource (140)}.java, settings/LegacyRouterSettingsBridge.java (126),
    util/ReflectionUtil.java (380)
  - what: priority order as in Java (`freerouting.json` is priority 10); `FREEROUTING__ROUTER__*` /
    `FREEROUTING__LOGGING__*` env vars; replace Java reflection with an explicit field registry that keeps the
    `@SerializedName` names. DsnFileSettings needs T117 (`DsnReader.readMetadata`), RulesFileSettings needs T204a.
  - done when: gtests merge defaults < json < env < dsn < rules < cli and match Java's effective settings.
- [ ] T307 | deps: T302 | RouterSettings.applyBoardSpecificOptimizations + scoring versions
  - spec: SOURCE_OF_TRUTH/plans/TrackB_settings_cli.md (section T307)
  - java: settings/RouterSettings.java (lines ~300-430), RouterScoringVersion.java, OptimizerScoringVersion.java
  - what: this method changes the effective router settings per board (layer count, net count, ...) and therefore the
    routing result; port verbatim.
  - done when: gtests on synthetic boards (2/4/6 layers, few/many nets) equal values taken from the Java harness.
- [ ] T305 | deps: T302 | `fallbackToBoardVias` fanout via-rule combination (needs FanoutSettings + BoardRules via rules)
  - spec: SOURCE_OF_TRUTH/plans/TrackB_settings_cli.md (section T305)

## Phase 4 — Autoroute pipeline (Java autoroute/pipeline ≈ 5.4k lines; C++ BatchAutorouter 352)

- [ ] T401 | deps: - | AutorouteAirlineCalculator
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T401)
  - java: autoroute/pipeline/AutorouteAirlineCalculator.java (214)
- [ ] T402 | deps: T401 | AutorouteConnectionRouter
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T402)
  - java: autoroute/pipeline/AutorouteConnectionRouter.java (270)
- [ ] T403 | deps: T402 | AutoroutePassRunner part 1 (item ordering, per-item routing loop)
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T403)
  - java: autoroute/pipeline/AutoroutePassRunner.java (535, first half)
- [ ] T404 | deps: T403 | AutoroutePassRunner part 2 (ripup escalation, pass result) + AutorouteRuntimeMetrics + AutorouteUnroutedReport
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T404)
  - java: AutoroutePassRunner.java second half, AutorouteRuntimeMetrics.java (66), AutorouteUnroutedReport.java (80)
- [ ] T420 | deps: - | Board deep copy + state hash (history, batch loop and optimizer all need it)
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T420)
  - java: board/facade/RoutingBoardUndoFacade.deepCopy (serialization) + BoardSnapshotManager.getHash (MD5)
  - cpp: src/board/RoutingBoard.{h,cpp}
  - what: `RoutingBoard::deep_copy()` returning an independent board (items, nets, rules, library, components, search
    trees rebuilt) and `state_hash()` — semantic equivalent, a stable hash over the same observable state, not MD5 of
    Java serialization bytes. Used for BoardHistory dedup and per-thread optimizer boards.
  - done when: gtests: the copy is independent (mutating one leaves the other), the hash is stable across copies and
    changes on insert/remove/move.
- [ ] T421 | deps: T302 | Router and optimizer scores (V2_CONTINUOUS / V2_LOWER_BOUND) + BoardStatisticsBounds/BoundsCalculator
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T421)
  - java: core/scoring/BoardStatistics.getRouterScore/getOptimizerScore (V2_CONTINUOUS, V2_LOWER_BOUND), RouterScoreSettings,
    OptimizerScoreSettings; DesignRulesChecker.calculateAllIncompletes
  - cpp: src/core/scoring/BoardStatistics.{h,cpp} (current API is the old calculateScore)
  - done when: scores for synthetic boards equal Java harness values; BatchLoop (T408) and optimizer acceptance can call them.
- [ ] T422 | deps: T403 | RoutingBoard::finish_autoroute + clear_transient_autoroute_state
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T422)
  - java: RoutingBoard.finishAutoroute / clearTransientAutorouteState
- [ ] T407 | deps: T404,T420 | BoardHistory (MAX_HISTORY_SIZE=30; deep-copied boards ranked by router score) + BoardSnapshotManager glue
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T407)
  - java: autoroute/BoardHistory.java (221), board/facade/BoardSnapshotManager.java (101)
  - note: no longer depends on BoardComparator (T405/T406 moved to Phase 7: only `--compare-boards`/debug use it)
- [ ] T408a | deps: T404,T407,T421,T422 | AutorouteBatchLoop part 1: setup, SMD fanout pre-pass, pass loop, restore cycle
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T408a)
  - java: autoroute/pipeline/AutorouteBatchLoop.java run() 40-458 + helpers 662-681
- [ ] T408b | deps: T408a | AutorouteBatchLoop part 2: stagnation detection, fanout recovery, final best-board restore, phase metrics
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T408b)
  - java: AutorouteBatchLoop.java run() 459-661
- [ ] T409 | deps: T408b | BatchAutorouter rewired onto PassRunner + BatchLoop (replaces the "2 no-progress passes" stop)
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T409)
  - java: autoroute/pipeline/BatchAutorouter.java (603)
  - cpp: src/autoroute/BatchAutorouter.{h,cpp}
- [ ] T410 | deps: T409,T305 | BatchFanout part 1
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T410)
  - java: autoroute/pipeline/BatchFanout.java (800, first half)
- [ ] T411 | deps: T410 | BatchFanout part 2
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T411)
- [ ] T412 | deps: T409 | BatchOptimizer part 1 (item ordering, pass structure)
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T412)
  - java: autoroute/pipeline/BatchOptimizer.java (1250, ~400 lines per part)
- [ ] T413 | deps: T412 | BatchOptimizer part 2 (per-item optimize: pull tight + via optimize)
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T413)
- [ ] T414 | deps: T413 | BatchOptimizer part 3 (scoring/acceptance, statistics)
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T414)
- [ ] T415 | deps: T409,T411,T414 | RoutingPipeline + NamedAlgorithm + TaskState; wire into the CLI route path
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T415)
  - java: autoroute/pipeline/{RoutingPipeline,NamedAlgorithm,NamedAlgorithmType,TaskState}.java
  - cpp: also src/Freerouting.cpp (route path)
- [x] T416 | deps: T415 | BatchAutorouterThread — dropped (dead code in the Java headless path)
  - result: dropped. Verified 2026-10-10: `BatchAutorouter.autoroutePass` always calls
    `AutoroutePassRunner.runSingleThread` (called from `AutorouteBatchLoop`); `autoroutePassMultiThread` /
    `runMultiThread` / `BatchAutorouterThread` have no callers. Nothing to port; the single-thread pass is T403/T404.
- [ ] T423 | deps: T414,T420 | Optimizer thread pool (`-mt N`)
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T423)
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

- [ ] T506a | deps: - | ShapeSearchTree45Degree: completeShape/restrainShape/divideLargeRoom/calculateTreeShapes; SearchTreeManager picks the subclass
  - spec: SOURCE_OF_TRUTH/plans/Phase5_board_gaps.md (section T506a)
  - finding: Java uses ShapeSearchTree45Degree as the autoroute tree on 45° boards (almost all test boards) and 90Degree on
    90° boards (SearchTreeManager.java 140-160); in C++ both subclasses are empty and never instantiated, so room
    completion runs the generic algorithm where Java runs the 45° one.
  - java: board/searchtree/ShapeSearchTree45Degree.java (667)
- [ ] T506b | deps: T506a | ShapeSearchTree90Degree (IntBox rooms, offsetShapes)
  - spec: SOURCE_OF_TRUTH/plans/Phase5_board_gaps.md (section T506b)
  - java: board/searchtree/ShapeSearchTree90Degree.java (523)
- [ ] T501 | deps: - | Drill-hole obstacles + clearance in ShapeSearchTree / HeadlessBoardManager (Java Via/Pin/Component hole insertion)
  - spec: SOURCE_OF_TRUTH/plans/Phase5_board_gaps.md (section T501)
  - source: "Next candidates" after WP-10 Inc U/V in 05_WORK_PACKAGES.md (read that section only)
- [ ] T502 | deps: - | Pin parity audit (Java 706 lines vs C++ 557): exit restrictions, per-layer shapes, swap info
  - spec: SOURCE_OF_TRUTH/plans/Phase5_board_gaps.md (section T502)
  - java: board/model/items/Pin.java
- [ ] T503 | deps: - | DrillItemMover
  - spec: SOURCE_OF_TRUTH/plans/Phase5_board_gaps.md (section T503)
  - java: board/actions/DrillItemMover.java (326)
- [ ] T504 | deps: - | ShapeEntrySide / ShapeAndEntrySide audit (verify against ShapeTraceEntries / CalcFromSide; port what is absent)
  - spec: SOURCE_OF_TRUTH/plans/Phase5_board_gaps.md (section T504)
  - java: board/model/structure/{ShapeEntrySide,ShapeAndEntrySide}.java
- [ ] T505 | deps: - | Board facade audit: BoardConnectivityQueries, BoardItemRepository, RoutingBoardOperations, RoutingBoardSearchFacade, RoutingBoardUndoFacade
  - spec: SOURCE_OF_TRUTH/plans/Phase5_board_gaps.md (section T505)
  - what: for each Java public method find the C++ RoutingBoard/BasicBoard equivalent; port the small
    gaps here, file tasks for the big ones.

## Phase 6 — End-to-end parity, one board per task

For each board: run `tools/e2e_compare.sh`, find the **first** divergence from Java (crash, missing
connections, wrong SES), fix it or file narrower tasks (T6xxa, …). Done when C++ routes the board with
#unrouted equal to Java and a valid SES (wire/via counts within ±10% of Java). Update E2E_STATUS.md.

- [ ] T601 | deps: T001,T117,T202,T415,T008,T611,T506a | E2E: empty_board.dsn
  - spec: SOURCE_OF_TRUTH/plans/M5_e2e.md (section T601)
- [ ] T602 | deps: T601 | E2E: Issue034-Green14SegLED.dsn
  - spec: SOURCE_OF_TRUTH/plans/M5_e2e.md (section T602)
- [ ] T603 | deps: T601 | E2E: Issue107-freq_teiler_200kHz_kicad.dsn
  - spec: SOURCE_OF_TRUTH/plans/M5_e2e.md (section T603)
- [ ] T604 | deps: T601 | E2E: Issue026-J2_reference.dsn
  - spec: SOURCE_OF_TRUTH/plans/M5_e2e.md (section T604)
- [ ] T605 | deps: T601 | E2E: Issue103-Board-Unrouted.dsn
  - spec: SOURCE_OF_TRUTH/plans/M5_e2e.md (section T605)
- [ ] T606 | deps: T601 | E2E: Issue006-LPC18XX_43XX_SCH.dsn (binary OLE file: parity = both report the same load error)
  - spec: SOURCE_OF_TRUTH/plans/M5_e2e.md (section T606)
- [ ] T607 | deps: T601 | E2E: Issue070-Autorouter_FQ101_PCB_2022-05-13.dsn
  - spec: SOURCE_OF_TRUTH/plans/M5_e2e.md (section T607)
- [ ] T608 | deps: T601 | E2E: BBD_Mars-64.dsn
  - spec: SOURCE_OF_TRUTH/plans/M5_e2e.md (section T608)
- [ ] T609 | deps: T608 | E2E: Issue029-hw48na.dsn (the original "19% headless" benchmark)
  - spec: SOURCE_OF_TRUTH/plans/M5_e2e.md (section T609)
- [ ] T610 | deps: T609 | E2E sweep over all 117 `tests/data/**/*.dsn`
  - spec: SOURCE_OF_TRUTH/plans/M5_e2e.md (section T610)
  - what: load parity (T118 results) for every DSN; route every board Java finishes within 60 s and record
    pass/fail + unrouted counts in E2E_STATUS.md. The DAC2020 `bm01-bm11` set (5-80 KB) is the best bit-exact corpus.

## Phase 7 — DRC, jobs, scoring stragglers, final docs

- [ ] T405 | deps: - | BoardComparator part 1 (`--compare-boards`; moved here from Phase 4, not on the route path)
  - spec: SOURCE_OF_TRUTH/plans/M7_stragglers.md (section T405)
  - java: board/state/BoardComparator.java (758, first half)
- [ ] T406 | deps: T405 | BoardComparator part 2
  - spec: SOURCE_OF_TRUTH/plans/M7_stragglers.md (section T406)
- [ ] T701 | deps: T117 | CLI DRC mode calls DesignRulesChecker; DrcReport JSON field names match Java
  - spec: SOURCE_OF_TRUTH/plans/M7_stragglers.md (section T701)
- [ ] T702 | deps: T303 | RoutingJobScheduler + RoutingJobSchedulerActionThread (headless, synchronous)
  - spec: SOURCE_OF_TRUTH/plans/M7_stragglers.md (section T702)
  - java: management/jobs/*.java (986)
- [ ] T703 | deps: T002,T421 | Scoring stragglers: BenchmarkScoreCalculator (`--calculate-benchmark-scores`), BoardScoreBreakdown, ScoringWeightComparison
  - spec: SOURCE_OF_TRUTH/plans/M7_stragglers.md (section T703)
- [ ] T704 | deps: T415,T421,T303 | core/results RoutingResultManifest (`--router.result_json`; the benchmark's per-phase metrics channel on BOTH sides, T801 depends on it)
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T704)
  - java: core/results/RoutingResultManifest.java (256)
- [ ] T706 | deps: T702,T704 | RoutingJob phase metrics + resource usage
  - spec: SOURCE_OF_TRUTH/plans/TrackC_pipeline.md (section T706)
  - java: core/RoutingJob.java (resultPhaseMetrics, log plumbing, stats JSON at RoutingJob.java:662-690), RouterJobResourceUsage
  - what: C++ `RouterJobResourceUsage` exists but nothing populates it. Fill cpu_time_seconds and peak_memory_mb
    (getrusage), record per-phase wall times (load / fanout / route / optimize / write) in the manifest.
- [ ] T705 | deps: T601,T602,T603,T604,T605,T606,T607,T608,T609,T610,T006 | Final docs sync: regenerate 02_PROGRESS/03_MISSING/INDEX from the CSV + E2E_STATUS; archive WP-10 increment logs to docs/migration/history/
  - spec: SOURCE_OF_TRUTH/plans/M7_stragglers.md (section T705)

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
  - spec: SOURCE_OF_TRUTH/plans/M8_benchmark.md (section T801)
  - cpp: tools/bench/bench.py (stdlib python only: no hyperfine/jq/perf on this machine)
  - done when: Java-vs-Java run on a small board gives a ratio ~1.0 with small spread (harness sanity), results in
    results/<date>/<board>.<impl>.<mode>.json with machine info (nproc, RAM, JDK, g++, flags, commit SHA).
  - note: C++ per-phase times need T704/T706; until then record process-level numbers only.
- [ ] T802 | deps: T801 | Neutral quality scorer
  - spec: SOURCE_OF_TRUTH/plans/M8_benchmark.md (section T802)
  - what: every produced SES (either implementation) goes through `java -jar <jar> -de x.dsn x.ses -drc out.json`;
    extract qualityScore, optimizerScore, unrouted, clearance violations, wire length, via count. After T203/T701 add the
    C++ DRC as a cross-check (both scorers must agree).
- [ ] T803 | deps: T801 | Tier definitions + per-tier reps/timeouts (smoke/tiny/small/medium/large/stress) in tools/bench/tiers.json
  - spec: SOURCE_OF_TRUTH/plans/M8_benchmark.md (section T803)
- [ ] T804 | deps: T802,T803 | Reporter: docs/migration/BENCHMARK.md + CSV (median/min/max, speed ratio, quality delta, memory ratio)
  - spec: SOURCE_OF_TRUTH/plans/M8_benchmark.md (section T804)
- [ ] T805 | deps: T804 | tools/bench/check_regression.py: fail when C++ quality < Java's or time regresses > X % vs stored baseline
  - spec: SOURCE_OF_TRUTH/plans/M8_benchmark.md (section T805)
- [ ] T806 | deps: T801 | Optional: warm-JVM measurement (compiled harness that routes N times in one JVM) to separate JIT warm-up from cold CLI runs
  - spec: SOURCE_OF_TRUTH/plans/M8_benchmark.md (section T806)
- [ ] T807 | deps: T610 | Profile the C++ router on a medium + a large board (valgrind/callgrind or `-pg`; `perf` is not installed), list hot spots
  - spec: SOURCE_OF_TRUTH/plans/M8_benchmark.md (section T807)
- [ ] T808 | deps: T807 | CMake preset `release-perf` (-O3 -march=native -flto, optional mimalloc); never used for parity gates
  - spec: SOURCE_OF_TRUTH/plans/M8_benchmark.md (section T808)
- [ ] T809 | deps: T807 | Profile-driven optimizations, each guarded by the T6xx parity suite (output must not change)
  - spec: SOURCE_OF_TRUTH/plans/M8_benchmark.md (section T809)
- [ ] T810 | deps: T809,T804,T423 | Final benchmark run (st + mt, all tiers that pass G4) and write-up in docs/migration/BENCHMARK.md
  - spec: SOURCE_OF_TRUTH/plans/M8_benchmark.md (section T810)

---

## State at plan time (2026-10-08)

- Build: `build/` reconfigured in this WSL checkout (earlier loop runs used `/mnt/d/projects/EDA/freerouting_cpp`).
  `run_tests` is now built by default; **ctest 761/761 green** at commit f25920f.
- Done and solid: geometry, rules, datastructures, board model, maze search / expansion rooms /
  connection locator / inserter / shove / fanout / `RoutingBoard::autoroute` (WP-1 … WP-10 Inc V).
- Biggest remaining gaps, by Java lines: DSN parser (~9k), autoroute pipeline (~5k), settings (~3k),
  SES writer (~0.5k + write paths), BoardComparator/history (~1k).
- `JavaToCppClassMap.csv` reports 171 "missing" but many are renamed ports (fixed by T002).
