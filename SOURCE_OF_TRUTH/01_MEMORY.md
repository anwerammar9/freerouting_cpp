# Memory: Architecture, Conventions, and Patterns

## Project Structure

```
freerouting_cpp/
├── src/                          # C++20 migration (no Java here)
│   ├── main.cpp                  # Entry point
│   ├── Freerouting.cpp/.h        # Main application class
│   ├── autoroute/                # Auto-routing algorithms
│   ├── board/                    # Board model
│   ├── constants/                # Constants
│   ├── core/                     # Core data model
│   ├── datastructures/           # Generic data structures
│   ├── designforms/specctra/     # Specctra DSN/SES I/O
│   ├── drc/                      # Design rules checking
│   ├── geometry/planar/          # 2D geometry primitives
│   ├── interactive/              # Interactive/headless
│   ├── logger/                   # Logging
│   ├── management/               # Session/job management
│   ├── rules/                    # Design rules
│   └── settings/                 # Configuration
├── java/                         # Java baseline (Freerouting 2.1.0), source of truth
│   └── app/freerouting/          # package root (incl. gui/, api/, analytics/)
├── tests/                        # Integration tests + test data
├── docs/migration/               # Per-module migration status
├── templates/                    # Migration templates
├── SOURCE_OF_TRUTH/              # This folder — compiled knowledge
└── build/                        # Build output
```

The Java reference files are no longer co-located in `src/`; read them from
`java/app/freerouting/<module>/ClassName.java`.

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
- Each migrated class produces 3 files: `.h`, `.cpp`, `_test.cpp` (Java reference in `java/`)
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
11. **DrillItem layer range comes from the padstack** (Java `DrillItem.firstLayer/lastLayer`) — `Via` ctor + `set_padstack` sync `from_layer_`/`to_layer_` from the padstack. `Padstack` is incomplete in `Via.h`, so `set_padstack` must be defined in `Via.cpp`.
12. **shared_ptr aliasing vs Java reference semantics (NetIncompletes.join_connected_sets)**: Java captures a set reference in a local before a loop that reassigns fields — the captured reference stays valid. Porting this to `const shared_ptr<...>&` params that alias **members the same loop reassigns** breaks: once the from-item is repointed mid-loop, `p_from` reads the new to-set and the rest of the from-group is skipped → too many airlines. **Pass shared_ptrs by value** for such join/union helpers. (3-island MST test would catch: 3 isolated vias must produce 2 airlines, gave 3.)

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
