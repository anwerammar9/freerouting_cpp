# C++20 Specific Patterns and Modernizations

## Overview
This document outlines C++20 specific patterns and modernizations to be applied during the Java to C++ migration, ensuring the codebase leverages modern C++ features for better performance, safety, and maintainability.

## Core C++20 Features to Utilize

### 1. Concepts and Constraints
Use concepts for template constraints instead of SFINAE:

```cpp
// Instead of complex SFINAE
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
void process_integer(T value);

// Use concepts
template<std::integral T>
void process_integer(T value);

// Custom concepts
template<typename T>
concept PointLike = requires(T t) {
    { t.x() } -> std::convertible_to<int>;
    { t.y() } -> std::convertible_to<int>;
};

template<PointLike P>
double distance(const P& p1, const P& p2);
```

### 2. std::span for Array Views
Replace raw pointers and size pairs with std::span:

```cpp
// Java: void processPoints(Point[] points, int count)
// Old C++: void process_points(Point* points, size_t count)
// Modern C++20:
void process_points(std::span<Point> points);

// Usage
std::vector<Point> vec = { /* ... */ };
process_points(vec);  // Implicit conversion

Point arr[10];
process_points(arr);  // Works with arrays
```

### 3. std::expected for Error Handling
Replace exceptions with std::expected (C++23) or custom implementation:

```cpp
// Java: throws IllegalArgumentException
// C++20 alternative:
std::expected<int, ErrorCode> calculate_value(int input);

// Usage
auto result = calculate_value(42);
if (!result) {
    // Handle error
    logger::error("Calculation failed: {}", result.error());
    return;
}
int value = result.value();
```

### 4. constexpr and consteval
Use compile-time computation where possible:

```cpp
// Compile-time geometry calculations
constexpr IntPoint origin(0, 0);
constexpr double PI = 3.14159265358979323846;

// Compile-time validation
consteval bool validate_layer_count(int count) {
    return count > 0 && count <= 32;
}

template<int LayerCount>
requires(validate_layer_count(LayerCount))
class LayerStack {
    // ...
};
```

### 5. Three-Way Comparison (Spaceship Operator)
Implement comparison operators efficiently:

```cpp
class Point {
public:
    // Auto-generate all comparison operators
    auto operator<=>(const Point& other) const = default;
    
    // Or custom implementation
    std::strong_ordering operator<=>(const Point& other) const {
        if (auto cmp = x <=> other.x; cmp != 0) return cmp;
        return y <=> other.y;
    }
};
```

### 6. Designated Initializers
Use for clear struct initialization:

```cpp
struct RoutingSettings {
    int max_attempts = 100;
    double tolerance = 0.01;
    bool use_advanced_algorithm = false;
};

// Clear initialization
RoutingSettings settings{
    .max_attempts = 200,
    .tolerance = 0.005
};
```

## Memory Management Patterns

### 1. Smart Pointer Usage Guidelines

```cpp
// Ownership semantics
class Board {
public:
    // Unique ownership - Board owns the components
    void add_component(std::unique_ptr<Component> comp);
    
    // Shared ownership - Multiple references
    std::shared_ptr<Layer> get_layer(int index) const;
    
    // Non-owning observation
    const Component* find_component(const std::string& name) const;

private:
    std::vector<std::unique_ptr<Component>> components_;
    std::vector<std::shared_ptr<Layer>> layers_;
};
```

### 2. RAII for Resource Management

```cpp
// File handling with RAII
class DsnFileReader {
public:
    explicit DsnFileReader(const std::string& path) 
        : file_(path) {
        if (!file_.is_open()) {
            throw std::runtime_error("Cannot open file: " + path);
        }
    }
    
    // File automatically closed when destructor runs
    ~DsnFileReader() = default;
    
    // Move-only type
    DsnFileReader(const DsnFileReader&) = delete;
    DsnFileReader& operator=(const DsnFileReader&) = delete;
    DsnFileReader(DsnFileReader&&) = default;
    DsnFileReader& operator=(DsnFileReader&&) = default;

private:
    std::ifstream file_;
};
```

### 3. Custom Deleters for Special Cases

