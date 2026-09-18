# Core Module - Migration Status

**Module**: `src/core/`
**CMake Target**: `freerouting_core`
**Java Files**: 36 | **C++ Headers**: 7 | **C++ Impls**: 6 | **Tests**: 3

---

## Completed (Fully Migrated)

| Java Class | C++ Header | C++ Impl | Test | Java Lines | Notes |
|------------|-----------|----------|------|-----------|-------|
| FileFormat | FileFormat.h | FileFormat.cpp | FileFormat_test.cpp | 4 | Simple enum with 7 values |
| RoutingJob | RoutingJob.h | RoutingJob.cpp | RoutingJob_test.cpp | 512 | Core routing job |
| RoutingStage | RoutingStage.h | RoutingStage.cpp | RoutingStage_test.cpp | 8 | Simple enum with 3 values |
| StopRequestState | StopRequestState.h | StopRequestState.cpp | StopRequestState_test.cpp | 20 | Simple enum with 3 values |
| Package | Package.h | Package.cpp | - | 390 | Core package class |
| Session | Session.h | Session.cpp | - | 1249 | Session management |
| BoardFileDetails | BoardFileDetails.h | BoardFileDetails.cpp | BoardFileDetails_test.cpp | 224 | File metadata + CRC32 + format detection + JSON; lives in `freerouting_scoring` lib (layering) |

## Partially Migrated

| Java Class | C++ Header | C++ Impl | Java Lines | What's Missing |
|------------|-----------|----------|-----------|----------------|
| LogicalPart | LogicalPart.h | - | 110 | Implementation file needed |

## Not Migrated

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| BoardLibrary | 4980 | HIGH | Board library management (very large) |
| Packages | 77 | MED | Collection wrapper for Package |
| Padstack | 4761 | HIGH | Padstack definitions |
| Padstacks | 95 | MED | Collection wrapper for Padstack |
| RouterCounters | 1326 | MED | Performance counters |
| RouterJobResourceUsage | 744 | MED | Resource usage tracking |
| StoppableThread | 1330 | MED | Thread abstraction (use std::thread) |

### Events Submodule (Not Migrated)

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| BoardSnapshotEvent | N/A | LOW | Event system (GUI-dependent) |
| BoardUpdatedEvent | N/A | LOW | Event system (GUI-dependent) |
| TaskStateChangedEvent | N/A | LOW | Event system (GUI-dependent) |
| BoardSnapshotEventListener | N/A | LOW | Event listener interface |
| BoardUpdatedEventListener | N/A | LOW | Event listener interface |
| TaskStateChangedEventListener | N/A | LOW | Event listener interface |

### Scoring Submodule (Fully Migrated: BoardStatistics.java)

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| BoardStatistics | 425 | MED | **MIGRATED (2026-09-18)** → `src/core/scoring/BoardStatistics.{h,cpp}`, lib `freerouting_scoring`. All 12 nested substructs + `BoardStatisticsDifficulty` + `BoardStatisticsFanout` in header. `init_from_board()` (host fallback `"Freerouting,"+VERSION`, MM-default units, areaCm2, difficulty C/D, totalLengthMm, fanout), Java-exact `calculateScore`/`getMaximumScore`/`getNormalizedScore`, DSN/SES file ctor (hostCad/hostVersion + token counting). 11 gtest cases. Wait: `pin_is_escaped` approximates Java's `clearanceViolations().isEmpty()` away (no DRC yet). |
| BoardStatisticsWithNetInfo | N/A | MED | Per-net statistics |
| DetailedBoardStatistics | N/A | MED | Detailed stats |
| EstimatedRouteResult | N/A | LOW | Estimated results |
| OptimizedRouteResult | N/A | LOW | Optimized results |
| RoutedNetStatistics | N/A | MED | Per-net routing stats |
| RoutedPathSubSection | N/A | LOW | Path subsection |
| RoutedPathSubSections | N/A | LOW | Path subsections container |
| RoutedPathWithLine | N/A | LOW | Path with line segment |
| RoutedPathWithTeardrop | N/A | LOW | Path with teardrop |
| RoutedPathWithVia | N/A | LOW | Path with via |
| UnroutedNetStatistics | N/A | MED | Unrouted net stats |

## Summary

- **Done**: 8 classes (22% of Java files)
- **Partial**: 1 class (3%)
- **Remaining**: 27 classes (75%), including critical Padstack and BoardLibrary
- **Blockers**: Padstack/BoardLibrary are very large and critical for board construction; scoring/events done (BoardFileDetails ported 2026-09-18 into `freerouting_scoring` lib because it needs scoring+board — `freerouting_core` stays a leaf lib)
