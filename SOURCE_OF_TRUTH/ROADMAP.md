# Freerouting C++ Migration — Roadmap and Execution Playbook

**Goal:** 100 % of Freerouting (Java 2.1.0, headless subset) running in C++20 so that
`freerouting -de board.dsn -do out.ses` gives the same routing result as the Java jar, then benchmark C++ against Java.

**Written for:** smaller Claude models (Sonnet, or Opus at low/medium effort) that implement the migration one task at a time.

**Status date:** 2026-10-10 · branch `migration/dsn-parser-foundations` · commits `1546861` (T109a) and `af80539` (queue + roadmap) · ctest 887/887.

`SOURCE_OF_TRUTH/TASKS.md` is the executable queue. **Where this file and TASKS.md differ, TASKS.md wins.**

---

## 1. Where things stand (read this first)

- The routing core is ported and unit-tested: geometry, rules, board model, maze search, expansion rooms, shove, fanout, `RoutingBoard::autoroute`.
- **Every test board still routes 0 wires.** The DSN loader in use is hand-rolled, not a Java port, so all pins of a component land on its origin. The Java-faithful parser is about half done: T101–T108 and T109a are done; T109b/c and T110–T118 remain.
- Consequence: the ported maze/shove code has only ever run on synthetic fixtures, never on a real board. Expect latent bugs when real boards arrive (task **T611**). This is the biggest schedule risk.
- No benchmark tooling exists yet, and only 4 of the 8 reference boards have a Java baseline SES file.
- **Scope (decided with the owner): the full headless CLI** — route, fanout, optimize, DRC mode, SES/rules import, settings sources, job scheduler, result manifest. Excluded: GUI, REST API, MCP, analytics, KiCad I/O, Fusion export.
- **Parity bar:** quality parity is the gate (unrouted count, DRC violations, wire/via counts). Byte-identical SES in single-thread mode is a stretch goal on small boards, and is also the best debugging tool.
- **Benchmark:** single-thread (`-mt 1`) as the headline, plus the default multi-thread run; wall, CPU, peak RSS, per-phase time and quality.

---

## 2. How to work (rules for every session)

1. **Read at session start, nothing more:** `SOURCE_OF_TRUTH/01_MEMORY.md` (architecture + numbered gotchas), `SOURCE_OF_TRUTH/CONVENTIONS.md`, the "Rules for every task" section at the top of `TASKS.md`, the one task you are doing **and its spec section in `plans/`**, the Java files they name, and the C++ files you touch. Do **not** read `02_PROGRESS.md`–`05_WORK_PACKAGES.md` (huge historical logs).
2. **One task per session.** Pick the first task in file order that is `[ ]` or `[~]` and whose `deps:` are all `[x]`. Do not start a second task.
3. **Java is the truth.** Port faithfully, including behaviour that looks wrong; mark it `// Java parity:`. Do not "fix" Java quirks (section 4).
4. **Three files per class:** `X.h`, `X.cpp`, `X_test.cpp`. Register sources in `src/<module>/CMakeLists.txt` and the test in `tests/CMakeLists.txt`.
5. **Size limit:** about 700 Java lines per task. If it is bigger, split it into `T123a`, `T123b` (a single lowercase letter suffix only — the loop script matches `T[0-9]+[a-z]?`), do the first, leave the rest `[ ]`, and repoint dependants to the last part.
6. **Gate** (all must pass before a task is marked done): `cmake --build build -j6` then `ctest --test-dir build -j6 --output-on-failure`. The ctest run includes `AllTests` = `build/tests/data/run_tests`. Never delete, skip or weaken an existing test. Use `-j6`, not `-j12`: the machine has 12 cores but only ~7.8 GB RAM.
7. **Verify against real Java, never against numbers computed by hand.** Use the Java reference harness (T007) or a throwaway `javac` program compiled against the jar (section 7). Golden values in tests must come from Java output.
8. **On finish:** mark the task `[x]` and add one indented `- result:` line (what landed, test count); add a short dated note to the matching `docs/migration/MIGRATION_<module>.md`. Partially done with the build green: mark `[~]` with a `- progress:` line saying exactly what is left. Impossible or needs a human decision: mark `[!]` with a `- blocked:` reason — do not guess.
9. **New work you discover:** append a new task in the right phase with the next free ID and its dependencies. Do not use an ID that appears in the format example at the top of TASKS.md (`T123`).
10. **Commits:** under `./loop_migration.sh` do **not** commit — the script builds, runs ctest and commits. In a manual session commit only when the gate is green, message `migration(Txxx): <title>`, and stage files explicitly (no `git add -A`).
11. **Never include or edit** the dead pair `src/board/Components.h/.cpp` (canonical is `Component.h`). `DrillItem`'s canonical declaration is `src/board/Via.h`.
12. **CMake link groups.** Executables use `$<LINK_GROUP:RESCAN,...>` groups. If a library that `freerouting_designforms` (or another grouped library) starts to depend on is not already in the groups, add it to every group that contains `freerouting_designforms`: top-level `CMakeLists.txt` (`dsn_token_dump`, ~line 83) and the designforms / board-loader groups in `tests/CMakeLists.txt`. Do not make a grouped library depend on a library outside the group that depends back into it — CMake reports a dependency cycle. `freerouting_settings` must **not** link `freerouting_autoroute` (this was already the cause of one cycle).

