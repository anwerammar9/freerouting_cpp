# M1 — DSN reader parity (gate G2)

Order on the critical path: **T109b → T126 → T109c → T110 → T111 → T112 → T125 → T113 → T114a → T114b → T115 → T116 →
T117 → T118**, then **T124a → T124b → T124c** (after T202 / Track B pieces).

What a DSN file looks like to the reader (Java `ScopeKeyword.readScope` on `PCB_SCOPE`): `(pcb <name> (parser …)
(resolution …) (unit …) (structure …) (placement …) (library …) (network …) (wiring …))`. Each sub-scope keyword is a
`ScopeKeyword` subclass whose `readScope` is dispatched by the generic loop (`ScopeKeyword.java` 45–80, already
ported as `ScopeKeyword::read_scope`). **Placement is read before library**, so placements are only *collected*
(`placementList`) and inserted by `Network.insertComponents` once library and nets exist.

Parser state lives in `ReadScopeParameter` (Java `parser/ReadScopeParameter.java`, 187 lines; C++
`parser/ReadScopeParameter.h`). Fields still missing in C++ are added by the task that first needs them (listed per
task).

---

## T109b — Structure: board rules from the structure scope  (S)

**Goal.** Turn the `(rule …)` / `(layer … (rule …))` data collected by `Structure` into `BoardRules` values.

**Java** (`parser/Structure.java`):
| Method | Lines | Notes |
|---|---|---|
| `updateBoardRules(par, info, boardRules)` | 611–670 | clearance rules (all layers) → width rules → layer rules → `setPinEdgeToTurnDist(minTraceHalfWidth)` unless `smd_to_turn_gap` was seen |
| `setClearanceRule(rule, layer, transform, boardRules, stringQuote)` | 676–808 | public; returns true if `smd_to_turn_gap` found |
| `containsWireClearancePair(pairs)` | 810–817 | |
| `createDefaultClearanceClasses(boardRules)` | 819–824 | appends via, smd, pin, area |
| `appendClearanceClass(boardRules, name)` | 826–840 | sets default item clearance classes of the default net class |
| `clearanceEquals(matrix, l1, l2)` | 843–855 | unused by readScope; port for completeness (DSN writer uses it later) |

**C++ target.** `parser/Structure.{h,cpp}`: add static `update_board_rules(const ReadScopeParameter&, const
BoardConstructionInfo&, rules::BoardRules&)`, `set_clearance_rule(const ClearanceRule&, int layer, const
CoordinateTransform&, rules::BoardRules&, const std::string& string_quote) -> bool`,
`contains_wire_clearance_pair`, `create_default_clearance_classes`, `append_clearance_class -> int`,
`clearance_equals`.

**Reuse.** `rules::ClearanceMatrix::{set_default_value(v), set_default_value(layer,v), set_value(c1,c2,v),
set_value(c1,c2,layer,v), get_no, append_class, get_value(c1,c2,layer,false), get_class_count}`;
`rules::BoardRules::{set_default_trace_half_widths, set_default_trace_half_width(layer,v),
get_min_trace_half_width, set_pin_edge_to_turn_dist, get_default_net_class}`;
`NetClass::get_default_item_clearance_classes().set(ItemClass::VIA|PIN|SMD|AREA, n)`;
`parser::CoordinateTransform::dsn_to_board(double)`; `parser::WidthRule`, `parser::ClearanceRule`
(`clearance_class_pairs` is a `std::vector<std::string>` in Java collection order).

**Steps.**
1. Port the six functions verbatim, including `Math.round` semantics: Java `Math.round(double)` = floor(x + 0.5)
   for `(int)` casts — use `static_cast<int>(std::floor(x + 0.5))`, not `std::lround` (they differ for negative .5).
2. The 2-element branch (`clearanceClassPairs.size() == 2`) strips `"` and a leading `_` from the **second** element
   inside the loop over both elements (Java quirk: the `startsWith("_")` check is inside the `for i` loop, it only
   touches `currentPair[1]`). Keep it.
3. `"wire"` maps to class 1 (default class) without lookup.
4. Debug log lines: keep them at debug level (they are noisy).

**Tests** (`Structure_test.cpp`, new cases). Build `BoardRules` with a 2-layer `LayerStructure` and
`ClearanceMatrix::get_default_instance(ls, 0)`; transform = `CoordinateTransform(10, 0, 0)` or the real one:
- `(clearance 0.2)` → default value 2 on all layers (scaled).
- `(clearance 0.3 (type wire_via))` → classes via/smd/pin/area appended (because of `wire_`), `via` class value
  set both ways; `get_no("via") > 0`.
- `(clearance 0.25 (type smd_to_turn_gap))` → returns true, `pin_edge_to_turn_dist` set.
- `(clearance 0.1 (type smd_via_same_net))` → silently ignored (3-part name).
- `"(clearance 0.2 (type \"a b\"_wire))"` with string quote `"` → class `a b` appended.
- Layer rule on layer 1 → only layer 1 changed.
- Width rule `(width 0.25)` → default trace half widths = round(dsn_to_board(0.25)/2).
Golden numbers: run the same snippets through `Structure.setClearanceRule` in a throwaway Java program.

