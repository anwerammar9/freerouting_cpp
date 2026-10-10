# M2 — SES / rules / DSN writers and readers (gate G3)

Order: **T122 → T201 → T202 → T203**, then **T204a (RulesReader) → T204b (RulesWriter) → T204c (DsnWriter)**.
T201 also needs **T125** (`Parser.writeScope`) and **T117** (a loaded board).

Shared writer infrastructure already in C++: `datastructures::IndentFileWriter` (`start_scope(new_line)`,
`end_scope`, `new_line`, `operator<<`) and `datastructures::IdentifierType(reserved_chars, string_quote).write(name,
ostream)`. Java's `IndentFileWriter.endScope` = decrement indent, newline, `)` — the C++ port already matches.

Number formatting: Java writes coordinates with `String.valueOf(double)`/`(float)` or `String.valueOf(int)`.
Find out per call site which one is used; Java `Double.toString` (shortest round-trip, `1.0E10` style exponents)
differs from `std::to_chars` for large/small magnitudes — add one helper `java_double_to_string(double)` (and the
float variant already in `AutorouteSettings.cpp` → move it to a shared `src/datastructures/JavaFormat.h`) and test it
against Java output for 0.0, -0.0, 1.0, 0.1, 1e7, 1.0E-4, 123456789.0, 1/3.

---

## T122 — WriteScopeParameter, shape write halves, board→DSN shape transforms  (S)

**Java.**
| Piece | File / lines |
|---|---|
| `WriteScopeParameter` (fields: board, autorouteSettings, file, identifierType, coordinateTransform, compatMode, …) | parser/WriteScopeParameter.java (whole, 38) |
| `Shape.writeScope` / `writeScopeInt` (abstract), `writeHoleScope` | parser/Shape.java 597–≈620 |
| `Rectangle.writeScope/writeScopeInt` | parser/Rectangle.java 73–96 |
| `Circle.writeScope/writeScopeInt` | parser/Circle.java 67–91 |
| `Polygon.writeScope/writeScopeInt` | parser/Polygon.java 76–111 |
| `PolygonPath.writeScope/writeScopeInt` | parser/PolygonPath.java 22–≈60 |
| `PolylinePath.writeScope/writeScopeInt` | parser/PolylinePath.java 20–≈55 |
| `Resolution.writeScope(file, communication)` | parser/Resolution.java 17–≈30 |
| board→DSN: `boardToDsn(double / FloatPoint / FloatPoint[] / Line[] / Vector / IntBox)`, **`boardToDsn(Shape, Layer)`**, `boardToDsnRel(…)`, **`boardToDsnRel(Shape, Layer)`** | io/CoordinateTransform.java 34–157 |

**C++ target.** `parser/WriteScopeParameter.h`; virtual `write_scope(IndentFileWriter&, const IdentifierType&) const`
and `write_scope_int(...)` on `parser::Shape` with overrides in each shape class; `write_hole_scope`;
`Resolution::write_scope(IndentFileWriter&, const board::Communication&)`; in `parser/CoordinateTransform.{h,cpp}`
the shape conversions `board_to_dsn(const geometry::planar::Shape&, std::shared_ptr<Layer>) ->
std::unique_ptr<Shape>` and `board_to_dsn_rel(...)` (IntBox → Rectangle, Circle → Circle, IntOctagon/Simplex/
PolygonShape → Polygon, PolylineShape with holes → as Java).

**Tests.** For each shape kind: build → `write_scope` → compare text with Java output (golden, generated once by a
throwaway Java program writing the same shapes); write → re-read with the scope reader → equal coordinates;
`board_to_dsn(IntBox)` with a non-trivial transform (scale 10, base offset) equals Java.

---

## T201 — SesWriter part 1: session header, base design, placement, was_is  (S)

**Java** (`io/specctra/SesWriter.java`): `write` 55–67 (reserved chars `( ) space ; - _ / ~ { }`, quote from
`board.communication.specctraParserInfo.stringQuote`, session name = design name with `.dsn`→`.ses`),
`writeSessionScope` 73–94 (scale factor `communication.coordinateTransform.dsnToBoard(1) / communication.resolution`),
`writePlacement` 96–115, `writeComponents` 117–151, `writeComponent` 153–181, `formatPlacementRotation` 259–270,
`writeWasIs` 183–218.