### Build and test commands

```bash
# one-time configure (already done in this checkout; build/ exists)
conan install . --build=missing -of build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DCMAKE_PREFIX_PATH=build/build/Release/generators

cmake --build build -j6                       # full build
cmake --build build --target Structure_test   # one test target
ctest --test-dir build -j6 --output-on-failure
./loop_migration.sh --dry-run --allow-dirty   # shows the next eligible task and its prompt
tools/e2e_compare.sh tests/data/Issue026-J2_reference.dsn   # C++ vs Java SES summary for one board
```

---

## 3. Definition of "100 % headless" (all gates must hold)

| Gate | Requirement | How it is checked |
|------|-------------|-------------------|
| G1 | Every headless-path Java class is `ported`; the rest are justified `out_of_scope` | `tools/class_map_check.py` prints `pending = 0` (needs T006) |
| G2 | All `tests/data/**/*.dsn` (117 files) that Java accepts load; layer / net / component / pin / padstack / net-class / keepout counts **and item-id order** equal Java | C++ `--dump-board-counts` vs the Java harness JSON (T007, T117, T118) |
| G3 | An unrouted board written as SES is token-equal to Java's; a routed C++ SES is accepted by `java -jar … -drc` | `tools/ses_diff.py`, Java `-drc` |
| G4 | Per board: #unrouted ≤ Java's, 0 DRC violations (scored by the Java jar), wires/vias within ±10 %. Stretch: byte-identical SES with `-mt 1` on small boards | `tools/e2e_compare.sh`, tasks T601–T610 |
| G5 | Every benchmark-relevant CLI flag behaves like Java | table-driven CLI test (T304) |
| G6 | `ctest` green (includes `run_tests`), no compiler warnings from `src/` | the gate in section 2 |

---

## 4. Gotchas that cost time

The full list is in `01_MEMORY.md`. The important ones:

- **Java nets are 1-based**; components are 1-based ids (`Components.get(id)`).
- **Never "simplify" `MazeRandom`** (`src/autoroute/MazeRandom.h`): it is a bit-exact port of `java.util.Random`. Do not replace it with `std::mt19937`.
- **Java `Polyline(Point[])` is not a corner list** — use `Polyline::from_corners`, not the C++-only ctor.
- **C++ `TileShape` is not a `Shape`**; padstack shapes are wrapped in `ConvexTileShape`. Use `TileShape::enlarge`, not `offset`, where Java calls `enlarge`.
- **C++ `IntOctagon::is_empty()` differs from Java's** (degenerate octagons). Do not change the shared class; use local helpers as `Sorted45DegreeRoomNeighbours` does.
- **Nullable Java values stay nullable** (`std::optional`), e.g. `ShapeEntrySide.borderIntersection`; a (0,0) sentinel breaks multi-segment inserts.
- **Pass `shared_ptr`s by value** in join/union helpers whose loop reassigns the members they alias (`NetIncompletes`).
- **`clearance_safety_margin` is added** for safety checks; `BasicBoard.clearanceValue()` always adds it.
- **Copy trace corners** instead of holding references (`combine_at_start/end` clears the cache).
- **Lexer facts:** illegal characters are warned and skipped (`+BATT` → `BATT`); `007` is a Double. Trust the shipped jar, not the old 2008 `.flex`.
- **A C++ operation that "always fails" is usually a mis-ported Java branch.** Check the Java before pinning odd behaviour in a test.
- **Same class name in two headers is a landmine.** Grep that a name is not already defined before reusing it.

