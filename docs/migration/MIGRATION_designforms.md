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
