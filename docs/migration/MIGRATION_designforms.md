# DesignForms (Specctra) Module - Migration Status

**Module**: `src/designforms/specctra/`
**CMake Target**: N/A (commented out in CMakeLists.txt)
**Java Files**: 41 | **C++ Headers**: 12 | **C++ Impls**: 9 | **Tests**: 0

---

## Completed (Fully Migrated)

| Java Class | C++ Header | C++ Impl | Test | Java Lines | Notes |
|------------|-----------|----------|------|-----------|-------|
| Circle | Circle.h | Circle.cpp | - | 85 | Geometry circle |

## Partially Migrated

| Java Class | C++ Header | C++ Impl | Java Lines | What's Missing |
|------------|-----------|----------|-----------|----------------|
| DsnFile | DsnFile.h | - | 321 | Implementation needed for DSN parsing |
| Keyword | Keyword.h | - | 133 | Tokenization implementation |
| Library | Library.h | Library.cpp | 310 | Parses image pins and padstack extents for board pin geometry; full padstack shape retention remains pending |
| Network | Network.h | - | N/A | Network section parsing |
| Path | Path.h | - | 29 | Implementation needed |
| Placement | Placement.h | - | N/A | Placement section parsing |
| Polygon | Polygon.h | Polygon.cpp | 109 | Full implementation |
| Scanner | Scanner.h | Scanner.cpp | N/A | Tokenizer |
| SessionWriter | SessionWriter.h | SessionWriter.cpp | N/A | Session output |
| Structure | Structure.h | - | 1152 | Structure section parsing |
| Unit | Unit.h | Unit.cpp | 29 | Already in board module |
| Wiring | Wiring.h | - | 594 | Wiring section parsing |

## Not Migrated

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| AutorouteSettings | 220 | HIGH | Autoroute configuration parsing |
| Circuit | 132 | MED | Circuit section |
| Component | 312 | MED | Component section |
| ComponentPlacement | 82 | MED | Placement data |
| CoordinateTransform | 198 | MED | Coordinate conversion |
| Layer | 66 | MED | Layer definition |
| LayerStructure | 77 | MED | Layer structure |
| Net | 125 | HIGH | Net definition |
| NetClass | 167 | HIGH | Net class definition |
| PlaceControl | 73 | MED | Place control settings |
| Plane | 78 | MED | Plane definition |
| PolygonPath | 132 | MED | Polygon path |
| PolylinePath | 77 | MED | Polyline path |
| ReadScopeParameter | 90 | MED | Scope parameter reading |
| Rectangle | 97 | MED | Rectangle geometry |
| Resolution | N/A | MED | Resolution settings |
| Rule | 320 | HIGH | Design rules |
| RulesFile | 231 | MED | Rules file |
| ScopeKeyword | 80 | MED | Scope keyword |
| Shape | 574 | HIGH | Shape definitions |
| PartLibrary | 314 | MED | Part library |

### SES/Parser Files (Not Migrated)

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| Parser | 198 | HIGH | DSN parser core |
| SesFileReader | 389 | HIGH | SES file reader |
| SessionToEagle | 593 | LOW | Eagle session conversion |
| SpecctraDsnStreamReader | 1467 | HIGH | DSN stream reader (critical) |
| SpecctraSesFileWriter | 416 | HIGH | SES file writer (critical) |
| WriteScopeParameter | N/A | MED | Scope parameter writing |

## Summary

- **Done**: 1 class (2% of Java files)
- **Partial**: 12 classes (29%)
- **Remaining**: 28 classes (68%)
- **Blockers**: SpecctraDsnStreamReader (1467 lines) and SpecctraSesFileWriter (416 lines) are **critical** for full DSN-load route SES-write parity. The current Scanner pipeline now supports the basic headless path and carries padstack extents into board pins, but full padstack shapes and complete rule/network scope handling remain pending.

- **2026-10-08 (found during T001/T417) — headless blocker**: `RoutingBoard::create_pins_from_parsed`
  places all pins of a component on the component origin as 100x100 boxes in raw DSN units (no
  resolution scaling, image pin offsets not found), e.g. all J2 pins of Issue034 at (60960,-53340).
  Pins of different nets therefore overlap, `ShapeSearchTree::complete_shape` restrains every maze
  start room to nothing, and every board routes 0 connections. Fix = T110–T115 (padstacks, images,
  placement, Java CoordinateTransform with `bounds.offset(1000)`).
