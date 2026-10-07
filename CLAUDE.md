# CLAUDE.md — Freerouting C++ Migration

## Project

Freerouting Java → C++20 PCB autorouter migration. Java source is truth. Goal: headless routing parity (load DSN → route → write SES).

## Source of Truth

Read these files at session start:
```
SOURCE_OF_TRUTH/README.md          — Quick status
SOURCE_OF_TRUTH/TASKS.md           — Task queue (what to do next; run by loop_migration.sh)
SOURCE_OF_TRUTH/01_MEMORY.md       — Architecture, conventions, gotchas
SOURCE_OF_TRUTH/02_PROGRESS.md     — Module-by-module status
SOURCE_OF_TRUTH/03_MISSING.md      — What's missing
SOURCE_OF_TRUTH/04_IMPLEMENTATION_PLAN.md — Phased plan
SOURCE_OF_TRUTH/05_WORK_PACKAGES.md      — Agent work packages
SOURCE_OF_TRUTH/CONVENTIONS.md     — C++20 patterns, naming, testing
```

## Build Commands

```bash
# Configure (one-time)
conan install . --build=missing -of build
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DCMAKE_PREFIX_PATH=build/build/Release/generators

# Build module
cmake --build build --target freerouting_board -j$(nproc)

# Run tests
ctest --test-dir build --output-on-failure
```

## Key Rules

1. **Java files in `java/app/freerouting/**`** are the reference — port faithfully
2. **3 files per class**: `.h`, `.cpp`, `_test.cpp`
3. **C++20 only**: smart pointers, RAII, `std::optional`, `#pragma once`
4. **Naming**: PascalCase classes, snake_case functions, trailing `_` for private
5. **Layering**: `freerouting_core` is leaf (no deps). BoardStatistics goes in `freerouting_scoring` lib
6. **Never include** `src/board/Components.h/.cpp` — it's dead code
7. **Java nets are 1-based** for RatsNest indexing
8. **Both** `ctest` AND `run_tests` must pass

## Critical Gotchas

- `Item::get_normal_contacts()` base returns `{}` — Trace overrides
- `DrillItem` canonical = `Via.h` (DrillItem base, Via, Pin)
- No JSON serializer — hand-roll matching Java `@SerializedName`
- `Components.h/.cpp` is DEAD CODE — canonical is in `Component.h`

## Session Shutdown

Before ending, update:
1. `SOURCE_OF_TRUTH/TASKS.md` — task status + `- result:` line
2. Relevant `docs/migration/MIGRATION_*.md` — add notes

02–05 and `docs/migration/INDEX.md` are historical; they are refreshed in bulk by task T705.
