# Track B — Settings and CLI (gate G5)

Independent of the parser; can run in parallel with M1/M2 in a separate worktree.

Order: **T301 → T302 → T303 → T304**, **T302 → T305**, **T303 → T306**, **T302 → T307**.

## The Java settings model (read once, applies to every task here)

- `RouterSettings` (settings/RouterSettings.java, 1042) holds **nullable boxed** public fields (`Boolean`, `Integer`,
  `Double`, `String`, arrays) and nested settings objects: `autorouter` (AutorouterSettings, 58), `fanout`
  (FanoutSettings, 123), `optimizer` (OptimizerSettings, 127), `scoring` (RoutingCostSettings, 86), `routerScoring`
  (RouterScoreSettings, 41), `optimizerScoring` (OptimizerScoreSettings, 35), layer settings (LayerSettings, 75).
  Every JSON / CLI name is the field's `@SerializedName`.
- **null means "not set by this source".** `SettingsMerger.merge()` (settings/SettingsMerger.java) sorts sources by
  priority, clones the first non-null `RouterSettings` as base and applies the others with
  `RouterSettings.applyNewValuesFrom` → `ReflectionUtil.copyFields(source, target)` (util/ReflectionUtil.java
  215–≈340): copy every public non-static field whose source value is non-null (primitives: only if not the type's
  default value); arrays of primitives/strings are copied if the target is null or the source array is non-empty;
  arrays of objects are merged element-wise; nested objects recurse. Then `validate()` (RouterSettings 1006–1042).
- Source priorities (verified): Default 0, JsonFile 10, DsnFile 20, SesFile 30, RulesFile 40, EnvironmentVariables 55,
  Cli 60, (Gui 65, Api 70 — out of scope).
- Defaults live in `settings/sources/DefaultSettings.java` 128–236 (e.g. `autorouter.maxPasses = 0` = unlimited,
  `jobTimeoutString = "12:00:00"`, `maxThreads = autorouter.maxThreads = optimizer.maxThreads = max(1, cores-1)`,
  fanout on with `maxMillisecondsPerPin = 10000`, optimizer on with `maxPasses = 100`, improvement threshold 2.5,
  scoring defaults via `DEFAULT_*` constants at the top of the file).
- The CLI builds `SettingsMerger(DefaultSettings, JsonFileSettings, CliSettings(args), EnvironmentVariablesSource)`
  (Freerouting.java 1596–1602), then per job adds `DsnFileSettings` and, with `-dr`, `RulesFileSettings`
  (Freerouting.java 131–151), then `routingJob.routerSettings = settingsMerger.merge()`.

**C++ representation (decide once in T301, keep everywhere):** one `struct` per Java settings class with
`std::optional<T>` for every boxed field, plain `T` for Java primitives, `std::optional<std::vector<T>>` for arrays,
nested structs by value. A hand-written `int copy_fields(const X& src, X& dst)` per struct reproduces
`ReflectionUtil.copyFields` (no reflection). A static field table
`{ "json_name", getter, setter-from-string }` per struct serves JSON load/save (T303), `--router.x.y=v` (T304) and
`FREEROUTING__ROUTER__X__Y` (T306) — write it once in T301 and reuse it.

**Remove the landmines first.** `src/settings/OptimizerSettings.h` declares invented `RouterOptimizerSettings` and a
**second** `RouterScoringSettings` (with `via_cost/corner_cost/length_cost`) in the same namespace as
`settings/RouterScoringSettings.h` — exactly 01_MEMORY gotcha 14. Delete them in T301 and fix the users.

---

## T301 — Settings data model: all settings structs, `copy_fields`, `clone`, `validate`  (S)

