# AGENTS.md — Freerouting C++ Migration

## Project Overview

Migrating Freerouting PCB autorouter from Java to C++20. The Java baseline
(`java/`, Freerouting 2.1.0) is truth. Goal: headless routing parity — load DSN →
route → write SES.

## Repository Layout

```
src/                          # C++20 migration (only C++ sources live here)
├── autoroute/                # Routing algorithms
├── board/                    # Board model, items, search trees
├── core/                     # Core data model, jobs, events
├── datastructures/           # Generic data structures
├── designforms/specctra/     # DSN/SES file I/O
├── drc/                      # Design rules checking
├── geometry/planar/          # 2D geometry primitives
├── interactive/              # Headless board manager
├── logger/                   # Logging
├── management/               # Session management
├── rules/                    # Design rules
└── settings/                 # Configuration
java/                         # Java baseline (Freerouting 2.1.0), source of truth
└── app/freerouting/          # package root; build with ./gradlew executableJar
tests/                        # Test data and integration tests
docs/migration/               # Per-module migration status
SOURCE_OF_TRUTH/              # Compiled knowledge base
templates/                    # Migration templates
```

## Session Start (MANDATORY)

1. Read `SOURCE_OF_TRUTH/README.md` for current status
2. Read `SOURCE_OF_TRUTH/01_MEMORY.md` for conventions and gotchas
3. Read `SOURCE_OF_TRUTH/05_WORK_PACKAGES.md` for what to work on
4. Verify build: `cmake --build build --target freerouting_board -j$(nproc)`

## Build & Test

```bash
# Build specific module
cmake --build build --target <module_target> -j$(nproc)

# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
ctest --test-dir build -R ClassName --output-on-failure

# Run assert-based smoke tests
cmake --build build --target run_tests -j$(nproc) && ./build/tests/run_tests

# Build the Java baseline (JDK 25); run it headless:
./gradlew executableJar
java -jar build/libs/freerouting-<version>-executable.jar -de in.dsn -do out.ses
```

Module targets: `freerouting_core`, `freerouting_board`, `freerouting_autoroute`, `freerouting_geometry`, `freerouting_rules`, `freerouting_datastructures`, `freerouting_logger`, `freerouting_constants`, `freerouting_interactive`, `freerouting_settings`, `freerouting_management`, `freerouting_designforms`, `freerouting_drc`

## Porting a Class

1. Read Java source: `java/app/freerouting/<module>/ClassName.java`
2. Check if C++ `.h`/`.cpp` already exists (partial port?)
3. Read dependencies — what C++ classes does this need?
4. Create 3 files: `.h`, `.cpp`, `_test.cpp`
5. Update `src/<module>/CMakeLists.txt` — add `.cpp` to target
6. Build and fix errors
7. Register test in `tests/CMakeLists.txt`
8. Run tests — fix until green

## Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Classes | PascalCase | `MazeSearchAlgo` |
| Functions | snake_case | `find_connection()` |
| Variables | snake_case | `curr_net_no` |
| Constants | kPascalCase | `kMaxPasses` |
| Private members | trailing `_` | `board_` |

## Critical Rules

- **Java `java/**/*.java`** = source of truth
- **3 files per class**: `.h`, `.cpp`, `_test.cpp`
- **C++20**: smart pointers, RAII, `std::optional`, `#pragma once`
- **Layering**: `freerouting_core` has NO deps (leaf lib)
- **Never include** `src/board/Components.h/.cpp` — dead code
- **Java nets are 1-based** — use `net_no - 1` for 0-based indexing
- **Both** `ctest` AND `run_tests` must pass

## Session End (MANDATORY)

Update these files before stopping:
1. `SOURCE_OF_TRUTH/02_PROGRESS.md` — move completed classes to "Completed"
2. `SOURCE_OF_TRUTH/03_MISSING.md` — remove completed items
3. `SOURCE_OF_TRUTH/04_IMPLEMENTATION_PLAN.md` — mark done tasks
4. `docs/migration/INDEX.md` — update module percentages
5. `docs/migration/MIGRATION_<module>.md` — add porting notes
