# Freerouting C++ Migration & Testing Plan

This document provides an assessment of the Java-to-C++ migration status and a detailed plan for implementing comprehensive unit testing across the C++ codebase.

## 1. Migration Status Report

The migration from Java to C++ is currently in progress. The focus has been on core data structures, geometry, and DSN parsing.

### Component Breakdown
| Component | Java Files | C++ Files | Migration % (approx) | Status |
|-----------|------------|-----------|----------------------|--------|
| **Geometry** | 22 | 8 | 100% (Core) | Core planar geometry mostly complete. |
| **Data Structures** | 24 | 5 | 60% | Basic stacks and generators migrated. |
| **Rules** | 12 | 6 | 80% | Net, NetClass, and BoardRules migrated. |
| **Board** | 45 | 13 | 40% | Basic items and layers migrated; complex interactions pending. |
| **Autoroute** | 46 | 8 | 17% | Algorithms like MazeSearch are partially migrated. |
| **DesignForms (DSN)**| 20 | 7 | 80% | Scanner and Structure handling mostly done. |
| **Settings** | 5 | 1 | 20% | RouterSettings migrated. |
| **DRC** | 10 | 1 | 10% | Only basic clearance violation logic migrated. |
| **GUI** | 150+ | 0 | 0% | Intentionally omitted (Headless focus). |

**Overall Progress**: ~24% of files migrated (based on main `src/` directory).

> [!WARNING]
> A secondary C++ codebase was discovered in `artifacete/src/` which contains 162 C++ files (65 .cpp, 97 .hpp). This version appears to use different dependencies (e.g., `boost::multiprecision`) and follows a different naming convention (lowercase filenames). Clarification is needed on which codebase is the primary target for testing.

---

## 2. Unit Testing Strategy

### Proposed Framework
- **Google Test (gtest)**: Recommended for its rich feature set, widespread adoption, and excellent CMake integration.

### Test Structure
Tests should be organized to mirror the `src` directory:
- `tests/unit/geometry/planar/`
- `tests/unit/autoroute/`
- `tests/unit/board/`
- ...etc.

### Coverage Target
- **Line Coverage**: > 90% for all core logic (geometry, rules, board).
- **Branch Coverage**: > 80% for algorithms (autoroute).

---

## 3. File-by-File Testing Checklist

### Geometry (High Priority)
- [ ] `src/geometry/planar/Direction.cpp`
- [ ] `src/geometry/planar/FloatLine.cpp`
- [ ] `src/geometry/planar/Geometry.cpp`
- [ ] `src/geometry/planar/IntBox.cpp`
- [ ] `src/geometry/planar/PolygonShape.cpp`
- [ ] `src/geometry/planar/Polyline.cpp`
- [ ] `src/geometry/planar/Shape.cpp`
- [ ] `src/geometry/planar/Vector.cpp`

### Board & Items
- [ ] `src/board/BasicBoard.cpp`
- [ ] `src/board/BoardBuilder.cpp`
- [ ] `src/board/ChangedArea.cpp`
- [ ] `src/board/Communication.cpp`
- [ ] `src/board/Component.cpp`
- [ ] `src/board/CoordinateTransform.cpp`
- [ ] `src/board/Item.cpp`
- [ ] `src/board/Layer.cpp`
- [ ] `src/board/Padstack.cpp`
- [ ] `src/board/Pin.cpp`
- [ ] `src/board/RoutingBoard.cpp`
- [ ] `src/board/ShapeSearchTree.cpp`
- [ ] `src/board/Via.cpp`

### Autoroute Algorithms
- [ ] `src/autoroute/AutorouteControl.cpp`
- [ ] `src/autoroute/AutorouteEngine.cpp`
- [ ] `src/autoroute/BatchAutorouter.cpp`
- [ ] `src/autoroute/DestinationDistance.cpp`
- [ ] `src/autoroute/ExpansionRoom.cpp`
- [ ] `src/autoroute/MazeSearchAlgo.cpp`
- [ ] `src/autoroute/MazeSearchElement.cpp`
- [ ] `src/autoroute/MazeSearchRoute.cpp`

### Rules & Settings
- [ ] `src/rules/BoardRules.cpp`
- [ ] `src/rules/ClearanceMatrix.cpp`
- [ ] `src/rules/DefaultItemClearanceClasses.cpp`
- [ ] `src/rules/Net.cpp`
- [ ] `src/rules/NetClass.cpp`
- [ ] `src/rules/ViaRule.cpp`
- [ ] `src/settings/RouterSettings.cpp`

### DSN Parsing (Integration Tests)
- [ ] `src/designforms/specctra/Keyword.cpp`
- [ ] `src/designforms/specctra/Library.cpp`
- [ ] `src/designforms/specctra/Network.cpp`
- [ ] `src/designforms/specctra/Placement.cpp`
- [ ] `src/designforms/specctra/Scanner.cpp`
- [ ] `src/designforms/specctra/SessionWriter.cpp`
- [ ] `src/designforms/specctra/Structure.cpp`

---

## 4. Implementation Steps for Next Agent

1. **Fix CMake Test Integration**:
   - The current `add_subdirectory(tests)` in root `CMakeLists.txt` fails because `tests/CMakeLists.txt` is missing (it's at `tests/data/CMakeLists.txt`).
   - Create `tests/CMakeLists.txt` that includes both `tests/data` (integration) and a new `tests/unit` directory.

2. **Setup Google Test**:
   - Add `FetchContent` for GTest in `tests/CMakeLists.txt`.
   - Setup a base test fixture for board-related tests.

3. **Incremental Implementation**:
   - Start with `geometry/planar` as these have no dependencies.
   - Move to `rules` and `datastructures`.
   - Finally, implement tests for `board` and `autoroute`.

4. **Continuous Integration**:
   - Ensure `make run_tests` or `ctest` is executed in the build pipeline.
