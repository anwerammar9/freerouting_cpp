# Java to C++ Migration Rules & Standards

## Overview
This document defines the comprehensive migration framework for converting Java freerouting codebase to modern C++20. All migrations must follow these rules to ensure consistency, performance, and maintainability.

## File Organization Standards

### Required File Structure
For each Java class migrated, create exactly 4 files in the same directory:

```
src/package/path/
├── ClassName.h          # Header declaration
├── ClassName.cpp        # Implementation  
├── ClassName_test.cpp   # Unit tests
└── ClassName.java       # Original Java reference (copy from freerouting_java/)
```

### Example: AutorouteAttemptState
```
src/autoroute/
├── AutorouteAttemptState.h
├── AutorouteAttemptState.cpp  
├── AutorouteAttemptState_test.cpp
└── AutorouteAttemptState.java
```

## C++20 Standards

### Language Features to Use
- **Smart Pointers**: `std::unique_ptr`, `std::shared_ptr` instead of raw pointers
- **RAII**: All resources managed by constructors/destructors
- **constexpr**: Compile-time computations where applicable
- **std::span**: For array views instead of raw pointers
- **std::optional**: For nullable return types
- **std::variant**: For union-like types
- **Concepts**: For template constraints (if needed)
- **std::format**: For string formatting (C++23 if available)

### Modern C++ Patterns
```cpp
// Use smart pointers
class Example {
public:
    Example(std::unique_ptr<Dependency> dep);
private:
    std::unique_ptr<Dependency> dependency_;
};

// Use RAII for resource management
class FileManager {
public:
    FileManager(const std::string& path) : file_(path) {}
private:
    std::ifstream file_;
};

// Use std::optional for nullable returns
std::optional<int> try_parse(const std::string& str);
```

## Java to C++ Translation Rules

### 1. Class Structure Translation

#### Java Class
```java
public class Example implements Serializable {
    private final String name;
    private int count;
    
    public Example(String name) {
        this.name = name;
        this.count = 0;
    }
    
    public String getName() { return name; }
    public int getCount() { return count; }
    public void increment() { count++; }
}
```

#### C++20 Equivalent
```cpp
// Example.h
#pragma once
#include <string>
#include <memory>

class Example {
public:
    explicit Example(std::string name);
    
    const std::string& get_name() const { return name_; }
    int get_count() const { return count_; }
    void increment() { count_++; }
    
private:
    const std::string name_;
    int count_ = 0;
};
```

### 2. Exception Handling
- **Java**: `try-catch` with checked/unchecked exceptions
- **C++20**: Use `std::expected` (C++23) or `std::optional` + error codes

```cpp
// Instead of throwing exceptions
std::expected<Result, ErrorCode> calculate();

// Or use optional for simple cases
std::optional<Value> try_get_value();
```

### 3. Collections Translation
| Java Collection | C++20 Equivalent |
|----------------|------------------|
| `ArrayList<T>` | `std::vector<T>` |
| `LinkedList<T>` | `std::list<T>` |
| `HashMap<K,V>` | `std::unordered_map<K,V>` |
| `HashSet<T>` | `std::unordered_set<T>` |
| `TreeMap<K,V>` | `std::map<K,V>` |

### 4. String Handling
```cpp
// Java: String str = "hello";
// C++20: std::string str = "hello";

// Use std::string_view for read-only string parameters
void process_name(std::string_view name);
```

### 5. Interface Translation
```java
// Java Interface
public interface Drawable {
    void draw(Graphics g);
}
```

```cpp
// C++20 Abstract Class
class Drawable {
public:
    virtual ~Drawable() = default;
    virtual void draw(Graphics& g) = 0;
};
```

## Naming Conventions

### C++ Naming Standards
- **Classes**: `PascalCase` (same as Java)
- **Functions**: `snake_case` (Java camelCase → C++ snake_case)
- **Variables**: `snake_case` 
- **Constants**: `kPascalCase` or `ALL_CAPS`
- **Private members**: trailing underscore `member_`

```cpp
class ExampleClass {
public:
    void calculate_result();  // snake_case function
    
private:
    int count_;               // trailing underscore
    static const int kMaxSize = 100;  // kPascal constant
};
```

## Memory Management Rules

### 1. Smart Pointer Usage
```cpp
// Ownership transfer
std::unique_ptr<Object> create_object();

// Shared ownership
std::shared_ptr<Config> config = std::make_shared<Config>();

// No ownership (non-owning view)
Object* observer = owned_object.get();  // Only for observing
```

### 2. Resource Management
```cpp
// RAII - Automatic cleanup
class FileProcessor {
public:
    FileProcessor(const std::string& path) : file_(path) {}
    ~FileProcessor() = default;  // File automatically closed
    
private:
    std::ifstream file_;
};
```

## Unit Testing Standards

