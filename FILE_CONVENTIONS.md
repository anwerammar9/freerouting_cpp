# File Naming and Organization Conventions

## Directory Structure

### Source Organization
```
freerouting_cpp/
├── src/                    # All source files (no separate include/ directory)
│   ├── core/              # Core functionality
│   ├── geometry/planar/   # Geometry classes
│   ├── board/             # Board management
│   ├── autoroute/         # Autorouting algorithms
│   ├── rules/             # Design rules
│   ├── settings/          # Configuration
│   ├── interactive/       # Interactive features
│   ├── designforms/specctra/ # Specctra file format
│   ├── drc/               # Design rule checking
│   ├── datastructures/    # Data structures
│   └── logger/            # Logging system
├── templates/             # Migration templates
├── tests/                 # Integration tests
├── build/                 # Build output
└── docs/                  # Documentation
```

## File Naming Rules

### Required Files Per Class
Every migrated Java class must have exactly 4 files:

```
src/package/path/
├── ClassName.h          # Header declaration
├── ClassName.cpp        # Implementation  
├── ClassName_test.cpp   # Unit tests
└── ClassName.java       # Original Java reference
```

### Naming Patterns

#### 1. Class Files
- **Header**: `PascalCase.h` (same as Java class name)
- **Implementation**: `PascalCase.cpp`
- **Tests**: `PascalCase_test.cpp`
- **Java Reference**: `PascalCase.java`

#### 2. Examples
| Java Class | C++ Header | C++ Implementation | C++ Tests |
|------------|------------|-------------------|-----------|
| `AutorouteAttemptState` | `AutorouteAttemptState.h` | `AutorouteAttemptState.cpp` | `AutorouteAttemptState_test.cpp` |
| `IntPoint` | `IntPoint.h` | `IntPoint.cpp` | `IntPoint_test.cpp` |
| `BoardManager` | `BoardManager.h` | `BoardManager.cpp` | `BoardManager_test.cpp` |

#### 3. Package Mapping
Java packages map directly to directory structures:

```
Java: app.freerouting.geometry.planar.IntPoint
C++:  src/geometry/planar/IntPoint.h/.cpp/_test.cpp/.java

Java: app.freerouting.autoroute.BatchAutorouterThread  
C++:  src/autoroute/BatchAutorouterThread.h/.cpp/_test.cpp/.java
```

## File Content Standards

### Header File Structure (.h)
```cpp
#pragma once

// System includes first
#include <memory>
#include <string>
#include <vector>

// Local includes
#include "other/Required.h"

namespace freerouting {
namespace package {

/**
 * @brief Brief description
 * 
 * Detailed description of class purpose and usage.
 */
class ClassName {
public:
    // Constructors
    ClassName();
    explicit ClassName(ParameterType param);
    
    // Public methods
    ReturnType method_name(ParameterType param);
    
    // Accessors
    const ValueType& get_value() const;
    void set_value(const ValueType& value);
    
private:
    // Private members with trailing underscore
    ValueType member_;
    
    // Private methods
    void helper_method();
};

} // namespace package
} // namespace freerouting
```

### Implementation File Structure (.cpp)
```cpp
#include "ClassName.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <stdexcept>

namespace freerouting {
namespace package {

ClassName::ClassName() 
    : member_(default_value) {
    logger::FRLogger::instance().info("Creating ClassName");
}

ClassName::ClassName(ParameterType param) 
    : member_(param) {
    logger::FRLogger::instance().info("Creating ClassName with parameter");
}

ReturnType ClassName::method_name(ParameterType param) {
    // Implementation
    return result;
}

} // namespace package
} // namespace freerouting
```

### Test File Structure (_test.cpp)
```cpp
#include "ClassName.h"
#include <gtest/gtest.h>
#include <memory>

namespace freerouting {
namespace package {

class ClassNameTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_object_ = std::make_unique<ClassName>();
    }
    
    std::unique_ptr<ClassName> test_object_;
};

TEST_F(ClassNameTest, BasicFunctionality) {
    EXPECT_NE(test_object_, nullptr);
}

} // namespace package
} // namespace freerouting
```

