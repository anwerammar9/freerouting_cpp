# Settings Module - Migration Status

**Module**: `src/settings/`
**CMake Target**: `freerouting_settings`
**Java Files**: 21 | **C++ Headers**: 6 | **C++ Impls**: 3 | **Tests**: 0

---

## Completed (Fully Migrated)

| Java Class | C++ Header | C++ Impl | Test | Java Lines | Notes |
|------------|-----------|----------|------|-----------|-------|
| RouterSettings | RouterSettings.h | RouterSettings.cpp | - | 461 | Router settings |
| RuntimeEnvironment | RuntimeEnvironment.h | RuntimeEnvironment.cpp | - | 37 | Runtime info |

## Partially Migrated

| Java Class | C++ Header | C++ Impl | Java Lines | What's Missing |
|------------|-----------|----------|-----------|----------------|
| GlobalSettings | GlobalSettings.h | - | 467 | Implementation file needed |
| OptimizerSettings | OptimizerSettings.h | - | N/A | Implementation file needed |
| ProfileSettings | ProfileSettings.h | - | N/A | Implementation file needed |
| SettingsSource | SettingsSource.h | - | N/A | Interface implementation |

## Not Migrated

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| DesignRulesCheckerSettings | 721 | HIGH | DRC settings |
| FeatureFlagsSettings | 505 | MED | Feature flags |
| RouterOptimizerSettings | 2123 | HIGH | Optimizer configuration (very large) |
| RouterScoringSettings | 2544 | HIGH | Scoring configuration (very large) |
| SettingsMerger | 122 | MED | Config merging logic |
| StatisticsSettings | 1253 | MED | Statistics configuration |
| UsageAndDiagnosticDataSettings | 1064 | LOW | Usage/diagnostics |
| UserProfileSettings | 442 | LOW | User profile config |

### Sources Submodule (Not Migrated)

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| ApiSettings | N/A | LOW | API settings source |
| CliSettings | 115 | MED | CLI settings parsing |
| DefaultSettings | N/A | MED | Default settings values |
| DsnFileSettings | N/A | MED | DSN file settings |
| EnvironmentVariablesSource | 129 | MED | Env var source |
| GuiSettings | N/A | LOW | GUI settings |
| JsonFileSettings | 76 | MED | JSON file settings |
| RulesFileSettings | N/A | MED | Rules file settings |
| SesFileSettings | N/A | MED | SES file settings |

## Summary

- **Done**: 2 classes (10% of Java files)
- **Partial**: 4 classes (19% - header-only)
- **Remaining**: 15 classes (71%)
- **Blockers**: RouterOptimizerSettings (2123 lines) and RouterScoringSettings (2544 lines) are very large and critical for router configuration. SettingsMerger needed for config pipeline. Sources submodule handles settings I/O from multiple sources.