```cpp
// Custom deleter for C-style resources
struct FileDeleter {
    void operator()(FILE* file) const {
        if (file) fclose(file);
    }
};

using UniqueFile = std::unique_ptr<FILE, FileDeleter>;

UniqueFile open_file(const std::string& path) {
    FILE* file = fopen(path.c_str(), "r");
    return UniqueFile(file);
}
```

## Performance-Oriented Patterns

### 1. Move Semantics and Perfect Forwarding

```cpp
// Efficient string handling
class Component {
public:
    // Perfect forwarding for constructors
    template<typename Name>
    explicit Component(Name&& name) 
        : name_(std::forward<Name>(name)) {}
    
    // Move operations
    Component(Component&& other) noexcept
        : name_(std::move(other.name_)) {}
    
    Component& operator=(Component&& other) noexcept {
        if (this != &other) {
            name_ = std::move(other.name_);
        }
        return *this;
    }

private:
    std::string name_;
};
```

### 2. Reserve and Emplace Patterns

```cpp
// Efficient container operations
class GeometryProcessor {
public:
    void add_points(const std::vector<Point>& points) {
        // Reserve space to avoid reallocations
        processed_points_.reserve(processed_points_.size() + points.size());
        
        // Use emplace_back to construct in-place
        for (const auto& point : points) {
            processed_points_.emplace_back(process_point(point));
        }
    }

private:
    std::vector<Point> processed_points_;
};
```

### 3. Compile-Time Optimizations

```cpp
// Template specialization for different coordinate types
template<typename T>
class PointT {
public:
    constexpr PointT(T x, T y) : x_(x), y_(y) {}
    
    constexpr T distance_squared(const PointT& other) const {
        const T dx = x_ - other.x_;
        const T dy = y_ - other.y_;
        return dx * dx + dy * dy;
    }

private:
    T x_, y_;
};

// Specialized for integer coordinates
template<>
constexpr int PointT<int>::distance_squared(const PointT<int>& other) const {
    // Use integer arithmetic to avoid floating point
    const int dx = x_ - other.x_;
    const int dy = y_ - other.y_;
    return dx * dx + dy * dy;
}
```

## Error Handling Patterns

### 1. std::optional for Nullable Returns

```cpp
// Replace null returns with optional
class BoardSearcher {
public:
    std::optional<Component*> find_component(const std::string& name) {
        auto it = component_map_.find(name);
        if (it != component_map_.end()) {
            return it->second.get();
        }
        return std::nullopt;
    }
    
    // Usage
    void process_component(const std::string& name) {
        if (auto comp = find_component(name)) {
            comp.value()->process();
        } else {
            logger::warn("Component not found: {}", name);
        }
    }
};
```

### 2. Result Types for Operations

```cpp
// Custom result type for operations that can fail
template<typename T, typename E = ErrorCode>
class Result {
public:
    static Result success(T value) {
        return Result(std::move(value));
    }
    
    static Result failure(E error) {
        return Result(std::unexpected(error));
    }
    
    bool is_success() const { return value_.has_value(); }
    bool is_failure() const { return !value_.has_value(); }
    
    const T& value() const { return value_.value(); }
    T& value() { return value_.value(); }
    
    E error() const { return error_; }

private:
    Result(T value) : value_(std::move(value)) {}
    Result(std::unexpected<E> error) : error_(error.value()) {}
    
    std::optional<T> value_;
    E error_;
};
```

## Concurrency Patterns

### 1. std::atomic for Thread Safety

```cpp
class ThreadSafeCounter {
public:
    void increment() { counter_.fetch_add(1, std::memory_order_relaxed); }
    
    int get() const { return counter_.load(std::memory_order_acquire); }
    
    void reset() { counter_.store(0, std::memory_order_release); }

private:
    std::atomic<int> counter_{0};
};
```

### 2. std::mutex and std::lock_guard

```cpp
class ThreadSafeBoard {
public:
    void add_component(std::unique_ptr<Component> comp) {
        std::lock_guard<std::mutex> lock(mutex_);
        components_.push_back(std::move(comp));
    }
    
    std::vector<Component*> get_components() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<Component*> result;
        result.reserve(components_.size());
        for (const auto& comp : components_) {
            result.push_back(comp.get());
        }
        return result;
    }

private:
    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<Component>> components_;
};
```

## String Handling Patterns

### 1. std::string_view for Read-Only Strings

