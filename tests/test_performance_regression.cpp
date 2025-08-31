#include "test_framework.h"
#include "../common/standard_tests.h"
#include "../common/aligned_buffer.h"
#include <atomic>
#include <chrono>

/**
 * @brief Performance regression test suite
 * 
 * These tests ensure that performance doesn't degrade below acceptable thresholds.
 * They run quick benchmarks and verify minimum performance standards.
 */

void test_sequential_read_performance() {
    const size_t buffer_size = 16 * 1024 * 1024; // 16MB for reliable measurement
    AlignedBuffer buffer(buffer_size, 128);
    
    std::atomic<bool> stop_flag{false};
    
    auto start = std::chrono::high_resolution_clock::now();
    auto stats = StandardTests::sequential_read_test(
        buffer.data(), buffer_size, 0, buffer_size, 3, stop_flag
    );
    auto end = std::chrono::high_resolution_clock::now();
    
    double total_seconds = std::chrono::duration<double>(end - start).count();
    
    // Performance regression thresholds
    ASSERT_TRUE(stats.bandwidth_gbps > 0.5); // Minimum 0.5 Gb/s (very conservative)
    ASSERT_TRUE(stats.time_seconds > 0.0);   // Should actually take some time
    ASSERT_TRUE(stats.bytes_processed > 0);  // Should process data
    ASSERT_TRUE(total_seconds < 60.0);       // Should complete within 60 seconds
    
    std::cout << "Sequential Read: " << stats.bandwidth_gbps << " Gb/s" << std::endl;
}

void test_sequential_write_performance() {
    const size_t buffer_size = 16 * 1024 * 1024; // 16MB
    AlignedBuffer buffer(buffer_size, 128);
    
    std::atomic<bool> stop_flag{false};
    
    auto start = std::chrono::high_resolution_clock::now();
    auto stats = StandardTests::sequential_write_test(
        buffer.data(), buffer_size, 0, buffer_size, 3, stop_flag
    );
    auto end = std::chrono::high_resolution_clock::now();
    
    double total_seconds = std::chrono::duration<double>(end - start).count();
    
    // Performance regression thresholds
    ASSERT_TRUE(stats.bandwidth_gbps > 0.5); // Minimum 0.5 Gb/s
    ASSERT_TRUE(stats.time_seconds > 0.0);
    ASSERT_TRUE(stats.bytes_processed > 0);
    ASSERT_TRUE(total_seconds < 60.0);
    
    std::cout << "Sequential Write: " << stats.bandwidth_gbps << " Gb/s" << std::endl;
}

void test_random_access_performance() {
    const size_t buffer_size = 4 * 1024 * 1024; // 4MB for random access
    AlignedBuffer buffer(buffer_size, 128);
    
    std::atomic<bool> stop_flag{false};
    
    auto start = std::chrono::high_resolution_clock::now();
    auto stats = StandardTests::random_access_test(
        buffer.data(), buffer_size, 0, buffer_size, 10000, false, stop_flag
    );
    auto end = std::chrono::high_resolution_clock::now();
    
    double total_seconds = std::chrono::duration<double>(end - start).count();
    
    // Random access is slower but should still have reasonable performance
    ASSERT_TRUE(stats.latency_ns > 0.0);      // Should have measurable latency
    ASSERT_TRUE(stats.latency_ns < 10000.0);  // But not more than 10µs per access
    ASSERT_TRUE(stats.bytes_processed > 0);
    ASSERT_TRUE(total_seconds < 60.0);
    
    std::cout << "Random Access Latency: " << stats.latency_ns << " ns" << std::endl;
}

void test_copy_performance() {
    const size_t buffer_size = 8 * 1024 * 1024; // 8MB
    AlignedBuffer src_buffer(buffer_size, 128);
    AlignedBuffer dst_buffer(buffer_size, 128);
    
    std::atomic<bool> stop_flag{false};
    
    auto start = std::chrono::high_resolution_clock::now();
    auto stats = StandardTests::copy_test(
        src_buffer.data(), dst_buffer.data(), buffer_size, 0, buffer_size, 2, stop_flag
    );
    auto end = std::chrono::high_resolution_clock::now();
    
    double total_seconds = std::chrono::duration<double>(end - start).count();
    
    // Copy involves both read and write, so expect lower bandwidth than pure read/write
    ASSERT_TRUE(stats.bandwidth_gbps > 0.3); // Minimum 0.3 Gb/s
    ASSERT_TRUE(stats.time_seconds > 0.0);
    ASSERT_TRUE(stats.bytes_processed > 0);
    ASSERT_TRUE(total_seconds < 60.0);
    
    std::cout << "Copy: " << stats.bandwidth_gbps << " Gb/s" << std::endl;
}

