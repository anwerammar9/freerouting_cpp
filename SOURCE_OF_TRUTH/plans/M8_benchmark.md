# M8 — Java vs C++ benchmark (T801–T810)

T801–T805 can be built now and validated Java-vs-Java; C++ numbers only count for boards that pass gate G4 (M5).

## What is measured

| Metric | Source | Notes |
|---|---|---|
| process wall time | `time.perf_counter_ns()` around the child | includes JVM start and Java's fixed overhead |
| user / sys CPU, peak RSS | `os.wait4(pid, 0)` → `ru_utime`, `ru_stime`, `ru_maxrss` (KB on Linux) | rusage covers all threads of the child (JIT/GC threads for Java) |
| per-phase time and CPU | result manifest `phases.{fanout,autorouter,optimizer}.{duration_seconds,cpu_seconds,passes_completed}` (`--router.result_json=<path>`, T704/T706 on C++) | **in-program time**, excludes JVM start-up and Java's ≈1 s analytics sleep, 500 ms wait loop, 250 ms scheduler poll |
| quality | neutral scorer: `java -jar $JAR -de <dsn> <ses> -drc <out.json>` for **both** implementations' SES | unrouted, clearance violations, `qualityScore`, `optimizerScore`; wire length / vias from the report or the SES |
| determinism | SHA-256 of each SES | reps of the same (board, impl, mode) should give identical SES in `st` mode |

## Modes

- **st** (headline): `-mt 1` on both; both processes pinned to the **same two cores** with `taskset -c 2,3` (the JVM's JIT/GC
  threads get the second core — pinning Java to a single core would measure JIT starvation, not the router).
- **st1** (optional, strict): pinned to one core, both sides.
- **mt**: default thread settings (`optimizer.max_threads = cores-1` on both; the autorouter is single-threaded in Java
  anyway), unpinned.

## Fixed run settings (identical for both)

`-de <dsn> -do <ses> -mp 100 -dl -ll INFO --router.job_timeout=<tier> --router.optimizer.timeout=<tier>
--router.fanout.max_milliseconds_per_pin=600000 --router.result_json=<manifest>` plus a fresh `--user_data_path`.
Java: `-Xmx2g` (record the JVM flags; default GC). C++: the Release build (`-O3`, no LTO/`-march=native` — those belong
to the T808 `release-perf` preset and are reported separately).

## Tiers (`tools/bench/tiers.json`)

| Tier | Boards | Reps | Timeout |
|---|---|---|---|
| smoke | empty_board | 10 | 60 s |
| tiny | Issue026-J2_reference, one DAC2020 `bm0x`, corney_island_wireless | 10 | 120 s |
| small | Issue034-Green14SegLED, BBD_Mars-64, Issue107-freq_teiler_200kHz_kicad | 10 | 300 s |
| medium | Issue029-hw48na, Issue214-freerouting, Issue289-…VGA | 5 | 900 s |
| large | Issue070-Autorouter_FQ101, Issue103-Board-Unrouted, Issue420-contribution-board, Issue555-CNH | 3 | 3600 s |
| stress | Issue190-processor.Z80, Issue187-processor.Z80 (alone, nothing else running) | 1 | 3600 s |

Use exact file names from `tests/data` in the JSON (resolve the abbreviations above with `ls tests/data`).

---

## T801 — `tools/bench/bench.py` runner  (S)

**CLI.** `bench.py --tier small [--boards a.dsn,b.dsn] --impl java,cpp --mode st,mt --reps N --out results/<date>
[--jar PATH] [--cpp build/freerouting] [--dry-run]`.

**Algorithm.**
1. Write `results/<date>/env.json`: `uname -a`, WSL detection (`/proc/version` contains `microsoft`), CPU model and
   `nproc` (`/proc/cpuinfo`), `MemTotal`/`MemAvailable` (`/proc/meminfo`), JDK `java -version`, jar SHA-256,
   `g++ --version`, CMake build type and flags (`build/CMakeCache.txt`), `git rev-parse HEAD` + dirty flag.
2. Refuse to start if `MemAvailable < 2.5 GB` unless `--force` (prints why).
3. For each board, for each rep, run the implementations **interleaved** (java, cpp, java, cpp …) to spread drift
   evenly; 2 s pause between runs. One un-timed warm-up run per implementation per board first (fills the OS file cache;
   discarded).
