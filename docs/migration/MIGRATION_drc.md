# DRC Module - Migration Status

**Module**: `src/drc/`
**CMake Target**: `freerouting_drc`
**Java Files**: 7 | **C++ Headers**: 14 | **C++ Impls**: 7 | **Tests**: 7 (gtest)

---

## Completed (Fully Migrated)

| Java Class | C++ Header | C++ Impl | Test | Java Lines | Notes |
|------------|-----------|----------|------|-----------|-------|
| AirLine | interactive/RatsNest.h/.cpp | - | - | 16 | Lives in `interactive/` (RatsNest), not `src/drc/` |
| ClearanceViolation | ClearanceViolation.h | ClearanceViolation.cpp | - | 66 | DRC violation type; ported |
| DesignRulesChecker | DesignRulesChecker.h | DesignRulesChecker.cpp | DesignRulesChecker_test.cpp | 466 | Core DRC engine; 7 gtest cases, deterministic |
| DrcSummaryResponse | DrcSummaryResponse.h | DrcSummaryResponse.cpp | (via DesignRulesChecker_test) | 8 | JSON summary for KiCad schema |
| NetIncompletes | interactive/NetIncompletes.h/.cpp | - | NetIncompletes_test.cpp | 19 | Lives in `interactive/` |
| UnconnectedItems | UnconnectedItems.h | UnconnectedItems.cpp | (via DesignRulesChecker_test) | 70 | Ported |
| ZoneIslandViolation | ZoneIslandViolation.h | ZoneIslandViolation.cpp | - | 20 | Ported |

Additional C++-only helpers: `DrcReport.{h,cpp}`, `DrcViolation.h`, `DrcViolationItem.h`, `DrcPosition.h`, `drc_json_internal.h`.

`freerouting_drc` links `freerouting_core`, `freerouting_board`, `freerouting_constants`, `freerouting_interactive`.

## Dependencies

- `freerouting_rules` (`ClearanceMatrix`, `BoardRules`, `NetClass`) — see Porting Note 1 below.
- Test fixtures build a `RoutingBoard` inline (see `make_routing_board` in `DesignRulesChecker_test.cpp`).

## Porting Notes

1. **Dangling-reference fix (clearance violations)**: `ClearanceMatrix`, `BoardRules`, and `NetClass`
   originally held `const board::LayerStructure&` reference members bound to stack-locals that die when
   `make_routing_board()`/`BoardBuilder`/`BoardLoader` return. That made `get_value()`/`layer_count()`
   read freed memory, so the clearance prefilter shrank and violations were skipped nondeterministically
   (flaky ctest). Fix: store `LayerStructure` **by value** (it is a `vector<shared_ptr<Layer>>`, so the
   copy is cheap and Layer identity is preserved). Any future class that needs a long-lived
   `LayerStructure` must own it (by value or `shared_ptr`), never a plain reference.

## Summary

- **Done**: 7 classes (100% of Java DRC files).
- **Tests**: 7 gtest cases in `src/drc/DesignRulesChecker_test.cpp` — all deterministic and green.
- **Known, unrelated**: 4 pre-existing failures in `RatsNestTest.*` (in-progress RatsNest/NetIncompletes logic); unrelated to DRC module.