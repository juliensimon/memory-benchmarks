#include "test_framework.h"
#include "../common/memory_utils.h"
#include "../common/constants.h"
#include <cstring>
#include <limits>

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

// SECURITY TESTS - Added to fix GitHub Issue #1
void test_validate_memory_operation_security() {
    size_t buffer_size = 1024;
    size_t cache_line = 64;
    
    // Valid operations
    ASSERT_TRUE(validate_memory_operation(0, 512, buffer_size, cache_line));
    ASSERT_TRUE(validate_memory_operation(100, 900, buffer_size, cache_line));
    
    // SECURITY: Test buffer overflow conditions
    ASSERT_FALSE(validate_memory_operation(0, buffer_size + 1, buffer_size, cache_line)); // end > buffer_size
    ASSERT_FALSE(validate_memory_operation(buffer_size + 1, buffer_size + 100, buffer_size, cache_line)); // start > buffer_size
    
    // SECURITY: Test integer overflow conditions
    ASSERT_FALSE(validate_memory_operation(std::numeric_limits<size_t>::max() - 10, 
                                         std::numeric_limits<size_t>::max(), 
                                         buffer_size, cache_line));
    
    // SECURITY: Test malicious cache line sizes
    ASSERT_FALSE(validate_memory_operation(0, 512, buffer_size, 0)); // zero cache line
    ASSERT_FALSE(validate_memory_operation(0, 512, buffer_size, 3)); // non-power-of-2
    ASSERT_FALSE(validate_memory_operation(0, 512, buffer_size, 2048)); // too large
    
    // SECURITY: Test range ordering attacks
    ASSERT_FALSE(validate_memory_operation(500, 400, buffer_size, cache_line)); // start > end
    ASSERT_FALSE(validate_memory_operation(300, 300, buffer_size, cache_line)); // start == end
}

void test_safe_memory_copy_security() {
    uint8_t src_buffer[100];
    uint8_t dst_buffer[100];
    
    // Fill source with test data
    for (int i = 0; i < 100; i++) {
        src_buffer[i] = static_cast<uint8_t>(i);
    }
    
    // Valid copy operations
    ASSERT_TRUE(safe_memory_copy(dst_buffer, sizeof(dst_buffer), 
                                src_buffer, sizeof(src_buffer), 0, 50));
    ASSERT_TRUE(safe_memory_copy(dst_buffer, sizeof(dst_buffer), 
                                src_buffer, sizeof(src_buffer), 10, 30));
    
    // SECURITY: Test null pointer attacks
    ASSERT_FALSE(safe_memory_copy(nullptr, 100, src_buffer, sizeof(src_buffer), 0, 50));
    ASSERT_FALSE(safe_memory_copy(dst_buffer, sizeof(dst_buffer), nullptr, 100, 0, 50));
    ASSERT_FALSE(safe_memory_copy(nullptr, 0, nullptr, 0, 0, 50));
    
    // SECURITY: Test buffer overflow attacks
    ASSERT_FALSE(safe_memory_copy(dst_buffer, sizeof(dst_buffer), 
                                 src_buffer, sizeof(src_buffer), 0, 150)); // size > dst_size
    ASSERT_FALSE(safe_memory_copy(dst_buffer, sizeof(dst_buffer), 
                                 src_buffer, sizeof(src_buffer), 90, 20)); // offset + size > src_size
    ASSERT_FALSE(safe_memory_copy(dst_buffer, sizeof(dst_buffer), 
                                 src_buffer, sizeof(src_buffer), 90, 20)); // offset + size > dst_size
    
    // SECURITY: Test integer overflow attacks
    size_t max_offset = std::numeric_limits<size_t>::max() - 10;
    ASSERT_FALSE(safe_memory_copy(dst_buffer, sizeof(dst_buffer), 
                                 src_buffer, sizeof(src_buffer), max_offset, 50));
    
    // SECURITY: Test zero-size copy (should succeed but not crash)
    ASSERT_TRUE(safe_memory_copy(dst_buffer, sizeof(dst_buffer), 
                                src_buffer, sizeof(src_buffer), 0, 0));
}

