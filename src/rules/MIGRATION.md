# Migration Report: src/rules/

## Summary

| File Type | Count |
|-----------|-------|
| `.cpp`     | 9     |
| `.h`       | 9     |
| `.java`    | 10    |

## Migration Status

### Mapped (10/10 = 100%)
| Java File      | CPP File  | H File   |
|---------------|----------|----------|
| BoardRules.java | BoardRules.cpp | BoardRules.h |
| ClearanceMatrix.java | ClearanceMatrix.cpp | ClearanceMatrix.h |
| DefaultItemClearanceClasses.java | DefaultItemClearanceClasses.cpp | DefaultItemClearanceClasses.h |
| Net.java | Net.cpp | Net.h |
| NetClass.java | NetClass.cpp | NetClass.h |
| NetClasses.java | BoardRules.cpp | BoardRules.h |
| Nets.java | Net.cpp | Net.h |
| ViaInfo.java | ViaInfo.cpp | ViaInfo.h |
| ViaInfos.java | ViaInfo.cpp | ViaInfo.h |
| ViaRule.java | ViaRule.cpp | ViaRule.h |

## Migration Percentage

- **Core Classes**: 10/10 = 100% migrated

## Notes

- `NetClasses`, `Nets` and `ViaInfos` are collection wrapper classes, kept as C++ classes (not replaced by raw `std::vector<T>`) to preserve the Java API (append/find/remove/get by name/name generation, 1-based net numbers).
- `ViaRule` now stores the padstack-based `rules::ViaInfo` from `ViaInfo.h`; the previous duplicate drill/layer-based `rules::ViaInfo` in `ViaRule.h` was removed.
- Pending: `ViaRule::get_layer_range()` and any method needing complete padstack internals, plus locale-based (`TextManager`) naming.