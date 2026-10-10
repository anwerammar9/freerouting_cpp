# ROADMAP — 100 % headless Freerouting C++ migration, then a Java-vs-C++ benchmark

> Approved 2026-10-10. `TASKS.md` is the executable queue; this file is the rationale, the gates and the
> benchmark design. Corrections made while implementing (the queue is authoritative where it differs):
> * `run_tests` exists (`build/tests/data/run_tests`, ctest `AllTests`) — the planned task T011 is not needed.
> * Circuit + Parser moved out of T117 as **T125** (the ID T123 collides with the format example in TASKS.md).
> * T109 was split: T109a (autoroute settings, done), T109b (rules from structure), T109c (createBoard + planes).
> * T416 closed as dead code (verified: `autoroutePassMultiThread` has no callers).
> * `freerouting_settings` must not link `freerouting_autoroute` (cycle with the RESCAN link groups); add new
>   libraries that designforms needs to the RESCAN groups.

## Context

Freerouting (Java 2.1.0, pruned to a headless subset in `java/app/freerouting/**`) is being ported to
C++20 in `src/**`. Target: `freerouting -de board.dsn -do out.ses` gives the same routing result as the
Java jar, so the two can be benchmarked head to head.

**Where things stand (2026-10-10, branch `migration/dsn-parser-foundations`)**

* Routing core is ported and unit-tested (geometry, rules, board model, maze search, shove, fanout,
  `RoutingBoard::autoroute`); ctest 874/874.
* **Every test board routes 0 wires** (`docs/migration/E2E_STATUS.md`). The DSN loader is hand-rolled, not
  a Java port, so pins collapse onto component origins. The Java-faithful parser is ~half done
  (T101–T108 done; T109 uncommitted and **does not compile**; T110–T118 todo). Consequence: the ported
  maze/shove code has only ever run on synthetic fixtures, never on real boards. That is the largest
  schedule risk.
* `SOURCE_OF_TRUTH/TASKS.md` already holds a dependency-ordered queue (T109 … T705) driven by
  `loop_migration.sh`. **This plan keeps it and corrects/extends it**; nothing is thrown away.
* No benchmark tooling exists beyond `tools/e2e_compare.sh` (one run, wall time, wire/via token count).

**Decisions confirmed with the user:** scope = full headless CLI (route + DRC + SES/rules import + settings
sources + scheduler; excluded: GUI, REST API, MCP, analytics, KiCad I/O, Fusion export). Parity bar =
quality parity as the gate, bit-identical SES in single-thread mode as a stretch. Benchmark = single-thread
(`-mt 1`) headline plus default multi-thread, reporting wall, CPU, peak RSS, per-phase time and quality.

## Definition of "100 % headless" (all gates must hold)

| Gate | Requirement | Check |
|------|-------------|-------|
| G1 | Every headless-path Java class is `ported`; the rest are justified `out_of_scope` | `tools/class_map_check.py` prints `pending = 0` |
| G2 | All `tests/data/**/*.dsn` that Java accepts load; layer/net/component/pin/padstack/netclass/keepout counts **and item-id order** equal Java | C++ `--dump-board-counts` vs Java harness (T007) |
| G3 | Unrouted board → SES token-equal to Java's; routed C++ SES is accepted by `java -jar … -drc` | `tools/ses_diff.py`, Java `-drc` |
| G4 | Per board: #unrouted ≤ Java, 0 DRC violations (Java scorer), wires/vias within ±10 %; stretch: byte-identical SES with `-mt 1` on small boards | `tools/e2e_compare.sh`, T6xx |
| G5 | Every benchmark-relevant CLI flag behaves like Java (table in T304) | CLI test table |
| G6 | `ctest` **and** `run_tests` green, 0 warnings from `src/` | loop gate |

## Corrections to the existing queue (found by this review)

1. **`JavaToCppClassMap.csv` "out_of_scope" is wrong.** Its 124 rows mix real non-headless classes with ~60
   *pending headless* classes (`BoardHistory`, `BatchFanout`, `AutoroutePassRunner`, `SesWriter/Reader`,
   `SettingsMerger` and the settings sources, `RoutingJobScheduler`, `BoardComparator`, facades …).
   "229/353" overstates progress → add a `pending` status (T006).