### Test File Structure
```cpp
// ClassName_test.cpp
#include "ClassName.h"
#include <gtest/gtest.h>
#include <memory>

class ClassNameTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_object_ = std::make_unique<ClassName>(test_param_);
    }
    
    std::unique_ptr<ClassName> test_object_;
    const int test_param_ = 42;
};

TEST_F(ClassNameTest, BasicFunctionality) {
    EXPECT_EQ(test_object_->get_value(), 42);
}

TEST_F(ClassNameTest, EdgeCases) {
    // Test edge cases
}
```

### Testing Requirements
- **Coverage**: Minimum 80% line coverage
- **Test Types**: Unit tests, integration tests, performance tests
- **Mocking**: Use Google Mock for dependencies
- **Fixtures**: Use test fixtures for common setup

## Build System Integration

### CMakeLists.txt Updates
Each migrated class must be added to appropriate CMake targets:

```cmake
# Add to library
target_sources(freerouting_core PRIVATE
    src/package/ClassName.cpp
    src/package/ClassName_test.cpp  # For testing
)

# Include directories
target_include_directories(freerouting_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)
```

## Migration Checklist

### Pre-Migration
- [ ] Java file analyzed for dependencies
- [ ] Header structure designed
- [ ] Test cases identified
- [ ] Performance requirements documented

### Migration Process
- [ ] Header file created with proper includes
- [ ] Class structure translated to C++20
- [ ] Memory management converted to RAII
- [ ] Exception handling modernized
- [ ] Collections converted to STL equivalents
- [ ] Unit tests written
- [ ] Build system updated

### Post-Migration Validation
- [ ] Compiles without warnings
- [ ] All tests pass
- [ ] Memory leak check (valgrind)
- [ ] Performance benchmark
- [ ] Code review completed
- [ ] Documentation updated

## Performance Guidelines

### Optimization Rules
1. **Prefer stack allocation** over heap when possible
2. **Use move semantics** for large objects
3. **Reserve capacity** for vectors when size known
4. **Avoid unnecessary copies** with const references
5. **Use constexpr** for compile-time computations

```cpp
// Good: Stack allocation
std::vector<int> local_data(1000);

// Good: Move semantics
std::vector<int> create_large_vector() {
    std::vector<int> result(1000000);
    // ... fill data
    return result;  // Move semantics
}

// Good: Reserve capacity
std::vector<int> data;
data.reserve(known_size);
```

## Error Handling Strategy

### Hierarchy of Error Handling
1. **Compile-time**: Use static_assert, concepts
2. **Logic errors**: Use assert() for debugging
3. **Runtime errors**: Use std::expected/std::optional
4. **Exceptional cases**: Use std::exception sparingly

```cpp
// Compile-time checking
static_assert(sizeof(int) == 4, "Int must be 32 bits");

// Logic errors
assert(ptr != nullptr && "Pointer cannot be null");

// Runtime errors
std::expected<int, ErrorCode> calculate();

// Exceptional cases (use sparingly)
throw std::runtime_error("Critical system error");
```

## Documentation Standards

### Header Documentation
```cpp
/**
 * @brief Brief description of the class
 * 
 * Detailed description of the class purpose and usage.
 * Include performance characteristics and thread safety.
 * 
 * @example
 * auto obj = ClassName(param);
 * obj.method();
 */
class ClassName {
    // ...
};
```

### Code Comments
- **Public API**: Always document purpose, parameters, return values
- **Complex algorithms**: Explain approach and complexity
- **Performance notes**: Mark expensive operations
- **Thread safety**: Document concurrent access guarantees

## Tools Configuration

### .clangd Configuration
```yaml
# .clangd configuration for C++20
CompileFlags:
  Add: [-std=c++20, -Wall, -Wextra, -Werror]
Diagnostics:
  UnusedIncludes: Strict
  ClangTidy:
    Add: [modernize-*, readability-*, performance-*]
    Remove: [modernize-use-trailing-return-type]
```

### VS Code Settings
```json
{
    "C_Cpp.default.cppStandard": "c++20",
    "C_Cpp.default.cStandard": "c11",
    "clangd.arguments": ["--header-insertion=never"]
}
```

## Migration Agent Guidelines

### For AI Agents Following This Plan
1. **Always read this document first** before starting migration
2. **Create all 4 required files** for each migration
3. **Follow naming conventions** exactly as specified
4. **Write comprehensive tests** before considering migration complete
5. **Update build system** and ensure compilation
6. **Document any deviations** from this standard
7. **Update this document** if new patterns emerge

### Validation Script
Create automated validation to ensure standards compliance:
```bash
#!/bin/bash
# validate_migration.sh
check_file_structure() {
    # Verify .h, .cpp, _test.cpp, .java exist
    # Verify naming conventions
    # Verify C++20 standards
}
```

This migration framework ensures consistent, high-quality C++20 code that maintains the functionality of the original Java codebase while leveraging modern C++ features for better performance and maintainability.
