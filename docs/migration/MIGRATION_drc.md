# DRC Module - Migration Status

**Module**: `src/drc/`
**CMake Target**: `freerouting_drc`
**Java Files**: 7 | **C++ Headers**: 2 | **C++ Impls**: 1 | **Tests**: 0

---

## Completed (Fully Migrated)

| Java Class | C++ Header | C++ Impl | Test | Java Lines | Notes |
|------------|-----------|----------|------|-----------|-------|
| ClearanceViolation | ClearanceViolation.h | ClearanceViolation.cpp | - | 66 | DRC violation type |

## Partially Migrated

| Java Class | C++ Header | C++ Impl | Java Lines | What's Missing |
|------------|-----------|----------|-----------|----------------|
| UnconnectedItems | UnconnectedItems.h | - | 70 | Implementation file needed |

## Not Migrated

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| DesignRulesChecker | 466 | HIGH | Core DRC engine |
| DrcPosition | N/A | MED | Violation position |
| DrcReport | 92 | MED | Violation reporting |
| DrcViolation | N/A | MED | Violation entity |
| DrcViolationItem | N/A | MED | Individual violation item |

## Summary

- **Done**: 1 class (14% of Java files)
- **Partial**: 1 class (14%)
- **Remaining**: 5 classes (71%)
- **Blockers**: DesignRulesChecker is the core DRC engine and critical for validation. Depends on board rules and clearance checks being complete.
