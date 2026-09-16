# Memory: Architecture, Conventions, and Patterns

## Project Structure

```
freerouting_cpp/
├── src/                          # All source (no separate include/)
│   ├── main.cpp                  # Entry point
│   ├── Freerouting.cpp/.h        # Main application class
│   ├── autoroute/                # Auto-routing algorithms (35 Java, 21 C++ compiled)
│   ├── board/                    # Board model (51 Java, 41 C++ headless)
│   ├── boardgraphics/            # NOT migrated (GUI only)
│   ├── constants/                # Constants (1 Java, 1 C++ — done)
│   ├── core/                     # Core data model (36 Java, 6 C++)
│   ├── datastructures/           # Generic data structures (14 Java, 6+2 C++)
│   ├── designforms/specctra/     # Specctra DSN/SES I/O (41 Java, 9 C++)
│   ├── drc/                      # Design rules checking (7 Java, 1 C++)
│   ├── geometry/planar/          # 2D geometry primitives (34 Java, 34 C++ — done)
│   ├── interactive/              # Interactive/headless (9 Java, 3 C++)
│   ├── logger/                   # Logging (4 Java, 4 C++ — done)
│   ├── management/               # Session/job management (18 Java, 2 C++)
│   ├── rules/                    # Design rules (10 Java, 10 C++ — done)
│   └── settings/                 # Configuration (21 Java, 2 C++)
├── tests/                        # Integration tests + test data
├── docs/migration/               # Per-module migration status
├── templates/                    # Migration templates
├── SOURCE_OF_TRUTH/              # This folder — compiled knowledge
└── build/                        # Build output
```

## Build System

- **CMake** 3.20+, **C++20**, **Conan** for deps
- Dependencies: Boost (multiprecision), GoogleTest 1.15, Threads
- Static libraries per module, linked into `freerouting` executable
- Tests: `ctest --test-dir build` (~377 tests registered)

### Build Commands
```bash
conan install . --build=missing -of build
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DCMAKE_PREFIX_PATH=build/build/Release/generators
cmake --build build --target freerouting_board -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Layering Constraints (CRITICAL)

- `freerouting_core` = leaf static lib (no target_link_libraries)
- `freerouting_interactive` links core + board
- `freerouting_board` links rules, geometry, logger, datastructures
- `freerouting_autoroute` links core + board
- **BoardStatistics** must go in a separate `freerouting_scoring` lib (core→interactive would be a cycle)

## C++20 Conventions

- Smart pointers (`unique_ptr`/`shared_ptr`), RAII, `std::optional`
- Naming: `PascalCase` classes, `snake_case` functions/variables, `kPascalCase` constants, trailing `_` for private members
- Each migrated class produces 4 files: `.h`, `.cpp`, `_test.cpp`, `.java`
- `#pragma once` headers
- Member initializer lists for constructors

## Key Gotchas (Verified, Do Not Rediscover)

1. **`Components` duplicate**: canonical is `board::Components` in `src/board/Component.h` (method `component_count()`). `src/board/Components.h/.cpp` is DEAD code.
2. **Java nets are 1-based** for RatsNest indexing (`get_net_no(i) - 1`)
3. **`Item::get_normal_contacts()`** default returns `{}`. Trace overrides with real implementation.
4. **`RoutingBoard::add_item`** only appends to `items_`; search-tree registration via `BasicBoard::insert_trace_without_cleaning`
5. **`DrillItem` canonical = `Via.h`** (DrillItem base, Via, Pin : DrillItem)
6. **`Trace::get_length()`** is pure virtual; PolylineTrace overrides
7. **`PlanarDelaunayTriangulation`**: turn Items into `shared_ptr<Storable>` adapters
8. **JSON**: hand-rolled; match Java `@SerializedName` exactly
9. **No JSON serializer** — BoardStatistics needs manual JSON string builder
10. **`run_tests`** (assert suite) is separate from ctest; both must pass

## Dependency Graph (What to Migrate Next)

```
DONE: geometry, rules, logger, constants, datastructures(core), board(84%), autoroute(60%)
                          │
                          ▼
   WP-1: Contact graph (Item contacts + DrillItem contacts)
   WP-2: NetIncompletes (full port)
   WP-3: RatsNest (full port)
                          │
                          ▼
   WP-4: BoardStatistics + scoring
                          │
                          ▼
   WP-5: Tests + CMake + docs
                          │
                          ▼
   WP-6: RoutingJob / Session / BoardFileDetails
                          │
                          ▼
   WP-7..15: Remaining modules (parallel)
```

## Critical Path for Headless Routing

1. `geometry/planar` — ✅ Done
2. `datastructures` — ✅ Core done
3. `rules` — ✅ Done
4. `board` — 84% headless (41/49)
5. `autoroute` — 60% (21/35 compiled)
6. `designforms/specctra` — 32% (DSN load works, SES write partial)
7. `drc` — 29%
8. `settings` — 29%
9. `core` — 19%
10. `management` — 11%