---

## 5. Corrections to the original queue (already applied in TASKS.md)

From the first review (2026-10-10, plan approval):
- The class map's `out_of_scope` status wrongly hides ~60 headless-path classes that are just not ported yet. T006 adds a `pending` status.
- `WriteScopeParameter` and the *write* halves of the parser classes were in no task (T106 was ticked without them) → T122, T204b, T204c.
- `Circuit` and `Parser` were scheduled after the task that needs them → moved out of T117 into **T125**.
- Unowned pieces now have tasks: settings sources, `applyBoardSpecificOptimizations`, board deep copy + state hash, router/optimizer scores, `finishAutoroute`, `RoutingJob` phase metrics.
- `BoardComparator` is not on the route path (only `--compare-boards`) → moved to Phase 7; it no longer blocks `BoardHistory`.
- `BatchAutorouterThread` and the multi-thread autorouter path are dead code in Java (T416 closed; verified `autoroutePassMultiThread` has no callers). The only multi-threaded part of the headless run is the **optimizer** thread pool.
- `run_tests` exists at `build/tests/data/run_tests` (ctest `AllTests`); a planned "make run_tests build" task was dropped.

From writing the per-task specs (same day, verified in the source):
- **Item order (T011a/b, new).** Java `Item.compareTo` returns `other.id - this.id` and the board item list is a
  `ConcurrentSkipListMap`, so `board.getItems()` and every `TreeSet<Item>` (≈45 uses) iterate **newest item first**.
  C++ iterates `items_` oldest first and uses 118 pointer-ordered `std::set<Item*>` — not even reproducible between C++ runs.
- **45°/90° search trees (T506a/b, new).** Java's autoroute tree is `ShapeSearchTree45Degree` on 45° boards (almost all
  test boards) and `ShapeSearchTree90Degree` on 90° boards; in C++ both are empty shells that are never instantiated, so
  room completion uses the generic algorithm. T506a is a prerequisite of the shakedown (T611).
- **Ordering inside M1.** The parser `NetList` is needed by `Structure` (planes) → new **T126** before T109c; T110 needs the
  board created by T109c; `Network` is split T113 / T114a / T114b / T115; HeadlessBoardManager is split T124a/b/c
  because its post-load processing (settings overrides, `reduceNetsOfRouteItems`, power-plane validation) changes routing.
- **SesWriter** needs `Parser.writeScope` (T125) and `Shape.writeScopeInt` + board→DSN shape transforms (T122).
- **Rules I/O** split: T204a RulesReader (affects routing input), T204b RulesWriter, T204c DsnWriter.
- **Determinism:** `PolygonShape.cpp` LCG multiplier `0x5DEECE66ULL` vs Java `0x5DEECE66D` (T009); Delaunay needs the
  full Java algorithm + seeded shuffle (T010a/b); `normalizeAllTraces` iterates a Java `HashMap` whose order is *not*
  plain ascending (bucket order) — ported with a helper in T116.
- **CLI facts:** Java `-mt N` sets **only `optimizer.max_threads`** (clamped 0..1024); the result manifest key is
  `--router.result_json=<path>` (not `result_json`); `-help` triggers only for arguments *starting with* `-help`;
  `-us X` always selects GLOBAL_OPTIMAL. C++ today: `-mp` clamps 1–99998 (Java 0–9999, 0 = unlimited), `-mt` ignored,
  `-oit` mishandled, `-us/-is/-inc` no-ops, `-dl` disables the batch autorouter, `--router.*` only logged.