### Java Reference File (.java)
```java
// Original Java file - keep as reference
package app.freerouting.package;

public class ClassName {
    // Original Java implementation
}
```

## Special Cases

### 1. Multiple Classes in One File
If a Java file contains multiple classes, create separate C++ files for each:

```java
// Java: Shape.java contains Shape, Circle, Rectangle
// C++: Create separate files for each
src/geometry/planar/
├── Shape.h/.cpp/_test.cpp/.java
├── Circle.h/.cpp/_test.cpp/.java  
└── Rectangle.h/.cpp/_test.cpp/.java
```

### 2. Inner Classes
Java inner classes become separate C++ classes with prefixed names:

```java
// Java
public class Outer {
    public static class Inner {
        // ...
    }
}

// C++
src/package/
├── Outer.h/.cpp/_test.cpp/.java
└── Outer_Inner.h/.cpp/_test.cpp/.java
```

### 3. Enums
Java enums become C++ enum classes:

```java
// Java
public enum Direction { UP, DOWN, LEFT, RIGHT }

// C++
// Direction.h
enum class Direction { UP, DOWN, LEFT, RIGHT };
```

## Build System Integration

### CMakeLists.txt Updates
Each new class must be added to the appropriate target:

```cmake
# Add header to include directories (if needed)
target_include_directories(freerouting_package PUBLIC 
    ${CMAKE_SOURCE_DIR}/src
)

# Add implementation to library
target_sources(freerouting_package PRIVATE
    src/package/ClassName.cpp
    src/package/ClassName_test.cpp  # For test builds
)
```

### Include Path Management
- **No separate include/ directory** - all headers in src/
- **Relative includes** use src-relative paths
- **Forward slashes** for all paths (cross-platform)

```cpp
// Correct include
#include "geometry/planar/Point.h"
#include "autoroute/AutorouteAttemptState.h"

// Incorrect (no include/ directory)
#include "include/geometry/planar/Point.h"
```

## Migration Validation Script

### Automated File Structure Check
```bash
#!/bin/bash
# validate_file_structure.sh

validate_class_files() {
    local class_name=$1
    local package_path=$2
    
    local required_files=(
        "$package_path/$class_name.h"
        "$package_path/$class_name.cpp" 
        "$package_path/$class_name_test.cpp"
        "$package_path/$class_name.java"
    )
    
    for file in "${required_files[@]}"; do
        if [[ ! -f "$file" ]]; then
            echo "ERROR: Missing required file: $file"
            return 1
        fi
    done
    
    echo "✓ All required files present for $class_name"
    return 0
}
```

## Documentation Requirements

### Header Documentation
- **@brief**: Required for all classes and public methods
- **@param**: Required for all parameters  
- **@return**: Required for all non-void methods
- **@example**: Recommended for complex classes

### Implementation Comments
- **Algorithm explanation**: For complex logic
- **Performance notes**: For expensive operations
- **Thread safety**: For concurrent access

### Test Documentation
- **Test purpose**: Each test should document what it verifies
- **Edge cases**: Document boundary conditions being tested
- **Performance expectations**: Document expected performance

## Compliance Checklist

For each migrated class, verify:

### File Structure
- [ ] All 4 required files exist
- [ ] Files are in correct directory structure
- [ ] Naming conventions followed exactly

### Code Quality  
- [ ] Header has proper include guards
- [ ] Implementation includes corresponding header
- [ ] Test file includes all necessary dependencies
- [ ] No compilation warnings

### Documentation
- [ ] Class has @brief description
- [ ] Public methods documented
- [ ] Complex algorithms explained
- [ ] Performance characteristics noted

### Testing
- [ ] Test coverage ≥ 80%
- [ ] All public methods tested
- [ ] Edge cases covered
- [ ] Error conditions tested

This file organization system ensures consistency across the entire migration and makes the codebase maintainable for future developers.
