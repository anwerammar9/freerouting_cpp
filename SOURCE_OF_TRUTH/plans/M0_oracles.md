# M0 — Oracles, baselines and determinism (T006–T011)

These tasks have no dependency on the parser work and make every later task checkable. Do them first.
Order inside M0 does not matter, except that T007 should exist before anyone needs golden values from Java.

---

## T006 — Class map: `pending` status and an explicit out-of-scope list  (S)

**Goal.** `JavaToCppClassMap.csv` must tell the truth, so "100 % migrated" is a number (`pending = 0`).

**Java.** Nothing to port. The class list is `find java/app/freerouting -name '*.java'`.

**C++ target.** `tools/class_map_check.py`, `JavaToCppClassMap.csv`, `SOURCE_OF_TRUTH/README.md` (Quick Status).

**Reuse.** The existing script already checks `cpp_header` exists and contains `class|struct <cpp_class>`.

**Steps.**
1. Replace the heuristic `is_out_of_scope()` (keyword guesses such as `'actions'`, `'sources'`, `'optimize'`)
   with an explicit table `OUT_OF_SCOPE = {java_file: reason}`. Reasons: `gui`, `api`, `mcp`, `analytics`,
   `kicad`, `fusion`, `gson-api`, `version-check`, `ui-settings`. Exact members:
   - every file under `gui/`, `api/`, `analytics/`, `io/kicad/`;
   - `io/specctra/parser/SessionToFusion.java`, `io/MultiOutputGenerator.java` (only if you confirm it only
     dispatches to KiCad/Fusion writers — otherwise `pending`), `io/KiCadNetClassNames.java`;
   - `util/VersionChecker.java`, `util/SafeObjectInputStream.java`, `util/gson/*` (but check
     `RouterSettingsTypeAdapterFactory` — if settings JSON load (T303) needs its logic, port the logic, mark the
     class `ported` against the C++ file that holds it);
   - settings that only serve GUI/API: `ApiAuthenticationSettings`, `ApiServerSettings`, `GuiApplicationSettings`,
     `GoogleSheetsProviderSettings`, `McpServerSettings`, `RateLimitSettings`, `NetworkSettings`,
     `UsageAndDiagnosticDataSettings`, `UserProfileSettings`, `settings/sources/ApiSettings`, `GuiSettingsSource`.
2. Every other row that is not `ported`/`header_only`/`partial` becomes `pending`. Add a column `reason`
   (empty for non-out-of-scope rows). Keep the CSV header order and add `reason` last.
3. Make the script print: counts per status, then the `pending` list grouped by package, and exit non-zero only
   on structural errors (missing header, class not found) — not on `pending > 0`.
4. Regenerate the README "Quick Status" table: `ported / header_only / partial / pending / out_of_scope`.

**Tests.** A tiny `tools/test_class_map_check.py` (stdlib `unittest`) with a 4-row CSV in a temp dir covering each
status; run it with `python3 tools/test_class_map_check.py`. (Not part of ctest.)

**Done when.** `python3 tools/class_map_check.py` prints the counts; README updated; every `out_of_scope` row has a
reason.

**Watch out.** `JavaToCppClassMap.csv` today marks the *legacy* `specctra/Library|Network|Wiring|Structure|…` as
`ported`. They are not Java ports; set them `pending` against the new `parser/` paths.

---

## T007 — Java reference harness  (S)

**Goal.** One command that prints, as JSON, what the Java reader built from a DSN — the oracle for G2 (T110–T118)
and for item-order parity (T011, T611).

**Java.** Uses public API only: `app.freerouting.io.specctra.DsnReader.readBoard(InputStream, BoardObservers,
IdGenerator, String)` → `BoardReadResult` (`sealed interface`; records `Success(board, metadata, warnings)`,
`OutlineMissing(...)`, `ParseError(location, detail)`, `IoError(cause)` in `io/BoardReadResult.java`).
Board accessors: `BasicBoard.getItems()` (line 604), `getPins()`, `getVias()`, `getTraces()`,
`getConductionAreas()`, `getOutline()`, `getLayerCount()`, `rules.nets.maxNetNumber()`, `rules.netClasses.count()`,
`rules.viaInfos.count()`, `rules.clearanceMatrix.getClassCount()/getValue(i,j,layer,false)/getName(i)`,
`library.padstacks.count()`, `components.count()`, `Item.getId()`.

