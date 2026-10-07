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

- [ ] T001 | deps: - | E2E comparison tool
  - cpp: tools/e2e_compare.sh, tools/e2e_all.sh, docs/migration/E2E_STATUS.md
  - what: `e2e_compare.sh <board.dsn>` runs `build/freerouting -de <dsn> -do <out.ses>` with a timeout
    (default 300s), then prints one line: board, exit code, seconds, #wires, #vias in the C++ SES vs the
    Java SES `java_output_ses/<base>_java.ses` (count `(wire` / `(via` inside `(network_out`), "n/a" when
    a baseline is missing. `e2e_all.sh` runs it over the boards listed in `test_java_boards.sh` and
    rewrites `docs/migration/E2E_STATUS.md` as a table with the date.
  - done when: both scripts run on all 8 boards without aborting (C++ crashes/timeouts are reported as
    rows, not fatal) and E2E_STATUS.md exists with today's numbers.
- [ ] T002 | deps: - | Reconcile JavaToCppClassMap.csv with the real C++ names
  - what: the CSV is name-based and wrong for renamed ports (MazeSearchEngine→MazeSearchAlgo,
    TraceShover→ShoveTraceAlgo, TraceTightener*→PullTightAlgo*, ViaOptimizer→OptViaAlgo,
    ForcedViaInserter→ForcedViaAlgo, ForcedPadRouter→ForcedPadAlgo, IntPoint/FloatPoint→Point.h,
    Line→Vector.h, ExpansionDoor→ExpansionRoom.h, ShapeSearchTree45/90Degree→ShapeSearchTree.h, …).
    Write `tools/class_map_check.py` that checks each row's `cpp_header` exists and contains
    `class|struct <cpp_class>`, and reports mismatches. Fix rows by hand. Mark Java classes that are
    intentionally out of scope (gui/api/analytics/io.kicad/util.gson/settings.sources UI bits) as
    `out_of_scope`.
  - done when: script exits 0; README "Quick Status" counts regenerated from the CSV.
- [ ] T003 | deps: - | Fix the known `TileShape::cutout(const TileShape&)` inverted branches
  - java: geometry/planar/TileShape.java `cutout` + Simplex/IntOctagon/IntBox `cutoutFrom`
  - cpp: src/geometry/planar/TileShape.cpp
  - what: documented latent bug (03_MISSING.md §2.1 "Known latent bug"): when the hole is a Simplex and
    the dividing shape is IntOctagon/IntBox the C++ returns hole-minus-divide instead of Java's result.
  - done when: gtests for all 9 hole/divide type combinations match the Java semantics; ctest green.
- [ ] T004 | deps: - | Clean `-Wunused-*` warnings (Item.h, ClearanceMatrix.cpp, SearchTreeManager.cpp, FRLogger.cpp)
  - done when: a clean full build prints zero warnings from `src/`; no behaviour change.

## Phase 1 — DSN reader parity (`io/specctra/parser`, Java ≈ 11.4k lines, C++ ≈ 2.2k)

Theme: every `tests/data/*.dsn` loads; board counts (layers, nets, components, pins, padstacks,
net classes, keepouts) equal Java's. T110 adds the count dump used as acceptance later.

- [ ] T101 | deps: - | Keyword table + ScopeKeyword parity
  - java: io/specctra/parser/Keyword.java (131), ScopeKeyword.java (82)
  - cpp: src/designforms/specctra/Keyword.{h,cpp}
  - done when: every Java keyword constant exists with the same spelling; gtest covers the full table.
- [ ] T102 | deps: T101 | Lexer: SpecctraDsnStreamReader behaviour
  - java: io/specctra/parser/SpecctraDsnStreamReader.java (1862, flex-generated — port the *behaviour*:
    token kinds, NAME vs IDENTIFIER states, `string_quote` switching, parentheses, numbers), IJFlexScanner.java (38)
  - cpp: src/designforms/specctra/Scanner.{h,cpp}
  - done when: gtests on tricky snippets (quoted names, names with `-`/`.`/`/`, string_quote change,
    negative/exponent numbers) and a smoke test that tokenizes every `tests/data/*.dsn` without error.