- Issue006-LPC18XX_43XX_SCH.dsn is a binary OLE document, not a DSN; the parse error is correct.
- **2026-10-08 (T101/T102)**: new parser package `src/designforms/specctra/parser/` (namespace
  `specctra::parser`), built next to the old reader. Lexer `SpecctraDsnStreamReader` matches the Java
  lexer token-for-token on all 76 test boards (`build/dsn_token_dump` + `tools/dsn_token_compare.py`;
  the Java reference dumps come from a throwaway harness over the 2.1.0 jar). Facts verified against the
  shipped jar, not the 2008 `.flex`: illegal characters are logged and skipped (so `+BATT` lexes as
  `BATT`), `|`, `` ` ``, `{`, `}`, backspace and non-Latin letters are illegal, `007` is a Double.
- **2026-10-08 (T103-T106)**: parser `Resolution`, `CoordinateTransform` (numeric part), `Layer`/`LayerStructure`,
  `Shape` readers (`Rectangle`, `Circle`, `Polygon`, `PolygonPath`, `PolylinePath`, areas with windows),
  `DsnFile` scope helpers and `Rule` (width/clearance/layer_rule) ported with tests. Next: T120 (BoardRules
  parity) and T121 (BasicBoard insert facade) are prerequisites of the Structure/Library/Network scopes.
- **2026-10-09 (T107)**: parser `Structure` and `Plane` ported in `parser/` (namespace
  `specctra::parser`), and `Keyword::STRUCTURE_SCOPE` / `Keyword::PLANE_SCOPE` are now the real
  `Structure` / `Plane` classes (virtual `read_scope` dispatch via the existing `ScopeKeyword` base,
  like `Resolution`). `Structure` reads the `layer` scopes (type signal/power/jumper, `use_net`,
  layer rules), the `boundary` scopes (`bounding_shape` for pcb, polyline/polygon paths and signal
  shapes into `outline_shapes`, `clearance_class`) and default `rule` scopes into its
  `BoardConstructionInfo`, and it reads `plane` scopes into `ReadScopeParameter::plane_list` (via
  `Plane`, which also reproduces the Allegro `skip_window_scopes` quirk). Layers already existed from
  T103/T104; `ReadScopeParameter` gained `layer_structure`, `via_padstack_names` and `plane_list`.
  `layer_structure` is only materialised when a plane/keepout needs it (Java parity — after only layer
  scopes it stays null). Java's final `createBoard` + keepout/plane/insert + autoroute-settings block
  of `Structure.read_scope` is intentionally deferred to T108/T109; the remaining structure scopes
  (`keepout`, `via_keepout`, `place_keepout`, `via` list, `snap_angle`, `control`, `flip_style`,
  `autoroute_settings`) are `skip_scope`d for now. Structure_test 13 tests.
- **2026-10-10 (T108)**: `Structure` now reads the remaining non-board-creation scopes of the structure
  scope: the `via` padstack name list (`read_via_padstacks`, `(spare ...)` sub-scopes appended at the
  end, unknown sub-scopes skipped), `snap_angle` (`read_snap_angle` → `ReadScopeParameter::snap_angle`,
  default `FORTYFIVE_DEGREE`, invalid token leaves the default), `control` (`read_control_scope` →
  `via_at_smd_allowed`; unknown sub-scopes skipped) and `keepout` / `via_keepout` / `place_keepout`
  areas (`Shape::read_area_scope` results collected in `ReadScopeParameter::keepout_list`,
  `via_keepout_list`, `place_keepout_list`). Note the lexer maps `wire_keepout` to `Keyword::KEEPOUT`,
  so the KEEPOUT branch covers it. `via_padstack_names` became `std::optional` to preserve Java's
  null-vs-empty distinction (read by `Network` later). Java's `createBoard` + keepout/plane insertion +
  `autoroute_settings` + `flip_style` remain deferred to T109/T112. Structure_test 23 tests
  (+10). ctest 874/874, run_tests PASSED.
- **2026-10-10 (T109a)**: `parser/AutorouteSettings` ported (`read_scope`, `read_layer_rule`, `write_scope`;
  floats are written like Java `Float.toString`, i.e. always with a fraction part) and hooked into
  `Structure::read_scope` → `ReadScopeParameter::autoroute_settings` (`std::shared_ptr<freerouting::settings::RouterSettings>`,
  nullptr = Java null). Java parity kept: the scope is only consumed while `layer_structure` is still null
  (before the first keepout/plane scope); afterwards its tokens are not consumed by that branch. `RouterSettings`
  gained `run_router` / `run_optimizer` (Java defaults true/false in the class, but the reader sets both from the
  `autoroute`/`postroute` scopes, default on), `plane_via_costs`, and the per-layer/cost setters with Java clamps
  (via/plane-via/ripup costs >= 1, trace costs >= 0.1; `set_layer_count` resets layer state and the preferred
  direction defaults to `layer % 2 == 1`). **Build-graph change:** `freerouting_settings` no longer links
  `freerouting_autoroute` (it only uses the header-only `ExpansionCostFactor.h`; the link edge made
  designforms → settings → autoroute → {designforms,…} a cycle in the RESCAN link groups) and
  `freerouting_settings` is listed in every RESCAN group that contains `freerouting_designforms`
  (top-level `dsn_token_dump`, tests `DESIGNFORMS_TEST_SOURCES`, the board-loader test group). Rule of thumb for
  the next parser tasks: a new library that designforms depends on must be added to those groups, not only to
  `target_link_libraries`. T109 was split: T109b (rules from the structure) and T109c (`createBoard`, keepout/
  plane insertion, `adjustPlaneAutorouteSettings`) remain. AutorouteSettings_test 13 tests. ctest 887/887
  (incl. AllTests/`run_tests`, which lives at `build/tests/data/run_tests`).
- **2026-10-10 (queue review)**: gap analysis against the whole headless path added tasks T006-T010 (class-map
  `pending` status, Java reference harness, Java baseline runner, LCG/normalize and Delaunay determinism fixes),
  T125 (Circuit + Parser moved out of T117; NetClass.read and Plane need them), T122 (WriteScopeParameter and the
  shape write halves — T106 was ticked without them), T124 (delete the legacy reader), T204a/b, T306/T307
  (settings sources, board-specific optimisations), T420-T423 (board deep copy + hash, router/optimizer scores,
  finish_autoroute, optimizer thread pool), T611 (core shakedown on real boards), T610, T706 and Phase 8
  (benchmark). T416 closed as dead code; BoardComparator (T405/T406) moved to Phase 7.
- **2026-10-10 (T007)**: Java reference harness (the G2 oracle, also items-order oracle for T011/T611).
  `tools/java_ref/BoardCounts.java` calls `DsnReader.readBoard(new FileInputStream(dsn), null, null,
  fileName)` — exactly the CLI path (Freerouting.java:1016) — and renders hand-written JSON (no lib):
  `file`, `result` (`success|outline_missing|parse_error|io_error`), `error`, `layers,[{name,signal}]`,
  `counts` (nets=maxNetNumber, net_classes, via_infos, via_rules, padstacks, packages, components, pins,
  vias, traces, conduction_areas, obstacle_areas(exact `ObstacleArea` class), via_obstacle_areas,
  component_obstacle_areas, component_outlines, board_outline), `clearance_classes`, `clearance_matrix`
  `[class][class][layer]` via `getValue(i,j,l,false)` (no safety margin), `items_omitted`, `items`
  (in `getItems()` descending-id order; id/type(`getSimpleName`)/layers/fixed(`getFixedState().name()`),
  Pin adds component+pin, PolylineTrace adds corners), `warnings`. `build.sh` compiles against the
  executable jar — jar is Java-25 classfiles (major 69), so if env `JAVA_HOME` is an older JDK (this box:
  21) it must be overridden via `JAVA25_HOME` (default `/home/anwer/.local/jdk-25.0.4.1+1`).
  `run_all.sh` loops `find tests/data -name '*.dsn'` (117) with `timeout 120` each, writes
  `tests/data/expected/<rel>.counts.json`; re-runs with `--omit-items` when a file exceeds 2 MB
  (triggered once: Issue289 FHT-VGA). Output goes to a file argument so stdout log noise cannot corrupt
  it (fallback: `===JSON===` marker). Load result facts to keep T117 honest: Issue006-LPC18XX is a binary
  OLE file → `parse_error` at the `(pcb` header (Java has no baseline either); empty_board → 1
  BoardOutline + 0 pins; nets count is `rules.nets.maxNetNumber()` (== registered net vector size), so
  boards whose nets are sparse still get the vector length, matching T011 expectations. Verified
  deterministic (bit-identical re-run) and descending-id item order. Run: `tools/java_ref/run_all.sh`
  (JAVA25_HOME/FREEROUTING_JAR optional). ctest 887/887. This sets the exact JSON contract for
  `freerouting --dump-board-counts` (T117); the C++ reader must emit the same schema and per-type counts
  (`obstacle_areas` counts only the plain `ObstacleArea` concrete class).
- **2026-10-10 (T008)**: Java baselines for the 8 E2E boards via `tools/java_baseline.sh --tier e2e --mode st`
  (`docs/migration/baseline.json`). Issue103-Board-Unrouted uses the full 30 min job limit in Java (optimizer cut short,
  state still COMPLETED) — treat its baseline as wall-clock limited; use a longer `--timeout` or optimizer off for
  parity comparisons on that board.
- **2026-10-10 (T109b)**: structure rules → `BoardRules` (`Structure::update_board_rules` / `set_clearance_rule` and
  helpers). Verified against Java `Structure.setClearanceRule` output for default/layer clearances, `wire_*` pairs
  (creates via/smd/pin/area classes and their default item clearance classes), `smd_to_turn_gap`, quoted names and the
  2-element pair form. Note Java's comment "pairs with more than 1 underline are not implemented" is wrong: the code
  splits at the first `_`, so `smd_via_same_net` creates classes `smd` and `via_same_net` — reproduced.
- **2026-10-10 (T109c)**: the structure scope now creates the board (`Structure::create_board` via
  `MinimalBoardManager`), inserts outline holes, keepouts, planes and missing power planes. For a test DSN with all of
  these, the C++ item list (ids, types, layers, nets) equals the Java reference harness output. Layering choices: the
  board-side `Communication` stores the coordinate transform as numbers; parsed shapes become board areas through
  `to_board_area` (C++ `Area` derives from `Shape`, the inverse of Java).
- **2026-10-10 (T110)**: `parser::Library` reads padstacks (images follow in T111). Per-layer pad shapes match the Java
  reader for circles, rects, concave polygons (convex hull → Simplex), zero-size rects (offset(1) → IntBox ±1) and
  multi-layer pads; padstack counts of 6 E2E boards equal the Java harness. Note: a `(parser (string_quote "))` scope
  breaks generic scope skipping (the lone quote starts a string), so boards like empty_board need T125 before their
  structure is read.
- **2026-10-10 (T111)**: images → board packages (`Library::create_board_packages`) and the part library reader.
  Package dump equals the Java reader for a DSN with duplicate/variant image names, back-side images, pin rotations,
  outline paths and keepouts. Circular areas use the new `geometry::planar::CircleArea` adapter.
- **2026-10-10 (T112)**: placement readers `parser::Component` (+ `ComponentPlacement`, header-only) and
  `parser::PlaceControl` (own `.h`/`.cpp`). `Component::read_component_scope` collects `(place …)` locations into
  `ReadScopeParameter::placement_list` (inserted later by `Network::insert_components`, T115); a place with no
  coordinates yields `coor == nullopt` (Java `coor == null` = unplaced). Item clearance info maps are keyed in
  `std::map` (Java `TreeMap`), last duplicate wins. `PLACEMENT_SCOPE` stays a plain `ScopeKeyword` because Java
  `Placement` only has `writeScope` (T204b); the `(place_control …)` scope is now a real `PlaceControl` keyword
  (flip_style). The parser component test is `ParserComponent_test.cpp` (suite `ParserComponentTest`) to avoid the
  target-name clash with the board `Component_test` — same convention as `ParserShape_test.cpp` (the earlier
  `Component_test.cpp` name made cmake fail to configure).

## T125 Parser + Circuit (2026-10-10)

- `Circuit.{h,cpp}` port `parser/Circuit.java`: `read_scope` reads `(length …)` (lengthArr[0]=max,
  lengthArr[1]=min — a failing `read_length_scope` keeps zeros and does NOT abort the scope, Java parity),
  `(use_via …)`/`(use_layer …)` via `DsnFile::read_via_padstacks`/`read_string_list_scope`. `ReadScopeResult`
  aggregates max/min length, use_via, use_layer. Both read_scope callsites here (net class read) return before
  the closing `)` in Java — the C++ reader does NOT consume the scope's closing bracket, so it must be invoked
  AFTER the `circuit` keyword token is consumed (test snippets start at the scope content).
- `Parser.{h,cpp}` port `parser/Parser.java`: `read_scope` (string_quote, space_in_quoted_tokens,
  host_cad/host_version, constant, write_resolution), `read_write_solution`, `read_constant`, `write_scope`,
  `read_quote_char`. `Keyword::PARSER_SCOPE` is now a real `Parser` instance in Keyword.cpp. `Parser::read_scope`
  is dispatched the same way — tests pass content-only snippets. `read_quote_char` needs a full (or >0-length)
  buffer because next_string() returns the empty string when the scanner is exhausted; `string_quote` switch
  (`par.string_quote = ...narrowed`) matches Java's `String.substring(0,1)`.
- `DsnFile::read_string_scope`/`read_quote_char` return `std::optional`; a missing `(host_cad)`/`(host_version)`
  scope yields `""` instead of Java `null` (callers guard with `value_or("")`). C++ `read_constant` reads both values as tokens (the java `NumberFormat.getIntegerInstance()
  .parse` overflow handling was dropped); const values come back as STRING tokens (test asserts value `"1"`).
- `Parser::write_scope` output (`string_quote`, `host_cad`/`host_version` (quoted when containing reserved
  chars), `constant` (number values quoted because strings starting with a digit get quoted — Java parity),
  `write_resolution`, `generated_by_freeroute`) was captured from the Java executable jar via a throwaway harness
  (`/tmp/opencode/ParserGolden.java`, reservedChars `{(,) ;-_}`) and embedded verbatim in `Parser_test.cpp`
  (full/minimal/reduced variants; note the full variant's `host_version "(5.1.5)"` — parentheses and the space
  are reserved chars — and the non-ASCII quote escape `\342\200\230` = `'`).
- Note: `Parser::read_scope` keeps an always-true `read_ok` bool for Java parity; `Circuit::read_scope`
  guards `nullopt` from `Structure::read_via_padstacks`/`read_string_list_scope` in place of Java's NPE.
- Registered `specctra/parser/{Circuit,Parser}.cpp` in `src/designforms/CMakeLists.txt` and
  `{Circuit,Parser}_test.cpp` in `tests/CMakeLists.txt`. The `Plane` reader already consumes `host_cad`
  with `equals_ignore_case(p_par.host_cad, "allegro")`; original T125 spec ("Plane must read the entire
  structure scope when host_cad==allegro") is the existing Plane.cpp behavior, verified end-to-end by
  `ParserTest.HostCadAllegroSkipsPlaneWindows` / `NonAllegroHostCadReadsPlaneWindows` (plane rects must be on a
  real DSN layer like F.Cu — the special `signal` layer has `no<0` and Java rejects it too).

## T113 Network part 1 (2026-10-11)

- `parser::Network` (Java `parser/Network.java` read part): `read_scope` loops the `(network …)` scope with the
  `(via …)`, `(via_rule …)`, `(class …)`, `(class_class …)` branches (filled by T114a/T114b), `read_net_scope` (name via `nextString()`, optional integer subnet number, `pins`/`order`/`fromto`/`rule`/
  `layer_rule` branches, one subnet per `fromto`, ordered 2-pin subnets for `order`, net-level width rule creating or
  finding a net class), `read_net_pins` and `create_ordered_subnets`. `Keyword::NETWORK_SCOPE` is the real `Network`
  instance.
- C++ shape differences (behaviour-preserving): `read_net_pins` returns `std::optional<std::vector<Net::Pin>>`
  (Java returns `boolean` and appends to an out-collection); the `fromto`/single-subnet collections are `std::set`
  so the dedup/sort matches Java `TreeSet<>(pinList)` in `Net.setPins`.
- `+5V` parity: `read_net_scope` uses `scanner.next_string()`, which reads the raw char buffer and keeps `+`
  (only `next_token()`'s identifier tokenizer skips illegal chars per gotcha 37), so the net is stored as `+5V`.
- The `Network_test.cpp` companion (9 tests: ordered subnets, pins, explicit subnet number, per-`fromto` subnets,
  `order`, empty net scope, width-rule net class, `+5V`, missing structure) existed from commit 311ba5c but was
  **not registered** in `tests/CMakeLists.txt` — added to `DESIGNFORMS_TEST_SOURCES`; ctest 962/962.
- Deferred: the via/class post-processing (via padstacks, `insertViaInfos`, `insertViaRules`, `insertNetClasses`,
  `insertClassPairs`) and `insertComponents`/`insertLogicalParts` (T114a/T114b/T115).

## T114a Network part 2a — via infos, via rules, via padstacks (2026-10-11)

- `parser::Network` gains `read_via_info`, `read_via_rule`, `insert_via_infos`, `create_default_via_infos`,
  `insert_via_rules` and `add_via_rule` (Java `parser/Network.java` 255–422). The `(via …)`/`(via_rule …)` branches
  of `read_scope` now call them; only `(class …)`/`(class_class …)` stay `skip_scope` stubs (T114b).
- `read_via_info` mirrors Java: three `yybegin(NAME)` string reads (via name, padstack name, clearance-class name),
  padstack lookup `library.get_via_padstack` → fallback `library.padstacks().get` + `add_via_padstack`, unknown
  clearance name → `BoardRules::default_clearance_class()` (1), then `ATTACH` or `)`; returns `nullptr` on any
  Java `null`/exception path.
- The readScope via-padstack block (Java 1273–1318): padstack names are cleaned with `replaceAll("\\.\\d+","")`,
  missing names are dropped with the exact Java `"Library.read_scope: via padstack with name '… not found at '…'"`
  warning, and the surviving list is stored with `BoardLibrary::set_via_padstacks`.
- `add_via_rule` needed `BoardRules::remove_via_rule` (Java `viaRules.remove`, pointer identity) to replace an
  existing same-named rule; the new rule is then added and `insert_via_rules` assigns the first via rule to every
  net class.
- Divergence from Java: none observed. **Deferred to T114b:** the loop that appends each net class's `use_via`
  names to `via_padstack_names` (needs the NetClass reader); it is marked with a `// T114b:` comment in `read_scope`.
- Verdict / risk: until T114b lands, `via_padstack_names` is only filled by the structure `(via …)` list, so a real
  DSN whose net classes declare the vias will fall back to default via infos (Java would use the class list). This
  is expected for an incremental port.
- `Network_test.cpp`: +4 tests (via info + via rule inserted; no via info → default via infos from via padstacks;
  missing via padstack dropped with a warning; via rule naming an unknown via falls back to the default rule).
  Network_test 13, ctest 966/966, `run_tests` PASSED.

## T114b Network part 2b — net classes and class pairs (2026-10-11)

- New `parser::NetClass` (`src/designforms/specctra/parser/NetClass.{h,cpp,_test.cpp}`) mirrors Java
  `parser/NetClass.java` (197 lines): fields for clearance class, width/via rules, active layers/switches, length,
  layer rules and the nested `ClassClass` (class names + clearance-class name). `read_scope` follows the Java
  token loop exactly (pre-loop `next_token()`, `open_bracket` tracking, `rules_missing` path for unused-class
  width rules); `read_class_class_scope` reads `(class_class (classes …) (rule …))` and `NULL_OBJECT` ends the loop.
- `parser::Network` gains `insert_net_classes`/`insert_net_class`, `insert_class_pairs`/`insert_class_pair_info`,
  `add_mixed_clearance_rule`, `add_clearance_rule`, `create_default_clearance_classes`, `get_clearance_class`,
  `create_via_rule` and `create_active_trace_layers` (Java `Network.java` 423–831). The `(class …)`/`(class_class …)`
  branches of `read_scope` now call the readers and, after the via-padstack block, run the `use_via` →
  `via_padstack_names` append (the T114a-deferred loop), then `insert_via_infos`, `insert_via_rules`,
  `insert_net_classes`, `insert_class_pairs` in Java order.
- Java quirks preserved: the net-class `use_via` names are appended to the single `via_padstack_names` collection
  (Java aliases `params.viaPadstackNames`), the `"-"` class-clearance separator (`DsnFile::kClassClearanceSeparator`)
  is used both for `(class_class)` pair parsing and for building clearance-class names, and `equals_ignore_case` /
  `java_round` match Java semantics. KiCad default-name mapping (`is_kicad_default_net_class_name` /
  `resolve_net_class`) is ported inline in `Network.cpp` (Java `io/KiCadNetClassNames.java`).
- C++ shape difference (behaviour-preserving): the `use_via` class names are cleaned of the `\.\d+` suffix (same as
  the structure via list) before the append.
- Known Java quirk carried over: a `(class NAME nets)` scope with **no** sub-scope leaves the class's `)` as the
  reader's look-back token, so it consumes the following scope. This matches the Java token loop; real DSN classes
  always carry a `(circuit …)`/`(rule …)` sub-scope. Tests therefore give every class a sub-scope.
- Tests: `NetClass_test.cpp` (7 cases: name/nets/width, clearance class + via rule + switches, circuit
  `use_via`/`use_layer`/length, layer rules + skipped unknown scope, EOF, `ClassClass` names + clearance, `ClassClass`
  EOF) and `Network_test.cpp` +6 (class creates/assigns a board net class, `kicad_default` mapping, clearance class
  built from the class, class-pair mixed clearance, `use_via` creates a via rule, `use_layer` restricts active
  trace layers). ctest 979/979, `run_tests` PASSED.

- **2026-10-11 (T122)**: DSN/SES write halves. `WriteScopeParameter` (Java `parser/WriteScopeParameter.java`) is a
  non-owning holder (board/settings pointers) that builds the reserved-char `IdentifierType` (`(`, `)`, space, `;`,
  `-`, `_`) with the configured quote. `Shape` gains pure-virtual `write_scope`/`write_scope_int` plus a non-virtual
  `write_hole_scope` (`window` wrapper, Java `Rectangle.writeHoleScope` pattern); `Rectangle`, `Circle`, `Polygon`,
  `PolygonPath`, `PolylinePath` implement them, and `Resolution::write_scope` writes `(resolution <unit> <n>)`.
  `CoordinateTransform` (Java `io/CoordinateTransform.java`) gained the numeric `board_to_dsn`/`_rel` overloads
  (`FloatPoint`, point array, `IntBox`, `Vector`, `Line[]`), `dsn_to_board[_rel]`, and the shape conversions in a new
  `CoordinateTransform.cpp`: `IntBox`→`Rectangle`, `PolylineShape`→`Polygon` (via `corner_approx_arr`),
  `Circle`→`Circle`. C++ deviation: `TileShape` is not a `Shape`, so board `IntBox`/octagon/simplex arrive wrapped in
  `geometry::planar::ConvexTileShape` — the transform unwraps it (IntBox→Rectangle, else corners→Polygon) to match
  Java where `IntBox`/`IntOctagon`/`Simplex` are `PolylineShape`s. Java quirk kept: `PolygonPath`/`PolylinePath` leave
  the width unrounded in the int variant, and `boardToDsnRel` still uses the non-rel diameter for circles.
  Tests: `ShapeWrite_test.cpp` (golden token-equal strings captured from the JDK-25 jar + write→read→equal
  round-trips), `WriteScopeParameter_test.cpp`, `CoordinateTransform_test.cpp` (golden values/strings). ctest
  1005/1005, `run_tests` PASSED. Layer/Rule write parts are deferred to the SesWriter/DsnWriter tasks (T201/T202).
