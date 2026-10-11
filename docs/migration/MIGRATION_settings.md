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

## T301 — Settings data model (2026-10-11)

- One struct per Java settings class (`AutorouterSettings`, `FanoutSettings`, `OptimizerSettings`,
  `RoutingCostSettings`, `RouterScoreSettings`, `OptimizerScoreSettings`, `LayerSettings`) with `std::optional`
  for every boxed Java field (null = "this source has no opinion"), snake_case of the Java field name, Java field
  order. Nested settings objects in `RouterSettings` are held by value (Java's constructor makes them non-null).
- `copy_fields(src, dst)` per struct = `ReflectionUtil.copyFields`. Java details kept: transient public fields
  (`layers`, per-layer trace cost arrays, optimizer strategies) ARE copied; enums are always counted; primitive /
  String arrays copy when the target is null or the source is non-empty; `LayerSettings[]` merges element-wise into
  a long-enough target, otherwise a fresh array is built. Divergence: Java counts boxed copies by reference
  inequality, C++ by value inequality (same resulting values; only the returned count can differ).
- `SettingsFieldTable.h`: `settings_fields<X>()` lists `{java_name, json_name (@SerializedName), alternates,
  is_transient, get, set}`; `set_field_value(obj, "optimizer.max_passes", "5")` reproduces
  `ReflectionUtil.setFieldValue` (path split at `.:-`, case-insensitive/camel/snake name matching, Java
  `convertValue` parsing, comma-separated values across object arrays). Reuse it for JSON (T303, skip
  `is_transient`), CLI (T304) and env vars (T306).
- `RouterSettings::clone()` is Java clone() and drops `result_json_path` (Java quirk); the C++ copy constructor
  copies everything. Accessors use Java's null fallbacks: `get_via_costs()`/`get_start_ripup_costs()` return 1 when
  unset, `get_automatic_neckdown()` false, `get_run_router()` true, `get_run_optimizer()` false. Code that needs real
  defaults must start from `sources::DefaultSettings::create_settings()` (ported early from
  settings/sources/DefaultSettings.java), exactly like Java's SettingsMerger.
- `BoardStatistics::calculateScore/getMaximumScore/getNormalizedScore` take `RoutingCostSettings` (as in Java) and
  unbox with `.value()` (throws where Java throws NPE).
