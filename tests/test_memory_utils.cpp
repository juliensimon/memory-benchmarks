#include "test_framework.h"
#include "../common/memory_utils.h"
#include "../common/constants.h"

using namespace MemoryUtils;
using namespace BenchmarkConstants;

void test_align_to_cache_lines_basic() {
    size_t cache_line = 64;
    
    // Test aligning start up, end down
    auto [aligned_start, aligned_end] = align_to_cache_lines(10, 200, cache_line);
    ASSERT_TRUE(aligned_start == 64);  // 10 aligned up to next 64-byte boundary
    ASSERT_TRUE(aligned_end == 192);   // 200 aligned down to previous 64-byte boundary
    
    // Test already aligned values
    auto [aligned_start2, aligned_end2] = align_to_cache_lines(64, 128, cache_line);
    ASSERT_TRUE(aligned_start2 == 64);
    ASSERT_TRUE(aligned_end2 == 128);
    
    // Test with different cache line sizes
    auto [aligned_start3, aligned_end3] = align_to_cache_lines(30, 250, 128);
    ASSERT_TRUE(aligned_start3 == 128);  // 30 aligned up to 128
    ASSERT_TRUE(aligned_end3 == 128);    // 250 aligned down to 128
}

void test_align_to_cache_lines_edge_cases() {
    // Test zero offsets
    auto [aligned_start1, aligned_end1] = align_to_cache_lines(0, 100, 64);
    ASSERT_TRUE(aligned_start1 == 0);
    ASSERT_TRUE(aligned_end1 == 64);
    
    // Test when end < start after alignment
    auto [aligned_start2, aligned_end2] = align_to_cache_lines(50, 70, 64);
    ASSERT_TRUE(aligned_start2 == 64);
    ASSERT_TRUE(aligned_end2 == 64);
    
    // Test with cache line size 1 (degenerate case)
    auto [aligned_start3, aligned_end3] = align_to_cache_lines(10, 20, 1);
    ASSERT_TRUE(aligned_start3 == 10);
    ASSERT_TRUE(aligned_end3 == 20);
    
    // Test large values
    auto [aligned_start4, aligned_end4] = align_to_cache_lines(1000000, 2000000, 64);
    ASSERT_TRUE(aligned_start4 % 64 == 0);
    ASSERT_TRUE(aligned_end4 % 64 == 0);
    ASSERT_TRUE(aligned_start4 >= 1000000);
    ASSERT_TRUE(aligned_end4 <= 2000000);
}

void test_calculate_working_set_size() {
    // Normal case
    ASSERT_TRUE(calculate_working_set_size(64, 192) == 128);
    
    // Equal start and end
    ASSERT_TRUE(calculate_working_set_size(100, 100) == 0);
    
    // End less than start
    ASSERT_TRUE(calculate_working_set_size(200, 100) == 0);
    
    // Zero start
    ASSERT_TRUE(calculate_working_set_size(0, 1024) == 1024);
    
    // Large values
    ASSERT_TRUE(calculate_working_set_size(1000000, 2000000) == 1000000);
}

void test_validate_buffer_range_valid_cases() {
    // Valid range
    ASSERT_TRUE(validate_buffer_range(0, 1000, 2000, 100));
    
    // Minimum size exactly met
    ASSERT_TRUE(validate_buffer_range(100, 200, 500, 100));
    
    // Large buffer
    ASSERT_TRUE(validate_buffer_range(0, 1000000, 2000000, 1000));
    
    // Start in middle of buffer
    ASSERT_TRUE(validate_buffer_range(500, 1500, 2000, 500));
}

void test_validate_buffer_range_invalid_cases() {
    // Start >= end
    ASSERT_FALSE(validate_buffer_range(100, 100, 500, 50));
    ASSERT_FALSE(validate_buffer_range(200, 100, 500, 50));
    
    // End > buffer_size
    ASSERT_FALSE(validate_buffer_range(0, 2000, 1500, 100));
    
    // Size too small
    ASSERT_FALSE(validate_buffer_range(100, 150, 500, 100)); // size = 50, min = 100
    
    // Zero min_size but other invalid conditions
    ASSERT_FALSE(validate_buffer_range(100, 50, 200, 0)); // start > end
}

void test_calculate_buffer_size_valid() {
    // Normal case - need large enough total to meet MIN_BUFFER_SIZE
    ASSERT_TRUE(calculate_buffer_size(40000, 4, 64) == 10000); // 40KB / 4 = 10KB > 4KB MIN
    
    // Large total size
    ASSERT_TRUE(calculate_buffer_size(1000000, 10, 64) == 100000);
    
    // Buffer size equals cache line size - need large enough total
    ASSERT_TRUE(calculate_buffer_size(512000, 8, 64) == 64000); // 512KB / 8 = 64KB > 4KB MIN
    
    // Buffer size larger than MIN_BUFFER_SIZE
    size_t result = calculate_buffer_size(MIN_BUFFER_SIZE * 10, 2, 64);
    ASSERT_TRUE(result == MIN_BUFFER_SIZE * 5);
}

void test_calculate_buffer_size_invalid() {
    // Zero total size
    ASSERT_TRUE(calculate_buffer_size(0, 4, 64) == 0);
    
    // Zero num_buffers
    ASSERT_TRUE(calculate_buffer_size(1000, 0, 64) == 0);
    
    // Buffer size too small (less than MIN_BUFFER_SIZE)
    ASSERT_TRUE(calculate_buffer_size(MIN_BUFFER_SIZE / 2, 2, 64) == 0);
    
    // Buffer size less than cache line size
    ASSERT_TRUE(calculate_buffer_size(100, 10, 64) == 0); // 100/10 = 10 < 64
}

