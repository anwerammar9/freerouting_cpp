# Reusable Migration Agent Guide

## Overview
This guide provides a comprehensive framework for AI agents to migrate Java code to C++20 in the freerouting project. It ensures consistency, quality, and reproducibility across different agents and migration sessions.

## Agent Setup and Initialization

### 1. Environment Validation
Before starting any migration, validate the environment:

```bash
# Required validation steps
1. Check .clangd configuration exists and is valid
2. Verify CMakeLists.txt is properly configured
3. Ensure build directory exists and is clean
4. Validate all required templates are available
5. Check git status is clean (or commit changes)
```

### 2. Required Reading
Agents MUST read these documents in order before migration:

1. `MIGRATION_RULES.md` - Core migration standards
2. `FILE_CONVENTIONS.md` - File organization rules
3. `CPP20_PATTERNS.md` - C++20 specific patterns
4. `MIGRATION_CHECKLIST.md` - Validation checklist

### 3. Tool Validation
Ensure all required tools are available:

```bash
# Check required tools
which clangd        # Language server
which cmake         # Build system
which clang-tidy    # Static analysis
which valgrind      # Memory checking (optional)
which git          # Version control
```

## Migration Workflow

### Phase 1: Analysis and Planning

#### Step 1.1: Identify Target Class
```bash
# Get migration target from user or migration_status.csv
# Example: AutorouteAttemptState
```

#### Step 1.2: Locate Java Source
```bash
# Find Java source file
find freerouting_java -name "TargetClass.java" -type f
# Expected: freerouting_java/src/main/java/app/freerouting/package/TargetClass.java
```

#### Step 1.3: Analyze Dependencies
```bash
# Extract import statements from Java file
grep "import" TargetClass.java > imports.txt

# Map Java imports to C++ includes
# app.freerouting.geometry.planar.Point -> geometry/planar/Point.h
# app.freerouting.board.Board -> board/Board.h
```

#### Step 1.4: Check Migration Status
```bash
# Check if dependencies are already migrated
grep -E "(Point|Board)" migration_status.csv | grep "Fully Migrated"
```

### Phase 2: File Creation

#### Step 2.1: Create Directory Structure
```bash
# Ensure target directory exists
mkdir -p src/package/path/
cd src/package/path/
```

#### Step 2.2: Copy Java Reference
```bash
# Copy Java source as reference
cp ../../../../freerouting_java/src/main/java/app/freerouting/package/TargetClass.java .
```

#### Step 2.3: Create Header File
```bash
# Create header from template
cp ../../../templates/ClassName.h TargetClass.h
```

#### Step 2.4: Create Implementation File
```bash
# Create implementation from template  
cp ../../../templates/ClassName.cpp TargetClass.cpp
```

#### Step 2.5: Create Test File
```bash
# Create test from template
cp ../../../templates/ClassName_test.cpp TargetClass_test.cpp
```

### Phase 3: Implementation

#### Step 3.1: Header Implementation
1. **Read Java source** thoroughly
2. **Translate class declaration** to C++
3. **Map Java types** to C++ equivalents
4. **Add proper includes** based on dependencies
5. **Apply C++20 patterns** from CPP20_PATTERNS.md

#### Step 3.2: Implementation Details
1. **Translate constructors** using member initializer lists
2. **Convert methods** following naming conventions
3. **Handle exceptions** with modern C++ error handling
4. **Apply RAII** for resource management
5. **Use smart pointers** for ownership

#### Step 3.3: Test Implementation
1. **Create test fixture** for class setup
2. **Write tests for all public methods**
3. **Add edge case tests**
4. **Include performance tests** if applicable
5. **Add integration tests** with dependencies

### Phase 4: Validation

#### Step 4.1: Build Validation
```bash
# Update CMakeLists.txt
# Add new files to appropriate targets

# Build project
cd build
make -j4

# Fix any compilation errors
# Repeat until build succeeds
```

#### Step 4.2: Test Validation
```bash
# Run unit tests
./tests/freerouting_test --gtest_filter="TargetClass.*"

# Verify 80%+ coverage
./tests/freerouting_test --gtest_filter="TargetClass.*" --gtest_print_time
```

#### Step 4.3: Quality Validation
```bash
# Run static analysis
clang-tidy src/package/path/TargetClass.cpp --config-file=.clangd

# Check for memory leaks (if applicable)
valgrind ./tests/freerouting_test --gtest_filter="TargetClass.*"
```

## Agent Decision Tree

### When to Proceed with Migration
```
START
│
├─ Are all 4 required files present? ── NO ── Create missing files
│
├─ Are dependencies already migrated? ── NO ── Migrate dependencies first
│
├─ Does Java code compile in original project? ── NO ── Fix Java issues first
│
├─ Can you map all Java types to C++? ── NO ── Research C++ equivalents
│
└─ PROCEED with migration
```

### When to Seek Help
```
├─ Circular dependencies detected
├─ Java patterns without clear C++ equivalent
├─ Performance requirements unclear
├─ Complex generic types
└─ Platform-specific code
```

### Error Resolution Flowchart
```
COMPILATION ERROR
│
├─ Missing include? ── Add proper include
│
├─ Type mismatch? ── Check type mapping
│
├─ Linker error? ── Update CMakeLists.txt
│
├─ Template error? ── Check template parameters
│
└─ Seek assistance if unresolved after 3 attempts
```

## Code Translation Patterns

### Java to C++ Quick Reference

| Java Pattern | C++20 Equivalent |
|--------------|------------------|
| `public class Name` | `class Name` |
| `private final Type field` | `const Type field_;` |
| `public Type method()` | `Type method() const;` |
| `ArrayList<Type>` | `std::vector<Type>` |
| `HashMap<K,V>` | `std::unordered_map<K,V>` |
| `throws Exception` | `std::expected<Type, ErrorCode>` |
| `null` | `std::nullopt` or `nullptr` |
| `@Override` | `override` keyword |
| `interface I` | `class I` with pure virtual methods |