**C++ target.** Rewrite `src/designforms/specctra/SessionWriter.{h,cpp}` in place as the port of `SesWriter`
(keep the file/class name used by callers; namespace `…::specctra`). Public API:
`static bool write(const board::RoutingBoard&, std::ostream&, const std::string& design_name)`.
Add `SessionWriter_test.cpp`.

**Reuse.** T122 `Resolution::write_scope`; `board::Packages`, `board::Components` (`get(i)`, `count()`,
`get_location`, `get_rotation_in_degree`, `placed_on_front`), `board::Communication` (T109c fields).

**Java parity notes.** Packages are iterated `1..count` (ids are 1-based); components of a package are written in the
order Java `writeComponents` uses (read the loop — do not sort). `formatPlacementRotation` rounds to 3 decimals and
strips zeros (`338.500` → `338.5`, `0` → `0`).

**Tests.** A board with two components (one back side, rotation 338.5) → placement text equal to Java golden.

---

## T202 — SesWriter part 2: routes, library_out, network_out  (S)

**Java.** `writeRoutes` 220–233 (writes `Resolution.writeScope` and `Parser.writeScope(file, parserInfo, identifierType,
true)` — **needs T125**), `writeLibrary` 235–257, `writePadstack` 271–313 (layer range from non-null padstack shapes,
`coordinateTransform.boardToDsnRel(shape, layer).writeScopeInt`, `(attach off)` when not attach-allowed),
`writeNetwork` 315–327, `writeNet` 329–370, `writeWire` 372–424, `snappedEndpoint` 426–453 (end-point snapping to
the contacted drill centre when inside the pad's border distance), `writeVia` 455–476, `writeFixedState` 478–490,
`writePath` 492–512, `writeConductionArea` 514–552.

**C++ target.** Same files as T201; `tools/ses_diff.py` (token-level compare of two SES files: tokenize parentheses,
quoted strings and atoms; numeric atoms compared with a tolerance option `--eps 0`; prints the first differing scope
path).

**Reuse.** `board::Padstack::get_shape(layer)`, `attach_allowed`; `PolylineTrace::{polyline, first_corner (copy —
gotcha 28), last_corner, get_half_width, get_layer, get_start_contacts, get_end_contacts, get_fixed_state}`;
`Via::{get_padstack, get_center, get_fixed_state}`; `DrillItem::{first_layer, last_layer, get_shape(i)}`,
`Shape::border_distance`; `ConductionArea::{get_area, get_layer, net_count}`; `BasicBoard::get_connectable_items`.

**Java parity notes.** Which nets/items are written and in which order follows Java's loops over `board.getItems()`
(descending id, T011) — do not sort by net. Trace end-point snapping (`snappedEndpoint`) returns null when the corner
is within 0.5 of the centre.

**Done when.** `Issue103-Board-Routed.dsn` loaded (T117) and written without routing produces an SES that
`tools/ses_diff.py --eps 0` reports equal to the Java SES for the same input (generate it once with
`java -jar … -de Issue103-Board-Routed.dsn -do expected.ses --router.autorouter.enabled=false
--router.optimizer.enabled=false --router.fanout.enabled=false`. Verified in `RoutingPipeline.runRoutingStage`
(RoutingPipeline.java 92–120): with these flags no routing runs, but `board.finishAutoroute()` still does — if the
C++ output differs only where `finishAutoroute` changes the board, do T422 first or note it. Store the file as
`tests/data/expected/Issue103-Board-Routed.ses`), and `java -jar … -de <dsn> <cpp.ses> -drc out.json`
loads the C++ SES without errors. Add a gtest that runs the writer on that board and diffs against the stored file
(skip with `GTEST_SKIP` if the expected file is missing).

---

## T203 — SesReader  (S)

**Java** (`io/specctra/SesReader.java`, 413): ctor 39–58 (scale denominator), `read(InputStream, board)` 60–94 →
`SesImportSummary` (io/specctra/SesImportSummary.java, 15), `processSessionScope` 116–158, `processRoutesScope`
160–185, `processNetworkScope` 187–212, `processNetScope` 214–268, `processWireScope` 270–339
(`board.insertTrace`), `processViaScope` 341–413 (`board.insertVia`). Skip `saveSpecctraSessionSesAsFusionScriptScr`
(105–114, Fusion export — out of scope).

**C++ target.** `src/designforms/specctra/SesReader.{h,cpp,_test.cpp}`, `SesImportSummary.h`.

**Tests.** Round trip: load DSN → write SES (T202) → load the same DSN again → `SesReader::read` the SES → trace and via
counts and corner lists equal the first board's. Also read one of the 25 `.ses` files in `tests/data` with its DSN.

---

## T204a — RulesReader  (S)

**Why before the writers.** `RulesFileSettings` (T306) and the scheduler's sibling `<design>.rules` auto-load (T702)
change routing input; the writer is only needed for `-do x.rules`.

**Java** (`io/specctra/RulesReader.java`, 376): `read(in, designName, board)` 47–63, `read(...)` 65–178,
`readRouterSettings(in)` 180–236, `discoverLayerStructure(byte[])` 238–278, `applyRules` 280–306, `applyLayerRules`
308–338, `applyViaInfo` 340–350 (uses `Network.readViaInfo`, T114a), `applyViaRule` 352–357 (`Network.readViaRule`),
`applyNetClass` 359–367 (`NetClass.readScope` + `Network.insertNetClass`, T114b).

**C++ target.** `src/designforms/specctra/RulesReader.{h,cpp,_test.cpp}`.

**Tests.** Write a `.rules` file by hand (or take one produced by the Java `-do x.rules`) for `Issue026`; read it onto
the loaded board → net classes, widths, clearances equal the Java harness after the same operation.

---

## T204b — RulesWriter + the write halves it needs  (S)

**Java.** `io/specctra/RulesWriter.java` (104): `write` 36–72, `writeRules` 74–104, which calls
`Structure.writeSnapAngle` (Structure.java 510–524), `Rule.writeDefaultRule` and friends (Rule.java 119–343:
`writeScope`, `writeLayerRule`, `writeDefaultRule`, `writeNonDefaultClearanceRules`, `writeNamedClearanceRules`,
`writeItemClearanceClass`), `Library.writePadstackScope` (Library.java 54–100), `Network.writeViaInfos` 58–77,
`writeViaRules` 78–92, `writeNetClasses` 93–98, `writeNetClass` 99–151, `writeCircuit` 152–192,
`AutorouteSettings.writeScope` (done in T109a).

**Split if needed.** If this exceeds ~700 lines, do `T204b1` (Rule + Structure snap angle + Library padstack writes)
and `T204b2` (Network writes + RulesWriter).

**Done when.** Load DSN → `RulesWriter` → text equal to `java -jar … -de x.dsn -do x.rules` output for Issue026 and
BBD_Mars-64 (`ses_diff.py` works on any S-expression file).

---

## T204c — DsnWriter + the remaining write halves  (S)

**Java.** `io/specctra/DsnWriter.java` (92: `write` 45–56, `writePcbScope` 58–92) calling `Parser.writeScope` (T125),
`Resolution.writeScope` (T122), `Structure.writeScope` 52–92 + `writeConductionAreas` 93–112, `writeKeepouts` 113–136,
`writeBoundaries` 137–175, `writeLayers` 176–185, `writeDefaultRules` 186–190, `writeViaPadstacks` 191–207,
`writeControlScope` 208–228, `writeKeepoutScope` 229–272; `Layer.writeScope` (Layer.java 48–67); `Library.writeScope`
35–53; `Package.writeScope` 152–208, `writePackageKeepout` 209–248, `writePlacementScope` 355–388;
`PartLibrary.writeScope` 23–83; `Placement.writeScope` (Placement.java 14–29); `Component.writeScope` 56–89,
`writePinInfo` 90–123, `writeKeepoutInfos` 124–169, `getKeepout` 170–186; `Network.writeScope` 45–57;
`Net.writeScope/writeNetId/writePin` (Net.java); `Wiring.writeScope` 47–78, `writeViaScope` 79–110,
`writeWireScope` 111–156, `writeConductionAreaScope` 157–197, `writeNet` 198–206, `writeFixedState` 207–222.

**Split.** T204c1 = Structure/Layer/Library/Package/PartLibrary writes; T204c2 = Placement/Component/Network/Net/Wiring
writes + DsnWriter. Both well under 700 lines each.

**Done when.** DSN → board → DSN equals Java's `-do x.dsn` output (ses_diff, eps 0) for the 8 E2E boards. Then add
`HeadlessBoardManager::calculate_crc32` (T124c TODO).