- `BoardHistory` keeps `MAX_HISTORY_SIZE = 30` boards (the old "BOARD_RANK_LIMIT=50" was wrong); the batch loop's
  scores use `RouterScoringVersion.V2_CONTINUOUS` / `OptimizerScoringVersion.V2_LOWER_BOUND` by default, and the V2
  optimizer score needs `BoardStatisticsBounds` (moved into T421).
- `settings/OptimizerSettings.h` still declares a second `RouterScoringSettings` with invented fields (01_MEMORY gotcha
  14 is not fully fixed) — removed in T301.

---

## 6. The task roadmap

**Per-task specs live in [`plans/`](plans/README.md)** — one file per milestone; every open task in TASKS.md has a
`- spec:` line pointing at its section (Java methods with line ranges, C++ targets, code to reuse, steps, tests,
acceptance). TASKS.md holds status, dependencies and order.

Order = file order in TASKS.md (the first eligible task wins). Model hint used in the specs:
**S** = Sonnet at low/medium is fine (mechanical port with a clear Java source and tests);
**O** = prefer Opus at medium (debugging against Java, algorithm parity, cross-cutting design).
If an S task keeps failing the gate, retry once, then mark it `[!]` with the failure output so a stronger session can pick it up.

**Parallel tracks** (separate git worktrees are fine; they share almost no files): **A** parser → SES → shakedown → E2E
(critical path); **B** settings/CLI; **C** pipeline pieces testable on synthetic boards; **D** benchmark tooling (can be
built now against the Java jar only).

```
Track A:  M0 (T006-T011b) -> M1 (T109b, T126, T109c, T110-T116, T125, T117, T118) -> M2 (T122, T201-T203)
          -> T506a -> M3 (T611) -> M5 (T601-T610) -> M7 -> M8
Track B:  T301 -> T302 -> T303 -> T304/T306, T305, T307 ----------------------------> (feeds T124b, M5, M7)
Track C:  T401, T402, T420, T421, T422 (synthetic-board tests) -> T403/T404 -> T407 -> T408a/b -> T409 -> T410/T411,
          T412-T414 -> T423, T415 -> T704 -> T706
Track D:  T801-T805 now against the Java jar (Java-vs-Java sanity first); C++ columns once boards pass G4
Phase 5:  T506a/b (search trees), T501-T505 audits — before M5
Later:    T124a-c (after T202 and Track B), T204a-c, T203, M7 (T701-T703, T405/T406, T705)
```

| Milestone | Spec file | Tasks |
|-----------|-----------|-------|
| M0 oracles & determinism | [plans/M0_oracles.md](plans/M0_oracles.md) | T006, T007, T008, T009, T010a/b, T011a/b |
| M1 DSN reader (G2) | [plans/M1_dsn_reader.md](plans/M1_dsn_reader.md) | T109b, T126, T109c, T110–T116 (T114a/b), T125, T117, T118, T124a–c |
| M2 SES/rules/DSN I/O (G3) | [plans/M2_ses_io.md](plans/M2_ses_io.md) | T122, T201, T202, T203, T204a–c |
| M3 shakedown | [plans/M3_shakedown.md](plans/M3_shakedown.md) | T611 |
| Track B settings/CLI (G5) | [plans/TrackB_settings_cli.md](plans/TrackB_settings_cli.md) | T301–T307 |
| Track C pipeline | [plans/TrackC_pipeline.md](plans/TrackC_pipeline.md) | T401–T404, T407, T408a/b, T409–T415, T420–T423, T704, T706 |
| Phase 5 board gaps | [plans/Phase5_board_gaps.md](plans/Phase5_board_gaps.md) | T506a/b, T501–T505 |
| M5 E2E (G4) | [plans/M5_e2e.md](plans/M5_e2e.md) | T601–T610 |
| M7 stragglers | [plans/M7_stragglers.md](plans/M7_stragglers.md) | T701–T703, T405, T406, T705 |
| M8 benchmark | [plans/M8_benchmark.md](plans/M8_benchmark.md) | T801–T810 |

---

## 7. Java reference tooling (how to get ground truth)