- [ ] T103 | deps: T102 | Resolution / Unit / CoordinateTransform in the parser
  - java: parser/Resolution.java (73), parser/Unit.java (27), board/state/CoordinateTransform.java (112)
  - done when: DSN→board unit conversion matches Java for mil/inch/mm/um resolutions (gtest).
- [ ] T104 | deps: T102 | Shape reader part 1: Shape dispatch, Rectangle, Circle
  - java: parser/Shape.java (646, read_scope / read_area part), Rectangle.java (96), Circle.java (91)
  - cpp: src/designforms/specctra/{Shape,Rectangle,Circle}.{h,cpp}
- [ ] T105 | deps: T104 | Shape reader part 2: Polygon, Path, PolygonPath, PolylinePath, areas with holes/window
  - java: parser/Polygon.java (111), Path.java (24), PolygonPath.java (131), PolylinePath.java (73), rest of Shape.java
- [ ] T106 | deps: T103 | Rule reader (width, clearance + types) and Read/WriteScopeParameter
  - java: parser/Rule.java (343), ReadScopeParameter.java (187), WriteScopeParameter.java (38)
- [ ] T107 | deps: T105,T106 | Structure part 1: layers, LayerStructure, boundary, planes
  - java: parser/Structure.java (layer/boundary/plane sections), Layer.java (67), LayerStructure.java (69), Plane.java (84)
  - cpp: src/designforms/specctra/Structure.{h,cpp}
- [ ] T108 | deps: T107 | Structure part 2: keepout / via_keepout / wire_keepout, via list, snap_angle, control
  - java: parser/Structure.java remaining read sections
- [ ] T109 | deps: T108 | Structure part 3: autoroute settings scope + create_board (outline, keepouts → board)
  - java: parser/AutorouteSettings.java (243), Structure.java create_board / insert_* parts
- [ ] T110 | deps: T105,T106 | Library part 1: padstacks → board Padstacks; board-count dump
  - java: parser/Library.java (padstack part), core/library/Padstacks.java (84)
  - also: add a test helper (or `freerouting --dump-board-counts <dsn>`) printing layer/net/component/
    pin/padstack/netclass/keepout counts; used as acceptance by T111–T118.
- [ ] T111 | deps: T110 | Library part 2: images / Package + PartLibrary + LogicalParts + BoardLibrary
  - java: parser/Package.java (425), PartLibrary.java (351), core/library/{Packages,LogicalParts,BoardLibrary}.java
- [ ] T112 | deps: T111 | Placement: Component, ComponentPlacement, PlaceControl
  - java: parser/Component.java (381), ComponentPlacement.java (92), PlaceControl.java (73), Placement.java (29)
- [ ] T113 | deps: T112 | Network part 1: nets, NetList, pin assignment
  - java: parser/Net.java (129), NetList.java (56), Network.java (net read part)
- [ ] T114 | deps: T113 | Network part 2: net classes, class pairs, via rules → BoardRules
  - java: parser/NetClass.java (197), Network.java (class part), rules/{NetClasses,Nets,ViaInfos}.java
- [ ] T115 | deps: T114 | Network part 3: insert components and pins into the board
  - java: parser/Network.java (insert components / pins part)
- [ ] T116 | deps: T115 | Wiring read: wires, vias, fixed states, polyline paths
  - java: parser/Wiring.java (715, read part)
- [ ] T117 | deps: T109,T116 | DsnFile.read orchestration + DsnReader + BoardParserCallback + Circuit + Parser
  - java: parser/DsnFile.java (220), io/specctra/DsnReader.java (291), parser/BoardParserCallback.java (42),
    parser/Circuit.java (143), parser/Parser.java (228)
  - done when: every `tests/data/*.dsn` loads without error through the CLI path; counts match Java
    for the 8 E2E boards (record them in docs/migration/MIGRATION_designforms.md).
- [ ] T118 | deps: T117 | DSN reader audit vs Java on all test boards; file a task per mismatch

## Phase 2 — SES writer parity (Java SesWriter 552 + write paths; C++ SessionWriter 88)

- [ ] T201 | deps: T117 | SesWriter part 1: session header, base_design, placement, was_is
  - java: io/specctra/SesWriter.java (552, first half)
  - cpp: src/designforms/specctra/SessionWriter.{h,cpp}
