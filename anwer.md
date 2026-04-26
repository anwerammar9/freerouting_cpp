# Freerouting Java -> C++ Migration Plan (Agent Guide)

## Goal

Complete headless routing parity in C++ for the core flow:
- load DSN
- route
- write SES

Primary C++ targets:
- `src` and `include`

Java source of truth:
- `src/main/java/app/freerouting`

## Scope

In scope:
- `geometry`, `datastructures`, `rules`, `board`, `autoroute`, `designforms/specctra`, `drc`, `settings`, `core`, `logger`

Out of scope for now:
- GUI-heavy modules
- non-critical management/analytics features (unless needed by headless flow)

## Current Baseline

- C++: ~57 `.cpp` files in `src`
- Java remaining: ~330 `.java` files in `src/main/java`
- Module builds already exist in root `CMakeLists.txt`
- Migration tracker exists: `migration_status.csv`

Important:
- Do not trust "Fully Migrated" labels blindly. Validate behavior and parity with tests.

## Dependency-First Order

Migrate in this order:
1. `geometry/planar` + `datastructures`
2. `rules` + lightweight `core`
3. `board`
4. `autoroute`
5. `designforms/specctra` (DSN/SES read/write)
6. `drc`
7. `settings`
8. minimal `logger`/`management` needed for headless runs

## Mandatory Class Migration Loop

For each Java class:
1. Map Java class -> C++ header/source:
   - `include/<module>/<Class>.h`
   - `src/<module>/<Class>.cpp`
2. Port API first (types, invariants, ownership model).
3. Port logic (avoid placeholders in core paths).
4. Resolve Java->C++ semantic gaps:
   - exceptions -> explicit error strategy
   - collections -> STL with deterministic behavior where needed
   - nullability -> references/pointers/`std::optional`
   - numeric precision -> explicit policy (`int64_t`, double tolerances, optional multiprecision where required)
5. Add tests.
6. Build and run tests.
7. Update `migration_status.csv`.

Definition of Done (per class):
- compiles
- behavior implemented (no core TODO stubs)
- test coverage exists or blocker documented
- tracker updated

## Priority Backlog

High priority (routing correctness):
- Geometry:
  - `IntPoint`, `IntVector`, `IntOctagon`, `Line`, `LineSegment`, `TileShape`, `Simplex`, `RationalPoint`, `RationalVector`
- Board:
  - `Trace`, `PolylineTrace`, `DrillItem`, `ObstacleArea`, `ConductionArea`, `MoveComponent`, `ShoveTraceAlgo`, `PullTightAlgo*`
- Autoroute:
  - `ExpansionDoor`, `SortedRoomNeighbours`, optimizer components
- DSN/SES:
  - `SpecctraDsnStreamReader`, `SpecctraSesFileWriter`, `Wiring`
- DRC:
  - `DesignRulesChecker`, `DrcReport`, `DrcViolation*`

Medium:
- Rules containers: `NetClasses`, `Nets`, `ViaInfo`, `ViaInfos`
- Settings pipeline: `GlobalSettings`, `SettingsMerger`, `SettingsSource` implementations

## Testing Gates

Phase gates:
- Geometry/rules: deterministic unit tests
- Board: connectivity and item invariants
- Autoroute: deterministic route behavior on fixed fixtures
- DSN/SES: golden-file tests using `tests/data`

Policy:
- no module migration without tests
- regressions in golden fixtures block merge

## PR Strategy

Use small vertical PRs:
- PR1 geometry primitives
- PR2 board entities/invariants
- PR3 autoroute batch
- PR4 specctra DSN/SES batch
- PR5 drc/settings stabilization

Each PR must include:
- migrated classes
- tests
- `migration_status.csv` updates
- migration notes (decisions and tradeoffs)

## Risk Controls

1. Behavior drift vs Java:
- use fixture-based parity checks and golden outputs

2. Ownership/lifetime bugs:
- default to `std::unique_ptr` ownership; minimize raw owning pointers

3. Numeric precision mismatch:
- define tolerance rules early and keep them centralized

4. False completion signal:
- enforce DoD by behavior/tests, not file status labels

## Immediate Next Sprint

Target:
- finish geometry primitives needed by board/autoroute:
  - `IntVector` parity gaps
  - `Line`
  - `LineSegment`
