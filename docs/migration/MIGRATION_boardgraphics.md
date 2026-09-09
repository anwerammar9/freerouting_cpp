# BoardGraphics Module - Migration Status

**Module**: `src/boardgraphics/`
**CMake Target**: N/A (commented out in CMakeLists.txt)
**Java Files**: 8 | **C++ Headers**: 0 | **C++ Impls**: 0 | **Tests**: 0

---

## Completed (Fully Migrated)

| Java Class | C++ Header | C++ Impl | Test | Java Lines | Notes |
|------------|-----------|----------|------|-----------|-------|
| _(none)_ | - | - | - | - | - |

## Not Migrated

| Java Class | Java Lines | Priority | Dependencies / Notes |
|------------|-----------|----------|---------------------|
| ColorIntensityTable | 57 | LOW | GUI color mapping |
| ColorTableModel | 56 | LOW | GUI table model |
| CoordinateTransform | 234 | LOW | Graphics-specific transform |
| Drawable | 39 | LOW | Drawable interface |
| GraphicsContext | 678 | LOW | Graphics rendering context |
| ItemColorTableModel | 245 | LOW | Item color table model |
| OtherColorTableModel | 128 | LOW | Other color table model |

## Summary

- **Done**: 0 classes (0%)
- **Remaining**: 8 classes (100%)
- **Blockers**: Entire module is GUI-dependent. Out of scope for headless routing. Will be addressed when GUI integration is planned.
- **Note**: This module is excluded from the CMake build (commented out). All classes depend on Java Swing/AWT. C++ equivalent would require a GUI framework decision (Qt, ImGui, etc.).
