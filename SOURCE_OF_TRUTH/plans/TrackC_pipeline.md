# Track C — Autoroute pipeline (Java autoroute/pipeline ≈ 5.4k lines)

## How the Java headless route runs (call chain to reproduce)

```
RoutingJobSchedulerActionThread.run (management/jobs, T702)
 └ RoutingPipeline.createForHeadless(job).run()              RoutingPipeline.java 50, 86
    ├ runRoutingStage 92–120
    │  ├ routerEnabled = settings.getRunRouter() && (autorouter.maxPasses == null || >= 0)
    │  ├ routerEnabled        → BatchAutorouter.runBatchLoop() → new AutorouteBatchLoop(...).run()
    │  ├ else fanout enabled  → same, with autorouter.maxPasses temporarily 0
    │  └ job.board.finishAutoroute()                          RoutingBoard.java 900
    └ runOptimizationStage 121–135 → BatchOptimizer.runBatchLoop()
AutorouteBatchLoop.run (40–661)
 ├ setup, BoardHistory bh (79)
 ├ SMD fanout pre-pass → BatchFanout.fanoutBoard (110–262, fills job.resultPhaseMetrics.fanout)
 ├ while (continueAutorouting && !stop) (301–571)
 │   bh.add(board); continueAutorouting = router.autoroutePass(pass)   → AutoroutePassRunner.runSingleThread (160–345)
 │   restore-best-board check every STOP_AT_PASS_MODULO(4) passes from STOP_AT_PASS_MINIMUM(8) (346–458)
 │   stagnation detection: STAGNATION_PASS_LIMIT(10), STAGNATION_SCORE_THRESHOLD(0.5), one-time fanout-via recovery (459–571)
 └ finish: restore best board from history, removeTails, bh.clear(), phase metrics, events (572–661)
AutoroutePassRunner.runSingleThread: for each item from BatchAutorouter.getAutorouteItems (347–435), for each net of the
item: BatchAutorouter.autorouteItem (545–556) → AutorouteConnectionRouter.route (30–167) → RoutingBoard.autoroute (ported)
```
Constants: `BatchAutorouter.java` 42–60 (`MAXIMUM_TRIES_ON_THE_SAME_BOARD = 3`, `STOP_AT_PASS_MINIMUM = 8`,
`STOP_AT_PASS_MODULO = 4`, `STAGNATION_PASS_LIMIT = 10`, `STAGNATION_SCORE_THRESHOLD = 0.5F`).

Dead in headless (do not port): `BatchAutorouterThread` (T416 closed), `AutoroutePassRunner.runMultiThread` 41–158,
`BatchAutorouter.autoroutePassMultiThread`, `BatchOptimizer.createForGui`.

Scoring used by the loop: default settings select `RouterScoringVersion.V2_CONTINUOUS` and
`OptimizerScoringVersion.V2_LOWER_BOUND` (DefaultSettings.java 89–94) — so T421 must port the V2 formulas, not just
the legacy score.