- add focused unit/integration assertions

Completion criteria:
- targeted classes compile cleanly
- tests pass in `ctest`
- migration tracker updated

---

## File-by-File Migration Checklist

### Geometry / Planar

| Priority | Java Class | C++ Target | Status | Notes |
|----------|------------|------------|--------|-------|
| HIGH | `IntPoint.java` | `geometry/planar/IntPoint.cpp` | PARTIAL | missing comparison operators |
| HIGH | `IntVector.java` | `geometry/planar/IntVector.cpp` | PARTIAL | parity gaps in rotate/scale |
| HIGH | `Line.java` | `geometry/planar/Line.cpp` | PARTIAL | intersection logic pending |
| HIGH | `LineSegment.java` | `geometry/planar/LineSegment.cpp` | PARTIAL | clip/perpendicular logic |
| HIGH | `IntBox.java` | `geometry/planar/IntBox.cpp` | DONE | needs test verification |
| HIGH | `Direction.java` | `geometry/planar/Direction.cpp` | DONE | needs test verification |
| HIGH | `Point.java` | `geometry/planar/Point.cpp` | PARTIAL | floating point variants |
| HIGH | `Vector.java` | `geometry/planar/Vector.cpp` | PARTIAL | floating point variants |
| MED | `IntOctagon.java` | `geometry/planar/IntOctagon.cpp` | PENDING | octagonal approximation |
| MED | `TileShape.java` | `geometry/planar/TileShape.cpp` | PENDING | complex polygon clipping |
| MED | `Simplex.java` | `geometry/planar/Simplex.cpp` | PENDING | optimization region |
| MED | `PolygonShape.java` | `geometry/planar/PolygonShape.cpp` | PARTIAL | shape boolean ops |
| MED | `Polyline.java` | `geometry/planar/Polyline.cpp` | PARTIAL | point list handling |
| LOW | `RationalPoint.java` | - | PENDING | requires boost::multiprecision |
| LOW | `RationalVector.java` | - | PENDING | requires boost::multiprecision |
| LOW | `Circle.java` | - | PENDING | circle geometry |

### Board / Items

| Priority | Java Class | C++ Target | Status | Notes |
|----------|------------|------------|--------|-------|
| HIGH | `Trace.java` | `board/Trace.cpp` | PARTIAL | route segment entity |
| HIGH | `PolylineTrace.java` | `board/PolylineTrace.cpp` | PARTIAL | multi-segment trace |
| HIGH | `Via.java` | `board/Via.cpp` | PARTIAL | via hole entity |
| HIGH | `DrillItem.java` | `board/DrillItem.cpp` | PENDING | pad connections |
| HIGH | `Pin.java` | `board/Pin.cpp` | PARTIAL | component pins |
| HIGH | `Component.java` | `board/Component.cpp` | PARTIAL | module instances |
| HIGH | `Padstack.java` | `board/Padstack.cpp` | PARTIAL | pad geometry |
| HIGH | `Item.java` | `board/Item.cpp` | PARTIAL | base item class |
| HIGH | `ObstacleArea.java` | `board/ObstacleArea.cpp` | PENDING | keepout regions |
| HIGH | `ConductionArea.java` | `board/ConductionArea.cpp` | PENDING | copper areas |
| HIGH | `BasicBoard.java` | `board/BasicBoard.cpp` | PARTIAL | board state |
| HIGH | `RoutingBoard.java` | `board/RoutingBoard.cpp` | PARTIAL | routing extensions |
| HIGH | `BoardBuilder.java` | `board/BoardBuilder.cpp` | PENDING | construction API |
| HIGH | `BoardOutline.java` | - | PENDING | board boundary |
| MED | `ShapeSearchTree.java` | `board/ShapeSearchTree.cpp` | PARTIAL | spatial indexing |
| MED | `Layer.java` | `board/Layer.cpp` | PARTIAL | layer stack |
| MED | `ChangedArea.java` | `board/ChangedArea.cpp` | PENDING | incremental updates |
| MED | `MoveComponent.java` | - | PENDING | component placement |
| MED | `ShoveTraceAlgo.java` | - | PENDING | trace shoving |
| MED | `PullTightAlgo*.java` | - | PENDING | trace tightening |

### Autoroute

