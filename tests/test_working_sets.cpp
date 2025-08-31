#include "test_framework.h"
#include "../common/working_sets.h"
#include "../common/constants.h"

using namespace BenchmarkConstants;

void test_working_set_constructor_basic() {
    CacheInfo cache = {};
    cache.l1_data_size = 32768;      // 32KB
    cache.l1_instruction_size = 32768;
    cache.l2_size = 262144;          // 256KB
    cache.l3_size = 8388608;         // 8MB
    cache.l1_line_size = 64;
    
    WorkingSetSizes working_sets(cache);
    
    ASSERT_TRUE(working_sets.sizes.size() > 0);
    ASSERT_TRUE(working_sets.descriptions.size() == working_sets.sizes.size());
    
    // Check that L1, L2, and L3 fractions are present
    bool found_l1 = false, found_l2 = false, found_l3 = false;
    
    for (const auto& desc : working_sets.descriptions) {
        if (desc.find("L1") != std::string::npos) found_l1 = true;
        if (desc.find("L2") != std::string::npos) found_l2 = true;
        if (desc.find("SLC") != std::string::npos) found_l3 = true;
    }
    
    ASSERT_TRUE(found_l1);
    ASSERT_TRUE(found_l2);
    ASSERT_TRUE(found_l3);
}

void test_working_set_size_filtering() {
    CacheInfo cache = {};
    cache.l1_data_size = 1024;       // 1KB - very small, some fractions will be filtered
    cache.l2_size = 4096;            // 4KB
    cache.l3_size = 16384;           // 16KB
    
    WorkingSetSizes working_sets(cache);
    
    // All sizes should be within bounds
    for (size_t size : working_sets.sizes) {
        ASSERT_TRUE(size >= MIN_WORKING_SET_SIZE);
        ASSERT_TRUE(size <= MAX_WORKING_SET_SIZE);
    }
}

void test_working_set_large_cache() {
    CacheInfo cache = {};
    cache.l1_data_size = 131072;     // 128KB
    cache.l2_size = 16777216;        // 16MB
    cache.l3_size = 67108864;        // 64MB
    
    WorkingSetSizes working_sets(cache);
    
    ASSERT_TRUE(working_sets.sizes.size() > 10); // Should have many working sets
    
    // Check that we have standard sizes
    bool found_64mb = false, found_1gb = false;
    for (const auto& desc : working_sets.descriptions) {
        if (desc.find("64MB") != std::string::npos) found_64mb = true;
        if (desc.find("1GB") != std::string::npos) found_1gb = true;
    }
    ASSERT_TRUE(found_64mb);
    ASSERT_TRUE(found_1gb);
}

void test_get_thread_aware_sizes_single_thread() {
    CacheInfo cache = {};
    cache.l1_data_size = 65536;      // 64KB
    cache.l2_size = 4194304;         // 4MB
    cache.l3_size = 33554432;        // 32MB
    
    auto [sizes, descriptions] = WorkingSetSizes::get_thread_aware_sizes(cache, 1);
    
    ASSERT_TRUE(sizes.size() > 0);
    ASSERT_TRUE(descriptions.size() == sizes.size());
    
    // With 1 thread, L3 per thread should equal full L3
    bool found_slc_per_thread = false;
    for (size_t i = 0; i < descriptions.size(); ++i) {
        if (descriptions[i] == "SLC per thread") {
            ASSERT_TRUE(sizes[i] == cache.l3_size);
            found_slc_per_thread = true;
        }
    }
    ASSERT_TRUE(found_slc_per_thread);
}

void test_get_thread_aware_sizes_multi_thread() {
    CacheInfo cache = {};
    cache.l1_data_size = 65536;      // 64KB
    cache.l2_size = 4194304;         // 4MB  
    cache.l3_size = 33554432;        // 32MB
    
    size_t num_threads = 8;
    auto [sizes, descriptions] = WorkingSetSizes::get_thread_aware_sizes(cache, num_threads);
    
    // L3 per thread should be divided by number of threads
    bool found_slc_per_thread = false;
    for (size_t i = 0; i < descriptions.size(); ++i) {
        if (descriptions[i] == "SLC per thread") {
            ASSERT_TRUE(sizes[i] == cache.l3_size / num_threads);
            found_slc_per_thread = true;
        }
    }
    ASSERT_TRUE(found_slc_per_thread);
    
    // L1 and L2 should be per-thread (not divided)
    bool found_l1_per_thread = false;
    for (size_t i = 0; i < descriptions.size(); ++i) {
        if (descriptions[i] == "L1 per thread") {
            ASSERT_TRUE(sizes[i] == cache.l1_data_size);
            found_l1_per_thread = true;
        }
    }
    ASSERT_TRUE(found_l1_per_thread);
}

void test_get_thread_aware_sizes_filtering() {
    CacheInfo cache = {};
    cache.l1_data_size = 512;        // Very small L1 - fractions will be filtered
    cache.l2_size = 2048;            // Small L2
    cache.l3_size = 8192;            // Small L3
    
    auto [sizes, descriptions] = WorkingSetSizes::get_thread_aware_sizes(cache, 4);
    
    // All sizes should meet minimum requirement
    for (size_t size : sizes) {
        ASSERT_TRUE(size >= MIN_WORKING_SET_SIZE);
        ASSERT_TRUE(size <= MAX_WORKING_SET_SIZE);
    }
    
    // Should still have some standard sizes
    bool found_standard = false;
    for (const auto& desc : descriptions) {
        if (desc.find("64MB") != std::string::npos || 
            desc.find("256MB") != std::string::npos ||
            desc.find("1GB") != std::string::npos) {
            found_standard = true;
        }
    }
    ASSERT_TRUE(found_standard);
}