**Java.** Field declarations of RouterSettings.java 20–140 and the nested classes AutorouterSettings, FanoutSettings,
OptimizerSettings (incl. enums `BoardUpdateStrategy`, `ItemSelectionStrategy` — find their files under
`java/app/freerouting/`), RoutingCostSettings, RouterScoreSettings, OptimizerScoreSettings, LayerSettings,
RouterScoringVersion, OptimizerScoringVersion; `RouterSettings.clone` 521–564, `applyNewValuesFrom` 979–1004,
`validate` 1006–1042, `defaultMaxThreads/normalizeMaxThreads` 143–160; `ReflectionUtil.copyFields` 215–≈340.

**C++ target.** `src/settings/{AutorouterSettings,FanoutSettings,OptimizerSettings,RoutingCostSettings,
RouterScoreSettings,OptimizerScoreSettings,LayerSettings}.h` (+ `.cpp` for `copy_fields`, `_test.cpp`),
`src/settings/SettingsFieldTable.h` (the field-table helper), and rework `RouterSettings.{h,cpp}` to hold the Java
fields. Keep the existing public getters (`get_via_costs`, `is_layer_active`, …) compiling: re-implement them on top
of the new fields with Java's null fallbacks (T302 completes them). `RouterScoringSettings` (existing, used by
`BoardStatistics`) becomes or wraps `RoutingCostSettings` — grep its users before renaming.

**Steps.** 1) Delete the invented classes in `OptimizerSettings.h`, fix compile errors. 2) Write each struct with
Java's field order and `@SerializedName` names. 3) `copy_fields` per struct; unit test it against hand-made inputs
covering: null source field (no copy), primitive default (no copy), non-empty array (copy), empty array onto null
target (copy), nested struct recursion, object array merge. 4) `validate()` incl. the `maxPasses` and `maxThreads`
rules (`maxThreads` capped at `std::thread::hardware_concurrency()`). 5) `clone()` is the copy constructor.

**Done when.** Old and new tests green; `grep -rn "RouterOptimizerSettings\|corner_cost" src` empty.

---

## T302 — RouterSettings behaviour: getters/setters and per-layer accessors  (S)

**Java.** RouterSettings.java 162–975 except `applyBoardSpecificOptimizations` (T307): `setMaxPasses` 184,
`setMaxThreads` 193, `getAutorouterMaxThreads` 216, `setJobTimeoutString` 223, `setEnabled` 232,
`setViasAllowed` 241/250, `setAlgorithm` 255, `setOptimizerEnabled` 264, `getLayerCount/setLayerCount` 476–520,
`getNeckWidthUm` 565, `isStrictDrc` 570, `get/setStartRipupCosts` 575–587, `get/setRunRouter` 588–598,
`get/setRunOptimizer` 599–611, `getRunFanout/isFanoutEnabled/setFanoutEnabled` 612–633, `getViasAllowed` 634,
`get/setViaCosts` 639–651, `get/setPlaneViaCosts` 652–664, `get/setPlaneNets` 665–678, `PlaneAsObstacle` 679–702,
`get/setLayerActive` 703–746, `get/setBendCost` 747–782, `PreferredDirectionIsHorizontal` 783–828,
`PreferredDirectionTraceCosts` 829–869, `getAgainstPreferredDirectionTraceCosts` 870–889, `getHorizontalTraceCosts`
890–909, `setAgainstPreferredDirectionTraceCosts` 910–932, `getVerticalTraceCosts` 933–948, `getTraceCosts`
949–963, `get/setAutomaticNeckdown` 964–978. Property-change listeners (162–175) are GUI-only — skip.

**C++ target.** `src/settings/RouterSettings.{h,cpp}`; tests in `RouterSettings_test.cpp`.

**Java parity notes.** Each getter has a null fallback (e.g. `getStartRipupCosts` returns 1 if unset? check each
one); keep them exactly. T109a added several of these with clamps — reconcile, do not duplicate (one
`get_start_ripup_costs` only).

**Tests.** For each getter: unset → Java fallback; set → value; per-layer setters out of range → warning and no
change (as T109a does).

---

## T303 — SettingsMerger, GlobalSettings fields, JSON load/save  (S)

