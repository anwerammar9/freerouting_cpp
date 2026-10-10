# M7 — Full-CLI stragglers (in scope by decision)

## T701 — CLI DRC mode  (S)

**Java.** `Freerouting.initializeDrc` 377–507 (input required, session + job, `setInput`, load board through
`HeadlessBoardManager`, optional SES/rules from `-de a.dsn b.ses`, run `DesignRulesChecker`, write the report JSON
including `qualityScore` and `optimizerScore`, exit code), `drc/DesignRulesChecker.java` report generation (find the
method that builds the JSON report and its `@SerializedName` field names), `drc/DrcReport*` classes.

**C++ today.** `Freerouting::InitializeDRC` (src/Freerouting.cpp ≈293–335) prints `{}`. `src/drc/DesignRulesChecker`
and `DrcReport` JSON exist.

**Deps.** T117, T124a (board loading), T203 (SES import for `-de x.dsn x.ses`), T421 (scores in the report), T303
(JSON writer).

**Tests.** `freerouting -de Issue026.dsn Issue026_java.ses -drc out.json` → key set and values equal the Java report for
the same inputs (store the Java report under `tests/data/expected/`).

## T702 — RoutingJobScheduler + action thread (headless, synchronous)  (S)

**Java.** `management/jobs/RoutingJobScheduler.java` (600): the scheduler loop inside the private ctor 48–310 (picks
READY jobs, loads the board through `HeadlessBoardManager`, **auto-loads a sibling `<design>.rules` file** (≈132), adds
`RulesFileSettings`/`SesFileSettings` to the merger (≈106), resumes from an initial session), `enqueueJob` 325–371,
`cancelJob` 572–600; skip `saveJob`/`saveJobToDisk` 372–482 unless `autorouter.saveIntermediateStages` is used by a
test board. `RoutingJobSchedulerActionThread.java` (372): `threadAction` 39–271 (timeout monitor thread at
`router.job_timeout`, `RoutingPipeline.createForHeadless(job).run()`, stage listeners 124–190, state/exit handling),
`monitorCpuAndMemoryUsage` 272–322, `setJobOutput` 323–372. `management/jobs/ThreadActionListener.java` (14).

**C++ design.** No scheduler thread pool is needed for the CLI: run the job synchronously on the main thread, with a
watchdog thread (or a `datastructures::TimeLimit`) implementing the job timeout like Java's monitor (state
`TIMED_OUT`, stop request to the router).

**Tests.** Job with `job_timeout=00:00:01` on a medium board → state TIMED_OUT, output still written (Java behaviour —
verify in `Freerouting.writeCliOutputIfAvailable` 263–341); sibling `.rules` file is picked up.

## T703 — Scoring stragglers  (S)

**Java.** `core/scoring/BoardScoreBreakdown.java` (192: `of(stats, weights)` 125, `validateWeights` 144,
`toSummaryString` 166), `ScoringWeightComparison.java` (232: `compare` 41, `Result`), `BenchmarkScoreCalculator.java`
(251: `run(input, output)` 57 for `--calculate-benchmark-scores.input=/.output=`, `calculateScores` 113, run-array
processing 142–250 — post-processes a `benchmarks.json`). Bounds (`BoardStatisticsBounds*`) moved to T421.

**Tests.** Breakdown of a known BoardStatistics equals Java's summary string; benchmark calculator on a small
hand-written `benchmarks.json` gives Java's output.

## T405 / T406 — BoardComparator (`--compare-boards=a,b`)  (S)

**Java.** `board/state/BoardComparator.java` (758): `compare(...)` 37–644 (split T405 = 37–≈340, T406 = ≈340–644 + helpers
645–747 + `ComparisonResult` 748–758). Caller: `Freerouting.compareBoardFiles` 960–998, `loadBoardFromFile` 999–1032.

**Tests.** Comparing a board with itself → equal; with a copy that has one moved via → report names it.

## T705 — Final docs sync  (S)

Regenerate `02_PROGRESS.md`, `03_MISSING.md`, `docs/migration/INDEX.md` from `JavaToCppClassMap.csv` (T006 statuses) and
`E2E_STATUS.md`; move the WP-10 increment logs to `docs/migration/history/`; README Quick Status shows `pending = 0`.
