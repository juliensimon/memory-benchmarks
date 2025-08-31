#include "test_framework.h"
#include "../common/test_patterns.h"
#include <cmath>

void test_get_pattern_name_all_patterns() {
    // Test all defined pattern names
    ASSERT_TRUE(get_pattern_name(TestPattern::SEQUENTIAL_READ) == "Sequential Read");
    ASSERT_TRUE(get_pattern_name(TestPattern::SEQUENTIAL_WRITE) == "Sequential Write");
    ASSERT_TRUE(get_pattern_name(TestPattern::RANDOM_READ) == "Random Read");
    ASSERT_TRUE(get_pattern_name(TestPattern::RANDOM_WRITE) == "Random Write");
    ASSERT_TRUE(get_pattern_name(TestPattern::COPY) == "Copy");
    ASSERT_TRUE(get_pattern_name(TestPattern::TRIAD) == "Triad");
    ASSERT_TRUE(get_pattern_name(TestPattern::MATRIX_MULTIPLY) == "Matrix Multiply (GEMM)");
}

void test_get_pattern_name_unknown() {
    // Test unknown/invalid pattern
    TestPattern invalid_pattern = static_cast<TestPattern>(999);
    ASSERT_TRUE(get_pattern_name(invalid_pattern) == "Unknown");
    
    // Test another invalid value
    TestPattern invalid_pattern2 = static_cast<TestPattern>(-1);
    ASSERT_TRUE(get_pattern_name(invalid_pattern2) == "Unknown");
}

void test_calculate_stats_basic() {
    size_t bytes = 1000000;     // 1MB
    double time = 0.5;          // 0.5 seconds  
    size_t operations = 100000; // 100K operations
    
    PerformanceStats stats = calculate_stats(bytes, time, operations);
    
    ASSERT_TRUE(stats.bytes_processed == bytes);
    ASSERT_TRUE(stats.time_seconds == time);
    
    // Calculate expected values
    double expected_bandwidth = bytes / (time * 1e9); // 1MB / (0.5s * 1e9) = 0.002 GB/s
    double expected_latency = (time * 1e9) / operations; // (0.5 * 1e9) / 100K = 5000 ns
    
    ASSERT_TRUE(std::abs(stats.bandwidth_gbps - expected_bandwidth) < 1e-10);
    ASSERT_TRUE(std::abs(stats.latency_ns - expected_latency) < 1e-10);
}

void test_calculate_stats_zero_time() {
    size_t bytes = 1000000;
    double time = 0.0;
    size_t operations = 100000;
    
    PerformanceStats stats = calculate_stats(bytes, time, operations);
    
    ASSERT_TRUE(stats.bytes_processed == bytes);
    ASSERT_TRUE(stats.time_seconds == time);
    ASSERT_TRUE(stats.bandwidth_gbps == 0.0);
    ASSERT_TRUE(stats.latency_ns == 0.0);
}

void test_calculate_stats_zero_operations() {
    size_t bytes = 1000000;
    double time = 0.5;
    size_t operations = 0;
    
    PerformanceStats stats = calculate_stats(bytes, time, operations);
    
    ASSERT_TRUE(stats.bytes_processed == bytes);
    ASSERT_TRUE(stats.time_seconds == time);
    ASSERT_TRUE(stats.bandwidth_gbps == 0.0);
    ASSERT_TRUE(stats.latency_ns == 0.0);
}

void test_calculate_stats_high_bandwidth_clamping() {
    // Test the bandwidth clamping feature for unrealistically high values
    size_t bytes = 1000000000000ULL; // 1TB
    double time = 0.001;              // 1ms - would give 1,000,000 GB/s 
    size_t operations = 1000000;
    
    PerformanceStats stats = calculate_stats(bytes, time, operations);
    
    // Should be clamped to MAX_REALISTIC_BANDWIDTH_GBPS (60.0)
    ASSERT_TRUE(stats.bandwidth_gbps == 60.0);
    ASSERT_TRUE(stats.bytes_processed == bytes);
    ASSERT_TRUE(stats.time_seconds == time);
    
    // Latency should still be calculated normally
    double expected_latency = (time * 1e9) / operations;
    ASSERT_TRUE(std::abs(stats.latency_ns - expected_latency) < 1e-10);
}

void test_calculate_stats_realistic_bandwidth() {
    // Test with realistic bandwidth that should not be clamped
    size_t bytes = 50000000;    // 50MB
    double time = 1.0;          // 1 second
    size_t operations = 1000000;
    
    PerformanceStats stats = calculate_stats(bytes, time, operations);
    
    double expected_bandwidth = bytes / (time * 1e9); // 50MB/s = 0.05 GB/s
    ASSERT_TRUE(std::abs(stats.bandwidth_gbps - expected_bandwidth) < 1e-10);
    ASSERT_TRUE(stats.bandwidth_gbps < 60.0); // Should not be clamped
}

void test_calculate_stats_boundary_bandwidth() {
    // Test bandwidth right at the boundary
    size_t bytes = 60000000000ULL; // 60GB
    double time = 1.0;             // 1 second -> exactly 60 GB/s
    size_t operations = 1000000;
    
    PerformanceStats stats = calculate_stats(bytes, time, operations);
    
    ASSERT_TRUE(stats.bandwidth_gbps == 60.0);
    ASSERT_TRUE(stats.bytes_processed == bytes);
}