- **JDK:** `/home/anwer/.local/jdk-25.0.4.1+1` (Temurin 25.0.4.1).
- **Jar:** `/home/anwer/.cache/freerouting-gradle-build/libs/freerouting-2.1.0-executable.jar`, built from `java/`. The Gradle init script used to build it lived in `/tmp` and is not in the repo; rebuild notes are in `AGENTS.md` (~lines 57–59).
- **Run Java headless:** `java -jar <jar> -de board.dsn -do out.ses -mt 1 -mp 20`. Add `--user_data_path <fresh dir>` so a stray `freerouting.json` cannot change settings.
- **Throwaway harnesses:** compile the whole `java/app/freerouting` tree (or a small class) with `javac` against the jar. When driving the router from a harness, set `board.changedArea = new ChangedArea(board.getLayerCount())`; otherwise the trailing `normalize()` throws an NPE that is only logged and the harness appears to do nothing.
- **Lexer reference dumps:** `build/dsn_token_dump` + `tools/dsn_token_compare.py` (all 76 top-level test DSNs already tokenize identically to Java).
- **Neutral scorer:** `java -jar <jar> -de x.dsn x.ses -drc out.json` works on SES files from either implementation (JSON contains `qualityScore`, `optimizerScore`).

---

## 8. Test data facts

- 117 DSN files under `tests/data` (76 top-level, 41 in subfolders such as `Issue508-DAC2020/bm01–bm11`).
- Resolution is `um 10` in 66 files and `mil` in 9; no file uses mm or inch. 8 files have no `(unit` line (including both large routed boards). `string_quote` is always `"`. 11 files contain Cyrillic names; `NormalPuzzle.dsn` is the only CRLF file; two files start with a blank line.
- Present: `(keepout` in 48 files, planes in 18 (4 are `type power`), `layer_rule` and `autoroute_settings` in 2 (Issue103-Board-Routed, Issue187), `via_rule` in 3, `snap_angle` 3, `control` 6, `flip_style` 3.
- **Absent from every test DSN** (test from hand-written snippets): `place_keepout`, `wire_keepout`, `part_library`, `logical_part_mapping`, `logical_part`, `constant`, `window`, `qarc`, `fromto`, `jumper`, `use_net`; `via_keepout` appears only in Issue039-bug-design.
- Java SES baselines exist for 4 boards only (`java_output_ses/`: empty_board, Issue026, Issue034, Issue107). `tests/data` also holds 25 reference `.ses` files that are not Java-routed baselines.
- `build/e2e_all.out` once showed BBD_Mars-64 timing out at 300 s while `E2E_STATUS.md` shows 0.6 s: an unexplained flaky hang to investigate during T611/T608.

---

## 9. Benchmark design (Phase 8; tooling can start now against the Java jar only)

Full design and per-task steps: [plans/M8_benchmark.md](plans/M8_benchmark.md). Summary:

- **Modes:** `st` (headline) = `-mt 1` on both, both pinned to the same **two** cores (`taskset -c 2,3`, so the JVM's
  JIT/GC threads are not starved); optional `st1` = one core; `mt` = default threads, unpinned.
- **Metrics:** process wall time, user+sys CPU and peak RSS (`os.wait4` rusage), **in-program per-phase time** from the
  result manifest (`--router.result_json=<path>`, T704/T706), SES hash (determinism), and quality from the **Java `-drc`
  scorer applied to both implementations' SES** (unrouted, clearance violations, scores).
- **Fixed settings for both:** `-mp 100`, explicit `--router.job_timeout`, `--router.optimizer.timeout`,
  `--router.fanout.max_milliseconds_per_pin=600000`, fresh `--user_data_path`, Java `-Xmx2g`, C++ Release build.
- **Protocol:** one discarded warm-up per implementation and board, then interleaved reps (java, cpp, java, cpp …),
  strictly sequential, refuse to start below 2.5 GB available RAM; record `env.json` (CPU, RAM, WSL, JDK, g++, flags,
  git SHA, jar hash).
- **Validity:** a board enters the benchmark only after it passes gate G4; timed-out runs are excluded and listed.
- **Report:** median/min/max, speed ratio median(Java)/median(C++) with bootstrap 95 % CI, memory ratio, per-phase table,
  quality columns; `check_regression.py` gates future changes.