**Done when.** New tests green; no behaviour change elsewhere.

---

## T126 — Parser `Net`, `Net::Id`, `Net::Pin`, `NetList` (moved forward from T113)  (S) — NEW

**Why now.** `Structure.readScope` inserts planes and missing power planes through `scopeParameter.netlist`
(Structure.java 1067–1123, 526–571), so `NetList` must exist before T109c.

**Java.** `parser/NetList.java` (56, whole file) and `parser/Net.java` (129): `Id` (name, subnetNumber, `compareTo`
by name then subnet), `Pin` (componentName, pinName, `compareTo`), `Net` (`id`, `pinList` as `TreeSet<Pin>`,
`getPins`, `setPins`). The `writeScope/writeNetId/writePin` statics belong to T204b — leave them out.

**C++ target.** `parser/Net.{h,cpp,_test.cpp}`, `parser/NetList.{h,cpp,_test.cpp}`; add `NetList netlist;` to
`ReadScopeParameter` (Java initialises it in the field declaration, so it always exists).

**C++ shape.**
```cpp
class Net {
public:
    struct Id  { std::string name; int subnet_number; auto operator<=>(const Id&) const = default; };   // name, then subnet
    struct Pin { std::string component_name; std::string pin_name; auto operator<=>(const Pin&) const = default; };
    explicit Net(Id id);
    const Id id;
    const std::optional<std::set<Pin>>& get_pins() const;   // Java returns null before setPins
    void set_pins(const std::vector<Pin>& pins);             // Java: new TreeSet<>(pinList)
};
class NetList {
public:
    bool contains(const Net::Id&) const;
    Net* add_net(const Net::Id&);            // nullptr if it exists (Java returns null)
    Net* get_net(const Net::Id&);            // nullptr if absent
    std::vector<Net*> get_nets(const std::string& component, const std::string& pin) const; // map order
private:
    std::map<Net::Id, std::unique_ptr<Net>> nets_;
};
```
Defaulted `<=>` on `{string, int}` compares like Java `compareTo` for ASCII names (note this in a comment).

**Tests.** add twice → second returns nullptr; `get_nets` returns nets in Id order; `get_pins` is nullopt before
`set_pins`; duplicate pins collapse; `Id{"A",2} > Id{"A",1}`; `Id{"B",1} > Id{"A",9}`.

**Done when.** Tests green.

---

## T109c — Structure: create the board, insert keepouts and planes  (O)

**Goal.** At the end of `Structure::read_scope`, build the `RoutingBoard` exactly like Java and insert outline holes,
keepouts, planes and missing power planes.

**Java.**
| Piece | File / lines |
|---|---|
| readScope tail: create board if none, flip style, insert keepout/via_keepout/place_keepout lists, insert planes, `insertMissingPowerPlanes`, apply DSN autoroute settings to the job | Structure.java 1031–1137 |
| `createBoard` | Structure.java 1139–1286 |
| `separateHoles` + inner class `OutlineShape` | 577–605, 1316–1353 |
| `insertKeepout` (2 overloads), `KeepoutType` | 857–933, 1288–1292 |
| `insertMissingPowerPlanes` | 526–571 |
| `BoardParserCallback` interface | parser/BoardParserCallback.java (whole, 42) |
| `MinimalBoardManager` (createBoard computes the outline clearance class, then `new RoutingBoard(…, outlineShapes, outlineClearanceNo, rules, communication)`) | ReadScopeParameter.java 129–172 |
| `Communication` full fields, `SpecctraParserInfo` (+`WriteResolution`), `hostIsOldKicad`, `hostCadIsFusion` | board/state/Communication.java (whole, 148) |
| `DsnFile.adjustPlaneAutorouteSettings` (called by DsnReader when no autoroute scope; port it here, T117 calls it) | parser/DsnFile.java 29–107 |

**C++ target.**
- `parser/BoardParserCallback.h` (interface) and `parser/MinimalBoardManager.{h,cpp}` (the Java nested class as
  its own file; `ReadScopeParameter` gets `std::unique_ptr<BoardParserCallback> board_handling` defaulting to a
  `MinimalBoardManager`, plus `get_board()` and `std::vector<std::string> warnings` already exists).
  `get_current_routing_job()` returns `core::RoutingJob*` (nullptr in MinimalBoardManager).
- Extend `src/board/Communication.h` to Java's field set: `unit`, `resolution`, `parser_info` (add `constants`
  `std::vector<std::array<std::string,2>>`, `write_resolution` `std::optional<WriteResolution{char_name, positive_int}>`),
  `coordinate_transform` (board-side `CoordinateTransform` — check `src/board/CoordinateTransform*`; if missing, port
  board/state/CoordinateTransform.java, 112 lines), `id_generator` pointer/reference to the board id generator.
  Implement `host_is_old_kicad()` like Java (case-insensitive "kicad" + first integer of host_version ≤ 5) — the
  current stub returns false.