| Priority | Java Class | C++ Target | Status | Notes |
|----------|------------|------------|--------|-------|
| HIGH | `MazeSearchAlgo.java` | `autoroute/MazeSearchAlgo.cpp` | PARTIAL | core maze search |
| HIGH | `MazeSearchElement.java` | `autoroute/MazeSearchElement.cpp` | PARTIAL | search nodes |
| HIGH | `MazeSearchRoute.java` | `autoroute/MazeSearchRoute.cpp` | PARTIAL | trace construction |
| HIGH | `ExpansionRoom.java` | `autoroute/ExpansionRoom.cpp` | PARTIAL | expansion grid |
| HIGH | `ExpansionDoor.java` | - | PENDING | expansion boundaries |
| HIGH | `SortedRoomNeighbours.java` | - | PENDING | neighbor sorting |
| HIGH | `DestinationDistance.java` | `autoroute/DestinationDistance.cpp` | PARTIAL | cost functions |
| HIGH | `AutorouteControl.java` | `autoroute/AutorouteControl.cpp` | PARTIAL | routing parameters |
| HIGH | `AutorouteEngine.java` | `autoroute/AutorouteEngine.cpp` | PARTIAL | routing coordinator |
| MED | `BatchAutorouter.java` | `autoroute/BatchAutorouter.cpp` | PARTIAL | batch routing |
| MED | `BatchOptimizer.java` | - | PENDING | optimization pass |
| MED | `ItemAutorouteInfo.java` | - | PENDING | item routing state |

### Rules

| Priority | Java Class | C++ Target | Status | Notes |
|----------|------------|------------|--------|-------|
| HIGH | `BoardRules.java` | `rules/BoardRules.cpp` | PARTIAL | rule container |
| HIGH | `NetClass.java` | `rules/NetClass.cpp` | PARTIAL | net class rules |
| HIGH | `Net.java` | `rules/Net.cpp` | PARTIAL | net entity |
| HIGH | `ClearanceMatrix.java` | `rules/ClearanceMatrix.cpp` | PARTIAL | clearance lookup |
| HIGH | `DefaultItemClearanceClasses.java` | `rules/DefaultItemClearanceClasses.cpp` | PARTIAL | default classes |
| HIGH | `ViaRule.java` | `rules/ViaRule.cpp` | PARTIAL | via constraints |
| MED | `NetClasses.java` | - | PENDING | net class container |
| MED | `Nets.java` | - | PENDING | nets container |
| MED | `ViaInfo.java` | - | PENDING | via definition |
| MED | `ViaInfos.java` | - | PENDING | via library |

### DesignForms / SPECCTRA

| Priority | Java Class | C++ Target | Status | Notes |
|----------|------------|------------|--------|-------|
| HIGH | `SpecctraDsnStreamReader.java` | - | PENDING | DSN parser |
| HIGH | `SpecctraSesFileWriter.java` | - | PENDING | SES writer |
| HIGH | `Scanner.java` | `designforms/specctra/Scanner.cpp` | PARTIAL | tokenization |
| HIGH | `Keyword.java` | `designforms/specctra/Keyword.cpp` | PARTIAL | keyword handling |
| HIGH | `Structure.java` | `designforms/specctra/Structure.cpp` | PARTIAL | structure section |
| HIGH | `Network.java` | `designforms/specctra/Network.cpp` | PARTIAL | network section |
| HIGH | `Wiring.java` | - | PENDING | wiring section |
| HIGH | `Library.java` | `designforms/specctra/Library.cpp` | PARTIAL | library section |
| MED | `Placement.java` | `designforms/specctra/Placement.cpp` | PARTIAL | component placement |
| MED | `SessionWriter.java` | `designforms/specctra/SessionWriter.cpp` | PARTIAL | session output |

### DRC

| Priority | Java Class | C++ Target | Status | Notes |
|----------|------------|------------|--------|-------|
| HIGH | `DesignRulesChecker.java` | - | PENDING | rule validation |
| HIGH | `DrcReport.java` | - | PENDING | violation reporting |
| HIGH | `DrcViolation.java` | - | PENDING | violation entity |
| MED | `ClearanceViolation.java` | - | PENDING | clearance checks |

### Settings / Core / Logger