- [ ] T202 | deps: T201 | SesWriter part 2: routes, library_out (via padstacks), network_out (wires, vias, fixed)
  - java: SesWriter.java second half, parser/Wiring.java write part, Shape write part
  - done when: `Issue103-Board-Routed.dsn` loaded and written without routing gives an SES token-equal
    to Java's (generate the Java SES once with the jar, store it in tests/data/expected/).
- [ ] T203 | deps: T202 | SesReader (enables round-trip tests)
  - java: io/specctra/SesReader.java (413), SesImportSummary.java (15)
- [ ] T204 | deps: T202 | DsnWriter + RulesWriter + RulesReader (lower priority)
  - java: io/specctra/{DsnWriter,RulesWriter,RulesReader}.java

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
- [ ] T405 | deps: - | BoardComparator part 1
  - java: board/state/BoardComparator.java (758, first half)
- [ ] T406 | deps: T405 | BoardComparator part 2
- [ ] T407 | deps: T406 | BoardHistory (board snapshots, BOARD_RANK_LIMIT=50) + BoardSnapshotManager
  - java: autoroute/BoardHistory.java (221), board/facade/BoardSnapshotManager.java (101)
- [ ] T408 | deps: T404,T407 | AutorouteBatchLoop (convergence, STOP_AT_PASS_MINIMUM/MODULO, restore best board)
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
- [ ] T416 | deps: T415 | BatchAutorouterThread (headless, single-threaded execution faithful to Java order)
  - java: autoroute/pipeline/BatchAutorouterThread.java (623)

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

- [ ] T601 | deps: T001,T117,T202,T415 | E2E: empty_board.dsn
- [ ] T602 | deps: T601 | E2E: Issue034-Green14SegLED.dsn
- [ ] T603 | deps: T601 | E2E: Issue107-freq_teiler_200kHz_kicad.dsn
- [ ] T604 | deps: T601 | E2E: Issue026-J2_reference.dsn
- [ ] T605 | deps: T601 | E2E: Issue103-Board-Unrouted.dsn
- [ ] T606 | deps: T601 | E2E: Issue006-LPC18XX_43XX_SCH.dsn
- [ ] T607 | deps: T601 | E2E: Issue070-Autorouter_FQ101_PCB_2022-05-13.dsn
- [ ] T608 | deps: T601 | E2E: BBD_Mars-64.dsn
- [ ] T609 | deps: T608 | E2E: Issue029-hw48na.dsn (the original "19% headless" benchmark)

## Phase 7 — DRC, jobs, scoring stragglers, final docs

- [ ] T701 | deps: T117 | CLI DRC mode calls DesignRulesChecker; DrcReport JSON field names match Java
- [ ] T702 | deps: T303 | RoutingJobScheduler + RoutingJobSchedulerActionThread (headless, synchronous)
  - java: management/jobs/*.java (986)
- [ ] T703 | deps: T002 | Scoring stragglers: BenchmarkScoreCalculator, BoardScoreBreakdown, ScoringWeightComparison, BoardStatistics* records (check WP-4 first)
- [ ] T704 | deps: T415 | core/results RoutingResultManifest
  - java: core/results/RoutingResultManifest.java (256)
- [ ] T705 | deps: T601,T602,T603,T604,T605,T606,T607,T608,T609 | Final docs sync: regenerate 02_PROGRESS/03_MISSING/INDEX from the CSV + E2E_STATUS; archive WP-10 increment logs to docs/migration/history/

---

## State at plan time (2026-10-08)

- Build: `build/` reconfigured in this WSL checkout (earlier loop runs used `/mnt/d/projects/EDA/freerouting_cpp`).
  `run_tests` is now built by default; **ctest 761/761 green** at commit f25920f.
- Done and solid: geometry, rules, datastructures, board model, maze search / expansion rooms /
  connection locator / inserter / shove / fanout / `RoutingBoard::autoroute` (WP-1 … WP-10 Inc V).
- Biggest remaining gaps, by Java lines: DSN parser (~9k), autoroute pipeline (~5k), settings (~3k),
  SES writer (~0.5k + write paths), BoardComparator/history (~1k).
- `JavaToCppClassMap.csv` reports 171 "missing" but many are renamed ports (fixed by T002).
