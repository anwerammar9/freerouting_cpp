# M3 — Core shakedown on real boards (T611)

## T611 — First real routes: Issue026, Issue107, Issue034 with the interim router  (O)

**Deps.** T117 (board loads like Java), T202 (SES write), T007 (harness), T008 (baselines), T009/T010 (geometry and
airline determinism), T011a/b (item order). Without T011 the C++ order differs from Java from the first item on, and
the divergence tool would only report that.

**Goal.** The ported maze search / locator / shove / inserter code has only run on synthetic fixtures. Run it on real
boards with the interim `BatchAutorouter` and fix divergences one at a time, starting with the earliest.

### The comparison hook already exists in Java

`autoroute/pipeline/AutoroutePassRunner.java` emits structured trace records per routed connection
(`logTraceRouteComparison` 406–446, `logRippedItems` 370–404), and other classes emit more
(`compare_trace_insert_segment`, `compare_trace_remove_item`, `compare_trace_remove_tails`,
`compare_trace_stub_cleanup`, `compare_trace_split_*`, `compare_trace_insert_forced_*`). Format
(`logger/FRLogger.java` 410–436):

```
[<method>] [<operation>] <message>: <impactedItems>
e.g. [BatchAutorouter.autoroute_pass] [compare_trace_route_item] Routing Pin -> result=ROUTED, details=..., incompletes=41, netIncomplete=0, ripped=0, netItems=2->3, maxItemId=219: Net #7,Item #12,Type=Pin
```

They are written only when `FRLogger.granularTraceEnabled` is true **and** the logger is at TRACE. Enable both with
`--debug.enable_detailed_logging=true` (Freerouting.java 1190–1196 sets file log level TRACE and the flag) and send
the file log somewhere known with `--logging.file.location=<file>`. `--debug.filter_by_net=3,7` restricts records to
nets (DebugControl.isInterested).

Per-pass summaries at DEBUG: `Pass #N: X incompletes across Y items to route` and `  Net 'name' has K incomplete(s)`
(`logIncompleteDetails` 347–368), and `Pass #N: Failed to route …` lines.

### Steps

1. **C++ logger.** Add `FRLogger::trace(std::string_view method, std::string_view operation, std::string_view message,
   std::string_view impacted_items)` printing exactly `"[%s] [%s] %s: %s"` at trace level, gated by a
   `granular_trace_enabled` flag; add CLI flags `--debug.enable_detailed_logging=true` and
   `--logging.file.location=<file>` to `src/Freerouting.cpp` (minimal now; T304 makes the CLI complete).
2. **Emit the same records in C++.** In the interim `BatchAutorouter` per-item loop emit `compare_trace_route_item` and
   `compare_trace_ripped_item` with the same fields and number formatting (result state names must equal Java's
   `AutorouteAttemptState` enum names). Emit `compare_trace_insert_segment` / `remove_item` / `remove_tails` /
   `stub_cleanup` at the same call sites as Java (grep the Java tree for each operation name; most are in
   `FoundConnectionInserter`, `RoutingBoard`, `ShoveTraceAlgo`).
   C++ already prints some `compare_trace_*_raw` lines (FoundConnectionLocator*); keep them, the tool ignores
   unknown operations unless asked.
3. **Tool** `tools/first_divergence.py java.log cpp.log [--ops route_item,ripped_item,...] [--context 5]`:
   - parse lines matching `\[([^\]]+)\] \[(compare_trace_[a-z_]+)\] (.*): (.*)$` plus the `Pass #N:` summaries;
   - normalise (strip timestamps/thread names; numbers stay exact);
   - walk both sequences in order and print the first record that differs, the preceding `--context` records of each
     side, and the pass number and item id where it happened;
   - exit 0 when identical.
4. **Run** (both sides `-mt 1`, same `-mp`, optimizer and fanout off to isolate the router first):
   ```
   java -jar $JAR -de tests/data/Issue026-J2_reference.dsn -do /tmp/j.ses -mt 1 -mp 20 \
        --router.optimizer.enabled=false --router.fanout.enabled=false \
        --user_data_path=$(mktemp -d) --debug.enable_detailed_logging=true --logging.file.location=/tmp/j.log
   build/freerouting -de tests/data/Issue026-J2_reference.dsn -do /tmp/c.ses -mt 1 -mp 20 \
        --router.optimizer.enabled=false --router.fanout.enabled=false \
        --debug.enable_detailed_logging=true --logging.file.location=/tmp/c.log
   python3 tools/first_divergence.py /tmp/j.log /tmp/c.log
   ```
   (Until T304 lands, add whatever minimal flag handling the C++ CLI needs for these exact flags.)
5. **Fix loop.** For the first divergence: find the Java code path that produced the Java record, compare with the
   C++ port line by line, fix, add a regression gtest that reproduces the situation on a small fixture, re-run. If a
   fix is bigger than ~700 lines or belongs to an unported class, file `T611a`, `T611b`, … with the exact divergence
   text and stop.
6. When Issue026 matches, do Issue107, then Issue034. Then switch fanout on (`--router.fanout.enabled=true`) — fanout
   is part of `AutorouteBatchLoop` (T408); if the interim router has no fanout, record that and stop there.

### Known suspects (check these first when the diff points into them)

- Item order (`TreeSet<Item>` = descending id) — T011.
- `java.util.Random` uses — `MazeRandom` (ripup randomisation only for pass ≥ 4 and pass % 3 != 0,
  `MazeRipupResolver` ≈160), `PolygonShape` split start corner (T009).
- Exact arithmetic vs `double`: Java `FloatPoint`/`double` math must stay `double` in C++ (no `long double`, no
  `-ffast-math` — check `CMakeLists.txt` flags).
- Java integer division/rounding: `(int) Math.round(x)` = floor(x+0.5); `(int)` cast truncates toward zero.
- `HashMap` iteration in Java ports (see the helper from T116).
- The interim `BatchAutorouter` itself (no BatchLoop, fixed "stop after 2 passes without progress") — differences
  in *which passes run* are expected until T408/T409; compare only pass 1 records first.

### Tests / acceptance

- `first_divergence.py` has a self-test with two small hand-written logs (identical → exit 0; one field differs →
  reports it).
- **Done when** Issue026 routes with the same `#unrouted` as Java (from the final `Pass #N` summary or the SES via the
  Java `-drc` scorer) and, stretch, `first_divergence.py` reports no difference for pass 1. Issue107 and Issue034 have
  their first divergence written into `docs/migration/E2E_STATUS.md`.
