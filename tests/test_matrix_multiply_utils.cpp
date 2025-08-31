#include "test_framework.h"
#include "../common/matrix_multiply_interface.h"
#include <cmath>
#include <vector>

using namespace MatrixMultiply;

void test_create_matrix_config() {
    MatrixConfig config = create_matrix_config(128, 100, false);
    ASSERT_TRUE(config.M == 128);
    ASSERT_TRUE(config.K == 128);
    ASSERT_TRUE(config.N == 128);
    ASSERT_TRUE(config.iterations == 100);
    ASSERT_FALSE(config.use_double);
    ASSERT_FALSE(config.transpose_b);
    
    MatrixConfig config_double = create_matrix_config(256, 50, true);
    ASSERT_TRUE(config_double.use_double);
}

void test_calculate_matrix_memory_footprint() {
    MatrixConfig config_float = create_matrix_config(100, 1, false);
    size_t footprint_float = calculate_matrix_memory_footprint(config_float);
    // 100x100 + 100x100 + 100x100 = 30000 floats = 30000 * 4 bytes
    ASSERT_TRUE(footprint_float == 30000 * sizeof(float));
    
    MatrixConfig config_double = create_matrix_config(100, 1, true);
    size_t footprint_double = calculate_matrix_memory_footprint(config_double);
    ASSERT_TRUE(footprint_double == 30000 * sizeof(double));
    
    // Test different dimensions
    MatrixConfig rect_config{200, 100, 150, 1, false, false};
    size_t rect_footprint = calculate_matrix_memory_footprint(rect_config);
    // 200x100 + 100x150 + 200x150 = 20000 + 15000 + 30000 = 65000 floats
    ASSERT_TRUE(rect_footprint == 65000 * sizeof(float));
}

void test_initialize_matrix_random_float() {
    const size_t rows = 10, cols = 10;
    std::vector<float> matrix(rows * cols);
    
    initialize_matrix_random(matrix.data(), rows, cols, 1.0f);
    
    // Check all elements are within range [-1.0, 1.0]
    for (float val : matrix) {
        ASSERT_TRUE(val >= -1.0f && val <= 1.0f);
    }
    
    // Test with different scale
    initialize_matrix_random(matrix.data(), rows, cols, 5.0f);
    bool found_large = false;
    for (float val : matrix) {
        ASSERT_TRUE(val >= -5.0f && val <= 5.0f);
        if (std::abs(val) > 1.0f) found_large = true;
    }
    ASSERT_TRUE(found_large); // Should have some values > 1.0 with scale 5.0
}

void test_initialize_matrix_random_double() {
    const size_t rows = 8, cols = 12;
    std::vector<double> matrix(rows * cols);
    
    initialize_matrix_random(matrix.data(), rows, cols, 2.0);
    
    // Check all elements are within range [-2.0, 2.0]
    for (double val : matrix) {
        ASSERT_TRUE(val >= -2.0 && val <= 2.0);
    }
    
    // Test randomness - not all values should be the same
    bool all_same = true;
    double first_val = matrix[0];
    for (size_t i = 1; i < matrix.size(); ++i) {
        if (std::abs(matrix[i] - first_val) > 1e-10) {
            all_same = false;
            break;
        }
    }
    ASSERT_FALSE(all_same);
}

void test_validate_matrix_result() {
    const size_t rows = 5, cols = 5;
    std::vector<float> matrix_a(rows * cols, 1.0f);
    std::vector<float> matrix_b(rows * cols, 1.0f);
    std::vector<float> matrix_c(rows * cols, 1.01f);
    std::vector<float> matrix_d(rows * cols, 2.0f);
    
    // Test identical matrices
    ASSERT_TRUE(validate_matrix_result(matrix_a.data(), matrix_b.data(), rows, cols, 0.0f));
    
    // Test within tolerance
    ASSERT_TRUE(validate_matrix_result(matrix_a.data(), matrix_c.data(), rows, cols, 0.02f));
    
    // Test outside tolerance
    ASSERT_FALSE(validate_matrix_result(matrix_a.data(), matrix_c.data(), rows, cols, 0.005f));
    
    // Test significantly different matrices
    ASSERT_FALSE(validate_matrix_result(matrix_a.data(), matrix_d.data(), rows, cols, 0.5f));
    
    // Test edge case with zero tolerance
    ASSERT_FALSE(validate_matrix_result(matrix_a.data(), matrix_c.data(), rows, cols, 0.0f));
}