- **Tiers:** smoke `empty_board`; tiny `Issue026`, a DAC2020 `bm0x`, `corney_island_wireless`; small `Issue034`,
  `BBD_Mars-64`, `Issue107`; medium `Issue029-hw48na`, `Issue214`, `Issue289`; large `Issue070`,
  `Issue103-Board-Unrouted`, `Issue420`, `Issue555-CNH`; stress `Issue190`, `Issue187` (alone).

---

## 10. Verification per milestone

- **Always:** `cmake --build build -j6` · `ctest --test-dir build -j6 --output-on-failure` (includes `run_tests`).
- **M0:** `python3 tools/class_map_check.py` shows `pending`; T009/T010 golden tests use values generated by Java.
- **M1:** `build/freerouting --dump-board-counts x.dsn` equals the Java harness JSON for all 117 DSNs (item-id order too).
- **M2:** `python3 tools/ses_diff.py cpp.ses java.ses` on the unrouted board; `java -jar <jar> -de x.dsn cpp.ses -drc o.json`.
- **M3 / M5:** `tools/e2e_compare.sh <dsn>` / `tools/e2e_all.sh`; `tools/first_divergence.py` on `-mt 1` logs.
- **G5:** CLI test table comparing the C++ argument parser's effects to the Java flag list.
- **M8:** `python3 tools/bench/bench.py --tier small --reps 5`, then `check_regression.py`; run the harness Java-vs-Java first and require a ratio ≈ 1.0 before trusting any C++ number.

---

## 11. Risks

1. **Latent bugs in ~25k lines of core code that never saw a real board.** Mitigation: M3 shakedown (T611) with the first-divergence tool before stacking more pipeline on top.
2. **Item ids and item list order from the DSN reader** drive `TreeSet` ordering in the router; counts alone will not catch a mismatch (T118 compares id sequences).
3. **Exact-arithmetic hot spots** may make C++ slower than Java on large boards until profiled (T807).
4. **Memory-constrained machine:** Z80 boards may swap.
5. **Smaller models stalling:** keep tasks at or below ~700 Java lines; split rather than struggle; mark `[!]` with the failure output instead of guessing.

---

## 12. Kickoff prompt for an implementing session

Paste this at the start of a Sonnet / Opus (low or medium effort) session in the repo:

```text
You are continuing the Freerouting Java -> C++20 migration in /home/anwer/projects/freerouting_cpp.
Java in java/app/freerouting/** is the source of truth. Goal: headless DSN -> route -> SES parity,
then a Java-vs-C++ benchmark. Work on EXACTLY ONE task this session.

1. Read SOURCE_OF_TRUTH/01_MEMORY.md, SOURCE_OF_TRUTH/CONVENTIONS.md, the "Rules for every task"
   section at the top of SOURCE_OF_TRUTH/TASKS.md, and SOURCE_OF_TRUTH/plans/README.md.
2. Run ./loop_migration.sh --dry-run --allow-dirty to see the next eligible task (first [ ]/[~] task in
   file order whose deps are all [x]). Read that task, its "- spec:" section in SOURCE_OF_TRUTH/plans/,
   and only the Java/C++ files they name.
3. Port faithfully (Java quirks included, marked "// Java parity:"). Add X.h, X.cpp, X_test.cpp and register
   them in the module CMakeLists.txt and tests/CMakeLists.txt. Golden values must come from real Java output.
4. Gate: cmake --build build -j6 && ctest --test-dir build -j6 --output-on-failure. Never delete, skip or
   weaken a test. If you add a library dependency for designforms, add it to the RESCAN link groups.
5. Update TASKS.md ([x] + "- result:" line, or [~] + "- progress:", or [!] + "- blocked:" reason) and add a dated
   note to docs/migration/MIGRATION_<module>.md. If the task is over ~700 Java lines, split it (T123a, T123b ...).
6. Commit as "migration(Txxx): <title>" only after the gate is green (skip when running under loop_migration.sh).
   Stage files explicitly.
Stop after this one task and report what landed, the test count, and anything you could not verify.
```
