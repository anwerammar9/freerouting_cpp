# Task specs — how to use them

One file per milestone. Every open task in `SOURCE_OF_TRUTH/TASKS.md` has a `- spec:` line pointing at its section
here. TASKS.md stays the queue (status, deps, result lines); the spec is the *how*.

| File | Tasks |
|------|-------|
| [M0_oracles.md](M0_oracles.md) | T006–T010 (class map, Java harness, Java baselines, determinism fixes) |
| [M1_dsn_reader.md](M1_dsn_reader.md) | T109b, T126, T109c, T110–T118, T125, T124a–c |
| [M2_ses_io.md](M2_ses_io.md) | T122, T201–T203, T204a, T204b |
| [M3_shakedown.md](M3_shakedown.md) | T611 |
| [TrackB_settings_cli.md](TrackB_settings_cli.md) | T301–T307 |
| [TrackC_pipeline.md](TrackC_pipeline.md) | T401–T415, T420–T423, T704, T706 |
| [Phase5_board_gaps.md](Phase5_board_gaps.md) | T501–T505 |
| [M5_e2e.md](M5_e2e.md) | T601–T610 |
| [M7_stragglers.md](M7_stragglers.md) | T701–T703, T405, T406, T705 |
| [M8_benchmark.md](M8_benchmark.md) | T801–T810 |

## Spec layout (every task uses the same headings)

- **Goal** — one or two sentences.
- **Java** — the files and methods to port, with line ranges (Java 2.1.0 tree under `java/app/freerouting/`).
  Line numbers were taken on 2026-10-10; if a range looks off, search for the method name.
- **C++ target** — files to create or change; the classes and functions to add, with signatures.
- **Reuse** — existing C++ code to call. Do not write a second version of something listed here.
- **Steps** — numbered, in the order to do them.
- **Java parity notes** — behaviour that looks wrong but must be kept.
- **Tests** — the gtests to write (name, input, expected). Golden values come from Java (T007 harness or a throwaway
  `javac` program), never from hand calculation.
- **Done when** — the acceptance check, as commands where possible.
- **Watch out** — known traps.

Steps assume the rules in `SOURCE_OF_TRUTH/ROADMAP.md` §2 (one task per session, 3 files per class, the gate,
Java parity, RESCAN link groups). They are not repeated per task.

## Conventions used in all specs

- **Namespaces / folders.** New DSN parser classes live in `src/designforms/specctra/parser/` in namespace
  `freerouting::designforms::specctra::parser` (as T101–T109a did). Do not touch the legacy reader in
  `src/designforms/specctra/*.cpp` until T124a deletes it.
- **Scope keyword classes.** A parser scope class (`Library`, `Network`, …) derives from `ScopeKeyword`, overrides
  `bool read_scope(ReadScopeParameter&) const`, and replaces the placeholder in `parser/Keyword.cpp`:
  change `const ScopeKeyword k_LIBRARY_SCOPE("library");` into `const Library k_LIBRARY_SCOPE;` and include the
  header. `Structure`, `Plane`, `Resolution` already follow this pattern — copy them.
- **Java `null` returns** become `std::optional<T>`, `nullptr`, or `bool false` exactly where Java returns null; keep
  Java's distinction between null and empty collections (see `via_padstack_names`).
- **Java `Collection`/`LinkedList`** keep insertion order → `std::vector`. **`TreeSet`/`TreeMap`** → `std::set`/`std::map`
  with a comparator equal to Java's `compareTo` (Java `String.compareTo` compares UTF-16 code units; for the ASCII
  names in the test boards `std::string` `<` gives the same order — note it in a comment).
- **Java `String.replaceAll("\\.\\d+", "")`** etc. → `std::regex_replace` with the same pattern (ECMAScript syntax
  is compatible for these patterns).
- **Logging:** `logger::FRLogger::instance().warn(...)` with the same message text as Java (tests may grep logs).
- **Tests for parser scopes** construct `SpecctraDsnStreamReader scanner("<snippet>)", true)` (the text starts *after*
  the scope keyword and ends with the scope's closing bracket), then call `read_scope`. See `Structure_test.cpp` and
  `AutorouteSettings_test.cpp`.
- **Tests that need a board** create it through the T109c `MinimalBoardManager` (once it exists) or build a
  `RoutingBoard` directly as `RoutingBoardAutoroute_test.cpp` does.

## Model hint legend

**S** — Sonnet at low/medium effort is fine. **O** — prefer Opus at medium effort. If an S task fails its gate twice,
mark it `[!]` with the failure output.