**Java.** `settings/SettingsMerger.java` (199, whole); `settings/SettingsSource.java` (42); `GlobalSettings.java`
fields 40–154, ctor 155–172, user-data path 173–211, `load` 290–393, `saveAsJson` 394–471, `setDefaultValue` 472,
`applyNonRouterEnvironmentVariables` 485–508, `setValue` 509–531, getters 923–960; nested settings classes that the
headless CLI reads: `LoggingSettings` (42), `DebugSettings` (56), `DesignRulesCheckerSettings` (30),
`FeatureFlagsSettings` (26), `StatisticsSettings` (62), `RuntimeEnvironment` (174), `AppPaths` (282 — only the
user-data/config/log path logic).

**C++ target.** `src/settings/SettingsMerger.{h,cpp,_test.cpp}`, `src/settings/Json.{h,cpp,_test.cpp}` (minimal
hand-rolled JSON value + parser + writer: objects, arrays, strings with escapes, numbers, true/false/null — the
project rule is "no JSON library"), rework `GlobalSettings.{h,cpp}`, `SettingsSource.h` (interface only; concrete
sources in T306).

**Steps.** 1) JSON value type + parser + pretty writer (2-space indent like Gson pretty printing; key order = field
order). 2) Field tables from T301 drive (de)serialisation of `RouterSettings` and the other settings. 3) Merger
exactly like Java (priority sort, clone base, `copy_fields`, `validate`). 4) GlobalSettings load/save to
`<user_data_path>/freerouting.json`.

**Tests.** Merger: three fake sources with priorities 0/10/60 setting overlapping fields → result equals Java
semantics; JSON round trip of a full RouterSettings; loading a `freerouting.json` produced by the Java jar (create one
by running the jar once with a fresh `--user_data_path`) yields the same values.

---

## T304 — CLI parity  (S)

**Java.** `GlobalSettings.applyCommandLineArguments` 541–919 (all flags — table below), `isMultiFileConcatenation`
961–996; `Freerouting.main` 1033–… (only the headless path: user-data path 1074–1083, logging flags 1160–1200,
`-help`), `initializeCli` 85–254 (job creation, output file deletion, settings merger, wait loop),
`writeCliOutputIfAvailable` 263–341, `computeCliExitCode` 342–351, `writeCliResultManifestIfRequested` 352–376.

**Flag table (verified in the Java source).**
| Flag | Java effect |
|---|---|
| `--compare-boards=a,b` | compare mode (needs T405/T406) |
| `--<section>.<key>=<v>` | generic setter through `@SerializedName` path (`--router.result_json=…`, `--router.job_timeout=…`, `--router.optimizer.timeout=…`, `--router.fanout.max_milliseconds_per_pin=…`, …); `--debug.enable_detailed_logging/single_step_execution/trace_insertion_delay/filter_by_net` handled specially; `--user_data_path=` handled before everything else; unknown → warning |
| `-de f1 [f2 …]` | files by extension: `.dsn` input, `.json` input (if no dsn yet) else session, `.ses` session, `.rules` rules; `a.dsn+b.rules` concatenation supported; duplicates → warning, last wins |
| `-di dir` | GUI input dir (ignore) |
| `-do f1 [f2 …]` | first = output file, rest = additional outputs; `+` concatenation |
| `-drc [report.json]` | must be checked before `-dr`; disables autorouter, enables DRC, optional report file |
| `-dr file` | rules file |
| `-mp N` | `autorouter.maxPasses = Integer.decode(N)` clamped 0..9999 (0 = no limit) |
| `-mt N` | **`optimizer.maxThreads` only**, clamped 0..1024 |
| `-oit F` | `optimizer.optimizationImprovementThreshold = (0<F<1) ? F*100 : F` |
| `-us X` | `optimizer.boardUpdateStrategy = GLOBAL_OPTIMAL` regardless of X (Java quirk) |
| `-is X` | `PRIORITIZED` if X starts with `prio`, else `SEQUENTIAL` |
| `-l locale` | locale (accept and ignore beyond storing) |
| `-dl` | `logging.file.enabled = false` |
| `-da` | disable analytics (no-op) |
| `-host name` | runtime host |
| `-help…` | help (only args **starting with** `-help`) |
| `-inc a,b` | `autorouter.ignoreNetClasses` |
| `-dct N` | GUI dialog timeout (store, ignore) |
| `-ll LEVEL` | console log level (upper-cased) |
| anything else | `"Unknown command line argument: …"` warning |
Exceptions during parsing are caught per argument (`"There was a problem parsing the '…' parameter"`).