void test_alignment_performance_impact() {
    const size_t buffer_size = 4 * 1024 * 1024; // 4MB
    
    // Test with different alignments
    AlignedBuffer buffer_64(buffer_size, 64);   // 64-byte aligned
    AlignedBuffer buffer_128(buffer_size, 128); // 128-byte aligned
    
    std::atomic<bool> stop_flag{false};
    
    auto stats_64 = StandardTests::sequential_read_test(
        buffer_64.data(), buffer_size, 0, buffer_size, 2, stop_flag
    );
    
    auto stats_128 = StandardTests::sequential_read_test(
        buffer_128.data(), buffer_size, 0, buffer_size, 2, stop_flag
    );
    
    // Both should have reasonable performance (alignment shouldn't cause huge differences)
    ASSERT_TRUE(stats_64.bandwidth_gbps > 0.5);
    ASSERT_TRUE(stats_128.bandwidth_gbps > 0.5);
    
    // Performance difference shouldn't be more than 10x
    double ratio = std::max(stats_64.bandwidth_gbps, stats_128.bandwidth_gbps) /
                   std::min(stats_64.bandwidth_gbps, stats_128.bandwidth_gbps);
    ASSERT_TRUE(ratio < 10.0);
    
    std::cout << "64-byte aligned: " << stats_64.bandwidth_gbps << " Gb/s" << std::endl;
    std::cout << "128-byte aligned: " << stats_128.bandwidth_gbps << " Gb/s" << std::endl;
}

void test_buffer_size_scaling() {
    // Test that larger buffers don't cause performance to degrade dramatically
    std::vector<size_t> buffer_sizes = {
        1 * 1024 * 1024,   // 1MB
        4 * 1024 * 1024,   // 4MB
        16 * 1024 * 1024   // 16MB
    };
    
    std::atomic<bool> stop_flag{false};
    double min_bandwidth = std::numeric_limits<double>::max();
    double max_bandwidth = 0.0;
    
    for (size_t buffer_size : buffer_sizes) {
        AlignedBuffer buffer(buffer_size, 128);
        
        auto stats = StandardTests::sequential_read_test(
            buffer.data(), buffer_size, 0, buffer_size, 1, stop_flag
        );
        
        ASSERT_TRUE(stats.bandwidth_gbps > 0.1); // Very conservative minimum
        
        min_bandwidth = std::min(min_bandwidth, stats.bandwidth_gbps);
        max_bandwidth = std::max(max_bandwidth, stats.bandwidth_gbps);
        
        std::cout << "Buffer " << (buffer_size / (1024*1024)) << "MB: " 
                  << stats.bandwidth_gbps << " Gb/s" << std::endl;
    }
    
    // Performance shouldn't vary by more than 100x across buffer sizes
    double ratio = max_bandwidth / min_bandwidth;
    ASSERT_TRUE(ratio < 100.0);
}

void test_iteration_consistency() {
    // Test that multiple iterations produce consistent results
    const size_t buffer_size = 8 * 1024 * 1024; // 8MB
    AlignedBuffer buffer(buffer_size, 128);
    
    std::atomic<bool> stop_flag{false};
    std::vector<double> bandwidths;
    
    // Run multiple iterations
    for (int i = 0; i < 3; ++i) {
        auto stats = StandardTests::sequential_read_test(
            buffer.data(), buffer_size, 0, buffer_size, 1, stop_flag
        );
        
        ASSERT_TRUE(stats.bandwidth_gbps > 0.1);
        bandwidths.push_back(stats.bandwidth_gbps);
        std::cout << "Iteration " << (i+1) << ": " << stats.bandwidth_gbps << " Gb/s" << std::endl;
    }
    
    // Results shouldn't vary wildly (within 10x of each other)
    double min_bw = *std::min_element(bandwidths.begin(), bandwidths.end());
    double max_bw = *std::max_element(bandwidths.begin(), bandwidths.end());
    double ratio = max_bw / min_bw;
    
    ASSERT_TRUE(ratio < 10.0);
}

void test_memory_allocation_performance() {
    // Test that buffer allocation doesn't take excessively long
    const size_t buffer_size = 64 * 1024 * 1024; // 64MB
    
    auto start = std::chrono::high_resolution_clock::now();
    AlignedBuffer buffer(buffer_size, 128);
    auto end = std::chrono::high_resolution_clock::now();
    
    double allocation_time = std::chrono::duration<double>(end - start).count();
    
    // Allocation should complete within reasonable time (5 seconds is very generous)
    ASSERT_TRUE(allocation_time < 5.0);
    ASSERT_NOT_NULL(buffer.data());
    ASSERT_TRUE(buffer.is_aligned());
    
    std::cout << "64MB allocation time: " << (allocation_time * 1000) << " ms" << std::endl;
}

int main() {
    TestFramework framework;
    
    std::cout << "=== Performance Regression Tests ===" << std::endl;
    std::cout << "These tests ensure performance doesn't degrade below acceptable thresholds." << std::endl;
    std::cout << std::endl;
    
    TEST_CASE("Sequential read performance", test_sequential_read_performance);
    TEST_CASE("Sequential write performance", test_sequential_write_performance);
    TEST_CASE("Random access performance", test_random_access_performance);
    TEST_CASE("Copy performance", test_copy_performance);
    TEST_CASE("Alignment performance impact", test_alignment_performance_impact);
    TEST_CASE("Buffer size scaling", test_buffer_size_scaling);
    TEST_CASE("Iteration consistency", test_iteration_consistency);
    TEST_CASE("Memory allocation performance", test_memory_allocation_performance);
    
    int result = framework.run_all();
    
    if (result == 0) {
        std::cout << std::endl << "✅ All performance regression tests passed!" << std::endl;
        std::cout << "Performance is within acceptable thresholds." << std::endl;
    } else {
        std::cout << std::endl << "❌ Performance regression detected!" << std::endl;
        std::cout << "Check the failed tests above for performance issues." << std::endl;
    }
    
    return result;
}