void test_calculate_matrix_stats() {
    // Test basic calculation
    size_t bytes = 1000000;  // 1MB
    double time = 0.5;       // 0.5 seconds
    size_t ops = 1000000000; // 1 billion operations
    std::string accel = "CPU";
    
    MatrixPerformanceStats stats = calculate_matrix_stats(bytes, time, ops, accel);
    
    ASSERT_TRUE(stats.time_seconds == 0.5);
    ASSERT_TRUE(stats.operations == 1000000000);
    ASSERT_TRUE(stats.bytes_processed == 1000000);
    TestAssert::assert_equal(std::string("CPU"), stats.acceleration);
    
    // Check calculated values
    double expected_gflops = 1000000000 / (0.5 * 1e9); // ops / (time * 1e9) = 2.0 GFLOPS
    ASSERT_TRUE(std::abs(stats.gflops - expected_gflops) < 1e-10);
    
    double expected_bandwidth = 1000000 / (0.5 * 1e9); // bytes / (time * 1e9) = 0.002 GB/s
    ASSERT_TRUE(std::abs(stats.bandwidth_gbps - expected_bandwidth) < 1e-10);
    
    double expected_latency = (0.5 * 1e9) / 1000000000; // (time * 1e9) / ops = 0.5 ns
    ASSERT_TRUE(std::abs(stats.latency_ns - expected_latency) < 1e-10);
}

void test_calculate_matrix_stats_edge_cases() {
    // Test zero time
    MatrixPerformanceStats stats_zero_time = calculate_matrix_stats(1000, 0.0, 1000, "GPU");
    ASSERT_TRUE(stats_zero_time.gflops == 0.0);
    ASSERT_TRUE(stats_zero_time.bandwidth_gbps == 0.0);
    ASSERT_TRUE(stats_zero_time.time_seconds == 0.0);
    
    // Test zero operations  
    MatrixPerformanceStats stats_zero_ops = calculate_matrix_stats(1000, 1.0, 0, "AMX");
    ASSERT_TRUE(stats_zero_ops.latency_ns == 0.0);
    ASSERT_TRUE(stats_zero_ops.operations == 0);
    
    // Test very small time
    MatrixPerformanceStats stats_small_time = calculate_matrix_stats(1000, 1e-9, 1000, "NEON");
    ASSERT_TRUE(stats_small_time.gflops > 0.0);
    ASSERT_TRUE(stats_small_time.bandwidth_gbps > 0.0);
}

void test_matrix_config_edge_cases() {
    // Test minimum size
    MatrixConfig min_config = create_matrix_config(1, 1, false);
    ASSERT_TRUE(min_config.M == 1);
    ASSERT_TRUE(calculate_matrix_memory_footprint(min_config) == 3 * sizeof(float));
    
    // Test large size
    MatrixConfig large_config = create_matrix_config(1000, 10, true);
    size_t large_footprint = calculate_matrix_memory_footprint(large_config);
    ASSERT_TRUE(large_footprint == 3000000 * sizeof(double));
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Create matrix config", test_create_matrix_config);
    TEST_CASE("Calculate matrix memory footprint", test_calculate_matrix_memory_footprint);
    TEST_CASE("Initialize matrix random float", test_initialize_matrix_random_float);
    TEST_CASE("Initialize matrix random double", test_initialize_matrix_random_double);
    TEST_CASE("Validate matrix result", test_validate_matrix_result);
    TEST_CASE("Calculate matrix stats", test_calculate_matrix_stats);
    TEST_CASE("Calculate matrix stats edge cases", test_calculate_matrix_stats_edge_cases);
    TEST_CASE("Matrix config edge cases", test_matrix_config_edge_cases);
    
    return framework.run_all();
}