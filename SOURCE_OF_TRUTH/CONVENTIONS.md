# Freerouting C++ Migration Conventions

## File Structure

Every migrated Java class produces exactly 4 files:
```
src/<module>/
├── ClassName.h          # Header declaration
├── ClassName.cpp        # Implementation
├── ClassName_test.cpp   # Unit tests (GTest)
└── ClassName.java       # Original Java reference
```

## Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Classes | PascalCase | `MazeSearchAlgo` |
| Functions | snake_case | `find_connection()` |
| Variables | snake_case | `curr_net_no` |
| Constants | kPascalCase | `kMaxPasses` |
| Private members | trailing underscore | `board_` |
| Static members | s_ prefix | `s_instance` |
| Enums | PascalCase values | `NOT_ROUTED` |
| Files | Match class name | `MazeSearchAlgo.h` |

## C++20 Patterns

### Smart Pointers
```cpp
// Ownership transfer
std::unique_ptr<Object> create_object();
// Shared ownership
std::shared_ptr<Config> config = std::make_shared<Config>();
// Non-owning view
Object* observer = owned_object.get();
```

### RAII
```cpp
class FileProcessor {
public:
    FileProcessor(const std::string& path) : file_(path) {}
    ~FileProcessor() = default;  // File automatically closed
private:
    std::ifstream file_;
};
```

### std::optional
```cpp
std::optional<int> try_parse(const std::string& str);
```

### Constructor Style
```cpp
Example::Example(std::string name)
    : name_(std::move(name))
    , count_(0) {}
```

## Java → C++ Translation

| Java | C++ |
|------|-----|
| `public class Name` | `class Name` |
| `private final Type field` | `const Type field_;` |
| `public Type method()` | `Type method() const;` |
| `ArrayList<Type>` | `std::vector<Type>` |
| `HashMap<K,V>` | `std::unordered_map<K,V>` |
| `throws Exception` | `std::expected<T, ErrorCode>` or `std::optional<T>` |
| `null` | `std::nullopt` or `nullptr` |
| `@Override` | `override` |
| `interface I` | `class I` with pure virtual methods |
| `instanceof` | `dynamic_cast<>` |
| `String` | `std::string` |

## Testing Standards

### Test File Structure
```cpp
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
```

### Test Registration
Tests go in `tests/CMakeLists.txt`:
```cmake
add_executable(ClassName_test src/module/ClassName_test.cpp)
target_link_libraries(ClassName_test
    GTest::gtest_main
    freerouting_<module>
    <other_deps>
)
gtest_discover_tests(ClassName_test)
```

## Build Commands

```bash
# Configure
conan install . --build=missing -of build
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DCMAKE_PREFIX_PATH=build/build/Release/generators

# Build specific module
cmake --build build --target freerouting_board -j$(nproc)

# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
ctest --test-dir build -R ClassName --output-on-failure

# Run assert-based tests
cmake --build build --target run_tests -j$(nproc)
./build/tests/run_tests
```

## Module Library Targets

| Module | CMake Target | Dependencies |
|--------|-------------|-------------|
| Core | `freerouting_core` | (none — leaf) |
| DataStructures | `freerouting_datastructures` | core |
| Logger | `freerouting_logger` | core |
| Constants | `freerouting_constants` | (none) |
| Geometry | `freerouting_geometry` | datastructures, core, Boost |
| Rules | `freerouting_rules` | core, board, geometry |
| Board | `freerouting_board` | core, rules, geometry |
| Autoroute | `freerouting_autoroute` | core, board |
| Interactive | `freerouting_interactive` | core, board |
| Settings | `freerouting_settings` | autoroute |
| Management | `freerouting_management` | logger, core |
| DesignForms | `freerouting_designforms` | logger, board, geometry |
| DRC | `freerouting_drc` | board, settings |