void test_thread_aware_beyond_cache_sizes() {
    CacheInfo cache = {};
    cache.l1_data_size = 65536;
    cache.l2_size = 4194304;
    cache.l3_size = 33554432;        // 32MB
    
    auto [sizes, descriptions] = WorkingSetSizes::get_thread_aware_sizes(cache, 2);
    
    // Should include multiples of L3
    bool found_2x_slc = false, found_4x_slc = false;
    for (const auto& desc : descriptions) {
        if (desc == "2x SLC") found_2x_slc = true;
        if (desc == "4x SLC") found_4x_slc = true;
    }
    ASSERT_TRUE(found_2x_slc);
    ASSERT_TRUE(found_4x_slc);
    
    // Should include standard large sizes
    bool found_1gb = false, found_2gb = false;
    for (const auto& desc : descriptions) {
        if (desc == "1GB") found_1gb = true;
        if (desc == "2GB") found_2gb = true;
    }
    ASSERT_TRUE(found_1gb);
    ASSERT_TRUE(found_2gb);
}

void test_working_set_fractions() {
    CacheInfo cache = {};
    cache.l1_data_size = 131072;     // 128KB
    cache.l2_size = 16777216;        // 16MB
    cache.l3_size = 67108864;        // 64MB
    
    auto [sizes, descriptions] = WorkingSetSizes::get_thread_aware_sizes(cache, 1);
    
    // Test L1 fractions
    bool found_quarter_l1 = false, found_half_l1 = false;
    for (size_t i = 0; i < descriptions.size(); ++i) {
        if (descriptions[i] == "1/4 L1 per thread") {
            ASSERT_TRUE(sizes[i] == cache.l1_data_size / 4);
            found_quarter_l1 = true;
        }
        if (descriptions[i] == "1/2 L1 per thread") {
            ASSERT_TRUE(sizes[i] == cache.l1_data_size / 2);
            found_half_l1 = true;
        }
    }
    ASSERT_TRUE(found_quarter_l1);
    ASSERT_TRUE(found_half_l1);
}

void test_edge_cases_zero_cache() {
    CacheInfo cache = {};
    cache.l1_data_size = 0;
    cache.l2_size = 0;
    cache.l3_size = 0;
    
    // Should not crash and should provide standard working sets
    auto [sizes, descriptions] = WorkingSetSizes::get_thread_aware_sizes(cache, 1);
    
    // Should still have some working sets from standard sizes
    ASSERT_TRUE(sizes.size() > 0);
    
    bool found_standard = false;
    for (const auto& desc : descriptions) {
        if (desc.find("64MB") != std::string::npos || desc.find("1GB") != std::string::npos) {
            found_standard = true;
        }
    }
    ASSERT_TRUE(found_standard);
}

void test_edge_cases_huge_cache() {
    CacheInfo cache = {};
    cache.l1_data_size = 1048576;    // 1MB L1 (unrealistic)
    cache.l2_size = 67108864;        // 64MB L2
    cache.l3_size = 1073741824;      // 1GB L3
    
    WorkingSetSizes working_sets(cache);
    
    // Should handle large caches without issues
    ASSERT_TRUE(working_sets.sizes.size() > 0);
    
    // All sizes should be within reasonable bounds
    for (size_t size : working_sets.sizes) {
        ASSERT_TRUE(size <= MAX_WORKING_SET_SIZE);
    }
}

void test_consistency_between_constructors() {
    CacheInfo cache = {};
    cache.l1_data_size = 65536;
    cache.l2_size = 4194304;
    cache.l3_size = 33554432;
    
    // Test that both construction methods produce valid results
    WorkingSetSizes ws1(cache);
    auto [sizes2, descriptions2] = WorkingSetSizes::get_thread_aware_sizes(cache, 1);
    
    ASSERT_TRUE(ws1.sizes.size() > 0);
    ASSERT_TRUE(sizes2.size() > 0);
    
    // Both should have descriptions matching their sizes
    ASSERT_TRUE(ws1.descriptions.size() == ws1.sizes.size());
    ASSERT_TRUE(descriptions2.size() == sizes2.size());
}

void test_working_set_ordering() {
    CacheInfo cache = {};
    cache.l1_data_size = 65536;
    cache.l2_size = 4194304;
    cache.l3_size = 33554432;
    
    auto [sizes, descriptions] = WorkingSetSizes::get_thread_aware_sizes(cache, 4);
    
    // Sizes don't need to be strictly ordered, but should all be valid
    for (size_t i = 0; i < sizes.size(); ++i) {
        ASSERT_TRUE(sizes[i] > 0);
        ASSERT_FALSE(descriptions[i].empty());
    }
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Working set constructor basic", test_working_set_constructor_basic);
    TEST_CASE("Working set size filtering", test_working_set_size_filtering);
    TEST_CASE("Working set large cache", test_working_set_large_cache);
    TEST_CASE("Get thread aware sizes single thread", test_get_thread_aware_sizes_single_thread);
    TEST_CASE("Get thread aware sizes multi thread", test_get_thread_aware_sizes_multi_thread);
    TEST_CASE("Get thread aware sizes filtering", test_get_thread_aware_sizes_filtering);
    TEST_CASE("Thread aware beyond cache sizes", test_thread_aware_beyond_cache_sizes);
    TEST_CASE("Working set fractions", test_working_set_fractions);
    TEST_CASE("Edge cases zero cache", test_edge_cases_zero_cache);
    TEST_CASE("Edge cases huge cache", test_edge_cases_huge_cache);
    TEST_CASE("Consistency between constructors", test_consistency_between_constructors);
    TEST_CASE("Working set ordering", test_working_set_ordering);
    
    return framework.run_all();
}