**C++ target.** `tools/java_ref/BoardCounts.java`, `tools/java_ref/build.sh`, `tools/java_ref/run_all.sh`,
output `tests/data/expected/<relative-dsn-path>.counts.json`.

**Steps.**
1. `build.sh`: `javac -cp "$JAR" -d build/java_ref tools/java_ref/BoardCounts.java` with
   `JAR=${FREEROUTING_JAR:-/home/anwer/.cache/freerouting-gradle-build/libs/freerouting-2.1.0-executable.jar}`
   and `JAVA_HOME=${JAVA_HOME:-/home/anwer/.local/jdk-25.0.4.1+1}`.
2. `BoardCounts.main(dsnPath)`: read the board exactly as the CLI does (call `DsnReader.readBoard` with
   `null, null, fileName`). Then print one JSON object (hand-written JSON, no library) with this schema
   (the C++ `--dump-board-counts` of T117 must emit the **same** schema):
   ```json
   {"file": "...", "result": "success|outline_missing|parse_error|io_error", "error": "...",
    "layers": [{"name": "F.Cu", "signal": true}],
    "counts": {"nets": 0, "net_classes": 0, "via_infos": 0, "via_rules": 0, "padstacks": 0,
               "packages": 0, "components": 0, "pins": 0, "vias": 0, "traces": 0,
               "conduction_areas": 0, "obstacle_areas": 0, "via_obstacle_areas": 0,
               "component_obstacle_areas": 0, "component_outlines": 0, "board_outline": 1},
    "clearance_classes": ["null", "default", "..."],
    "clearance_matrix": [[[0]]],
    "items": [{"id": 1, "type": "Pin", "layers": [0, 1], "nets": [3], "fixed": "SYSTEM_FIXED",
               "component": 2, "pin": 0, "corners": 0}],
    "warnings": 0}
   ```
   `items` in `board.getItems()` iteration order (that is Java's item-list order: **descending id**, see T011).
   `type` = `getClass().getSimpleName()`. `corners` = corner count for traces, else 0. For pins add
   `component` (component id) and `pin` (pin index); otherwise omit them.
3. `run_all.sh`: loop over `find tests/data -name '*.dsn'`, write one JSON per file, `timeout 120` each, keep going
   on failures.
4. Commit the generated JSON (it is small except for the two Z80 boards — if a file exceeds 2 MB, drop the
   `items` array for that file and set `"items_omitted": true`).

**Tests.** Manual: `tools/java_ref/run_all.sh` completes; `Issue006-LPC18XX_43XX_SCH.dsn` yields
`"result": "parse_error"`; `empty_board.dsn` yields 1 board outline and 0 pins.

**Done when.** `tests/data/expected/*.counts.json` exist for all 117 DSNs.

**Watch out.** The jar prints log lines to stdout; write the JSON to a file given as the 2nd argument (or print a
`===JSON===` marker line before it) so log noise cannot corrupt it. `BoardReadResult` records are Java 21+
records; use `instanceof BoardReadResult.Success s` pattern matching (the JDK is 25).

---

## T008 — Java baseline runner  (S)

**Goal.** Reproducible Java reference runs (SES + manifest + log + timing) for every board in the E2E set and the
benchmark tiers. Replaces `test_java_boards.sh`.

**Java CLI facts (verified in `settings/GlobalSettings.java` 575–919 and `Freerouting.java` 1074–1083):**
- `-de <file.dsn>` input, `-do <file.ses>` output; with both present the router runs.
- `-mp N` → `router.autorouter.max_passes` (clamped 0..9999, **0 = no limit**, `Integer.decode`, so `0x10` works).
- `-mt N` → **`router.optimizer.max_threads` only** (clamped 0..1024). It does *not* change the autorouter.
- `-oit F` → optimizer improvement threshold; values in (0,1) are multiplied by 100.
- `--user_data_path=<dir>` → isolates `freerouting.json` and logs (read before anything else).
- Generic setters `--<section>.<key>=<value>` use the `@SerializedName` path, e.g.
  `--router.result_json=<path>` (**not** `result_json_path`), `--router.job_timeout=00:10:00`,
  `--router.optimizer.enabled=false`, `--router.optimizer.timeout=00:05:00`,
  `--router.fanout.enabled=false`, `--router.fanout.max_milliseconds_per_pin=<ms>`,
  `--router.autorouter.max_threads=<n>`.
- `-dl` disables the file log only; `-ll <LEVEL>` sets the console log level.

**C++ target.** `tools/java_baseline.sh`, `java_output_ses/<board>_java.{ses,json,log,time}` (gitignored, as
today), `docs/migration/baseline.json` (committed summary), a section in `docs/migration/E2E_STATUS.md`.

**Steps.**
1. Script signature: `tools/java_baseline.sh [--tier smoke|tiny|small|medium|large|stress|e2e|all] [--mode st|mt]
   [--timeout S] [board.dsn ...]`. Board lists per tier are the ones in `ROADMAP.md` §9; `e2e` = the 8 boards in
   `test_java_boards.sh`.
2. Per board: `mktemp -d` user-data dir; run
   `/usr/bin/time -v "$JAVA" -Xmx2g -jar "$JAR" -de "$dsn" -do "$out.ses" -mp 100 -dl -ll INFO
   --user_data_path="$tmp" --router.result_json="$out.json" --router.job_timeout=<tier timeout as HH:MM:SS>
   [-mt 1 for st mode]` with an outer `timeout` of tier timeout + 60 s. Capture stdout+stderr to `$out.log`,
   the `time -v` block to `$out.time`.
3. Append one JSON line per board to `docs/migration/baseline.json`:
   `{"board","mode","exit","wall_s","user_s","sys_s","max_rss_kb","timed_out","ses_bytes","wires","vias","jdk","jar_sha256","date"}`.
   Count wires/vias the same way `tools/e2e_compare.sh` does (inside `(network_out`).
4. For each board that produces no SES, write the reason (timeout, exception class from the log, parse error) into
   the summary and into E2E_STATUS.md.

**Tests.** Run `tools/java_baseline.sh --tier e2e --mode st`; the 4 boards that have baselines today must still
produce an SES.

**Done when.** `baseline.json` has a row for every e2e board; E2E_STATUS.md explains each missing baseline.

**Watch out.** Machine RAM is tight (~1.3 GB free): run boards sequentially, never in parallel; keep `-Xmx2g`.
Java writes `freerouting.json` into the user-data dir — that is why each run gets a fresh one.

---

## T009 — Determinism: one `java.util.Random` port, fix the PolygonShape LCG  (S)

**Goal.** The convex split of polygons starts at the same corner as Java.

**Java.** `geometry/planar/PolygonShape.java`: `private static final Random randomGenerator = new Random(seed)`
(line 19, seed 99); `splitToConvex()` (≈531) calls `randomGenerator.setSeed(seed)` when the pieces are not
precalculated; `splitToConvexRecu()` (≈553) takes `randomGenerator.nextInt(corners.length)` as start corner.

**C++ today.** `src/geometry/planar/PolygonShape.cpp` lines ~26–77 define a private `JavaRandomFixed` whose
`advance()` multiplies by `0x5DEECE66ULL` — Java's multiplier is **`0x5DEECE66DULL`** (the `D` is missing), so every
start corner differs from Java. `reset(99)` is called at line ~874, `next_int` at ~891.
`src/autoroute/MazeRandom.h` is a separate, correct port used by the ripup randomisation.

**C++ target.** New header-only `src/datastructures/JavaRandom.h` (namespace `freerouting::datastructures`):
```cpp
class JavaRandom {
public:
    explicit JavaRandom(int64_t seed);
    void set_seed(int64_t seed);          // seed = (seed ^ 0x5DEECE66DL) & ((1L<<48)-1)
    int32_t next(int bits);               // seed = (seed*0x5DEECE66DL + 0xBL) & mask; return (int)(seed >>> (48-bits))
    int32_t next_int();                   // next(32)
    int32_t next_int(int32_t bound);      // exact JDK algorithm incl. power-of-two branch and rejection loop
    double  next_double();                // ((long)next(26) << 27) + next(27)) * 0x1.0p-53
};
template <class T> void java_shuffle(std::vector<T>& list, JavaRandom& rnd); // for i=size; i>1; --i: swap(i-1, rnd.next_int(i))
```
Use it in `PolygonShape.cpp` (delete `JavaRandomFixed`). Leave `MazeRandom` as is unless it becomes a thin wrapper
with identical output (then keep its tests green).

**Steps.** 1) Write `JavaRandom.h` + `JavaRandom_test.cpp` (register in `tests/CMakeLists.txt` with the datastructures
tests). 2) Switch `PolygonShape.cpp`. 3) Re-run all geometry tests; some golden convex-split expectations may
change — regenerate them **from Java**, never by copying the new C++ output.