2. **T109 working tree does not compile** (verified): `RouterSettings.h` declares `get_start_ripup_costs()`
   at lines 96 *and* 160; `parser/ReadScopeParameter.h:18` forward-declares `settings::RouterSettings` inside
   `specctra::parser` (wrong namespace → type mismatch at `Structure.cpp:135`); `tests/CMakeLists.txt:244`
   lists the missing `AutorouteSettings_test.cpp`. Fix these first.
3. **`WriteScopeParameter` was never ported** though T106 is `[x]`; the **write halves** of all parser classes
   (Shape/Rule/Layer/Plane/Structure/Library/Package/Network/Component/Net/Wiring/Parser/Resolution) are in no
   task — T204 names only DsnWriter/RulesWriter/RulesReader.
4. **Ordering bug:** `Circuit` and `Parser` (needed by `NetClass.read`, T114, and by `Plane`'s Allegro
   `host_cad` handling) sit in T117, after T114.
5. **Unowned pieces:** `DsnFile.adjustPlaneAutorouteSettings`, `DsnReader.readMetadata`, `BoardReadResult` /
   `BoardMetadata`, `MinimalBoardManager` shim, `RulesReader.readRouterSettings`; settings sources
   (`DefaultSettings`, `CliSettings`, `DsnFileSettings`, `RulesFileSettings`, `JsonFileSettings`,
   `EnvironmentVariablesSource`, `ReflectionUtil`, `LegacyRouterSettingsBridge`);
   `RouterSettings.applyBoardSpecificOptimizations` (Java 300–430, changes results per board); sibling
   `<design>.rules` auto-load in the scheduler; `RoutingJob` phase metrics / `RouterJobResourceUsage`;
   `RoutingBoard.finishAutoroute` / `clearTransientAutorouteState`; **board deep copy + state hash**
   (Java: serialization + MD5; history, batch loop and optimizer all need it); `getRouterScore` /
   `getOptimizerScore` (V2_CONTINUOUS, V2_LOWER_BOUND).
6. **Over-constrained deps:** T407 (BoardHistory) depends on BoardComparator T405/T406, which a plain route
   never calls (only `--compare-boards`/debug). `BatchAutorouterThread` (T416) and the multi-thread autorouter
   path are dead code in Java (`runMultiThread`/`autoroutePassMultiThread` have no callers) — verify with
   grep, then drop T416.
7. **Determinism blockers in C++** (bit-exact stretch): `geometry/planar/PolygonShape.cpp:67` uses LCG
   multiplier `0x5DEECE66ULL` (Java `0x5DEECE66D`); `PlanarDelaunayTriangulation.cpp` lacks Java's
   `Collections.shuffle(seed 99)` and differs algorithmically (ties on co-circular pads change airline order);
   `normalizeAllTraces` iterates a `HashMap` in Java (C++ must use ascending net number); item ids / `itemList`
   order come from the DSN reader and drive `TreeSet<Item>` order (compare id sequences in T118).
8. **Settings/CLI diffs:** C++ `-mp` clamps 1–99998 (Java 0–9999, 0 = unlimited); `-mt` ignored; `-oit`
   mishandled; `-us/-is/-inc` no-ops; `-dl` disables the autorouter; `--router.*` only logged; `-de` lists and
   multi-output `-do` missing; any argument containing "help" prints help.
9. **Stale harness facts:** only 4 of 8 boards have a Java baseline SES; `build/run_tests` is not present in the
   current build tree (CLAUDE.md requires it to pass — confirm the target builds); `build/e2e_all.out` shows
   `BBD_Mars-64` timing out at 300 s while `E2E_STATUS.md` shows 0.6 s (flaky hang to investigate).

## Roadmap

Milestones are sequential on the critical path (A); tracks B–D can run in parallel (separate worktrees /
loop instances) because they share no files with A until the end.

