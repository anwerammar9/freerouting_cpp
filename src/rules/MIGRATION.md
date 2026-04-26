# Migration Report: src/rules/

## Summary

| File Type | Count |
|-----------|-------|
| `.cpp`     | 9     |
| `.h`       | 9     |
| `.java`    | 10    |

## Migration Status

### Mapped (7/10 = 70%)
| Java File      | CPP File  | H File   |
|---------------|----------|----------|
| BoardRules.java | BoardRules.cpp | BoardRules.h |
| ClearanceMatrix.java | ClearanceMatrix.cpp | ClearanceMatrix.h |
| DefaultItemClearanceClasses.java | DefaultItemClearanceClasses.cpp | DefaultItemClearanceClasses.h |
| Net.java | Net.cpp | Net.h |
| NetClass.java | NetClass.cpp | NetClass.h |
| ViaInfo.java | ViaInfo.cpp | ViaInfo.h |
| ViaRule.java | ViaRule.cpp | ViaRule.h |

### Reference Only (3/10 = 30%)
These are Java collection/helper classes (no C++ equivalent - use std::vector, std::list, etc.)

| Java File | Notes |
|----------|-------|
| NetClasses.java | Java collection class |
| Nets.java | Java collection class |
| ViaInfos.java | Java collection class |

## Migration Percentage

- **Core Classes**: 7/7 = 100% migrated
- **Total**: 7/10 = 70% migrated