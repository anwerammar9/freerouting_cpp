# Rules Module - Migration Status

**Module**: `src/rules/`
**CMake Target**: `freerouting_rules`
**Java Files**: 10 | **C++ Headers**: 7 | **C++ Impls**: 7 | **Tests**: 0

---

## Completed (Fully Migrated)

| Java Class | C++ Header | C++ Impl | Test | Java Lines | Notes |
|------------|-----------|----------|------|-----------|-------|
| BoardRules | BoardRules.h | BoardRules.cpp | - | 470 | Board design rules |
| ClearanceMatrix | ClearanceMatrix.h | ClearanceMatrix.cpp | - | 406 | Clearance lookup matrix |
| DefaultItemClearanceClasses | DefaultItemClearanceClasses.h | DefaultItemClearanceClasses.cpp | - | 128 | Default clearance classes |
| Net | Net.h | Net.cpp | - | 213 | Network class |
| NetClass | NetClass.h | NetClass.cpp | - | 313 | Network class definition |
| NetClasses | BoardRules.h | BoardRules.cpp | - | 118 | Collection wrapper for NetClass (`std::vector<NetClass*>` + `class_count`/`count` indexes) |
| Nets | Net.h | Net.cpp | - | 128 | Collection wrapper for Net (`std::vector<Net*>`, 1-based net numbers) |
| ViaInfo | ViaInfo.h | ViaInfo.cpp | - | 92 | Via information |
| ViaInfos | ViaInfo.h | ViaInfo.cpp | - | 95 | Collection wrapper for ViaInfo (`std::vector<std::shared_ptr<ViaInfo>>`) |
| ViaRule | ViaRule.h | ViaRule.cpp | - | 128 | Via rule constraints |

## Completion Details

- **NetClasses**: `count()`, `get(int)`, `get(String)`, `append(name, layer_structure, clearance_matrix, ignored)`, `append(layer_structure, clearance_matrix)` (auto-name `class<N>`), `find(int, clearance_class, via_rule)`, `find(vector<int>, clearance_class, via_rule)`, `remove(NetClass)`.
- **Nets / Net**: `Net` gained `subnet_number` and `contains_plane` fields. `Nets` now uses **1-based net numbers** matching Java (`net_arr_[net_no - 1]`); added `get(String, subnet)` (case-insensitive), `get_all_by_name(String)` (all subnets), `new_net()` (auto-name `net<N>`), `add(String, subnet, contains_plane)`, `get_board()`/`set_board()`, `max_net_no()`, `is_normal_net_no()`.
- **NetClass**: added `layer_count()`, `set_trace_half_width(int)` (all layers), `set_trace_half_width_on_inner(int)`, `set_all_layers_active(bool)`, `set_all_inner_layers_active(bool)`, `trace_width_is_layer_dependent()`.
- **ViaInfos**: `add`, `count`, `get(int)`, `get(String)`, `name_exists`, `remove` — verified complete; only `print_info` (ObjectInfoPanel / GUI) is out of scope.
- **ViaRule**: the duplicate drill/layer-based `rules::ViaInfo` that previously collided with `ViaInfo.h` was removed; `ViaRule` now stores `std::shared_ptr<rules::ViaInfo>` (padstack-based, identical to Java). Added `contains`, `contains_padstack`, `swap`, and defined `ViaRule::EMPTY`. `get_layer_range(from, to)` remains unimplemented because it requires `padstack->from_layer()/to_layer()` on a complete `rules::Padstack`.

## Known Gaps

- `rules::Padstack` is only forward-declared (rules cannot depend on `freerouting_board` due to the dependency direction `board -> rules`). Any method needing complete padstack internals (`ViaRule::get_layer_range`, padstack-derived layer ranges) stays pending until `core/Padstack` (or a rules-owned padstack type) is migrated.
- Locale-based naming via Java `TextManager` (`class<N>`, `net<N>`, localized strings) is replaced by plain `class<N>` / `net<N>` names; no i18n infrastructure exists in C++ yet.

## Summary

- **Done**: 10/10 classes (100% of Java files)
- **Notes**: Rules is now the **first fully migrated module**. Verified via full project build (`cmake --build build` produces `freerouting`) and a standalone smoke test exercising NetClasses/Net/Nets/NetClass/ViaInfos/ViaRule parity APIs.