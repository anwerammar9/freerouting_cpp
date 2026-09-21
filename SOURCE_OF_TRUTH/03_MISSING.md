# Missing Logic: Detailed Gap Analysis

## 1. AUTOROUTE MODULE — Core Routing Engine

### 1.1 MazeSearchAlgo (227 lines C++ vs 1291 lines Java)

**Architecture difference:** C++ uses grid-based A*. Java uses expansion-room/door-based maze search.

| Method | Java Lines | C++ Status | What's Missing |
|--------|-----------|------------|----------------|
| `init()` | 886-976 | Partial | Creates rooms from tree shapes, TargetItemExpansionDoors, seeds queue with door centers |
| `expand_to_room_doors()` | 298-469 | Stub | Weighted distance, DrillPage expansion, ripup, trace shoving |
| `expand_to_other_layers()` | 748-880 | Missing | Via padstack checking, layer drillability, SMD attachment, via masks |
| `check_ripup()` | 1003-1085 | Missing | Fanout protection, cost scaling, randomization |
| `shove_trace_room()` | 1093-1144 | Missing | Delegates to MazeShoveTraceAlgo |
| `expand_to_target_doors()` | 475-538 | Missing | Target item expansion with line-of-sight |
| `door_is_small()` | 594-618 | Missing | Narrow door detection |
| Distance calculation | line 174 | Hardcoded `100.0` | Should be `shape_entry_middle.weighted_distance()` |

### 1.2 Missing Autoroute Classes (12 files, ~4300 lines)

| Class | Lines | Purpose |
|-------|-------|---------|
| `MazeShoveTraceAlgo` | 316 | Trace shoving during maze search |
| `Sorted45DegreeRoomNeighbours` | 810 | 45° room neighbor calculation |
| `SortedOrthogonalRoomNeighbours` | 482 | Orthogonal room neighbor calculation |
| `SortedRoomNeighbours` | 680 | Base room neighbor calculation |
| `LocateFoundConnectionAlgo45Degree` | 238 | 45° connection location |
| `LocateFoundConnectionAlgoAnyAngle` | 363 | Any-angle connection location |
| `BatchAutorouterThread` | 546 | Multi-threaded routing worker |
| `BatchOptimizer` | 397 | Post-route optimization |
| `BatchOptimizerMultiThreaded` | 294 | Multi-threaded optimizer |
| `BoardHistory` | ~200 | Backtracking/convergence |
| `MazeListElement` | 130 | Priority queue element pooling |
| `BatchAutorouterV19` | 490 | Legacy comparison router |

> **Resolved 2026-09-19 (WP-9 + WP-10 Increment A):** `DrillPage`, `DrillPageArray`, and `ExpansionDrill::calculate_expansion_rooms()` are now ported and tested (DrillPage.java:189, DrillPageArray.java, drill/ExpansionDrill.java), and complete free space expansion rooms are now `SearchTreeObject`s stored in the autoroute search tree (rooms are found/reused by overlap queries). What is still missing is the *use* of drill pages inside `MazeSearchAlgo` (Java `expand_to_room_doors`/`expand_to_other_layers` rooms with via doors), `AutorouteEngine.completeExpansionRoom`'s full `ShapeSearchTree.completeShape` free-space expansion (blocker #6), and `CompleteFreeSpaceExpansionRoom::calculateTargetDoors` (needs the `Connectable::get_trace_connection_shape()` analogue).

### 1.3 BatchAutorouter — Missing Convergence Logic

Java `runBatchLoop()` (lines 549-686) contains:
- Board history recording after each pass
- Score comparison and backtracking
- Stop conditions: `STOP_AT_PASS_MINIMUM=8`, check every `STOP_AT_PASS_MODULO=4`
- Board restoration up to `BOARD_RANK_LIMIT=50`
- C++ has none of this — simple loop stops after 2 no-progress passes

### 1.4 AutorouteEngine — Grid vs Expansion Rooms

C++ `autoroute_connection()` (lines 119-416):
- Uniform grid A* on single layer
- Manhattan distance heuristic
- Binary blocked/unblocked cells
- Creates PolylineTrace directly

Java uses:
- Expansion room graph traversal
- Multi-layer routing with via insertion
- Ripup and trace shoving
- Neckdown handling
- Fanout routing
- Integration with InsertFoundConnectionAlgo

---

## 2. BOARD MODULE — Stub Methods

### 2.1 Critical Stubs (blocks routing)

