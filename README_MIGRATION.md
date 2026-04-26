# Freerouting Java to C++20 Migration Framework

## Overview
This repository contains a comprehensive framework for migrating the freerouting Java codebase to modern C++20. The framework ensures consistency, quality, and maintainability across the entire migration process.

## Framework Components

### 📋 Core Documentation
- **`MIGRATION_RULES.md`** - Complete migration standards and C++20 patterns
- **`FILE_CONVENTIONS.md`** - File naming and organization conventions  
- **`CPP20_PATTERNS.md`** - C++20 specific patterns and modernizations
- **`MIGRATION_CHECKLIST.md`** - Comprehensive validation checklist
- **`AGENT_GUIDE.md`** - Reusable guide for AI migration agents

### 🔧 Development Environment
- **`.clangd`** - Language server configuration for C++20
- **`templates/`** - Reusable templates for all file types
- **Automated validation scripts** - Quality assurance automation

### 📁 Required File Structure
Every migrated Java class requires exactly 4 files:
```
src/package/path/
├── ClassName.h          # Header declaration
├── ClassName.cpp        # Implementation  
├── ClassName_test.cpp   # Unit tests
└── ClassName.java       # Original Java reference
```

## Quick Start for Migration Agents

### 1. Environment Setup
```bash
# Verify tools are available
which clangd cmake clang-tidy git

# Check .clangd configuration
cat .clangd

# Validate build system
cd build && cmake .. && make -j4
```

### 2. Migration Workflow
1. **Read** all framework documentation (mandatory)
2. **Analyze** Java source and dependencies
3. **Create** 4 required files using templates
4. **Implement** following C++20 patterns
5. **Validate** build, tests, and quality
6. **Commit** with descriptive message

### 3. Quality Gates
- ✅ All 4 files present with correct naming
- ✅ Code compiles without errors
- ✅ Tests pass with ≥80% coverage
- ✅ Static analysis passes
- ✅ Performance meets Java baseline

## C++20 Features Utilized

### Modern Language Features
- **Smart pointers** for memory management
- **RAII** for resource management
- **std::optional** for nullable returns
- **std::span** for array views
- **Concepts** for template constraints
- **constexpr** for compile-time computation
- **Three-way comparison** for operators

### Performance Optimizations
- **Move semantics** to avoid copies
- **Reserve/emplace** for efficient containers
- **Stack allocation** preference
- **STL algorithms** over manual loops
- **Compile-time** optimizations

## Migration Status

### ✅ Completed Infrastructure
- Migration framework and standards
- Development environment configuration
- Templates and automation scripts
- Git repository with version control

### ✅ Sample Migrations
- **Point.cpp** - Core geometry foundation
- **RoutingJob.cpp** - Core routing functionality
- **Components.cpp** - Board structure management
- **DrillItem.cpp** - Core board elements

### 🔄 In Progress
- Remaining partially migrated files
- Autoroute algorithm implementations
- Board manipulation algorithms

## File Organization

```
freerouting_cpp/
├── src/                    # All source files (no separate include/)
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
├── docs/                  # This documentation
├── build/                 # Build output
└── tests/                 # Integration tests
```

## Quality Assurance

### Automated Validation
```bash
# Run complete validation
./validate_migration_complete.sh ClassName package/path

# Check dependencies
./check_dependencies.sh ClassName

# Build and test
cd build && make -j4 && ./tests/freerouting_test
```

### Manual Review Checklist
- [ ] Naming conventions followed exactly
- [ ] C++20 features used appropriately
- [ ] Memory management correct (RAII)
- [ ] Error handling modernized
- [ ] Performance optimized
- [ ] Documentation complete

## Performance Targets

### Benchmarks
- **Compilation time**: < 2 minutes for full project
- **Test execution**: < 30 seconds for full test suite
- **Memory usage**: ≤ Java baseline + 20%
- **Runtime performance**: ≥ Java baseline

### Memory Management
- **No raw pointers** for ownership
- **RAII compliance** for all resources
- **Smart pointers** for dynamic allocation
- **Stack allocation** preference

## Testing Strategy

### Test Types
- **Unit tests**: Individual class functionality
- **Integration tests**: Class interactions
- **Performance tests**: Benchmark validation
- **Memory tests**: Leak detection

### Coverage Requirements
- **Minimum**: 80% line coverage
- **Target**: 90% line coverage
- **Critical paths**: 100% coverage

## Contributing Guidelines

### For AI Agents
1. **Read all documentation** before starting
2. **Follow patterns exactly** as specified
3. **Validate thoroughly** before committing
4. **Document deviations** from standards
5. **Update framework** with improvements

### For Human Developers
1. **Review AI migrations** for quality
2. **Test performance** against Java baseline
3. **Update documentation** with lessons learned
4. **Optimize critical paths** as needed

## Next Steps

### Immediate Actions
1. **Test framework** with complex class migration
2. **Create automation scripts** for validation
3. **Begin systematic migration** of remaining classes

### Long-term Goals
1. **Complete migration** of all Java classes
2. **Performance optimization** beyond Java baseline
3. **Additional features** enabled by C++20
4. **Documentation** for end users

## Support and Troubleshooting

### Common Issues
- **Circular dependencies**: Use forward declarations
- **Template errors**: Check concepts and constraints
- **Memory leaks**: Verify RAII compliance
- **Performance issues**: Profile and optimize

### Getting Help
1. **Check framework documentation** first
2. **Review similar successful migrations**
3. **Consult troubleshooting guide**
4. **Document new patterns** for future reference

This migration framework provides a solid foundation for converting the entire freerouting codebase to modern C++20 while ensuring high quality, performance, and maintainability.
