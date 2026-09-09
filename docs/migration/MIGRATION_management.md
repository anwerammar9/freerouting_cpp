# Management Module - Migration Status

**Module**: `src/management/`
**CMake Target**: `freerouting_management`
**Java Files**: 18 | **C++ Headers**: 2 | **C++ Impls**: 2 | **Tests**: 0

---

## Completed (Fully Migrated)

| Java Class | C++ Header | C++ Impl | Test | Java Lines | Notes |
|------------|-----------|----------|------|-----------|-------|
| SessionManager | SessionManager.h | SessionManager.cpp | - | 109 | Session management |
| TextManager | TextManager.h | TextManager.cpp | - | 228 | Text/i18n management |

## Not Migrated

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| ReflectionUtil | 257 | LOW | Java reflection (not needed in C++) |
| RoutingJobScheduler | 395 | MED | Job scheduling |
| RoutingJobSchedulerActionThread | 264 | MED | Scheduler thread |
| VersionChecker | N/A | LOW | Version checking |

### Analytics Submodule (Not Migrated)

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| FreeroutingAnalyticsClient | 114 | LOW | Analytics client |
| FRAnalytics | 280 | LOW | Analytics facade |
| BoardStatistics | 425 | MED | Board statistics |
| BoardStatisticsWithNetInfo | N/A | MED | Per-net statistics |
| DetailedBoardStatistics | N/A | MED | Detailed statistics |
| EstimatedRouteResult | N/A | LOW | Estimated results |
| OptimizedRouteResult | N/A | LOW | Optimized results |
| RoutedNetStatistics | N/A | MED | Routed net stats |
| RoutedPathSubSection | N/A | LOW | Path subsection |
| RoutedPathSubSections | N/A | LOW | Path subsections |
| RoutedPathWithLine | N/A | LOW | Path with line |
| RoutedPathWithTeardrop | N/A | LOW | Path with teardrop |
| RoutedPathWithVia | N/A | LOW | Path with via |
| UnroutedNetStatistics | N/A | MED | Unrouted net stats |

### Gson Adapter Submodule (Not Migrated)

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| GsonLocalizationAdapter | N/A | LOW | Gson adapter (Java-specific) |
| GsonRoutingJobAdapter | N/A | LOW | Gson adapter (Java-specific) |
| GsonTimeAdapter | N/A | LOW | Gson adapter (Java-specific) |
| GsonUtil | N/A | LOW | Gson utility (Java-specific) |

## Summary

- **Done**: 2 classes (11% of Java files)
- **Remaining**: 16 classes (89%)
- **Blockers**: Most analytics/scoring classes are optional for headless routing. ReflectionUtil is Java-specific and not needed. Gson adapters are Java-specific (C++ would use nlohmann::json or similar). RoutingJobScheduler needed for production job management.