```cpp
class Logger {
public:
    // Efficient string handling for logging
    void info(std::string_view message) const {
        log_message(LogLevel::Info, message);
    }
    
    void info(std::string_view format, std::string_view arg) const {
        // Efficient formatting without allocations
        std::string message = std::format("{}: {}", format, arg);
        log_message(LogLevel::Info, message);
    }

private:
    void log_message(LogLevel level, std::string_view message) const;
};
```

### 2. std::format for String Formatting

```cpp
// Modern string formatting (C++20)
class Component {
public:
    std::string to_string() const {
        return std::format("Component(name={}, x={}, y={})", 
                          name_, x_, y_);
    }
    
    void debug_print() const {
        logger::debug("Component details: {}", to_string());
    }

private:
    std::string name_;
    int x_, y_;
};
```

## Algorithm Patterns

### 1. STL Algorithms instead of Manual Loops

```cpp
class GeometryUtils {
public:
    // Use STL algorithms
    static std::vector<Point> filter_points(
        const std::vector<Point>& points,
        const auto& predicate) {
        
        std::vector<Point> result;
        std::copy_if(points.begin(), points.end(), 
                    std::back_inserter(result), predicate);
        return result;
    }
    
    // Use ranges for more expressive code
    static double total_distance(const std::vector<Point>& points) {
        if (points.size() < 2) return 0.0;
        
        auto distances = points | 
            std::views::adjacent_transform([](const Point& a, const Point& b) {
                return a.distance(b);
            });
        
        return std::accumulate(distances.begin(), distances.end(), 0.0);
    }
};
```

### 2. Lambda Expressions for Custom Operations

```cpp
class RoutingOptimizer {
public:
    void optimize_routes() {
        // Use lambdas for complex operations
        auto by_length = [](const auto& a, const auto& b) {
            return a.length() < b.length();
        };
        
        std::sort(routes_.begin(), routes_.end(), by_length);
        
        // Remove routes that are too long
        routes_.erase(
            std::remove_if(routes_.begin(), routes_.end(),
                [](const Route& route) { 
                    return route.length() > max_length_; 
                }),
            routes_.end()
        );
    }

private:
    std::vector<Route> routes_;
    double max_length_;
};
```

## Template Metaprogramming Patterns

### 1. Type Traits and SFINAE

```cpp
// Custom type traits
template<typename T>
struct is_point_like : std::false_type {};

template<>
struct is_point_like<IntPoint> : std::true_type {};

template<>
struct is_point_like<FloatPoint> : std::true_type {};

template<typename T>
constexpr bool is_point_like_v = is_point_like<T>::value;

// Conditional compilation
template<typename Point>
auto calculate_distance(const Point& p1, const Point& p2)
    -> std::enable_if_t<is_point_like_v<Point>, double> {
    return p1.distance(p2);
}
```

### 2. Template Specialization for Performance

```cpp
// Generic template
template<typename T>
class Calculator {
public:
    static T add(T a, T b) { return a + b; }
};

// Specialization for integer overflow checking
template<>
class Calculator<int> {
public:
    static std::optional<int> add(int a, int b) {
        if ((b > 0 && a > INT_MAX - b) || 
            (b < 0 && a < INT_MIN - b)) {
            return std::nullopt;  // Overflow
        }
        return a + b;
    }
};
```

## Migration Guidelines

### When to Use Modern C++ Features

1. **Always Use**: Smart pointers, RAII, const correctness
2. **Prefer**: std::optional, std::span, std::string_view
3. **Use When Appropriate**: Concepts, ranges, constexpr
4. **Consider Carefully**: Complex template metaprogramming

### Performance Considerations

1. **Profile First**: Optimize based on actual measurements
2. **Prefer Stack**: Use stack allocation when possible
3. **Minimize Allocations**: Reserve capacity, use emplace
4. **Consider Cache Locality**: Data layout matters

### Maintainability Guidelines

1. **Clear Over Clever**: Readability beats cleverness
2. **Consistent Style**: Follow established patterns
3. **Document Complexities**: Explain non-obvious code
4. **Test Thoroughly**: Modern features need good tests

This C++20 patterns guide ensures that migrated code leverages modern C++ features effectively while maintaining clarity and performance.