void test_calculate_stats_small_values() {
    // Test with values that will definitely exceed the 60 GB/s limit
    size_t bytes = 1000000000;  // 1GB  
    double time = 0.001;        // 1 millisecond -> 1000 GB/s raw, should be clamped
    size_t operations = 1000;
    
    PerformanceStats stats = calculate_stats(bytes, time, operations);
    
    ASSERT_TRUE(stats.bytes_processed == bytes);
    ASSERT_TRUE(stats.time_seconds == time);
    
    // Should be clamped to exactly 60.0 
    // Raw calculation: 1GB / (0.001s * 1e9) = 1000 GB/s -> clamped to 60.0
    ASSERT_TRUE(stats.bandwidth_gbps == 60.0);
    
    double expected_latency = (time * 1e9) / operations; // 1000000 ns
    ASSERT_TRUE(std::abs(stats.latency_ns - expected_latency) < 1e-10);
}

void test_calculate_stats_large_operations() {
    // Test with large number of operations
    size_t bytes = 1000000;
    double time = 2.0;
    size_t operations = 10000000; // 10M operations
    
    PerformanceStats stats = calculate_stats(bytes, time, operations);
    
    double expected_bandwidth = bytes / (time * 1e9);
    double expected_latency = (time * 1e9) / operations; // Should be small latency
    
    ASSERT_TRUE(std::abs(stats.bandwidth_gbps - expected_bandwidth) < 1e-10);
    ASSERT_TRUE(std::abs(stats.latency_ns - expected_latency) < 1e-10);
    ASSERT_TRUE(stats.latency_ns < 1000.0); // Should be sub-microsecond
}

void test_calculate_stats_edge_case_negative_time() {
    // Test with invalid negative time (should be handled gracefully)
    size_t bytes = 1000000;
    double time = -1.0; // Invalid
    size_t operations = 1000;
    
    PerformanceStats stats = calculate_stats(bytes, time, operations);
    
    // Should treat as zero time
    ASSERT_TRUE(stats.bandwidth_gbps == 0.0);
    ASSERT_TRUE(stats.latency_ns == 0.0);
    ASSERT_TRUE(stats.bytes_processed == bytes);
    ASSERT_TRUE(stats.time_seconds == time);
}

void test_pattern_name_consistency() {
    // Test that pattern names are consistent and don't contain unexpected characters
    std::string sequential_read = get_pattern_name(TestPattern::SEQUENTIAL_READ);
    std::string random_write = get_pattern_name(TestPattern::RANDOM_WRITE);
    std::string matrix_multiply = get_pattern_name(TestPattern::MATRIX_MULTIPLY);
    
    ASSERT_TRUE(sequential_read.find("Sequential") != std::string::npos);
    ASSERT_TRUE(sequential_read.find("Read") != std::string::npos);
    ASSERT_FALSE(sequential_read.empty());
    
    ASSERT_TRUE(random_write.find("Random") != std::string::npos);
    ASSERT_TRUE(random_write.find("Write") != std::string::npos);
    ASSERT_FALSE(random_write.empty());
    
    ASSERT_TRUE(matrix_multiply.find("Matrix") != std::string::npos);
    ASSERT_TRUE(matrix_multiply.find("GEMM") != std::string::npos);
    ASSERT_FALSE(matrix_multiply.empty());
}

void test_performance_stats_structure() {
    // Test that PerformanceStats structure is properly initialized
    PerformanceStats stats = calculate_stats(1000, 1.0, 1000);
    
    // All fields should be set to reasonable values
    ASSERT_TRUE(stats.bytes_processed > 0);
    ASSERT_TRUE(stats.time_seconds > 0);
    ASSERT_TRUE(stats.bandwidth_gbps >= 0);
    ASSERT_TRUE(stats.latency_ns >= 0);
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Get pattern name all patterns", test_get_pattern_name_all_patterns);
    TEST_CASE("Get pattern name unknown", test_get_pattern_name_unknown);
    TEST_CASE("Calculate stats basic", test_calculate_stats_basic);
    TEST_CASE("Calculate stats zero time", test_calculate_stats_zero_time);
    TEST_CASE("Calculate stats zero operations", test_calculate_stats_zero_operations);
    TEST_CASE("Calculate stats high bandwidth clamping", test_calculate_stats_high_bandwidth_clamping);
    TEST_CASE("Calculate stats realistic bandwidth", test_calculate_stats_realistic_bandwidth);
    TEST_CASE("Calculate stats boundary bandwidth", test_calculate_stats_boundary_bandwidth);
    TEST_CASE("Calculate stats small values", test_calculate_stats_small_values);
    TEST_CASE("Calculate stats large operations", test_calculate_stats_large_operations);
    TEST_CASE("Calculate stats edge case negative time", test_calculate_stats_edge_case_negative_time);
    TEST_CASE("Pattern name consistency", test_pattern_name_consistency);
    TEST_CASE("Performance stats structure", test_performance_stats_structure);
    
    return framework.run_all();
}