void test_safe_memory_set_security() {
    uint8_t buffer[100];
    
    // Valid set operations
    ASSERT_TRUE(safe_memory_set(buffer, sizeof(buffer), 0x42, 50));
    ASSERT_TRUE(safe_memory_set(buffer, sizeof(buffer), 0, sizeof(buffer)));
    
    // SECURITY: Test null pointer attack
    ASSERT_FALSE(safe_memory_set(nullptr, 100, 0x42, 50));
    
    // SECURITY: Test buffer overflow attack
    ASSERT_FALSE(safe_memory_set(buffer, sizeof(buffer), 0x42, 200)); // size > buffer_size
    
    // SECURITY: Test zero-size set (should succeed but not crash)  
    ASSERT_TRUE(safe_memory_set(buffer, sizeof(buffer), 0x42, 0));
    
    // Verify the actual memset worked for valid case
    memset(buffer, 0, sizeof(buffer)); // Clear first
    ASSERT_TRUE(safe_memory_set(buffer, sizeof(buffer), 0xAA, 10));
    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(buffer[i] == 0xAA);
    }
    // Rest should still be zero
    for (int i = 10; i < 100; i++) {
        ASSERT_TRUE(buffer[i] == 0);
    }
}

void test_memory_operation_integration_security() {
    // Test that validate_memory_operation correctly predicts what safe_memory_copy will accept
    uint8_t src_buffer[1000];
    uint8_t dst_buffer[1000];
    size_t buffer_size = sizeof(src_buffer);
    size_t cache_line = 64;
    
    // Test case 1: Valid parameters - both should succeed
    size_t start1 = 100, end1 = 800;
    bool validation_result1 = validate_memory_operation(start1, end1, buffer_size, cache_line);
    
    if (validation_result1) {
        auto [aligned_start, aligned_end] = align_to_cache_lines(start1, end1, cache_line);
        size_t working_set_size = calculate_working_set_size(aligned_start, aligned_end);
        
        bool copy_result = safe_memory_copy(dst_buffer, buffer_size,
                                          src_buffer, buffer_size,
                                          aligned_start, working_set_size);
        ASSERT_TRUE(copy_result); // Should succeed if validation passed
    }
    
    // Test case 2: Invalid parameters - validation should fail, copy should not be attempted
    size_t start2 = 100, end2 = 1200; // end > buffer_size
    bool validation_result2 = validate_memory_operation(start2, end2, buffer_size, cache_line);
    ASSERT_FALSE(validation_result2); // Should fail validation
    
    // Test case 3: Edge case - very small buffer
    uint8_t small_src[10];
    uint8_t small_dst[10];
    size_t small_start = 0, small_end = 10;
    
    bool validation_result3 = validate_memory_operation(small_start, small_end, 10, cache_line);
    if (validation_result3) {
        auto [aligned_start3, aligned_end3] = align_to_cache_lines(small_start, small_end, cache_line);
        size_t working_set_size3 = calculate_working_set_size(aligned_start3, aligned_end3);
        
        if (working_set_size3 > 0) {
            bool copy_result3 = safe_memory_copy(small_dst, 10, small_src, 10,
                                               aligned_start3, working_set_size3);
            ASSERT_TRUE(copy_result3); // Should succeed if we get here
        }
    }
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
    
    // SECURITY TEST CASES - Added to fix GitHub Issue #1
    TEST_CASE("SECURITY: Validate memory operation", test_validate_memory_operation_security);
    TEST_CASE("SECURITY: Safe memory copy", test_safe_memory_copy_security);
    TEST_CASE("SECURITY: Safe memory set", test_safe_memory_set_security);
    TEST_CASE("SECURITY: Memory operation integration", test_memory_operation_integration_security);
    
    return framework.run_all();
}