```
Track A (critical path):  M0 → M1 loads like Java → M2 writes like Java → M3 core shakedown → M5 E2E per board → M7 → M8
Track B (settings/CLI):   T301..T307 ──────────────────────────────────────────────────────┘ (feeds M5, M7)
Track C (pipeline):       T401,T402,T420..T422 (synthetic-board tests) → T403..T415 (need M1 for E2E)
Track D (benchmark):      T801..T805 built now against the Java jar only, so baselines exist before C++ routes
```

### M0 — Oracles and hygiene (days, no ordering constraints)
| ID | Deps | Task / done when |
|----|------|------------------|
| T109 (existing) | T108 | Fix the 3 compile defects above, add `AutorouteSettings_test.cpp`, finish `adjustPlaneAutorouteSettings`; build + ctest green |
| T006 | – | CSV gets `pending`; `class_map_check.py` lists true out-of-scope (gui, api, mcp, analytics, io.kicad, SessionToFusion, Gson API adapters, VersionChecker …) and prints `pending` count; README Quick Status regenerated |
| T007 | – | Java reference harness `tools/java_ref/` (javac against the jar, per MEMORY #29): dumps board counts + item-id/order + per-pass router log → JSON in `tests/data/expected/` |
| T008 | – | `tools/java_baseline.sh`: runs the jar with a clean `--user_data_path`, `-mt 1`, fixed `-mp`, per-tier timeout, `--router.result_json_path`; stores SES + manifest + log + wall for all 8 E2E boards and the benchmark tiers; explains the 4 missing baselines |
| T009 | – | Fix PolygonShape LCG (`0x5DEECE66D`), verify with a throwaway `java.util.Random` program (MEMORY #19); `normalizeAllTraces` net-order; golden tests |
| T010 | – | `PlanarDelaunayTriangulation` parity: algorithm audit + `Collections.shuffle(seed 99)` equivalent via the existing `java.util.Random` port; golden test from Java output |

### M1 — Loads like Java (G2)
Existing T109 → T110 → T111 → T112 → T113 → T114 → T115 → T116 → T117 → T118, with:
* **T125 (new, deps T106):** `Circuit` + `Parser` moved out of T117; T114 and `Plane` depend on it.
* **T117 gains:** `DsnReader`, `BoardReadResult`/`BoardMetadata`, `MinimalBoardManager` shim, `--dump-board-counts`.
* **T118 uses the Java harness (T007)** on all 117 DSNs; compares counts **and item-id order**; files one task
  per mismatch. `Issue006-LPC18XX_43XX_SCH.dsn` is a binary OLE file — an error is the correct result.
* **T124 (new, deps T117):** delete the legacy reader (`specctra/Scanner, Structure, Library, Network,
  Placement, Wiring, Keyword, Circle, Polygon, Path.h`, `BoardLoader`, `RoutingBoard::create_pins_from_parsed`,
  `parsed_result_`, dead `BoardBuilder`, stub `HeadlessBoardManager`); port Java `HeadlessBoardManager`
  load/save on the new parser; switch `Freerouting.cpp` (lines 258/276/277/317) and `BatchAutorouter.cpp:186`.
* Acceptance fixtures: `empty_board`, `Issue026` (8 KB), `Issue103-Board-Unrouted`, `BBD_Mars-64`, plus
  `Issue103-Board-Routed`/`Issue187` (existing wiring, `layer_rule`, `autoroute_settings`, `via_rule`).
  `place_keepout`, `wire_keepout`, `part_library`, `logical_part_mapping` appear in **no** test DSN — unit-test
  those readers from hand-written snippets instead.

### M2 — Writes like Java (G3)
* **T122 (new, deps T106):** `WriteScopeParameter` + `write_scope` of Shape/Rectangle/Circle/Polygon/Path/PolylinePath/Resolution.
* T201 → T202 as queued (deps += T122); add Library `library_out` via-padstack writer and `Wiring` write.
* T203 `SesReader` (+ `SesImportSummary`). Needed for resume (`-de x.dsn x.ses`) and for C++-side DRC on an SES.
* **T204 split:** T204a `RulesReader` (+`readRouterSettings`) / `RulesWriter`; T204b `DsnWriter` + remaining write
  halves (Structure, Layer, Plane, Rule, Package, Network, Component, Net, Parser, Placement).
* New `tools/ses_diff.py` (token-level SES compare). Done when an unrouted `Issue103-Board-Unrouted` round-trips
  token-equal to Java's and `java -jar … -drc` accepts a C++ SES.

### M3 — Core shakedown (new; highest risk)
First time the ported maze/shove/locator code meets real pads and clearances, using the interim
`BatchAutorouter` already in the tree.
* **T611 (new, deps T117,T202,T009,T010):** run `empty_board`, `Issue026`, `Issue107`, `Issue034` with `-mt 1`;
  build `tools/first_divergence.py` (compare Java log vs C++ log per pass: item order, per-connection result,
  incomplete count after each pass, then SES). Fix the **first** divergence, repeat. Done when `Issue026`
  routes with #unrouted equal to Java.
* Rule: no new pipeline feature is declared done on an E2E basis until it passes the bit-exact diff on `Issue026`.

### Track B — Settings & CLI (G5), independent of the parser
T301 → T302 → T303 → T304/T305 as queued, plus:
* **T306 (deps T303):** settings sources + priority order (Java `freerouting.json` is priority 10): `DefaultSettings`,
  `CliSettings`, `DsnFileSettings` (needs T117 metadata), `RulesFileSettings` (needs T204a), `JsonFileSettings`,
  `EnvironmentVariablesSource` (`FREEROUTING__ROUTER__*`), `LegacyRouterSettingsBridge`. Replace Java reflection
  with an explicit field registry that keeps `@SerializedName` names.
* **T307 (deps T302):** `RouterSettings.applyBoardSpecificOptimizations` (Java 300–430) + scoring versions.
* **T304 acceptance** = the flag table: `-de` (dsn/json/ses/rules, `+` lists), `-do` multi-output, `-dr`, `-mp`
  (0–9999, 0 = unlimited), `-mt`, `-oit` (0–1 ×100, default 2.5), `-us -is -inc`, `-dl` (file log only), `-ll`,
  `--router.*=v` generic setter (keys that matter: `autorouter.max_passes/max_threads`, `optimizer.enabled/
  max_passes/max_threads/improvement_threshold/timeout`, `fanout.enabled`, `job_timeout`, `scoring.*`,
  `result_json_path`, `strict_drc`), `--user_data_path`, fix the "help" substring bug.

### Track C — Autoroute pipeline (Java ≈ 5.4k lines)
T401 → T402 → T403 → T404 → T408 → T409 → T410/T411 → T412..T414 → T415, with these changes:
* **T420 (new, deps -):** `RoutingBoard::deep_copy()` + `state_hash()` (semantic equivalent of Java
  `deepCopy`/MD5). Tests: copy is independent; hash stable and changes on insert/remove.
* **T421 (new, deps T302):** `BoardStatistics::get_router_score/get_optimizer_score` (V2_CONTINUOUS,
  V2_LOWER_BOUND) + `calculateAllIncompletes` parity. Replaces the vague part of T703.
* **T422 (new, deps T403):** `RoutingBoard::finish_autoroute` / `clear_transient_autoroute_state`.
* **T407** deps → `T404,T420` (drop BoardComparator); **T408** deps += `T421,T422`; **T415** drops T416.
  **T416** → dropped (dead code) after grep confirms.
* **T423 (new, deps T414,T420):** optimizer thread pool (`-mt N`, per-thread board copies, results consumed in item
  order — same as Java, so thread count must not change the outcome). Required for the multi-thread benchmark.
* **T704** (`RoutingResultManifest`, `--router.result_json_path`) is promoted: deps T415 stays, but T801 depends
  on it — it is the benchmark's per-phase metrics channel on both sides. Add `RoutingJob` phase metrics and
  populate `RouterJobResourceUsage` (cpu_time_seconds, peak_memory_mb) as **T706**.
* Board gaps T501–T505 stay as audits; run them before M5 so E2E divergences are not caused by known holes
  (drill-hole obstacles T501 matter for routing around vias/pins).

### M5 — End-to-end parity per board (G4)
T601 … T609 as queued (deps += T008, T611), using the bit-exact diff first, then the quality gate. Add:
* **T610 (deps T609):** sweep all 117 DSNs: load parity (G2) for every one; route every board under 60 s in Java
  and record pass/fail in `E2E_STATUS.md`; the DAC2020 `bm01–bm11` set (5–80 KB) is the best bit-exact corpus.

### M7 — Full-CLI stragglers (decision: in scope)
T701 CLI DRC mode (`-drc`, report JSON field names) · T702 `RoutingJobScheduler` + action thread (headless,
synchronous; board load, `.rules` auto-load, timeout monitor, SES resume) · T703 scoring stragglers
(`BenchmarkScoreCalculator`, `BoardScoreBreakdown`, `ScoringWeightComparison`, `--calculate-benchmark-scores`)
· T405/T406 `BoardComparator` for `--compare-boards` (moved here, deps -) · T705 docs sync (regenerates the CSV
status, archives increment logs).

### M8 — Benchmark (Track D; tooling can start immediately, runs after G4 per board)
| ID | Deps | Deliverable |
|----|------|-------------|
| T801 | T008 | `tools/bench/bench.py` (stdlib Python; no hyperfine/jq/perf on this box). Matrix board × impl × mode × reps. Modes: **st** = `-mt 1` pinned with `taskset -c N`; **mt** = default threads, unpinned. Captures wall (monotonic), user+sys CPU and peak RSS via `os.wait4` rusage, exit code, stdout/stderr log. Per-phase times (load / fanout / route / optimize / write) from the result manifest (T704) on both sides |
| T802 | T801 | **Neutral scorer:** every SES from either binary goes through `java -jar … -de x.dsn x.ses -drc out.json` → `qualityScore`, `optimizerScore`, unrouted, clearance violations, wire length, vias. After T203/T701, add the C++ DRC as a cross-check (both scorers must agree) |
| T803 | T801 | Tiers + fixtures (below) and per-tier reps/timeouts |
| T804 | T802 | Reporter: `docs/migration/BENCHMARK.md` + CSV (median/min/max, speed ratio, quality delta, memory ratio, JVM-startup-excluded in-program time next to process wall time) |
| T805 | T804 | `tools/bench/check_regression.py` — fails when a board's C++ quality drops below Java's or time regresses >X % vs the stored baseline |
| T806 | T801 | Optional warm-JVM measurement: compiled harness class (MEMORY #29 approach) that runs the router N times in one JVM, so JIT warm-up is reported separately from cold CLI runs |
| T807 | G4 on tier | Profile C++ (install `valgrind`/callgrind or build with `-pg`; `perf` is not installed and is awkward on WSL2); list hot spots |
| T808 | T807 | Separate CMake preset `release-perf` (`-O3 -march=native -flto`, optional mimalloc). **Never** used for parity gates |
| T809 | T807 | Targeted optimizations driven by the profile, each guarded by the T6xx parity suite (output must not change) |
| T810 | T809 | Final run + write-up |

**Tiers (a board enters the benchmark only after it passes G4):**
| Tier | Boards | Reps / timeout |
|------|--------|----------------|
| smoke | `empty_board` | 10 / 60 s |
| tiny | `Issue026-J2_reference` (24 nets), a DAC2020 `bm0x`, `corney_island_wireless` | 10 / 120 s |
| small | `Issue034-Green14SegLED` (101), `BBD_Mars-64` (94), `Issue107` (77) | 10 / 300 s |
| medium | `Issue029-hw48na` (163), `Issue214` (4 layers), `Issue289` (6 layers) | 5 / 900 s |
| large | `Issue070` (688), `Issue103-Unrouted` (282), `Issue420` (411), `Issue555-CNH` (764) | 3 / 3600 s |
| stress | `Issue190`, `Issue187` (Z80) | 1 / 3600 s, run alone |

**Fairness rules written into the harness:** identical DSN, `-mp`, `-oit`, `--router.job_timeout`,
`optimizer.timeout`, log level; fresh `--user_data_path` per run (Java reads `freerouting.json`); runs strictly
sequential (box has 12 cores but only ~7.8 GB RAM, ~1.3 GB free — cap Java with a fixed `-Xmx`, close other
apps, never overlap with a build); record commit SHA, JDK (Temurin 25.0.4.1), g++ version, flags, `nproc`;
report in-program time because Java adds fixed overhead outside routing (≈1 s analytics sleep, 500 ms wait loop,
250 ms scheduler poll, CPU-score measurement). Java's wall-clock limits (fanout 10 s/pin, optimizer timeout, job
timeout) make large-board runs non-deterministic — set them explicitly and exclude timed-out runs from
bit-exact comparison.

## Risks

1. **Latent bugs in ~25k lines of core code never run on real boards** (all boards route 0 wires today). Mitigation:
   M3 shakedown with the first-divergence tool before the rest of the pipeline is stacked on top.
2. **Item-id / list order from the DSN reader** drives `TreeSet` ordering everywhere; counts alone will not catch
   it. T118 compares id sequences.
3. **Exact-arithmetic hot spots** may make C++ slower than Java on large boards until profiled (T807); do not
   optimize before parity.
4. **Memory-constrained WSL2 box** (≈1.3 GB free): Z80 boards may swap; run the stress tier alone.
5. **Weak-model loop:** keep every task ≤ ~700 Java lines; split rather than stall (rule 3 in TASKS.md).

## Critical files

* Queue/docs to edit when implementing: `SOURCE_OF_TRUTH/TASKS.md` (new tasks T006–T010, T122–T124, T204a/b,
  T306–T307, T420–T423, T610–T611, T706, T801–T810 in the exact loop format; dep changes for T114, T117, T201,
  T407, T408, T415, T416, T601), `SOURCE_OF_TRUTH/README.md`, `docs/migration/MIGRATION_*.md`,
  `JavaToCppClassMap.csv`, `tools/class_map_check.py`.
* Code: `src/designforms/specctra/parser/*` (Structure, ReadScopeParameter, AutorouteSettings, new Library/Package/
  Component/Network/Wiring/Circuit/Parser), `src/designforms/specctra/SessionWriter.*`, `src/settings/*`,
  `src/Freerouting.cpp`, `src/autoroute/BatchAutorouter.*`, `src/board/RoutingBoard.*`,
  `src/geometry/planar/PolygonShape.cpp`, `src/datastructures/PlanarDelaunayTriangulation.cpp`.
* Reuse (do not rewrite): `MazeRandom.h` (java.util.Random port), `tools/dsn_token_dump` + `dsn_token_compare.py`,
  `tools/e2e_compare.sh`/`e2e_all.sh`, `test_java_boards.sh` (superseded by T008), `loop_migration.sh`,
  Java jar `/home/anwer/.cache/freerouting-gradle-build/libs/freerouting-2.1.0-executable.jar`, JDK
  `/home/anwer/.local/jdk-25.0.4.1+1`.

## Verification (per milestone)

* Always: `cmake --build build -j$(nproc)` · `ctest --test-dir build --output-on-failure` · `run_tests`.
* M0: `python3 tools/class_map_check.py` (shows `pending`); T009/T010 golden tests against Java-generated values.
* M1: `build/freerouting --dump-board-counts x.dsn` equals the Java harness JSON for all 117 DSNs (id order too).
* M2: `python3 tools/ses_diff.py cpp.ses java.ses` on the unrouted board; `java -jar <jar> -de x.dsn cpp.ses -drc o.json`.
* M3/M5: `tools/e2e_compare.sh <dsn>` / `tools/e2e_all.sh`; `tools/first_divergence.py` on `-mt 1` logs.
* G5: CLI test table comparing `Freerouting::parse_args` effects to the Java flag list.
* M8: `python3 tools/bench/bench.py --tier small --reps 5`, then `check_regression.py`; sanity-run the harness
  Java-vs-Java first (ratio must be ≈ 1.0, spread small) before trusting any C++ number.

## First actions once approved

1. Fix the 3 T109 compile defects, finish T109, commit.
2. Write the new/changed tasks into `SOURCE_OF_TRUTH/TASKS.md` (loop format) and update README Quick Status.
3. Start Track D (T008, T801–T802 against Java only) in parallel with Track A (T110 onward).
