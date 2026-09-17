# Migration Continuation Prompt

Copy and paste this prompt into a new agent session to continue the migration work.

---

## Prompt

```
Read SOURCE_OF_TRUTH/README.md, then 01_MEMORY.md, then 05_WORK_PACKAGES.md. Pick the highest-priority incomplete work package and implement it. After each class port, build and test. When done, update 02_PROGRESS.md, 03_MISSING.md, and docs/migration/INDEX.md.
```

---

## Extended Prompt (for longer sessions)

```
You are continuing the Freerouting Java-to-C++20 migration. Java source is truth. Goal: headless routing parity (load DSN → route → write SES).

STEP 1: Read these files:
- SOURCE_OF_TRUTH/README.md (status overview)
- SOURCE_OF_TRUTH/01_MEMORY.md (architecture, conventions, gotchas)
- SOURCE_OF_TRUTH/05_WORK_PACKAGES.md (what to work on)
- SOURCE_OF_TRUTH/CONVENTIONS.md (C++20 patterns, naming)

STEP 2: Verify the build works:
cmake --build build --target freerouting_board -j$(nproc)

STEP 3: Pick the highest-priority incomplete work package from 05_WORK_PACKAGES.md and implement it. For each class:
1. Read the Java source in java/app/freerouting/<module>/ClassName.java
2. Check if C++ .h/.cpp already exists
3. Create/update .h, .cpp, _test.cpp files
4. Update src/<module>/CMakeLists.txt
5. Build and fix errors
6. Register test in tests/CMakeLists.txt
7. Run tests until green

STEP 4: Before stopping, update these files:
- SOURCE_OF_TRUTH/02_PROGRESS.md (move classes to Completed)
- SOURCE_OF_TRUTH/03_MISSING.md (remove completed items)
- docs/migration/INDEX.md (update module percentages)
- docs/migration/MIGRATION_<module>.md (add porting notes)

Do NOT commit unless explicitly asked.
```

---

## Commit Prompt (run after finishing a work package)

```
Commit the migration work just completed. Follow these steps:

1. Check what changed:
git status
git diff --stat

2. Review the diff for correctness:
git diff

3. Stage only migration files (not build artifacts):
git add src/ tests/ docs/migration/ SOURCE_OF_TRUTH/

4. Write a descriptive commit message:
- First line: module + what was ported (max 72 chars)
- Body: list ported classes, line counts, test results
- Example:
  "board: port ForcedPadAlgo and ForcedViaAlgo

   Ported from Java:
   - ForcedPadAlgo (352 lines) — force-pad placement algorithm
   - ForcedViaAlgo (223 lines) — force-via wrapper

   Added tests: ForcedPadAlgo_test, ForcedViaAlgo_test
   All 377 ctest tests pass.
   Board module: 84% -> 88% headless."

5. Commit:
git commit -m "<message>"

6. Do NOT push unless explicitly asked.
```

---

## Commit + Update Prompt (commit and update source of truth together)

```
Commit the migration work and update all tracking files. Steps:

1. Build and test one final time:
cmake --build build --target freerouting_board -j$(nproc)
ctest --test-dir build --output-on-failure

2. Update source of truth:
- SOURCE_OF_TRUTH/02_PROGRESS.md
- SOURCE_OF_TRUTH/03_MISSING.md
- docs/migration/INDEX.md
- docs/migration/MIGRATION_<module>.md

3. Stage everything:
git add src/ tests/ docs/migration/ SOURCE_OF_TRUTH/

4. Commit with descriptive message (see commit prompt format above).

5. Do NOT push unless explicitly asked.
```

---

## Short Prompt (for quick tasks)

```
Continue the Freerouting C++ migration. Read SOURCE_OF_TRUTH/05_WORK_PACKAGES.md, do the next task, build, test, update progress.
```
