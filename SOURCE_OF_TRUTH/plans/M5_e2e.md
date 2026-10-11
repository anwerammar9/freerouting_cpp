# M5 — End-to-end parity per board (gate G4)

## Common procedure for T601–T609  (O)

**Deps of T601** (and therefore all): T001, T117, T202, T415, T008, T611, T506a. Do T501–T505 and T506b first if they
are still open.

1. **Java reference.** `tools/java_baseline.sh --mode st <board>` (T008) gives `java_output_ses/<board>_java.{ses,json,log}`.
   For a bit-exact attempt also produce a detailed log:
   `--debug.enable_detailed_logging=true --logging.file.location=<file>` (M3).
2. **C++ run.** Same flags. Fixed settings for both sides:
   `-mt 1 -mp 100 --router.job_timeout=<tier timeout> --router.fanout.max_milliseconds_per_pin=600000
   --router.optimizer.timeout=<tier timeout> --user_data_path=<fresh dir>` (C++ ignores the last one until T303).
3. **Compare** in this order and stop at the first failure:
   1. load parity: `--dump-board-counts` vs `tests/data/expected/<board>.counts.json` (T118 tool);
   2. `tools/first_divergence.py java.log cpp.log` (bit-exact stretch; note the first divergence even if you do not
      chase it);
   3. quality via the neutral scorer: `java -jar $JAR -de <dsn> <cpp.ses> -drc cpp_drc.json` and the same for the Java SES
      → unrouted count, clearance violations, wire length, via count;
   4. `tools/e2e_compare.sh <dsn>` wire/via counts.
4. **Fix or file.** Fix the first divergence if it is small; otherwise file `T60xa`, … with the divergence text, the
   Java/C++ code locations, and a minimal reproduction.
5. **Gate G4 per board:** #unrouted(C++) ≤ #unrouted(Java), 0 clearance violations reported by the Java scorer,
   wires and vias within ±10 %. Record the row in `docs/migration/E2E_STATUS.md` (`tools/e2e_all.sh` regenerates it).

**Done when (each T60x).** G4 holds for the board, or every remaining difference has its own task.

## Board notes

| Task | Board | Notes |
|---|---|---|
| T601 | empty_board | trivial; checks the whole CLI path and SES header |
| T602 | Issue034-Green14SegLED | 101 nets; Java baseline exists |
| T603 | Issue107-freq_teiler_200kHz_kicad | 77 nets; Java routes 296 wires, 0 vias |
| T604 | Issue026-J2_reference | 24 nets; the M3 shakedown board — should already match |
| T605 | Issue103-Board-Unrouted | 282 nets; no Java baseline today (T008 must explain why) |
| T606 | Issue006-LPC18XX_43XX_SCH | binary OLE file, not a DSN: parity = both report a parse error and exit non-zero; Java message `"Not a Specctra DSN file: expected '(pcb <name>' header"` |
| T607 | Issue070-Autorouter_FQ101_PCB_2022-05-13 | 688 nets; long run — use the large-tier timeout |
| T608 | BBD_Mars-64 | 94 nets; `build/e2e_all.out` once showed a 300 s C++ timeout while E2E_STATUS showed 0.6 s — reproduce first; a hang is a bug (likely an endless loop in a ported algorithm, compare with Java's iteration limits) |
| T609 | Issue029-hw48na | 163 nets; the original "19 % headless" benchmark board |

## T610 — Sweep over all 117 DSNs  (O)

1. Load parity for every file (T118 results — re-run them; the reader may have changed since).
2. Route every board for which the Java baseline finishes within 60 s in st mode; run C++ with the same flags.
3. Write a table into E2E_STATUS.md: board, load parity, Java unrouted, C++ unrouted, Java/C++ wires, vias, seconds.
4. The DAC2020 `bm01–bm11` set (5–80 KB) is the best bit-exact corpus: try `first_divergence.py` on all of them and list
   the first divergence per board; group by root cause and file one task per cause.

**Done when.** Table complete; every board failing G4 has a task.