**Tests.** `JavaRandom_test`: values from a throwaway Java program (`new Random(99)`, `setSeed`, `nextInt(7)` ×10,
`nextInt(16)` ×10 (power-of-two branch), `nextDouble()` ×5, `Collections.shuffle(List.of(0..9), new Random(99))`).
Known anchor from 01_MEMORY.md: `new Random(1000).next(32) == -1244746321`.
`PolygonShape` test: a concave polygon whose split depends on the start corner; expected piece list from Java.

**Done when.** Both tests green; ctest green.

**Watch out.** Java's `nextInt(bound)` loop uses signed 32-bit overflow (`bits - val + (bound-1) < 0`); do the
arithmetic in `int32_t` exactly like the JDK, the existing `JavaRandomFixed::next_int` already shows how.

---

## T010a / T010b — Determinism: PlanarDelaunayTriangulation parity  (O)

**Goal.** Airlines (and therefore incomplete counts and routing order) equal Java's.

**Java.** `datastructures/PlanarDelaunayTriangulation.java` (978 lines): de Berg et al. ch. 9.3 with a search DAG
(`TriangleGraph`), `Corner`, `Edge`, `Triangle`, legalisation by edge flips, degenerate edges for equal corners,
input corners shuffled with `Collections.shuffle(cornerList, randomGenerator)` after `setSeed(99)` (lines 40–70).
Consumer: `drc/NetIncompletes.java` 166–172 (C++: `src/interactive/NetIncompletes.cpp` 154–164).