4. Each run: fresh temp dir, build the command, `subprocess.Popen` with stdout/stderr to files, `os.wait4` for
   rusage, `timeout` enforced by the runner (kill the process group). Save `<board>.<impl>.<mode>.<rep>.json` with
   command, exit code, wall, user, sys, max_rss_kb, timed_out, ses_sha256, manifest (parsed if present).
5. Never run two measured processes at the same time.

**Done when.** A Java-vs-Java run (`--impl java,java2` alias pointing at the same jar) on the small tier gives a median
ratio within 1.00 ± 0.05 and a spread report.

## T802 — neutral quality scorer  (S)

`tools/bench/score.py results/<date>`: for each produced SES run the Java `-drc` scorer (cache by SES hash), parse the
report, add `quality` to the run JSON. After T203/T701 also run `build/freerouting -de <dsn> <ses> -drc` and record both;
flag any disagreement between the two scorers.

## T803 — tiers file  (S)

`tools/bench/tiers.json` as in the table above, validated by `bench.py --list-tiers` (prints boards that exist and warns
on missing files).

## T804 — report  (S)

`tools/bench/report.py results/<date> > docs/migration/BENCHMARK.md` (+ `results/<date>/summary.csv`):
- per board × mode: median / min / max wall (process and in-program), user+sys CPU, peak RSS for each implementation;
  **speed ratio = median(Java) / median(C++)** with a bootstrap 95 % CI (stdlib `random.Random(42)`, 2000 resamples);
  memory ratio; quality columns (unrouted, violations, scores) for both sides; determinism flag;
- per-phase table (fanout / autorouter / optimizer) from the manifests;
- an "excluded" section: boards that failed G4 or timed out, with the reason;
- the `env.json` summary at the top. Plain Markdown tables (no plotting dependency).

## T805 — regression gate  (S)

`tools/bench/check_regression.py results/<date> --baseline tools/bench/baseline.json [--time-tolerance 0.10]`: exit 1
when, for any board, C++ unrouted > Java unrouted, C++ has clearance violations, or C++ median in-program time is more
than the tolerance slower than the stored baseline. `--update-baseline` writes the new numbers.

## T806 — warm-JVM measurement (optional)  (S)

A compiled Java harness (approach from 01_MEMORY gotcha 29) that loads a board once and runs the routing pipeline N
times in one JVM (fresh board copy each iteration), printing per-iteration in-program time. Reported in a separate
BENCHMARK.md section "Java steady state" — CLI cold runs remain the headline because that is how users run both tools.

## T807 — profile the C++ router  (O)

**Tools on this machine:** no `perf`, `valgrind`, `hyperfine`. Options: install valgrind (`sudo apt install valgrind`)
and use callgrind on a small board; or build with `-pg` in a scratch build dir and use gprof; plus the project's own
`src/autoroute/PerformanceProfiler.*` timers (extend them to the same scopes Java's `PerformanceProfiler` measures, so
per-scope numbers are comparable). `perf` on WSL2 needs a perf built for the WSL kernel — avoid.

**Output.** Top 20 functions by inclusive time for one medium and one large board, written to
`docs/migration/PROFILE.md`, with a short hypothesis per hot spot.

## T808 — `release-perf` preset  (S)

Add to `CMakePresets.json` a `release-perf` configure preset (`-O3 -march=native -flto`, optional mimalloc via conan).
Builds into `build-perf/`. Never used for parity gates; the benchmark report states which preset produced each number.
Fix the existing preset's `binaryDir` that points at `/home/anwer/projects/EDA/...`.

## T809 — profile-driven optimisations  (O)

One task per hot spot (`T809a`, …). Rule: the T6xx parity suite (`first_divergence.py` on the bit-exact corpus +
G4 checks) must produce **identical output** before and after. Typical candidates to check first (only if the profile
shows them): `std::shared_ptr` churn in geometry, allocation of expansion rooms/doors (arena/pool), exact-arithmetic
fallbacks (Boost.Multiprecision) on hot paths, repeated `get_items()` copies, search-tree node allocation.

## T810 — final run and write-up  (S)

Run all tiers that pass G4 in `st` and `mt` (release build; optionally `release-perf` as an extra column), regenerate
BENCHMARK.md, update `tools/bench/baseline.json`, summarise in README.
