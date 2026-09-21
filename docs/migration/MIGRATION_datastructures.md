# DataStructures Module - Migration Status

**Module**: `src/datastructures/`
**CMake Target**: `freerouting_datastructures`
**Java Files**: 14 | **C++ Headers**: 14 | **C++ Impls**: 11 | **Tests**: 9

---

## Completed (Fully Migrated)

| Java Class | C++ Header | C++ Impl | Test | Java Lines | Notes |
|------------|-----------|----------|------|-----------|-------|
| Signum | Signum.h | Signum.cpp | - | 69 | Sign function utility |
| UndoableObjects | UndoableObjects.h | UndoableObjects.cpp | UndoableObjects_test | 338 | Undo system; full port with restore lists in this session |
| Stoppable | Stoppable.h | Stoppable.cpp | Stoppable_test | 17 | Stop request flag |
| TimeLimit | TimeLimit.h | TimeLimit.cpp | TimeLimit_test | 40 | Time based stop limit |
| ShapeTree | ShapeTree.h | ShapeTree.cpp | datastructures_shape_tree_test | 254 | Abstract shape search tree; ported in this session |
| MinAreaTree | MinAreaTree.h | MinAreaTree.cpp | datastructures_shape_tree_test | 180 | Min area binary search tree; ported in this session |
| BigIntAux | BigIntAux.h | Header-only | BigIntAux_test | 113 | Determinant, rational coordinate addition, and unsigned binary GCD |
| IdentifierType | IdentifierType.h | IdentifierType.cpp | IdentifierType_test | 91 | Identifier quoting and reserved-character handling |
| IndentFileWriter | IndentFileWriter.h | IndentFileWriter.cpp | IndentFileWriter_test | 70 | Indented parenthesized-scope output |
| FileFilter | FileFilter.h | FileFilter.cpp | FileFilter_test | N/A | Case-insensitive filename extension filter |
| Observers | Observers.h | Header-only | Observers_test | N/A | Generic observer notification interface |
| PlanarDelaunayTriangulation | PlanarDelaunayTriangulation.h | PlanarDelaunayTriangulation.cpp | PlanarDelaunayTriangulation_test | 975 | Deterministic Bowyer-Watson triangulation |

| ArrayStack | ArrayStack.h | Header-only | ArrayStack_test | 57 | Header-only template with tested dynamic growth |
| IdentificationNumberGenerator | IdentificationNumberGenerator.h | IdentificationNumberGenerator.cpp | IdentificationNumberGenerator_test | 17 | Interface with compiled atomic-counter implementation |

## Not Migrated

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|

## Summary

- **Done**: 14 classes (100% of Java files)
- **Partial**: 0 classes (0%)
- **Remaining**: 0 classes (0%)
- **Blockers**: ShapeTree/MinAreaTree were the datastructures prerequisite for board spatial indexing; board::ShapeSearchTree (extends MinAreaTree) and SearchTreeObject (implements ShapeTree.Storable) are the next consumers.

## This Session Notes

- Ported `ShapeTree` (abstract) and `MinAreaTree` implementing the contract for board's `ShapeSearchTree`.
- Java's `ArrayStack<TreeNode> node_stack` (capacity 10000) is replaced by a plain `std::vector<TreeNode*>` in `MinAreaTree` because the existing `ArrayStack` port had an unchecked first-push (see Partially Migrated). The overlap walk itself is identical.
- Ownership model: the tree transfers ownership of its `Leaf` objects to the `Storable` via `set_search_tree_entries(std::vector<std::unique_ptr<Leaf>>, ...)` (mirroring Java, where the storable stores the `Leaf[]` array), while `ShapeTree` owns its `InnerNode`s in a registry (`all_nodes_`) and frees them on `remove_leaf`. Removed leaves are unlinked from the tree but stay alive under the storable (Java parity).
- Geometry gap filled to make the tree work: added polymorphic `intersects(const RegularTileShape&)` to `RegularTileShape`/`IntBox`/`IntOctagon`, and `TileShape::bounding_shape(const ShapeBoundingDirections&)` implemented on `IntBox`/`IntOctagon`/`Simplex` via `ShapeBoundingDirections::bounds`. (`ConvexShape::bounding_shape` already existed but the C++ tile hierarchy does not inherit `ConvexShape`.)
- C++ module cycle note: the Java packages datastructures and geometry reference each other (geometry uses `datastructures/Signum`; datastructures' trees use geometry tiles). Because includes are global and static-library symbols are resolved at the final executable link (where both libraries are linked explicitly), no declared CMake dependency edge exists between the two targets.