| Method | File:Line | Issue |
|--------|-----------|-------|
| `ShoveTraceAlgo::spring_over()` | ShoveTraceAlgo.cpp:235-246 | Returns input polyline unchanged |
| `ShoveTraceAlgo::insert()` | ShoveTraceAlgo.cpp:167-212 | No board mutation |
| `RoutingBoard::opt_changed_area()` | RoutingBoard.cpp:67-74 | Empty body |
| `Item::get_normal_contacts()` | Item.cpp:43-48 | Returns empty (Trace/DrillItem/ConductionArea override) |
| `Item::first_common_layer()` | Item.cpp:86-91 | WAS returning -1; resolved 2026-09-18 |

**Resolved 2026-09-18:** `RoutingBoard::combine_traces()`, `get_outline()`, `contains_trace_tails()`, `get_trace_tail()` ported (BasicBoard.java:1307 `getTraceTail`, 684 `combineTraces`, RoutingBoard.java:1188 `containsTraceTails`; Java `getOutline` scans the item list for a `BoardOutline`). `BoardOutline` restored as a compilable build member (was excluded from `src/board/CMakeLists.txt` with broken namespace forward-declarations). `additional_update_after_change()` is a faithful no-op until an autoroute engine is wired into the item-change path (Java RoutingBoard.java:96 returns when `autorouteEngine == null`). Note base `Trace::normalize()/combine()/split()` still return false/empty — the `PolylineTrace` overrides are real.

**Resolved 2026-09-18 (WP-1 Contact Graph):** `Item::first_common_layer()` ported (was returning -1 unconditionally); `Item::get_connection_items()` ported (was returning `{}`); `Item::is_fanout_via()` added; `DrillItem::first_layer()/last_layer()` overrides added and layer range now synced from the padstack in the `Via` ctor / `set_padstack` (fixes vias always reporting 0..0); `Trace::normal_contact_point()` extended for DrillItem; `DrillItem::normal_contact_point()` added. Verified by 8 new `ItemContactGraphTest` cases and a diagnostics run (14/14 green, 413/413 ctest + run_tests green).

**Resolved 2026-09-18 (WP-2 NetIncompletes):** `connected_group_count` / `get_connected_group_count()` added (was missing vs `NetIncompletes.java:46,141,155`). Fixed real aliasing bug in `join_connected_sets()`: passed `const shared_ptr<...>&` aliasing `from_item`/`to_item` members that the join reassigns, so from-group items beyond the first were never repointed (3 isolated vias → 3 airlines, expected 2); now takes shared_ptrs by value. 415/415 ctest + run_tests green.

### 2.2 Missing Board Classes

| Class | Java Lines | Purpose |
|-------|-----------|---------|
| Pin (complete) | 722 | Only 25 lines in C++ — missing padstack, shapes, exit restrictions |
| ShapeSearchTree90Degree methods | 287 | create_incomplete_room, create_complete_room, cutout_drill_page |
| ShapeSearchTree45Degree methods | 457 | Same as 90° + border_line_of_obstacle |

---

## 3. DESIGIFORMS MODULE — I/O Gaps

### 3.1 Write Path Almost Entirely Missing

| Component | Java Write | C++ Write | Status |
|-----------|-----------|-----------|--------|
| SessionWriter | Full session | Traces only | STUB |
| Structure | Full DSN write | None | MISSING |
| Network | Full DSN write | None | MISSING |
| Library | Full DSN write | None | MISSING |
| Placement | Write-only | Read-only | INVERTED |
| Wiring | Read/write | Header-only | MISSING |
| DsnFile | Read/write | Header-only | MISSING |

### 3.2 Read Path Missing Logic

| Component | Missing in C++ |
|-----------|---------------|
| Scanner | No keyword recognition, no lexical states |
| Structure | Skips keepouts, autoroute settings, snap_angle, control, rules |
| Network | Skips via infos/rules, net classes, class pairs |
| Library | Only reads circle/rect shapes |
| CoordinateTransform | Entire class missing (199 lines) |

---

## 4. ENTRY POINT — CLI/DRC Gaps

| Gap | Impact |
|-----|--------|
| DRC mode is a stub | Never calls DesignRulesChecker |
| `GlobalSettings::load()` returns nullptr | Settings persistence broken |
| `GlobalSettings::save_as_json()` returns false | Settings can't be saved |
| No async job scheduling | No job queue support |
| `session.add_job()` commented out | Session-job link broken |
| `set_input()` doesn't read file | Manual output path required |
| `-dl` sets wrong field | Bug |
| Generic `--property=value` unimplemented | CLI limited |