**C++ today.** `src/datastructures/PlanarDelaunayTriangulation.cpp` is 191 lines, a different algorithm, no shuffle.

**Split.** T010a = classes `Corner`, `Edge`, `Triangle`, `TriangleGraph` and the constructor up to inserting all
corners (Java ≈ lines 1–500); T010b = edge legalisation/flip, degenerate edges, `getEdgeLines()` result
extraction, `validate()` (≈ 500–978). Keep the public C++ API (`Storable`, `ResultEdge`, ctor, the result
accessor used by NetIncompletes) so the consumer does not change.

**Steps.** Port faithfully, use `JavaRandom` + `java_shuffle` from T009 (T010a deps T009). Keep Java's edge id
ordering (`lastEdgeId`) — Java uses it to order edges in sets.

**Tests.** Golden edge sets from Java for: 4 points on a circle (co-circular tie), 5 collinear points, duplicate
points (degenerate edges), 20 random integer points (seeded), and the pin centres of `Issue026` (needs T117 —
add it later as a T118 follow-up if not available yet).

**Done when.** Golden tests pass; existing NetIncompletes/RatsNest tests pass (update their expectations only from
Java output).

---

## T011a / T011b — Item ordering parity (descending item id)  (S, careful)  — NEW

**Goal.** Iterate board items and item sets in the same order as Java. This is required for byte-identical runs
and even for *reproducible* C++ runs: today several sets are ordered by pointer address, which varies between runs.