| Priority | Java Class | C++ Target | Status | Notes |
|----------|------------|------------|--------|-------|
| HIGH | `RouterSettings.java` | `settings/RouterSettings.cpp` | PARTIAL | routing params |
| MED | `GlobalSettings.java` | - | PENDING | runtime config |
| MED | `SettingsMerger.java` | - | PENDING | config merging |
| MED | `Connectivity.java` | - | PENDING | net connectivity |
| MED | `BoardLogger.java` | `logger/BoardLogger.cpp` | PARTIAL | logging |

---

## Status Legend

- **PENDING**: Not yet started
- **PARTIAL**: Started, core logic implemented, gaps remain
- **DONE**: Fully migrated, tests passing
- **BLOCKED**: Waiting on dependency or clarification

---

## Implementation Notes

### Geometry / Planar

- All planar primitives use integer coordinates (`int64_t`) for PCB routing
- `IntPoint` and `IntVector` are the workhorses; avoid floating point in core algorithms
- `IntOctagon` provides octagonal approximation for quick bounds checks
- `TileShape` handles complex polygon clipping; defer until needed by board

### Board / Items

- Item ownership: use `std::unique_ptr<>` for contained items
- Coordinate transformation: `CoordinateTransform` handles DSN -> internal units
- Layer counting: layers are 0-indexed; respect `board.get_layer_count()`
- Items track their own net/clearance class via integer IDs

### Autoroute

- Maze search uses expansion cost grid; `ExpansionRoom` is the search space
- Cost functions in `DestinationDistance` must be deterministic
- `AutorouteControl` parameters control search depth, timeout, cleanup passes

### Rules

- `ClearanceMatrix` is indexed by `[from_class][to_class][layer]`
- Net classes: default class ID 0 is unconnected
- Via rules: use `ViaRule` for individual via constraints

### DesignForms / SPECCTRA

- DSN scanner: case-insensitive keywords, quoted strings
- Coordinate units: convert to internal units on parse
- Structure section defines board outline and layers
- Network section defines nets and components

---

## Known Issues & Blockers

| Component | Issue | Severity | Workaround |
|-----------|-------|----------|------------|
| Geometry | Missing `RationalPoint/Vector` for high-precision arcs | MED | Use double with 1e-6 tolerance |
| Board | `BoardOutline` migration pending | HIGH | Use placeholder boundary |
| Autoroute | ExpansionDoor neighbor sorting incomplete | HIGH | Implement sorted vector |
| DRC | No checker implementation | HIGH | Build empty shell first |
| DesignForms | DSN reader incomplete | HIGH | Implement incrementally |

---

## Build & Test Commands

```bash
# Build all modules
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Run specific module tests
ctest -R geometry
ctest -R board
ctest -R autoroute
```

---

## Decision Log

| Date | Decision | Rationale |
|------|----------|----------|
| - | Use STL containers over boost::containers | Minimize dependencies |
| - | `int64_t` for PCB coordinates | Avoid overflow on large boards |
| - | `std::unique_ptr` ownership | RAII, clear lifetime |
| - | Exception-free inner loops | Performance in autoroute |
| - | Layer as integer index | Fast array lookup |

---

## Sprint Tracking

### Sprint 1: Geometry Primitives (In Progress)
- [x] `IntPoint` - core type
- [x] `IntVector` - basic operations
- [ ] `Line` - intersection logic
- [ ] `LineSegment` - clipping
- [ ] Test coverage for above

### Sprint 2: Board Items
- [ ] `Trace` - route segment
- [ ] `Via` - via hole
- [ ] `Pin` / `Component` - placements
- [ ] `Item` base class gaps

### Sprint 3: Autoroute Core
- [ ] `MazeSearchAlgo` - completion
- [ ] `ExpansionRoom` - optimization
- [ ] `ExpansionDoor` - boundaries

### Sprint 4: DSN I/O
- [ ] `SpecctraDsnStreamReader` - full parse
- [ ] `SpecctraSesFileWriter` - output
- [ ] Golden file tests

### Sprint 5: DRC & Integration
- [ ] `DesignRulesChecker` shell
- [ ] `RouterSettings` completion
- [ ] End-to-end test

---

## PR Template

