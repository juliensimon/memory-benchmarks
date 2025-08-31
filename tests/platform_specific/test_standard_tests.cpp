#include "test_framework.h"
#include "../common/standard_tests.h"
#include "../common/aligned_buffer.h"
#include <atomic>

void test_sequential_read_basic() {
    const size_t buffer_size = 1024 * 1024; // 1MB
    AlignedBuffer buffer(buffer_size, 64);
    
    std::atomic<bool> stop_flag{false};
    auto stats = StandardTests::sequential_read_test(
        buffer.data(), buffer_size, 0, buffer_size, 1, stop_flag
    );
    
    // Verify basic metrics
    ASSERT_TRUE(stats.bytes_processed > 0);
    ASSERT_TRUE(stats.time_seconds > 0.0);
    ASSERT_TRUE(stats.bandwidth_gbps > 0.0);
}

void test_sequential_write_basic() {
    const size_t buffer_size = 1024 * 1024; // 1MB
    AlignedBuffer buffer(buffer_size, 64);
    
    std::atomic<bool> stop_flag{false};
    auto stats = StandardTests::sequential_write_test(
        buffer.data(), buffer_size, 1, 64, stop_flag
    );
    
    // Verify basic metrics
    ASSERT_TRUE(stats.total_bytes > 0);
    ASSERT_TRUE(stats.total_time > 0.0);
    ASSERT_TRUE(stats.bandwidth_gbps > 0.0);
}

void test_random_read_test() {
    const size_t buffer_size = 1024 * 1024; // 1MB
    AlignedBuffer buffer(buffer_size, 64);
    
    std::atomic<bool> stop_flag{false};
    auto stats = StandardTests::random_access_test(
        buffer.data(), buffer_size, 1000, 64, false, stop_flag // read operations
    );
    
    // Verify basic metrics
    ASSERT_TRUE(stats.total_bytes > 0);
    ASSERT_TRUE(stats.total_time > 0.0);
    ASSERT_TRUE(stats.latency_ns > 0.0);
}

void test_random_write_test() {
    const size_t buffer_size = 1024 * 1024; // 1MB
    AlignedBuffer buffer(buffer_size, 64);
    
    std::atomic<bool> stop_flag{false};
    auto stats = StandardTests::random_access_test(
        buffer.data(), buffer_size, 1000, 64, true, stop_flag // write operations
    );
    
    // Verify basic metrics
    ASSERT_TRUE(stats.total_bytes > 0);
    ASSERT_TRUE(stats.total_time > 0.0);
    ASSERT_TRUE(stats.latency_ns > 0.0);
}

void test_copy_test() {
    const size_t buffer_size = 1024 * 1024; // 1MB
    AlignedBuffer src_buffer(buffer_size, 64);
    AlignedBuffer dst_buffer(buffer_size, 64);
    
    std::atomic<bool> stop_flag{false};
    auto stats = StandardTests::copy_test(
        src_buffer.data(), dst_buffer.data(), buffer_size, 1, 64, stop_flag
    );
    
    // Verify basic metrics
    ASSERT_TRUE(stats.total_bytes > 0);
    ASSERT_TRUE(stats.total_time > 0.0);
    ASSERT_TRUE(stats.bandwidth_gbps > 0.0);
}

void test_triad_test() {
    const size_t buffer_size = 1024 * 1024; // 1MB
    AlignedBuffer a_buffer(buffer_size, 64);
    AlignedBuffer b_buffer(buffer_size, 64);
    AlignedBuffer c_buffer(buffer_size, 64);
    
    std::atomic<bool> stop_flag{false};
    auto stats = StandardTests::triad_test(
        a_buffer.data(), b_buffer.data(), c_buffer.data(), 
        buffer_size, 1, 64, stop_flag
    );
    
    // Verify basic metrics
    ASSERT_TRUE(stats.total_bytes > 0);
    ASSERT_TRUE(stats.total_time > 0.0);
    ASSERT_TRUE(stats.bandwidth_gbps > 0.0);
}

void test_stop_flag_functionality() {
    const size_t buffer_size = 1024 * 1024; // 1MB
    AlignedBuffer buffer(buffer_size, 64);
    
    std::atomic<bool> stop_flag{true}; // Set to true immediately
    auto stats = StandardTests::sequential_read_test(
        buffer.data(), buffer_size, 100000, 64, stop_flag // Many iterations
    );
    
    // Should complete very quickly due to stop flag
    ASSERT_TRUE(stats.total_time < 1.0); // Less than 1 second
}

void test_cache_line_alignment() {
    const size_t buffer_size = 1024 * 1024; // 1MB
    AlignedBuffer buffer(buffer_size, 128); // 128-byte alignment
    
    std::atomic<bool> stop_flag{false};
    auto stats = StandardTests::sequential_read_test(
        buffer.data(), buffer_size, 1, 128, stop_flag
    );
    
    // Should work with different cache line sizes
    ASSERT_TRUE(stats.total_bytes > 0);
    ASSERT_TRUE(stats.bandwidth_gbps > 0.0);
}

void test_performance_stats_calculation() {
    // Test performance stats with known values
    const size_t total_bytes = 1024 * 1024 * 1024; // 1GB
    const double total_time = 1.0; // 1 second
    const size_t operations = 1000000; // 1M operations
    
    // These values should produce predictable results
    // 1GB in 1 second = 8 Gbps
    // 1M operations in 1 second = 1M ops/sec
    // Average latency = 1 second / 1M operations = 1000 ns
    
    PerformanceStats stats;
    stats.total_bytes = total_bytes;
    stats.total_time = total_time;
    stats.operations_per_second = operations / total_time;
    stats.bandwidth_gbps = (total_bytes * 8.0) / (total_time * 1e9);
    stats.latency_ns = (total_time * 1e9) / operations;
    
    // Verify calculations
    TestAssert::assert_equal(8.0, stats.bandwidth_gbps); // 8 Gbps
    TestAssert::assert_equal(1000000.0, stats.operations_per_second); // 1M ops/sec
    TestAssert::assert_equal(1000.0, stats.latency_ns); // 1000 ns
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Sequential read basic", test_sequential_read_basic);
    TEST_CASE("Sequential write basic", test_sequential_write_basic);
    TEST_CASE("Random read test", test_random_read_test);
    TEST_CASE("Random write test", test_random_write_test);
    TEST_CASE("Copy test", test_copy_test);
    TEST_CASE("Triad test", test_triad_test);
    TEST_CASE("Stop flag functionality", test_stop_flag_functionality);
    TEST_CASE("Cache line alignment", test_cache_line_alignment);
    TEST_CASE("Performance stats calculation", test_performance_stats_calculation);
    
    return framework.run_all();
}