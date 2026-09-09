/**
 * @file NamedAlgorithmType_test.cpp
 * @brief Unit tests for NamedAlgorithmType enum class
 */

#include "autoroute/NamedAlgorithmType.h"
#include <gtest/gtest.h>

namespace freerouting {
namespace autoroute {

class NamedAlgorithmTypeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
};

// Test basic enum functionality
TEST_F(NamedAlgorithmTypeTest, BasicEnumValues) {
    EXPECT_EQ(static_cast<int>(NamedAlgorithmType::ROUTER), 0);
    EXPECT_EQ(static_cast<int>(NamedAlgorithmType::OPTIMIZER), 1);
}

// Test string conversion
TEST_F(NamedAlgorithmTypeTest, ToStringConversion) {
    EXPECT_EQ(to_string(NamedAlgorithmType::ROUTER), "ROUTER");
    EXPECT_EQ(to_string(NamedAlgorithmType::OPTIMIZER), "OPTIMIZER");
}

// Test algorithm descriptions
TEST_F(NamedAlgorithmTypeTest, AlgorithmDescriptions) {
    EXPECT_EQ(get_algorithm_description(NamedAlgorithmType::ROUTER), 
              "Algorithm for routing PCB traces and connections");
    EXPECT_EQ(get_algorithm_description(NamedAlgorithmType::OPTIMIZER), 
              "Algorithm for optimizing existing PCB routes");
}

// Test router algorithm detection
TEST_F(NamedAlgorithmTypeTest, RouterAlgorithmDetection) {
    EXPECT_TRUE(is_router_algorithm(NamedAlgorithmType::ROUTER));
    EXPECT_FALSE(is_router_algorithm(NamedAlgorithmType::OPTIMIZER));
}

// Test optimizer algorithm detection
TEST_F(NamedAlgorithmTypeTest, OptimizerAlgorithmDetection) {
    EXPECT_FALSE(is_optimizer_algorithm(NamedAlgorithmType::ROUTER));
    EXPECT_TRUE(is_optimizer_algorithm(NamedAlgorithmType::OPTIMIZER));
}

// Test all algorithm types array
TEST_F(NamedAlgorithmTypeTest, GetAllAlgorithmTypes) {
    auto types = get_all_algorithm_types();
    EXPECT_EQ(types.size(), 2);
    
    // Verify correct order
    EXPECT_EQ(types[0], NamedAlgorithmType::ROUTER);
    EXPECT_EQ(types[1], NamedAlgorithmType::OPTIMIZER);
}

// Test algorithm type exclusivity
TEST_F(NamedAlgorithmTypeTest, AlgorithmTypeExclusivity) {
    auto types = get_all_algorithm_types();
    for (const auto& type : types) {
        bool is_router = is_router_algorithm(type);
        bool is_optimizer = is_optimizer_algorithm(type);
        
        // Types should be mutually exclusive
        EXPECT_FALSE(is_router && is_optimizer) 
            << "Algorithm type cannot be both router and optimizer: " << to_string(type);
        
        // Each type should be either router or optimizer
        EXPECT_TRUE(is_router || is_optimizer) 
            << "Algorithm type must be either router or optimizer: " << to_string(type);
    }
}

// Test edge cases and robustness
TEST_F(NamedAlgorithmTypeTest, EdgeCases) {
    auto types = get_all_algorithm_types();
    for (const auto& type : types) {
        // Should not crash for any valid type
        std::string str = to_string(type);
        std::string desc = get_algorithm_description(type);
        
        // Should return non-empty strings
        EXPECT_FALSE(str.empty());
        EXPECT_FALSE(desc.empty());
        
        // Should have consistent behavior
        bool is_router = is_router_algorithm(type);
        bool is_optimizer = is_optimizer_algorithm(type);
        
        // Should be mutually exclusive
        EXPECT_FALSE(is_router && is_optimizer);
    }
}

// Test consistency
TEST_F(NamedAlgorithmTypeTest, ConsistencyChecks) {
    // Ensure string representations are unique
    std::string router_str = to_string(NamedAlgorithmType::ROUTER);
    std::string optimizer_str = to_string(NamedAlgorithmType::OPTIMIZER);
    
    EXPECT_NE(router_str, optimizer_str);
    
    // Ensure descriptions are unique and meaningful
    std::string router_desc = get_algorithm_description(NamedAlgorithmType::ROUTER);
    std::string optimizer_desc = get_algorithm_description(NamedAlgorithmType::OPTIMIZER);
    
    EXPECT_NE(router_desc, optimizer_desc);
    
    // Descriptions should contain relevant keywords
    EXPECT_NE(router_desc.find("routing"), std::string::npos);
    EXPECT_NE(optimizer_desc.find("optimizing"), std::string::npos);
}

// Test practical usage scenarios
TEST_F(NamedAlgorithmTypeTest, PracticalUsage) {
    // Test algorithm selection logic
    NamedAlgorithmType type = NamedAlgorithmType::ROUTER;
    
    // Should be identified as router
    EXPECT_TRUE(is_router_algorithm(type));
    EXPECT_FALSE(is_optimizer_algorithm(type));
    EXPECT_EQ(to_string(type), "ROUTER");
    
    // Switch to optimizer
    type = NamedAlgorithmType::OPTIMIZER;
    
    // Should be identified as optimizer
    EXPECT_FALSE(is_router_algorithm(type));
    EXPECT_TRUE(is_optimizer_algorithm(type));
    EXPECT_EQ(to_string(type), "OPTIMIZER");
    
    // Test description usage
    std::string desc = get_algorithm_description(type);
    EXPECT_NE(desc.find("optimizing"), std::string::npos);
}

// Test compilation-time evaluation
TEST_F(NamedAlgorithmTypeTest, CompileTimeEvaluation) {
    // These should be evaluated at compile-time
    constexpr bool is_router = is_router_algorithm(NamedAlgorithmType::ROUTER);
    constexpr bool is_optimizer = is_optimizer_algorithm(NamedAlgorithmType::OPTIMIZER);
    
    EXPECT_TRUE(is_router);
    EXPECT_TRUE(is_optimizer);
    
    constexpr auto types = get_all_algorithm_types();
    EXPECT_EQ(types.size(), 2);
}

} // namespace autoroute
} // namespace freerouting