```markdown
## Summary
Brief description of migrated classes.

## Changes
- `include/module/Class.h` - new header
- `src/module/Class.cpp` - implementation

## Testing
- [ ] Unit tests added in `tests/unit/`
- [ ] Builds cleanly (`make`)
- [ ] Existing tests pass

## Notes
Semantic decisions, workarounds, deviations from Java.
```

---

## Quick Start Checklist

Before starting migration:
- [ ] Verify Java source in `src/main/java/app/freerouting`
- [ ] Check existing C++ in `src/` and `include/`
- [ ] Review `migration_status.csv`
- [ ] Identify dependencies (what's needed first)

For each class:
- [ ] Create header in `include/<module>/`
- [ ] Create source in `src/<module>/`
- [ ] Add to `CMakeLists.txt` if new
- [ ] Implement public API
- [ ] Implement core logic
- [ ] Add test in `tests/unit/`
- [ ] Build and verify
- [ ] Update checklist above
- [ ] Update CSV

---

## Resources

- Java source: `src/main/java/app/freerouting/`
- C++ headers: `include/`
- C++ sources: `src/`
- Tests: `tests/unit/`
- Migration CSV: `migration_status.csv`
- CMake: `CMakeLists.txt`
- Design spec: `docs/`

---

## Code Style & Conventions

### Naming
- Classes: `CamelCase` (e.g., `RoutingBoard`)
- Functions: `snake_case` (e.g., `calculate_distance`)
- Variables: `snake_case` (e.g., `layer_count`)
- Constants: `kCamelCase` or `SCREAMING_SNAKE_CASE`
- Files: `CamelCase.cpp` / `CamelCase.h`

### Headers
- One class per header file
- Include guards: `#pragma once` (preferred) or `__MODULE_CLASS_H__`
- Order: related header, STL, third-party, project
- Use `inline` for trivial getters

### Types
- Coordinates: `int64_t` for positions, `int` for counts
- Floating point: `double` (not `float`)
- Boolean: `bool`
- Returns: prefer references for output params, `std::optional` fornullable

### Memory
- Prefer stack allocation
- Use `std::vector` instead of raw arrays
- Use `std::unique_ptr` for polymorphic/heterogeneous contents
- Use `std::shared_ptr` only when shared ownership needed
- Avoid raw `new`/`delete`

### Error Handling
- Return error codes for expected failures
- Use exceptions only for truly exceptional cases
- In loops: avoid exceptions in hot paths
- Log errors before propagating

### Testing
- Name: `test_class_name.cpp` in `tests/unit/`
- Use Google Test (`TEST_F` for fixtures)
- Arrange-Act-Assert pattern
- Test error cases, not just happy path

---

## Java -> C++ API Reference

### Collections

| Java | C++ | Notes |
|------|-----|-------|
| `List<T>` | `std::vector<T>` | dynamic array |
| `ArrayList<T>` | `std::vector<T>` | use `.reserve()` for perf |
| `Set<T>` | `std::unordered_set<T>` / `std::set<T>` | hash vs ordered |
| `Map<K,V>` | `std::unordered_map<K,V>` / `std::map<K,V>` | hash vs ordered |
| `Iterator` | range-based for / `.begin()/.end()` | |
| `list.get(i)` | `list[i]` | no bounds check in release |
| `list.size()` | `list.size()` | same |

### Strings

| Java | C++ | Notes |
|------|-----|-------|
| `"string"` | `"string"` | UTF-8 assumed |
| `s.length()` | `s.size()` | |
| `s.substring(i, n)` | `s.substr(i, n)` | |
| `s.startsWith(p)` | `s.rfind(p, 0) == 0` | |
| `s.equals(t)` | `s == t` | |
| `s.contains(t)` | `s.find(t) != npos` | |
| `StringBuilder` | `std::ostringstream` | or `fmt::format` |
| `String.format()` | `fmt::format()` | or `snprintf` |

### Optional / Null

| Java | C++ | Notes |
|------|-----|-------|
| `null` | `nullptr` | pointer only |
| `T?` | `std::optional<T>` | non-pointer types |
| `Optional.of(x)` | `std::optional<T>(x)` | |
| `opt.isPresent()` | `opt.has_value()` | |
| `opt.get()` | `*opt` or `opt.value()` | |
| `opt.orElse(def)` | `opt.value_or(def)` | |
| `obj.field` | `obj.field` | `.` for refs/values |
| `ptr.field` | `ptr->field` | `->` for pointers |
| `ptr?.field` | `ptr ? ptr->field : null` | explicit null check |

### Exceptions

| Java | C++ | Notes |
|------|-----|-------|
| `throw new Exception()` | throw or error code | prefer error codes in hot paths |
| `try { } catch (E e)` | `try { } catch (E& e)` | always catch by reference |
| `throws IOException` | return `bool` or `std::error_code` | explicit error handling |
| `assert` | `assert()` from `<cassert>` | debug only |

### Math

| Java | C++ | Notes |
|------|-----|-------|
| `Integer.MAX_VALUE` | `std::numeric_limits<int>::max()` | |
| `Math.max(a,b)` | `std::max(a,b)` | |
| `Math.min(a,b)` | `std::min(a,b)` | |
| `Math.abs(a)` | `std::abs(a)` | |
| `Math.round(a)` | `std::lround(a)` | |
| `Math.floor(a)` | `std::floor(a)` | |
| `Math.ceil(a)` | `std::ceil(a)` | |
| `Math.sqrt(a)` | `std::sqrt(a)` | |
| `Math.sin/cos/tan` | `std::sin/cos/tan` | `<cmath>` |
| `Double.POSITIVE_INFINITY` | `std::numeric_limits<double>::infinity()` | |
| `Double.isNaN(x)` | `std::isnan(x)` | |

### Time

| Java | C++ | Notes |
|------|-----|-------|
| `System.nanoTime()` | `steady_clock::now().time_since_epoch().count()` | |
| `System.currentTimeMillis()` | `duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()` | |
| `TimeUnit.MILLISECONDS` | `std::chrono` | |

### Files / IO

| Java | C++ | Notes |
|------|-----|-------|
| `new FileReader(f)` | `std::ifstream` | |
| `new FileWriter(f)` | `std::ofstream` | |
| `BufferedReader` | `std::getline` | |
| `Scanner` | custom parser | see existing `Scanner.cpp` |
| `Paths.get()` | `std::filesystem::path` | |

### Functional

| Java | C++ | Notes |
|------|-----|-------|
| `list.stream().map(f).collect(toList())` | `transform` + `back_inserter` | |
| `list.stream().filter(p).collect(toList())` | `copy_if` | |
| `list.stream().anyMatch(p)` | `std::any_of` | |
| `list.stream().allMatch(p)` | `std::all_of` | |

### Threading

| Java | C++ | Notes |
|------|-----|-------|
| `synchronized` | `std::mutex` | |
| `Thread.sleep(ms)` | `std::this_thread::sleep_for` | |
| `ExecutorService` | `std::async` or thread pool | |

---

## Common Porting Patterns

### Getter/Setter
```cpp
// Java
public int getX() { return x; }
public void setX(int v) { x = v; }

// C++
int x() const { return x_; }
void set_x(int v) { x_ = v; }
```

### Singleton (if needed)
```cpp
// C++11 thread-safe
class BoardRules {
public:
    static BoardRules& instance() {
        static BoardRules inst;
        return inst;
    }
private:
    BoardRules() = default;
};
```

### Builder Pattern
```cpp
class RoutingBoard::Builder {
public:
    Builder& set_layers(int n) { layers_ = n; return *this; }
    Builder& set_bounds(IntBox b) { bounds_ = b; return *this; }
    std::unique_ptr<RoutingBoard> build() {
        return std::make_unique<RoutingBoard>(layers_, bounds_);
    }
private:
    int layers_ = 0;
    IntBox bounds_;
};
```

### Enum
```cpp
// Java
enum Direction { UP, DOWN, LEFT, RIGHT }

// C++
enum class Direction { Up, Down, Left, Right };
// Usage: Direction::Up
```

### Interface / Abstract
```cpp
// Java
public interface ItemVisitor<T> {
    T visit(Trace t);
    T visit(Via v);
}

// C++
template<typename T>
class ItemVisitor {
public:
    virtual T visit(Trace& t) = 0;
    virtual T visit(Via& v) = 0;
    virtual ~ItemVisitor() = default;
};
```

### Comparator
```cpp
// Java
Collections.sort(list, (a,b) -> a.x - b.x);

// C++
std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
    return a.x < b.x;
});
```
