# Freerouting Java->C++ Migration Execution Plan (Agent Handoff)

This plan is written for an implementation agent that will continue the migration from `src/main/java/app/freerouting` to C++ in `src` and `include`.

## 1) Goal and Scope

Primary goal:
- Complete headless routing parity for core flow (load DSN -> route -> write SES), with stable C++ behavior and tests.

In-scope:
- `geometry`, `datastructures`, `rules`, `board`, `autoroute`, `designforms/specctra`, `drc`, `settings`, `core`, `logger`.

Out-of-scope (for now):
- GUI-heavy Java modules.
- Full Java service/management stack unless required for headless flow.

## 2) Current Baseline (from repository state)

- C++ files in `src`: ~57 `.cpp`.
- Remaining Java files in `src/main/java`: ~330 `.java`.
- Build already compiles modular C++ static libraries in root `CMakeLists.txt`:
  - `freerouting_geometry`, `freerouting_board`, `freerouting_rules`, `freerouting_autoroute`, `freerouting_designforms`, etc.
- `migration_status.csv` exists and should be treated as migration tracker source.

Important:
- Some files are marked "Fully Migrated" in CSV but with low Java/C++ line parity. Treat them as "ported skeletons until parity verified."

## 3) Migration Strategy

Use dependency-first order, not alphabetical order.

Execution order:
1. `geometry/planar` + `datastructures`
2. `rules` + `core` lightweight entities
3. `board`
4. `autoroute`
5. `designforms/specctra` parser/IO glue for DSN/SES
6. `drc`
7. `settings`
8. `logger` + minimal `management` pieces needed for CLI/headless execution

Why:
- `autoroute` and parser logic depend heavily on geometry/board/rules correctness.

## 4) Work Cycle Per Class (mandatory loop)

For each Java class being migrated:
1. Identify target C++ location:
   - `src/<module>/<Class>.cpp`
   - `include/<module>/<Class>.h`
2. Port public API first (types, invariants, ownership semantics).
3. Port core logic methods (avoid stubs unless explicitly temporary).
4. Resolve Java->C++ semantic gaps:
   - Exceptions -> explicit error handling.
   - Collections -> STL containers with deterministic ordering when needed.
   - Big numbers -> choose `boost::multiprecision` only where required.
   - Nullability -> references, pointers, `std::optional`.
5. Add or update tests (unit first, then integration if needed).
6. Build and run tests.
7. Update `migration_status.csv` entry with actual progress notes.

Definition of Done (per class):
- Builds cleanly.
- No placeholder TODO for core behavior.
- Has at least one test path covering core behavior or documented blocker.
- CSV status updated.

## 5) Priority Backlog (next classes to migrate)

Highest priority for routing correctness:
- Geometry:
  - `IntPoint`, `IntVector`, `IntOctagon`, `Line`, `LineSegment`, `TileShape`, `Simplex`, `RationalPoint`, `RationalVector`.
- Board:
  - `Trace`, `PolylineTrace`, `DrillItem`, `ObstacleArea`, `ConductionArea`, `MoveComponent`, `ShoveTraceAlgo`, `PullTightAlgo*`.
- Autoroute:
  - `ExpansionDoor`, `SortedRoomNeighbours`, optimizer logic (`BatchOptimizer` equivalents), remaining maze helpers.
- DSN/SES:
  - `SpecctraDsnStreamReader`, `SpecctraSesFileWriter`, `Wiring`, related rule/network scopes.
- DRC:
  - `DesignRulesChecker`, `DrcReport`, `DrcViolation*`.

Medium priority:
- `rules`: `NetClasses`, `Nets`, `ViaInfo`, `ViaInfos`.
- `settings`: `GlobalSettings`, `SettingsMerger`, runtime/settings sources.

Lower priority (headless optional):
- analytics and non-essential management adapters.

## 6) Testing Gates (must pass before advancing phase)

Phase gate requirements:
- Geometry/rules phase:
  - Deterministic unit tests for geometric predicates, intersections, orientation, bounds.
- Board phase:
  - Connectivity and item-placement invariants.
- Autoroute phase:
  - Route search deterministic outcomes on fixed fixtures.
- DSN/SES phase:
  - Golden-file tests using sample inputs from `tests/data` and output SES comparison.

Minimum policy:
- No new migrated module without tests.
- Regression discovered in golden fixture blocks merge until fixed.

## 7) Branching and PR Slicing

Use small vertical PRs (one coherent area each):
- PR1: geometry primitive batch
- PR2: board entity batch
- PR3: autoroute algorithm batch
- PR4: specctra DSN/SES batch
- PR5: drc/settings stabilization

Each PR should include:
- migrated classes
- tests
- `migration_status.csv` updates
- brief migration notes in PR description (semantic decisions and tradeoffs)

## 8) Risk Register and Mitigation

Risk 1: Silent behavior drift from Java logic.
- Mitigation: golden DSN->SES fixtures, side-by-side outputs where possible.

Risk 2: Ownership/lifetime bugs after Java GC assumptions.
- Mitigation: explicit ownership policy (`unique_ptr` default), avoid raw owning pointers.

Risk 3: Numeric precision differences.
- Mitigation: define precision policy early, isolate geometric tolerances.

Risk 4: False sense of completion from CSV labels.
- Mitigation: require behavior-based DoD, not status label alone.

## 9) Agent Operating Instructions

When taking this plan:
1. Start with one module batch only.
2. Keep commits scoped and reversible.
3. After every class port:
   - build
   - run related tests
   - update CSV
4. If blocked by unresolved dependency:
   - create minimal interface needed
   - log blocker in commit/PR
   - move to next unblocked class in same phase
5. Do not migrate GUI unless explicitly requested.

## 10) Immediate First Sprint (recommended)

Sprint target:
- Complete geometry primitives needed by board/autoroute:
  - `IntPoint`, `IntVector`, `Line`, `LineSegment`.
- Add focused unit tests for these.
- Validate existing board/autoroute code compiles against them without stubs.

Sprint completion criteria:
- All targeted classes implemented with tests.
- No compile-time placeholders in targeted paths.
- `migration_status.csv` updated for each class.
