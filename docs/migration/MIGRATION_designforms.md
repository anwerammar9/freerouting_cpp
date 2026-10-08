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