C++ today: `src/autoroute/BatchAutorouter.{h,cpp}` (317 lines, interim star loop "stop after 2 passes without
progress"), `NamedAlgorithm` (partial), `NamedAlgorithmType`, `TaskState`, events headers, `BoardHistoryEntry`,
`OptimizeRouteTask` (stale draft — delete it in T412 if unused).

Order: **T401 → T402 → T403 → T422 → T404**, **T420** (anytime), **T421** (after T302), **T407 (T404,T420) → T408a →
T408b → T409 → T410 → T411**, **T409 → T412 → T413 → T414 → T423**, **T415**, **T704 → T706**.

Every task here: keep emitting the `compare_trace_*` records at the same call sites as Java (M3 tool depends on them).

---

## T401 — AutorouteAirlineCalculator  (S)

**Java.** `autoroute/pipeline/AutorouteAirlineCalculator.java` (214, whole): `calculateAirline(from, to)` 16–41,
`nearestPointOnTrace` 42–84, `findClosestPointsBetweenTraces` 85–161, `calculateItemDistance` 162–178,
`calculateMinDistance` 179–203, `getItemReferencePoint` 204–214.

**C++ target.** `src/autoroute/AutorouteAirlineCalculator.{h,cpp,_test.cpp}` (static functions in a namespace or a
class with deleted ctor, like Java).

**Reuse.** `geometry::planar::FloatLine`, `FloatPoint::distance`, `PolylineTrace` corners, `DrillItem::get_center`.

**Tests.** Two pins → airline between centres; pin to trace → nearest point on the trace; trace to trace → closest
points; distance of an item (Java semantics).

---

## T402 — AutorouteConnectionRouter  (S)

**Java.** `autoroute/pipeline/AutorouteConnectionRouter.java` (270): `route(...)` 30–167 (calls
`RoutingBoard.autoroute`, neckdown retry, strict-DRC post check), `retryConnectionNecked` 168–248,
`applyStrictDrcAfterRoute` 249–270 (uses `BatchAutorouter.enforceStrictDrc` 307–332 — port that static helper here
or in T409 and call it).

**C++ target.** `src/autoroute/AutorouteConnectionRouter.{h,cpp,_test.cpp}`.

**Reuse.** `RoutingBoard::autoroute(...)` (src/autoroute/RoutingBoardAutoroute.cpp), `AutorouteControl`,
`AutorouteAttemptResult`, `drc::DesignRulesChecker`.

**Tests.** Simple 2-pin board → ROUTED; blocked connection → neckdown retry attempted (assert via result state /
details text equal to Java's).

---

## T403 — AutoroutePassRunner part 1: the single-thread pass  (O)

**Java.** `AutoroutePassRunner.runSingleThread` 160–345 (item list from `getAutorouteItems`, `RouterCounters`,
`DesignRulesChecker.calculateAllIncompletes` for the incomplete count, per-item × per-net loop with `maxItems` stop,
`startMarkingChangedArea`, `rippedItemList` as **`TreeSet<Item>`** (descending id, T011) and `rippedItemCosts`
`LinkedHashMap` (insertion order), result classification ROUTED / skipped (ALREADY_CONNECTED, NO_UNCONNECTED_NETS,
CONNECTED_TO_PLANE) / failed with `board.failureLog.recordFailure`, tail removal with
`StopConnectionOption.NONE` or `FANOUT_VIA`, final statistics, return `routed > 0 || notRouted > 0`);
`logIncompleteDetails` 347–368, `logRippedItems` 370–404, `logTraceRouteComparison` 406–446 (needed by M3),
`logTailRemoval` 527–535. Skip `logNet94Items` 448–496 (net-94 debug dump) and the benchmark-profile branches.

**C++ target.** `src/autoroute/AutoroutePassRunner.{h,cpp,_test.cpp}`; it holds a reference to the `BatchAutorouter`
like Java's `router` field.

**Reuse.** `RoutingFailureLog` (exists in src/autoroute), `RouterCounters` (src/core), `BoardStatistics`,
`DesignRulesChecker::calculate_all_incompletes`.

**Tests.** 3-net synthetic board: pass 1 routes all, pass 2 returns false; one blocked net → counted as failed and
recorded in the failure log; ripped item ordering uses descending id.

---

## T404 — AutoroutePassRunner part 2 + AutorouteRuntimeMetrics + AutorouteUnroutedReport  (O)

**Java.** `AutoroutePassRunner.updateProgress` 498–526 (+ event firing), `AutorouteRuntimeMetrics.java` (66: current
thread CPU seconds, peak memory — map to `getrusage(RUSAGE_THREAD)` / `RUSAGE_SELF` maxrss),
`AutorouteUnroutedReport.java` (80: text report of unrouted connections).

**C++ target.** `src/autoroute/AutorouteRuntimeMetrics.{h,cpp}`, `src/autoroute/AutorouteUnroutedReport.{h,cpp,_test.cpp}`.

**Tests.** Report text for a board with 2 unrouted connections equals Java's format.

---

## T420 — Board deep copy + state hash  (O)

**Java.** `board/facade/RoutingBoardUndoFacade.deepCopy` (≈41) — Java serialises the whole board and deserialises
it; `board/facade/BoardSnapshotManager.java` (101): `serialize` 26–44, `deserialize` 46–56, **`getHash` 58–74 (MD5 of
the basic-profile serialisation)**, `generateSnapshot` 75, `popSnapshot` 81, `diffTraces` 86.

**C++ target.** `RoutingBoard::deep_copy() const -> std::unique_ptr<RoutingBoard>` and
`RoutingBoard::state_hash() const -> std::string` (or `uint64_t`); `src/board/BoardSnapshotManager.{h,cpp,_test.cpp}`
for snapshot/pop/diff.

**Design.** No serialisation. `deep_copy` constructs a new board with copied rules (incl. nets, net classes, clearance
matrix, via infos/rules), library (padstacks, packages, logical parts), components, communication, and **re-inserts
copies of every item with the same id** (so item order and ids are identical), then rebuilds search trees. Every item
class needs a clone: today `copy(int id)` exists on `ObstacleArea`, `ViaObstacleArea`, `ComponentObstacleArea`,
`ComponentOutline`, `ConductionArea` (returning `std::unique_ptr<Item>`) and on `Via` (returning a raw `Item*`), but it
is not a common virtual. Make `virtual std::unique_ptr<Item> copy(int p_id_no) const` on `Item` (Java
`Item.copy(int)`), align `Via`, and add it for `Pin`, `PolylineTrace`, `BoardOutline` and any other item class. `state_hash` hashes, in descending id order, each item's id, type,
layers, nets, fixed state and geometry (corners / centre / area corners) — it must be equal for equal boards and change
on any geometric/net change. It does **not** need to equal Java's MD5 (BoardHistory only compares hashes within one
run).

**Tests.** copy independence (mutate copy → original unchanged, and vice versa); copy has same item ids and order;
`state_hash(copy) == state_hash(original)`; hash changes after inserting a trace, moving a via, changing a net;
`diffTraces` semantics.

---

## T421 — Router and optimizer scores (V2) + bounds  (S)

**Java.** `core/scoring/BoardStatistics.java` 632–843: `calculateScore`, `getMaximumScore`,
`getLegacyNormalizedScore`, `getRouterScore(RoutingCostSettings|RouterSettings)`, **`getV2RouterScore` 688–737**,
`ensureDifficulty` 743–778, `legacyScoringOrDefault`, `getOptimizerScore(...)`, **`getV2OptimizerScore` 808–835**
(needs `bounds`); `core/scoring/BoardStatisticsBounds.java` (17), **`BoardStatisticsBoundsCalculator.java` (268)**;
`drc/DesignRulesChecker.calculateAllIncompletes` parity (check the C++ port returns identical counts — it uses
NetIncompletes → T010 Delaunay).

**C++ target.** `src/core/scoring/BoardStatistics.{h,cpp}` (add the methods; keep the existing `calculateScore` API
callers compiling), `src/core/scoring/BoardStatisticsBounds{,Calculator}.{h,cpp,_test.cpp}` (in `freerouting_scoring`
— 01_MEMORY layering rule).

**Tests.** For three small boards (after T117 they can be real DSNs; before, synthetic): `get_router_score` (V2 and
legacy) and `get_optimizer_score` (V2) equal values printed by a throwaway Java program using the same board and
default settings.

---

## T422 — `RoutingBoard::finish_autoroute` / `clear_transient_autoroute_state`  (S)

**Java.** RoutingBoard.java 900–917: `finishAutoroute` clears and drops the autoroute engine;
`clearTransientAutorouteState` = finish + `clearAllItemTemporaryAutorouteData` + `changedArea = null` +
`shoveFailingObstacle = null` + `shoveFailingLayer = -1`.

**Reuse.** `RoutingBoard::clear_all_item_temporary_autoroute_data` (exists since T417).

**Tests.** After `autoroute` + `finish_autoroute`, the engine is gone and its rooms are removed from the search tree
(no dangling pointers — 01_MEMORY gotcha 16).

---

## T407 — BoardHistory + BoardSnapshotManager glue  (S)

**Java.** `autoroute/BoardHistory.java` (221): ctors 38–94 (history size `MAX_HISTORY_SIZE = 30`, line 30 — the old
TASKS.md title "BOARD_RANK_LIMIT=50" was wrong),
`add` (deep copy + hash + score), `contains` 107–133 (by hash), `getMaxScore` 134–176, `restoreBoard(maxTries)`,
`restoreBestBoard` 177–181, `size` 182, `getRank` 192–221, `clear`.

**C++ target.** `src/autoroute/BoardHistory.{h,cpp,_test.cpp}` (reuse/replace `BoardHistoryEntry`).

**Reuse.** T420 `deep_copy`/`state_hash`, T421 `get_router_score(RouterSettings)`.

**Tests.** add 3 boards with different scores → max score/rank; duplicate board (same hash) not added twice;
`restoreBoard(3)` returns the best board at most 3 times, then the next best (Java semantics).

---

## T408a — AutorouteBatchLoop part 1: setup, fanout pre-pass, pass loop, restore cycle  (O)

**Java.** `AutorouteBatchLoop.run` 40–458: settings/layer logging, BoardHistory, SMD fanout pre-pass 110–262 (pin
counting, `BatchFanout.fanoutBoard`, escape statistics, `job.resultPhaseMetrics.fanout` before/after/duration/cpu),
unrouted count and router-enabled check, main `while` loop 301–458 (`bh.add`, `autoroutePass`, restore check at
`currentPass % 4 == 0 && currentPass >= 8` using strict `bh.getMaxScore() > boardScoreAfter`, `restoreBoard(3)`,
rank logging, counters reset). Helpers 662–681.

**Deps.** T404, T407, T421, T422 (and T410/T411 for the fanout call — until then call a stub that returns an empty
summary and mark the call site `// T411`).

**C++ target.** `src/autoroute/AutorouteBatchLoop.{h,cpp,_test.cpp}`.

## T408b — AutorouteBatchLoop part 2: stagnation, recovery, final best-board restore  (O)

**Java.** `run` 459–661: stagnation detection (pass-local and global best trackers, 0.5 threshold over 10 passes,
fully-routed-with-positive-score special case), one-time fanout recovery `removeTails(NONE)`, end of loop: restore
the best history board if better than the final one (572–), cleanup `removeTails(NONE)` when complete and fanout used,
`bh.clear()`, autorouter phase metrics (≈614), cancellation vs timeout distinction for `TaskStateChangedEvent`.

**Tests (both parts).** Synthetic boards where (a) everything routes in pass 1 → loop ends after pass 1; (b) one net
cannot route → loop stops by stagnation after the Java number of passes; (c) a pass makes the board worse → restore
fires at pass 8/12. Assert pass counts and restore events against the same scenario run in Java (throwaway harness)
or, at minimum, against the rule text above.

---

## T409 — BatchAutorouter rewired onto PassRunner + BatchLoop  (O)

**Java.** `autoroute/pipeline/BatchAutorouter.java` (603): ctors 112–160, settings accessors 162–198, benchmark profile
198–246 (optional; keep behind a flag), `autoroutePassesForOptimizingItem` 247–284, `getImpactedPoints` 285–306,
`enforceStrictDrc` 307–332, `isFanoutTimedOut` 333, `shouldFireBoardUpdate` 337, **`getAutorouteItems` 347–435**
(item selection incl. ignored net classes, plane items via `isPlaneItem` 436–448), `autoroutePass` 457
(→ `runSingleThread`), NamedAlgorithm metadata 462–503, `runBatchLoop` 517, `buildUnroutedConnectionsReport` 521,
`removeTails` 525–544, `autorouteItem` 545–556, `getAirLine/setAirLine`, `calculateIncompleteCount` 594–603.

**C++ target.** Rewrite `src/autoroute/BatchAutorouter.{h,cpp}`; delete the interim `route_all` loop; update
`BatchAutorouter_test.cpp` expectations from Java behaviour.

**Java parity notes.** `getAutorouteItems` iterates the board item list (descending id, T011) — the order of this list
is the routing order. Keep every filter in Java's order.

**Done when.** Issue026 with fanout and optimizer off routes with the same pass count and unrouted count as Java
(`first_divergence.py` from M3 on pass records).

---

## T410 / T411 — BatchFanout  (S)

**Java.** `autoroute/pipeline/BatchFanout.java` (800). T410: ctor 35–80, `fanoutBoard` 81–165 (two overloads,
component/pin sorting by `pinSortingOrder`), inner `Component` 651–720 and `Component.Pin` 722–800 (`compareTo`
orders), records `EscapeStatistics` 611–628, `FanoutPassStatus` 629–644, `FanoutRunSummary` 645–650, interface
`FanoutProgressListener` 600–610. T411: `fanoutPass` 166–527 (per-pin `RoutingBoard.fanout`, per-pin time limit
`maxMillisecondsPerPin` — **wall-clock dependent**, ripup), progress publishing 528–599.

**Reuse.** `RoutingBoard::fanout` (src/autoroute/RoutingBoardFanout.cpp, 01_MEMORY gotcha 30), T305 via fallback.

**Java parity notes.** The per-pin time limit makes results depend on machine speed; for parity runs set
`--router.fanout.max_milliseconds_per_pin` very high on both sides.

**Tests.** Board with 2 SMD components: pins fanned out in Java's order (`outer_first`), escape statistics equal.

---

## T412 / T413 / T414 — BatchOptimizer  (O)

**Java.** `autoroute/pipeline/BatchOptimizer.java` (1250).
- **T412** (structure): ctor/factories 73–107 (`createForHeadless`), `isTimedOut` 108, `containsOnlyUnfixedTraces`
  112, CPU/alloc/heap sampling 121–156 (map to getrusage; allocation MB has no C++ equivalent → report 0 and note it),
  `evaluatePreFlightGuards` 157–226, `areAllViasMandatoryLayerTransitions` 227–279, `prepareCandidateItems` 589–631,
  inner `ReadSortedRouteItems` 1158–1250, `calculateIncompleteCount` 1146–1157, NamedAlgorithm metadata 1117–1145.
- **T413** (per item): `optRoutePass` 632–831, `optRouteItem` 832–853, `optRouteItemOnBoard` 854–942,
  `getItemPosition` 943–951.
- **T414** (loop/acceptance): `runBatchLoop` 280–558 (pass structure, improvement threshold, optimizer phase metrics
  ≈297, timeout), `restoreIncumbentBoard` 559–570, `optimizerCandidateRejectionReason` 571–588.
- The worker classes `CandidateResult` 952–977, `WorkerBoardState` 978–1008, `OptimizeCandidateTask` 1009–1116 are
  the thread pool — **T423**. In T412–T414 run candidates sequentially in item order (equivalent to Java with one
  thread).

**C++ target.** `src/autoroute/BatchOptimizer.{h,cpp,_test.cpp}`; delete `OptimizeRouteTask.*` if nothing uses it.

**Reuse.** `PullTightAlgo*`, `OptViaAlgo`, `RoutingBoard::opt_changed_area`, T420 `deep_copy`, T421 optimizer score,
`BatchAutorouter::autoroute_passes_for_optimizing_item`.

**Tests.** Board with one detour trace → optimizer shortens it (score improves, accepted); candidate that worsens the
score → rejected with Java's rejection reason text; preflight guard trips on a board with only fixed traces.

---

## T423 — Optimizer thread pool  (O)

**Java.** `BatchOptimizer` worker classes 952–1116 and the executor use inside `runBatchLoop`/`optRoutePass`: each
worker gets its own board copy (`WorkerBoardState.acquire` = deep copy of the baseline), candidates are evaluated in
parallel, **results are consumed in item order**, worker boards are reset with `clearTransientAutorouteState` (T422).

**C++ target.** `std::thread` pool sized by `optimizer.max_threads` (`-mt`), per-worker `RoutingBoard` from T420.

**Tests.** Same SES for `-mt 1` and `-mt 4` on a small board with optimizer on; ThreadSanitizer run once
(`-fsanitize=thread` build in a scratch dir) without reports.

**Watch out.** Shared statics: the `JavaRandom` instance in `PolygonShape` is `thread_local` in C++ — Java uses one
static instance shared by all threads (a Java race). Per-thread is the deterministic choice; note the deliberate
deviation.

---

## T415 — RoutingPipeline + NamedAlgorithm + TaskState; CLI route path  (S)

**Java.** `autoroute/pipeline/RoutingPipeline.java` (148, whole; headless factory 50, `run` 86–90, stages 92–135,
`normalizeRouterAlgorithm` 136–148), `NamedAlgorithm.java` (126: listeners/events), `NamedAlgorithmType.java`,
`TaskState.java`.

**C++ target.** `src/autoroute/RoutingPipeline.{h,cpp,_test.cpp}`; complete `NamedAlgorithm`; call the pipeline from
`src/Freerouting.cpp` (until T702 exists, call it directly after loading the board).

**Tests.** Pipeline with router disabled and fanout enabled runs the batch loop with `max_passes` temporarily 0 and
restores it afterwards (Java 108–115); `finish_autoroute` is called after the routing stage.

---

## T704 — RoutingResultManifest (`--router.result_json=<path>`)  (S)

**Java.** `core/results/RoutingResultManifest.java` (256): schema fields 24–76 (`schema_version`, `generated_at`,
`app_version`, `git_sha`, `fixture{filename,sha256}`, `settings_snapshot`, `phases{fanout,autorouter,optimizer}` each
`{before, after, duration_seconds, cpu_seconds, passes_completed, total_allocated_gb, peak_heap_mb}`,
`board_statistics`, `bounds`, `normalized_score`, `optimizer_score`, `resource_usage`, `final_state`, `exit_code`,
`output_written`, `cpu_score`), `PhaseSnapshot.fromBoardStatistics` 148–160, `fromJob` 162–221, `write` 222–230,
`resolveGitSha` 231–245; caller `Freerouting.writeCliResultManifestIfRequested` 352–376.

**Deps.** T415, T421, T303 (JSON writer). **This is the benchmark's per-phase data channel on both sides** (M8).

**C++ target.** `src/core/results/RoutingResultManifest.{h,cpp,_test.cpp}` (library: `freerouting_scoring` or a new
`freerouting_results` — mind the layering rule in 01_MEMORY), hook in `src/Freerouting.cpp`.

**Tests.** Manifest JSON for a finished small job contains every key in the Java schema with the same names and types;
compare key sets against a manifest produced by the Java jar for the same board.

---

## T706 — RoutingJob phase metrics + resource usage  (S)

**Java.** `core/RoutingJob.java` 134 (`resultPhaseMetrics`), `resourceUsage` fields (`cpuTimeUsed`, `peakMemoryUsed`)
updated by `RoutingJobSchedulerActionThread` 265–320; phase details are written by AutorouteBatchLoop (fanout ≈244,
autorouter ≈614) and BatchOptimizer (≈297).

**C++ target.** `src/core/RoutingJob.{h,cpp}` (`result_phase_metrics`, populate `RouterJobResourceUsage`); CPU via
`getrusage(RUSAGE_SELF)` user+sys, peak memory via `ru_maxrss` (KB → MB). Java reports heap MB — record both
`peak_rss_mb` (C++) and keep the Java key `peak_memory_mb` meaning "process peak" for C++; document the difference in
BENCHMARK.md.

**Tests.** After a pipeline run all three phase entries exist and durations are > 0 for enabled phases.