### Common Translation Templates

#### 1. Simple Class
```java
// Java
public class Point {
    private final int x, y;
    
    public Point(int x, int y) {
        this.x = x;
        this.y = y;
    }
    
    public int getX() { return x; }
    public int getY() { return y; }
}
```

```cpp
// C++20
// Point.h
#pragma once
#include <memory>

class Point {
public:
    Point(int x, int y);
    
    int get_x() const { return x_; }
    int get_y() const { return y_; }

private:
    const int x_;
    const int y_;
};
```

#### 2. Class with Methods
```java
// Java
public class Calculator {
    private int value;
    
    public Calculator(int initialValue) {
        this.value = initialValue;
    }
    
    public int add(int amount) {
        return value += amount;
    }
    
    public int getValue() {
        return value;
    }
}
```

```cpp
// C++20
// Calculator.h
#pragma once

class Calculator {
public:
    explicit Calculator(int initial_value);
    
    int add(int amount);
    int get_value() const;

private:
    int value_;
};

// Calculator.cpp
Calculator::Calculator(int initial_value) 
    : value_(initial_value) {}

int Calculator::add(int amount) {
    return value_ += amount;
}

int Calculator::get_value() const {
    return value_;
}
```

## Quality Assurance Checklist

### Pre-Commit Validation
- [ ] All 4 files created with correct names
- [ ] Code compiles without errors
- [ ] All tests pass
- [ ] Code coverage ≥ 80%
- [ ] Static analysis passes
- [ ] Documentation complete
- [ ] Performance acceptable

### Code Review Points
- [ ] Naming conventions followed
- [ ] Smart pointers used correctly
- [ ] RAII principles applied
- [ ] Error handling appropriate
- [ ] C++20 features used effectively
- [ ] Thread safety considered
- [ ] Memory management correct

## Automation Scripts

### Migration Validation Script
```bash
#!/bin/bash
# validate_migration_complete.sh

set -e

CLASS_NAME="$1"
PACKAGE_PATH="$2"

echo "Validating complete migration for $CLASS_NAME"

# File structure check
files=("$PACKAGE_PATH/$CLASS_NAME.h" 
       "$PACKAGE_PATH/$CLASS_NAME.cpp"
       "$PACKAGE_PATH/${CLASS_NAME}_test.cpp"
       "$PACKAGE_PATH/$CLASS_NAME.java")

for file in "${files[@]}"; do
    if [[ ! -f "$file" ]]; then
        echo "❌ Missing: $file"
        exit 1
    fi
done

# Build check
cd build
if ! make -j4 2>/dev/null; then
    echo "❌ Build failed"
    exit 1
fi

# Test check
if ! ./tests/freerouting_test --gtest_filter="$CLASS_NAME.*" 2>/dev/null; then
    echo "❌ Tests failed"
    exit 1
fi

# Style check
if ! clang-tidy "$PACKAGE_PATH/$CLASS_NAME.cpp" --config-file=../.clangd 2>/dev/null; then
    echo "⚠️ Style issues - review required"
fi

echo "✅ Migration validation complete"
```

### Dependency Checker
```bash
#!/bin/bash
# check_dependencies.sh

CLASS_NAME="$1"

# Extract Java imports
java_file="freerouting_java/src/main/java/app/freerouting/**/*/$CLASS_NAME.java"
imports=$(grep "import" "$java_file" | sed 's/import //g' | sed 's/;//g')

# Check migration status
for import in $imports; do
    class=$(basename "$import")
    status=$(grep "$class" migration_status.csv | cut -d',' -f6)
    echo "$class: $status"
done
```

## Troubleshooting Guide

### Common Issues and Solutions

#### Issue: Circular Dependencies
**Symptoms**: Compilation errors with missing symbols
**Solution**: 
1. Use forward declarations in headers
2. Move implementations to .cpp files
3. Consider redesigning class relationships

#### Issue: Template Instantiation Errors
**Symptoms**: Long template error messages
**Solution**:
1. Check template parameter requirements
2. Verify concepts/constraints
3. Simplify template code

#### Issue: Memory Leaks
**Symptoms**: Valgrind reports lost memory
**Solution**:
1. Use smart pointers consistently
2. Follow Rule of Three/Five
3. Check RAII compliance

#### Issue: Performance Regression
**Symptoms**: Slower than Java implementation
**Solution**:
1. Profile with perf/valgrind
2. Check for unnecessary copies
3. Optimize algorithms and data structures

## Best Practices for Agents

### 1. Consistency Over Creativity
- Follow established patterns exactly
- Use provided templates without deviation
- Maintain naming conventions strictly

### 2. Quality First Approach
- Never skip testing phases
- Address all compiler warnings
- Document any deviations from standards

### 3. Incremental Progress
- Commit working code frequently
- Validate each step before proceeding
- Roll back to last working state if needed

### 4. Communication Standards
- Document decisions and reasoning
- Note any Java patterns that were difficult to translate
- Report any missing dependencies or blockers

### 5. Continuous Learning
- Update templates based on experience
- Improve this guide with new patterns
- Share successful migration strategies

## Success Metrics

A migration is considered successful when:

1. **Functional**: All tests pass and behavior matches Java version
2. **Performant**: Performance meets or exceeds Java baseline
3. **Maintainable**: Code follows all established patterns and conventions
4. **Complete**: All 4 required files present and properly structured
5. **Integrated**: Builds and works with existing C++ codebase

This guide ensures that any agent can successfully migrate Java code to C++20 while maintaining consistency and quality across the entire freerouting project.