void test_is_cache_line_aligned() {
    // Aligned values
    ASSERT_TRUE(is_cache_line_aligned(0, 64));
    ASSERT_TRUE(is_cache_line_aligned(64, 64));
    ASSERT_TRUE(is_cache_line_aligned(128, 64));
    ASSERT_TRUE(is_cache_line_aligned(1024, 64));
    
    // Non-aligned values
    ASSERT_FALSE(is_cache_line_aligned(1, 64));
    ASSERT_FALSE(is_cache_line_aligned(63, 64));
    ASSERT_FALSE(is_cache_line_aligned(65, 64));
    ASSERT_FALSE(is_cache_line_aligned(127, 64));
    
    // Different cache line sizes
    ASSERT_TRUE(is_cache_line_aligned(256, 128));
    ASSERT_FALSE(is_cache_line_aligned(200, 128));
    
    // Cache line size 1
    ASSERT_TRUE(is_cache_line_aligned(100, 1)); // Everything is aligned to 1
}

void test_scale_iterations() {
    size_t base_iter = 1000;
    
    // Small cache (should use SMALL_CACHE_ITER_MULTIPLIER)
    size_t small_size = SMALL_CACHE_THRESHOLD - 100;
    size_t small_result = scale_iterations(base_iter, small_size);
    ASSERT_TRUE(small_result == base_iter * SMALL_CACHE_ITER_MULTIPLIER);
    
    // Medium cache (should use MEDIUM_CACHE_ITER_MULTIPLIER)
    size_t medium_size = SMALL_CACHE_THRESHOLD + 1000;
    if (medium_size <= MEDIUM_CACHE_THRESHOLD) {
        size_t medium_result = scale_iterations(base_iter, medium_size);
        ASSERT_TRUE(medium_result == base_iter * MEDIUM_CACHE_ITER_MULTIPLIER);
    }
    
    // Large cache (should use LARGE_CACHE_ITER_MULTIPLIER)
    size_t large_size = MEDIUM_CACHE_THRESHOLD + 1000;
    if (large_size <= LARGE_CACHE_THRESHOLD) {
        size_t large_result = scale_iterations(base_iter, large_size);
        ASSERT_TRUE(large_result == base_iter * LARGE_CACHE_ITER_MULTIPLIER);
    }
    
    // Very large working set (should use base iterations)
    size_t very_large_size = LARGE_CACHE_THRESHOLD + 1000000;
    size_t very_large_result = scale_iterations(base_iter, very_large_size);
    ASSERT_TRUE(very_large_result == base_iter);
}

void test_scale_iterations_boundary_conditions() {
    size_t base_iter = 500;
    
    // Exactly at thresholds
    ASSERT_TRUE(scale_iterations(base_iter, SMALL_CACHE_THRESHOLD) == base_iter * SMALL_CACHE_ITER_MULTIPLIER);
    ASSERT_TRUE(scale_iterations(base_iter, MEDIUM_CACHE_THRESHOLD) == base_iter * MEDIUM_CACHE_ITER_MULTIPLIER);
    ASSERT_TRUE(scale_iterations(base_iter, LARGE_CACHE_THRESHOLD) == base_iter * LARGE_CACHE_ITER_MULTIPLIER);
    
    // Just over thresholds
    if (MEDIUM_CACHE_THRESHOLD > SMALL_CACHE_THRESHOLD) {
        ASSERT_TRUE(scale_iterations(base_iter, SMALL_CACHE_THRESHOLD + 1) == base_iter * MEDIUM_CACHE_ITER_MULTIPLIER);
    }
    if (LARGE_CACHE_THRESHOLD > MEDIUM_CACHE_THRESHOLD) {
        ASSERT_TRUE(scale_iterations(base_iter, MEDIUM_CACHE_THRESHOLD + 1) == base_iter * LARGE_CACHE_ITER_MULTIPLIER);
    }
    ASSERT_TRUE(scale_iterations(base_iter, LARGE_CACHE_THRESHOLD + 1) == base_iter);
}

void test_integration_align_and_calculate() {
    // Test realistic scenario: align memory range and calculate working set
    size_t start = 100, end = 5000, cache_line = 64;
    auto [aligned_start, aligned_end] = align_to_cache_lines(start, end, cache_line);
    size_t working_set = calculate_working_set_size(aligned_start, aligned_end);
    
    ASSERT_TRUE(aligned_start >= start);
    ASSERT_TRUE(aligned_end <= end);
    ASSERT_TRUE(is_cache_line_aligned(aligned_start, cache_line));
    ASSERT_TRUE(is_cache_line_aligned(aligned_end, cache_line));
    ASSERT_TRUE(working_set == (aligned_end - aligned_start));
    
    // Validate the range is usable
    ASSERT_TRUE(validate_buffer_range(aligned_start, aligned_end, end, cache_line));
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Align to cache lines basic", test_align_to_cache_lines_basic);
    TEST_CASE("Align to cache lines edge cases", test_align_to_cache_lines_edge_cases);
    TEST_CASE("Calculate working set size", test_calculate_working_set_size);
    TEST_CASE("Validate buffer range valid cases", test_validate_buffer_range_valid_cases);
    TEST_CASE("Validate buffer range invalid cases", test_validate_buffer_range_invalid_cases);
    TEST_CASE("Calculate buffer size valid", test_calculate_buffer_size_valid);
    TEST_CASE("Calculate buffer size invalid", test_calculate_buffer_size_invalid);
    TEST_CASE("Is cache line aligned", test_is_cache_line_aligned);
    TEST_CASE("Scale iterations", test_scale_iterations);
    TEST_CASE("Scale iterations boundary conditions", test_scale_iterations_boundary_conditions);
    TEST_CASE("Integration align and calculate", test_integration_align_and_calculate);
    
    return framework.run_all();
}