**C++ target.** `GlobalSettings::apply_command_line_arguments(const std::vector<std::string>&)` and the generic
`set_value(path, value)` (field tables), `src/Freerouting.cpp` main/CLI flow (output deletion, exit codes,
result manifest hook — the manifest itself is T704). Fix today's bugs: `-mp` clamp/default, `-mt` ignored, `-oit`
divides by 100, `-us/-is/-inc` no-ops, `-dl` disabling the autorouter, `--router.*` only logged, any argument
containing "help" printing help.

**Tests.** `GlobalSettingsCli_test.cpp`: one test per table row (feed the args, assert the settings), plus
`Integer.decode` hex (`-mp 0x10` → 16), clamping, order dependence (`-drc` vs `-dr`), unknown flag warning.

---

## T305 — `fallbackToBoardVias` fanout via-rule combination  (S)

**Java.** Search `fallbackToBoardVias` / `fallback_to_board_vias` in `autoroute/` and `board/` (FanoutSettings.java
101; the consumer builds the via rule used by fanout from the fanout via diameters and falls back to the board's via
rules). Port the consumer logic into the C++ fanout path (`src/autoroute/RoutingBoardFanout.cpp`).

**Tests.** Board with a via rule whose vias are larger than `start_via_diameter_mm`: fallback on → board via used;
off → no via.

---

## T306 — Settings sources  (S)

**Java.** `settings/sources/DefaultSettings.java` (237), `CliSettings.java` (203), `DsnFileSettings.java` (69, uses
`DsnReader.readMetadata` — T117), `RulesFileSettings.java` (109, uses `RulesReader.readRouterSettings` — T204a),
`JsonFileSettings.java` (79), `EnvironmentVariablesSource.java` (140, `FREEROUTING__ROUTER__<PATH>` with `__` as path
separator), `SesFileSettings.java` (52), `settings/LegacyRouterSettingsBridge.java` (126, deprecated flag aliases and
warnings).

**C++ target.** `src/settings/sources/*.{h,cpp,_test.cpp}`; replace the stubs in `SettingsSource.h`.

**Deps.** T303 (merger), T117 (DSN metadata), T204a (rules) — if T117/T204a are not done yet, land the other sources
and leave these two as `[~]` with a progress note.

**Tests.** Each source returns only the fields it sets (others null); env var `FREEROUTING__ROUTER__OPTIMIZER__ENABLED=false`
→ optimizer disabled; DSN with `(autoroute_settings (via_costs 33))` → DsnFileSettings sets via costs 33; full merge
order test (default < json < dsn < rules < env < cli).

---

## T307 — `RouterSettings.applyBoardSpecificOptimizations` + scoring versions  (S)

**Java.** RouterSettings.java `applyBoardSpecificOptimizationsIfNeeded` 279–290, `areBoardSpecificTraceCostsApplied`
291–299, **`applyBoardSpecificOptimizations` 300–475**; RouterScoringVersion.java, OptimizerScoringVersion.java.
Called from `HeadlessBoardManager.applyRouterSettingsForLoadedBoard` (T124b).

**Why it matters.** It rewrites per-layer trace costs and other routing settings based on the board (layer count,
preferred directions, …) — routing results depend on it.

**Tests.** Synthetic boards with 2, 4 and 6 signal layers (+ one with a power plane): settings after the call equal
values printed by a throwaway Java program running the same method on the same boards (loaded from small DSN files so
both sides see identical input).
