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
1. Read the Java source in src/<module>/ClassName.java
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

## Short Prompt (for quick tasks)

```
Continue the Freerouting C++ migration. Read SOURCE_OF_TRUTH/05_WORK_PACKAGES.md, do the next task, build, test, update progress.
```