**Java facts (verified).**
- `Item.compareTo` (board/model/items/Item.java ≈95) returns `other.id - this.id` → items sort by **descending id**.
- The board item list is `UndoableObjects` backed by a `ConcurrentSkipListMap` (datastructures/UndoableObjects.java
  ≈21/37) → `board.getItems()` and every `itemList` walk visit items **newest (highest id) first**.
- Java uses `TreeSet<Item>` (same descending-id order) in ~45 places in `board/`, `autoroute/`, `drc/`, and
  `HashSet<Item>` in exactly one place (`drc/NetIncompletes.java:295`, only used for de-duplication).

**C++ today.**
- `RoutingBoard::items_` is a `std::vector<Item*>` in insertion order; `get_items()` returns it oldest-first.
- 118 occurrences of `std::set<Item*>` / `std::unordered_set<Item*>` / `std::set<const Item*>` in ~33 files
  (ordered by **pointer**). Comments in `Item.cpp:156` and `RoutingBoard.cpp:673` already note the divergence.

**C++ target.**
- In `src/board/Item.h`: `struct ItemIdDescending { bool operator()(const Item* a, const Item* b) const { return a->get_id() > b->get_id(); } };`
  and `using ItemSet = std::set<Item*, ItemIdDescending>;` (plus `ConstItemSet` if needed).
- `RoutingBoard`: keep ownership as today, but `get_items()` must return items sorted by descending id. Either keep
  `items_` sorted on insert (binary-search insert by id; ids are almost always increasing, so insertion at the
  front/back is cheap) or store `std::map<int, Item*, std::greater<int>>` next to the vector. Removal must keep it
  consistent. Measure: `get_items()` is called in hot loops — return a `const std::vector<Item*>&` if possible.

**Split.** T011a: `ItemSet`, `get_items()` order, and all sets in `src/board/` (Item, Trace, PolylineTrace, Via,
BasicBoard, RoutingBoard, ShapeTraceEntries, PullTightAlgo*, OptViaAlgo, ItemSelectionFilter).
T011b: `src/autoroute/` (AutorouteEngine, Connection, FoundConnectionLocator/Inserter, MazeSearchAlgo,
MazeRipupResolver, RoutingBoardAutoroute, RoutingBoardFanout, BatchAutorouter), `src/interactive/NetIncompletes`,
`src/drc/DesignRulesChecker`, `src/core/scoring/BoardStatistics`.

**Steps (per occurrence).**
1. Find the Java counterpart method. If Java uses `TreeSet<Item>`/`SortedSet<Item>` or iterates `itemList`,
   switch to `ItemSet` / descending-id order.
2. If Java uses `HashSet<Item>` (only NetIncompletes:295, de-dup only) keep a set but do not rely on its order.
3. If the C++ set is a pure membership test that is never iterated, switching is still preferred (cheap and keeps
   the code uniform) — but do not change public signatures used by many callers in the same task without need.
4. Remove the "Divergence: … pointer ordered" comments you resolve.

**Tests.**
- `Item_order_test` (new): insert three items with ids 1,2,3 → `get_items()` yields 3,2,1; remove 2 → 3,1; insert an
  item with id 0 created earlier but inserted later → it comes last.
- Re-run the full suite. Tests that asserted the old (pointer/insertion) order must be updated **only** after checking
  the Java order for the same fixture (run the Java counterpart through a throwaway harness if unsure).

**Done when.** `grep -rn "std::set<Item\*>\|std::set<board::Item\*>\|std::unordered_set<Item\*>" src | grep -v _test`
returns only places documented as intentional; ctest green.

**Watch out.** Any behavioural test that changes outcome (not just order) is a real signal: the old order was not
Java's. Note such cases in the `- result:` line. Do not change `Item::get_id()` semantics or id generation
(`BasicBoard::generate_item_id`, ids start at 1 and increase).