- `parser/Structure.cpp`: `create_board(ReadScopeParameter&, BoardConstructionInfo&) -> bool`,
  `separate_holes(std::vector<std::shared_ptr<PolylineShape>>&) -> std::vector<…>`, `insert_keepout(…)` ×2,
  `insert_missing_power_planes(…)`. Move the keepout lists from `ReadScopeParameter` back into locals only if it is
  free to do so; otherwise leave them (they are a harmless C++ deviation — note it).
- `parser/DsnFile.{h,cpp}`: `adjust_plane_autoroute_settings(board::RoutingBoard*) -> bool`.

**Reuse.** `RoutingBoard(bounding_box, layer_structure, rules, communication*)` + `insert_outline(shapes,
clearance_class)` (the C++ ctor does **not** insert the outline; Java's ctor does — MinimalBoardManager must call
`insert_outline` right after construction to match Java's item ids: outline is the first item); `insert_obstacle`,
`insert_via_obstacle`, `insert_component_obstacle`, `insert_conduction_area` (RoutingBoard.h 350–390);
`ClearanceMatrix::get_default_instance(ls, 0)`; `BoardRules(ls, matrix)`; `rules.set_trace_angle_restriction`;
`Shape::transform_area_to_board` (parser/Shape.h:75); `Shape::bounding_box()` / `Rectangle::union`;
`geometry::planar::Limits::CRIT_INT`; `IntBox::offset`; `PolylineShape::split_to_convex/corner/border_line_count/
bounding_box/dimension`; `TileShape::contains(Point)`; `rules::Nets::add(name, subnet, contains_plane)`,
`Nets::get(name, subnet)`; `NetClass::get_default_item_clearance_classes().get(ItemClass::AREA)`;
`BasicBoard::set_flip_style_rotate_first` (Java calls `board.components.setFlipStyleRotateFirst` — check where the
C++ flag lives: `BasicBoard.h:80`).

**Steps.**
1. Port `Communication` additions + `BoardParserCallback` + `MinimalBoardManager`; unit-test MinimalBoardManager
   creates a board whose first item (id 1) is the `BoardOutline`.
2. Port `separate_holes` (uses `split_to_convex` of each outline; `contains_all_corners` checks every corner of
   the other shape against the convex pieces).
3. Port `create_board` step by step (layer validation, bounding box fallback from outline shapes, scale factor loop
   `while (5*maxCoor >= CRIT_INT) { scaleFactor /= 10; maxCoor /= 10; }` with **int** division for
   `scaleFactor`, new `CoordinateTransform(scaleFactor, 0, 0)`, `bounds = boundingBox.transformToBoard(t).offset(1000)`,
   zero-width `PolygonPath` replacement, `dimension() > 0` filter, outline-from-bounding-shape fallback, holes, rules
   via T109b `update_board_rules`, snap angle, communication, `board_handling->create_board(...)`, holes inserted as
   obstacles with clearance class 0 on every layer, `FixedState::SYSTEM_FIXED`).
   `scopeParameter.coordinateTransform` and `layerStructure` are (re)assigned inside — keep that order.
4. Port the readScope tail (keepouts in the order keepout → via_keepout → place_keepout; planes; missing power
   planes). The "apply autoroute settings to the routing job" block calls
   `RouterSettings::apply_new_values_from` — if that method does not exist yet (T301), add a minimal port of
   `RouterSettings.applyNewValuesFrom` (RouterSettings.java 979–1005) now and mark it in T301's spec as done.
5. Port `DsnFile::adjust_plane_autoroute_settings`.

**Java parity notes.**
- `insertKeepout`: a keepout with `dimension() < 2` is skipped with a warning and **returns true** (does not fail the
  read).
- Plane/keepout clearance class lookup: unknown class name → warning + `clearance_class_none()` (0).
- Plane on an unknown layer (`no < 0`) → `return false` (fails the read); keepout on unknown layer → false too.
- `Layer::signal()` keepouts are inserted on every signal layer. `Layer::pcb()` has `no == -1` (verified,
  parser/Layer.java:11), and Java tests `currentLayer == Layer.SIGNAL` first, then `currentLayer.no >= 0`, so a
  keepout on layer `pcb` fails the read with "Structure.insert_keepout: unknown layer name" — reproduce, and compare
  the layer by identity (`== Layer::signal()` shared_ptr), not by name.

**Tests** (`Structure_test.cpp` + `MinimalBoardManager_test.cpp`): full structure snippets with
`(layer F.Cu (type signal))(layer B.Cu (type signal))(boundary (rect pcb 0 0 1000 1000))` plus each of:
keepout on F.Cu, via_keepout on signal, place_keepout, plane GND on B.Cu (net GND auto-created in netlist and board
rules), power layer with `use_net VCC` and no plane (→ `insert_missing_power_planes` creates a conduction area over
the bounding box), boundary polygon with an inner hole polygon (→ one obstacle per layer), degenerate keepout
(skipped). Assert item counts per type, layers, clearance classes, `rules.get_nets().max_net_no()`, and the
coordinate transform scale. Compare against the Java harness (T007) output for the same snippet wrapped in a minimal
`(pcb …)`.

**Done when.** Tests green; `Structure::read_scope` returns a board for every `tests/data/*.dsn` structure scope (a
throwaway loop test, not committed, is fine).

**Watch out.** The board's item ids start in createBoard; any extra item inserted earlier in C++ shifts every id
and breaks T011/T118 order parity.

---

## T110 — Library part 1: padstacks  (S)

**Deps.** T109c (Library.readScope needs `board_handling->get_routing_board()`).

**Java** (`parser/Library.java`): `readPadstackScope` 101–224 (public static), `readScope` 262–307 (the loop:
`PADSTACK` → readPadstackScope, `IMAGE` → `Package.readScope` (**T111**), else skip). For T110, collect images by
calling `ScopeKeyword::skip_scope` and add a `// T111` marker.

**C++ target.** `parser/Library.{h,cpp,_test.cpp}`; `Keyword.cpp`: `const Library k_LIBRARY_SCOPE;`.

**Reuse.** `board::Padstacks::get(name)`, `Padstacks::add(name, shapes, attach_allowed, placed_absolute)` — check the
exact C++ signature (`Padstacks.h:27`); `BoardLibrary` via `RoutingBoard::library()`; parser `Shape::read_scope`
and `transform_to_board_rel`; `PolygonShape::convex_hull`, `split_to_convex`, `Simplex::simplify`,
`ConvexShape::offset`, `dimension()`; `ConvexTileShape` wrapper (01_MEMORY gotcha 35).

**Steps.** Port `readPadstackScope` verbatim:
padstack name with `"\\.\\d+"` removed; `(shape …)` → `Shape::read_scope`, then skip nested unknown scopes until the
closing bracket; `(attach on|off)` → drillable; `(absolute on|off)` → placed absolute; already-existing padstack →
return true; no shapes → warning, return true; per shape: non-convex → convex hull → split, warn if not exactly one
piece, simplify simplex; `dimension() < 2` → offset(1) once, else drop; `PCB`/`SIGNAL` layer → fill all layers,
else layer index (invalid → return false). `Library.readScope` replaces `board.library.padstacks` with a fresh
`Padstacks(board layer structure)` first.

**Tests.** circle pad on all layers; rect pad on F.Cu only (B.Cu shape null); polygon pad (concave → hull);
`(attach off)`; duplicate padstack name ignored; zero-size rect → offset(1) workaround; name `Round.5` stored as
`Round`. Assert per-layer bounding boxes against Java harness values.

**Done when.** Tests green; for the 8 E2E boards the padstack count equals the Java harness `counts.padstacks`
(temporary check via the board-count dump helper you add to the test, or wait for T117).

---

## T111 — Library part 2: images / packages, part library  (S)

**Java.** `parser/Package.java` 57–151 (`readScope`), 249–331 (`readPinInfo`), 332–354 (`readRotation`), 389–403
(`readPlacementSide`), 404–425 (`PinInfo`); `Library.readScope` 309–450 (package creation incl. `"::N"` de-duplication
and `arePackagePinsIdentical` 226–259, `generateMissingKeepoutNames` 454–472); `parser/PartLibrary.java` 84–301
(`readScope`, `readLogicalPartMapping`, `readLogicalPart`, `readPartPin`, the three record classes).

**C++ target.** `parser/Package.{h,cpp,_test.cpp}`, `parser/PartLibrary.{h,cpp,_test.cpp}`; extend `parser/Library.cpp`;
`ReadScopeParameter`: `logical_part_mappings`, `logical_parts`; `Keyword.cpp`: `const PartLibrary k_PART_LIBRARY_SCOPE;`.

**Reuse.** `board::Packages::add(name, pins, outlines, outline_widths, outline_is_closed, keepouts, via_keepouts,
place_keepouts, is_front)` and `Packages::get(name, is_front)` (Package.h:93–101); `Package::Pin`, `Package::Keepout`;
`Shape::transform_area_to_board_rel`.

**Java parity notes.** Package name base = name with `"::\\d+$"` removed; loop `suffix` until a free name or an
identical package (pin names, padstack ids, relative locations within 0.001, rotation within 0.001) is found. The
`catch (Exception)` fallback has no C++ equivalent — skip it with a comment. Outline closed-ness: `Path` with first
coord pair == last pair; non-path shapes are closed.

**Tests.** image with 2 pins + outline path + keepout without name (→ `keepout_1`); two identical images with the
same name → one package; two different images with the same name → `Name` and `Name::1`; back-side images
(Java adds front and back packages — verify with `Packages::get(name,false)`); part_library snippet with
`logical_part_mapping` and `logical_part` (no test DSN has these — snippets only).

---

## T112 — Placement: Component, ComponentPlacement, PlaceControl  (S)

**Java.** `parser/Component.java` 29–55 (static `readScope` → `ComponentPlacement`), 187–310 (`readPlaceScope`),
311–351 (`readItemClearanceInfo`), 352–367 (`readLockType`), 368–381 (instance `readScope` → append to
`placementList`); `parser/ComponentPlacement.java` (whole, 92); `parser/PlaceControl.java` 16–73
(`readFlipStyleRotateFirst` — already used by Structure; and `readScope`); `parser/Placement.java` has only
`writeScope` (T204b) — reading uses the generic ScopeKeyword loop which dispatches `(component …)` to
`COMPONENT_SCOPE`.

**C++ target.** `parser/Component.{h,cpp,_test.cpp}`, `parser/ComponentPlacement.h`, `parser/PlaceControl.{h,cpp}`;
`ReadScopeParameter::placement_list`; `Keyword.cpp`: real `Component` and `PlaceControl` instances
(`PLACE_CONTROL` keyword — check its name in Keyword.h). Keep `PLACEMENT_SCOPE` as a plain `ScopeKeyword("placement")`
(Java `Placement` has no `readScope` override).

**Java parity notes.** `ComponentLocation.coor == null` means "not placed" (Network skips pin insertion). Maps in
`ComponentLocation` are `TreeMap`/`HashMap`? Check Java `readPlaceScope`; use `std::map` if TreeMap, otherwise any
map (only lookups). Rotation is a double in degrees; side `front|back`; `(lock_type position)` → position fixed.

**Tests.** `(component "R0805" (place R1 1000 2000 front 90) (place R2 1500 2000 back 0 (lock_type position)))`
→ one ComponentPlacement with 2 locations; place with `(pin 1 (clearance_class x))`; place without coordinates.

---

## T125 — Parser scope + Circuit  (S)

**Java.** `parser/Parser.java` 18–52 (`readWriteSolution`), 53–91 (`readConstant`), 147–170 (`readQuoteChar`),
171–228 (`readScope`), **92–146 (`writeScope`, needed by SesWriter T201 — port it here)**; `parser/Circuit.java`
(whole, 143: `readScope` → `ReadScopeResult{maxLength, minLength, useVia, viaRule}`, `readLengthScope`).

**C++ target.** `parser/Parser.{h,cpp,_test.cpp}`, `parser/Circuit.{h,cpp,_test.cpp}`; `ReadScopeParameter`:
`constants`, `write_resolution`, `host_cad`/`host_version` already exist; `Keyword.cpp`: `const Parser k_PARSER_SCOPE;`.

**Reuse.** `IndentFileWriter`, `IdentifierType`, `Communication::SpecctraParserInfo` (extended in T109c).

**Java parity notes.** `(string_quote X)` changes `scopeParameter.stringQuote` **and** the scanner's quote char —
check how the C++ `SpecctraDsnStreamReader` exposes this (T102 notes say `string_quote` switching is ported).
`(generated_by_freeroute)` → `dsnFileGeneratedByHost = false`.

**Tests.** read `(parser (string_quote ') (host_cad "KiCad's Pcbnew") (host_version "(5.1.5)") (constant A 1)
(write_resolution D 1000))` → fields set, quote switched; write → text equal to Java's `Parser.writeScope` output for
the same info (golden from Java).

---

## T113 — Network part 1: net scopes and the Network skeleton  (S)

**Java** (`parser/Network.java`): `readScope` 1207–1271 (the loop) — port the whole loop now with the `VIA`,
`VIA_RULE`, `CLASS`, `CLASS_CLASS` branches calling stubs that T114a/T114b fill; `readNetScope` 1326–1464;
`readNetPins` 212–254; `createOrderedSubnets` 193–211.

**C++ target.** `parser/Network.{h,cpp,_test.cpp}`; `Keyword.cpp`: `const Network k_NETWORK_SCOPE;`.

**Reuse.** T126 `NetList`; `rules::Nets::add/get`; `rules::NetClasses::find(half_width, clearance_class, via_rule)`,
`BoardRules::get_new_net_class`, `NetClass::set_trace_half_width`, `Net::set_net_class`; `LayerStructure::contains_plane`;
`Rule::read_scope`; `IJFlexScanner::next_string()`.

**Java parity notes.** Subnet number defaults to 1; an integer token right after the net name sets it; each
`(fromto …)` creates a subnet, subnet numbers increase per subnet list; `(order …)` builds ordered 2-pin subnets via
`createOrderedSubnets`; `layer_rule` in a net → warning + skip; net-level `(rule (width …))` → find or create a net
class with that width and the default class' clearance class / via rule; other net rules → warning.

**Tests.** nets with pins; subnet number; fromto subnets; order; width rule creating a new net class; empty net
scope `(net X)`; net named `+5V`: 01_MEMORY gotcha 37 says the shipped Java lexer warns and skips illegal characters
(`+BATT` → `BATT`), so the expected name is whatever `build/dsn_token_dump` prints for `(net +5V` — it already
matches the Java lexer token-for-token; assert that value, do not "fix" it.

---

## T114a — Network part 2a: via infos, via rules, via padstacks  (S)

**Java** (`parser/Network.java`): `readViaInfo` 255–323, `readViaRule` 324–346, `insertViaInfos` 347–358,
`createDefaultViaInfos` 359–378, `insertViaRules` 379–397, `addViaRule` 398–422, and the via-padstack block in
`readScope` 1273–1314 (net-class `use_via` names appended to `viaPadstackNames`; names cleaned with `"\\.\\d+"`;
missing padstacks dropped with a warning; `board.library.setViaPadstacks`), then `insertViaInfos(…,
viaAtSmdAllowed)` and `insertViaRules` (1316–1318).

**Reuse.** `rules::ViaInfo`, `ViaInfos::add/get/name_exists`, `ViaRule::append_via`, `BoardRules::add_via_rule`,
`BoardRules::create_default_via_rule`, `BoardLibrary::set_via_padstacks`, `Padstacks::get(name)`.

**Tests.** `(via "Via[0-1]_600:300_um")` structure list + network `(via V1 "Via[0-1]_600:300_um" default)` +
`(via_rule default V1)`; no via info → default via infos created from the via padstacks; missing padstack name.

---

## T114b — Network part 2b: net classes and class pairs  (S)

**Java.** `parser/NetClass.java` (whole, 197: `readScope`, `readClassClassScope`, `ClassClass`, fields incl. `useVia`,
`viaRule`, `rules`, `layerRules`, `traceClearanceClass`, `shoveFixed`, `pullTight`, `minTraceLength`,
`maxTraceLength`, `useLayer`, Circuit data — uses T125 `Circuit`); `parser/Network.java` `insertNetClasses` 423–435,
`insertNetClass` 436–544, `insertClassPairs` 545–577, `insertClassPairInfo` 578–621, `addMixedClearanceRule`
622–680, `createDefaultClearanceClasses` 681–688, `createViaRule` 689–710, `createActiveTraceLayers` 711–729,
`addClearanceRule` 730–789, `getClearanceClass` 790–831. Called at readScope 1319–1320.

**Reuse.** `BoardRules::append_net_class(name)`, `NetClass::{set_trace_half_width(layer,w), set_trace_clearance_class,
set_via_rule, set_shove_fixed, set_pull_tight, set_minimum/maximum_trace_length, set_active_routing_layer}`,
`ClearanceMatrix` API (see T109b), `Nets::get_all_by_name`.

**Tests.** `(class PWR GND VCC (circuit (use_via V1)) (rule (width 0.5) (clearance 0.3)))` → class with width and a
new clearance class; `(class_class (classes PWR SIG) (rule (clearance 0.4)))` → mixed clearance rule;
`(class X N1 (circuit (use_layer F.Cu)))` → active layers; net in two classes (Java: last wins? verify).

**Watch out.** Clearance-class creation order changes class numbers, which feed every later clearance lookup —
compare `clearance_classes` names in order with the Java harness JSON.

---

## T115 — Network part 3: insert components, pins, keepouts, outlines, logical parts  (O)

**Java** (`parser/Network.java`): `insertComponents` 832–838, `insertComponent` 934–1204, `insertLogicalParts`
844–898, `searchLibPackage` 904–931; readScope calls 1321–1322. This is the code that fixes "all pins at the
component origin".

**Reuse.** `board::Components::add(name, location, rotation, is_front, front_pkg, back_pkg, position_fixed,
part_number)` (Component.h — check signature; T418 rewrote it Java-faithfully); `RoutingBoard::insert_pin(component_id,
pin_index, net_nos, clearance_class, fixed_state)`; `insert_obstacle/insert_via_obstacle/insert_component_obstacle`
overloads taking `(area, layer, translation, rotation, side_changed, clearance_class, component_id, name, fixed)`;
`insert_component_outline(area, is_front, translation, rotation, component_id, is_courtyard, is_fabrication,
is_closed, fixed)`; `NetList::get_nets(component, pin)`; `core::LogicalParts::add`, `Component::set_logical_part`.

**Java parity notes.** Missing front or back package → warning, return (component not added). Location rounding:
`dsnToBoard(coor).round()`. Unplaced component (`coor == null`) is still added to `components` but gets no pins.
Pin clearance class: explicit `pin_infos` → else SMD class if `padstack.fromLayer()==toLayer()` else PIN class, from
the **first net's** net class (or default class). Keepout layer on back side: `layerCount - layer - 1`; layer < 0 →
every signal layer. Keepout info clearance class used only if `> 0`. Courtyard detection: largest bounding-box area
among >1 outlines, or width == 0; fabrication = width ≤ 110 and not courtyard.

**Tests.** After a full `(pcb …)` snippet with structure + library + placement + network: pin positions =
component location + rotated pin offset (compare with Java harness for a 2-component board, one on the back side
rotated 90°); pin nets; SMD vs PIN clearance class; package keepout on the back side mapped to the mirrored layer.

**Done when.** `Issue026-J2_reference.dsn` loaded through the new reader (call the scope readers directly in the test
until T117) has pin count, component count and pin centres equal to the Java harness.

---

## T116 — Wiring read + `normalizeAllTraces`  (S)

**Java.** `parser/Wiring.java` read part: `readScope` 307–355, `readWireScope` 356–582, `tryCorrectNet` 583–600,
`readViaScope` 601–715, helpers `calcFixed` 257–280, `readNetId` 281–306, `getSubnets` 223–237, `viaExists` 238–256.
`board/facade/BasicBoard.java` `normalizeAllTraces` 799–≈890 (called at the end of `Wiring.readScope`;
`MAX_NORMALIZE_ITERATIONS = 2000`, line 64).

**C++ target.** `parser/Wiring.{h,cpp,_test.cpp}`; `BasicBoard::normalize_all_traces()` (or on RoutingBoard if the
helpers live there); `Keyword.cpp`: `const Wiring k_WIRING_SCOPE;`.

**Reuse.** `BasicBoard::insert_trace_without_cleaning` (BasicBoard.h:130), `RoutingBoard::insert_via`,
`insert_conduction_area`, `PolylineTrace::normalize(nullptr)`, `BasicBoard::remove_if_cycle`, `Item::is_user_fixed`,
`Padstacks::get(name)`, `Polyline::from_corners` (01_MEMORY gotcha 22).

**Java parity notes.**
- `normalizeAllTraces` groups traces by net in a **`java.util.HashMap<Integer, List>`** and iterates `entrySet()`.
  Java's iteration order for Integer keys is *not* plain ascending: it is ascending by bucket
  `key & (capacity-1)` (Integer hash = value; spread `h ^ (h>>>16)` is a no-op below 65536), entries in the same
  bucket in insertion order, where `capacity` is the final table size (16, doubled whenever size > 0.75·capacity).
  Add a small helper `java_hashmap_int_iteration_order(keys_in_insertion_order)` in `src/datastructures/` and use it,
  with a unit test (keys {1,17,2} with capacity 16 → 1,17,2; 13 keys → capacity 32).
- Items are collected walking the item list → in **descending id** order (T011).
- Wire on an unknown net → `tryCorrectNet`; wire with `(type protect)` / fixed states via `calcFixed`.
- Exceptions during normalize are caught and recorded as warnings (`scopeParameter.warnings`).

**Tests.** wire path on a net → one trace with the right corners and half width; via with padstack → via at
coordinates; conduction-area `(wire (poly …))`; unknown padstack → warning; `normalize_all_traces` merges two
collinear traces of one net. Use `Issue103-Board-Routed.dsn` as a smoke test: trace/via counts equal the Java harness.

---

## T117 — DsnReader orchestration, `--dump-board-counts`  (O)

**Java.** `io/specctra/DsnReader.java`: `readBoard` 58–161 (header check `(pcb <name>` with `yybegin(NAME)` after
`PCB_SCOPE`, `Keyword.PCB_SCOPE.readScope(par)`, `adjustPlaneAutorouteSettings` when no autoroute scope, result
variants), `readMetadata` 182–280; `io/BoardReadResult.java` (56), `io/BoardMetadata.java` (34);
`parser/DsnFile.java` scope helpers already ported.

**C++ target.** `src/designforms/specctra/DsnReader.{h,cpp,_test.cpp}` (namespace `…::specctra`), `BoardReadResult.h`
(a `std::variant<Success, OutlineMissing, ParseError, IoError>` with the same fields), `BoardMetadata.h`;
`src/Freerouting.cpp`: new flag `--dump-board-counts <dsn> [out.json]` that prints the T007 JSON schema (implement
the dump in `tools/`-independent code, e.g. `src/designforms/specctra/BoardCountsDump.{h,cpp}` so a test can call it).

**Steps.** Port both functions; design-name resolution identical; the stream is read through
`SpecctraDsnStreamReader(std::istream&)`. The dump iterates `get_items()` (descending id after T011) and writes the
same fields in the same order as `BoardCounts.java`.

**Tests.** `DsnReader_test`: empty_board → Success, 1 outline; file not starting with `(` → ParseError
`"Not a Specctra DSN file: expected '(pcb <name>' header"`; boundary-less DSN → OutlineMissing; `readMetadata` stops
after structure (pass a stream that is truncated after `(structure …)` and still get Success).

**Done when.** `build/freerouting --dump-board-counts tests/data/X.dsn` matches `tests/data/expected/X.counts.json`
(T007) for the 8 E2E boards, ignoring `warnings`; record the comparison table in MIGRATION_designforms.md.

---

## T118 — Reader audit on all 117 DSNs  (S)

**Steps.** Write `tools/compare_board_counts.py <expected.json> <actual.json>` (stdlib): compares `result`, layers,
`counts`, `clearance_classes`, `clearance_matrix`, and `items` element by element (id, type, layers, nets, fixed,
component, pin, corners); prints the first difference per file. Loop over all DSNs. File one task per distinct root
cause (`T118a`, …), not per file.

**Done when.** All files match or each mismatch has a task; summary table in MIGRATION_designforms.md.

---

## T124a — HeadlessBoardManager core + delete the legacy reader  (S, careful)

**Deps.** T117, T202.

**Java.** `management/HeadlessBoardManager.java`: ctor 172, `getRoutingBoard` 259, `replaceRoutingBoard` 280,
`createBoard` 314–350 (without the four `apply*Override` calls — T124b), `getCurrentRoutingJob` 615,
`loadFromSpecctraDsn` 718–750, `applyParsedBoardResult` 756–782 (without `applyRouterSettingsForLoadedBoard`/
post-load — T124b/c), `saveAsSpecctraSessionSes` 922–945. `management/BoardManager.java` (170) is the interface.

**C++ target.** `src/interactive/HeadlessBoardManager.{h,cpp,_test.cpp}` (replace the uncompiled stub; drop the
GUI-ish members of `interactive/BoardManager.h` such as locale/settings, or give `HeadlessBoardManager` its own
narrow interface). Delete: `specctra/{Scanner,Structure,Library,Network,Placement,Wiring,Keyword,Circle,Polygon}.*`,
`specctra/Path.h`, `specctra/DsnFile.h` (header-only stub), `Wiring_test.cpp`, `src/board/BoardLoader.*` (+test),
`src/board/BoardBuilder.*`, `RoutingBoard::create_pins_from_parsed` (RoutingBoard.cpp ≈275–452), `get_pin`,
`get_pin_count`, `get/set_parsed_result`, `parsed_result_`, the `Scanner.h` include in RoutingBoard.h. Update
`src/designforms/CMakeLists.txt`, `tests/CMakeLists.txt`. Switch callers: `src/Freerouting.cpp` (BoardLoader at
≈258/317, `get_parsed_result().board_name` ≈276, `SessionWriter` ≈277), `src/autoroute/BatchAutorouter.cpp:186`
(`get_pin_count` → `get_pins().size()` or remove the log), `src/core/RoutingJob.cpp` (`job.board = board.release()`
leak → shared ownership).

**Done when.** `grep -rn "BoardLoader\|create_pins_from_parsed\|parsed_result\|specctra/Scanner.h" src tests` is
empty; build + ctest green; `tools/e2e_all.sh` runs (numbers may change — they should improve).

---

## T124b — HeadlessBoardManager settings overrides after load  (S)

**Deps.** T124a, T301, T307.

**Java.** `applyRouterSettingsForLoadedBoard` 784–796 (`setLayerCount`, `applyBoardSpecificOptimizations`, then the
four overrides), `applyHoleClearanceOverride` 352–409, `assignHoleKeepoutClearanceClass` 410–471,
`applyCopperToEdgeClearanceOverride` 472–559, `applyPlaneNetsOverride` 560–585, `applyPlaneAsObstacleOverride`
586–614; also called from `createBoard` 346–349.

**Why it matters.** These change clearances and plane handling from `router.*` settings (defaults in
`settings/sources/DefaultSettings.java`, e.g. `DEFAULT_HOLE_CLEARANCE_UM = 0.0`) — they alter routing results.

**Tests.** With default settings each override is a no-op (assert rules unchanged); with
`hole_clearance_um = 200` the board rules' hole clearance equals the converted value and a keepout clearance class is
assigned like Java (golden from a throwaway Java run).

---

## T124c — Post-load processing  (S)

**Deps.** T124b.

**Java.** `applyImmediatePostLoadProcessing` 798–808 → `RoutingBoard.reduceNetsOfRouteItems()` (board/facade/
RoutingBoard.java ≈1310), `validatePowerPlanes` 997–1107 (+ `conductionAreasOverlap` 962–982,
`getConductionAreaNetNames` 983–996), `validateBoardDesignErrors` 1168–1198; deferred part 810–844: run it
**synchronously** in C++ but keep only what affects routing: `preExistingClearanceViolationsCount` (via
`drc::DesignRulesChecker::get_all_clearance_violations`) and `warnPreExistingClearanceViolations` 1108–1166. Skip
analytics, `compareCounterpartBoardIfPresent` (debug) and the CRC (needs T204b; add a TODO pointing at T204b).

**Tests.** A board with a pin carrying two nets where one net is unused → `reduce_nets_of_route_items` drops it
(golden from Java); overlapping planes of different nets → warning text equal to Java.
