#include "test_framework.h"
#include "../common/memory_types.h"
#include "../common/constants.h"
#include "../common/working_sets.h"

void test_memory_types_structures() {
    // Test CacheInfo structure
    CacheInfo cache = {};
    cache.l1_data_size = 32768;
    cache.l2_size = 262144;
    cache.l3_size = 8388608;
    
    ASSERT_TRUE(cache.l1_data_size == 32768);
    ASSERT_TRUE(cache.l2_size == 262144);
    ASSERT_TRUE(cache.l3_size == 8388608);
}

void test_memory_specs_structure() {
    MemorySpecs specs = {};
    specs.total_size_gb = 32;
    specs.speed_mtps = 3200;
    specs.data_width_bits = 64;
    specs.theoretical_bandwidth_gbps = 51.2;
    specs.is_unified_memory = false;
    
    ASSERT_TRUE(specs.total_size_gb == 32);
    ASSERT_TRUE(specs.speed_mtps == 3200);
    ASSERT_FALSE(specs.is_unified_memory);
}

void test_system_info_structure() {
    SystemInfo info = {};
    info.total_ram_gb = 16;
    info.available_ram_gb = 12;
    info.cpu_cores = 8;
    info.cpu_threads = 16;
    info.cache_line_size = 64;
    info.cpu_name = "Test CPU";
    
    ASSERT_TRUE(info.total_ram_gb == 16);
    ASSERT_TRUE(info.cpu_cores == 8);
    ASSERT_TRUE(info.cpu_threads == 16);
    TestAssert::assert_equal(std::string("Test CPU"), info.cpu_name);
}

void test_benchmark_constants() {
    // Test that constants are reasonable values
    ASSERT_TRUE(BenchmarkConstants::DEFAULT_ITERATIONS > 0);
    ASSERT_TRUE(BenchmarkConstants::MAX_MEMORY_SIZE_GB > 0);
    ASSERT_TRUE(BenchmarkConstants::MAX_THREAD_OVERSUBSCRIPTION > 1);
}

void test_cache_constants() {
    // Test cache line constants
    ASSERT_TRUE(CacheConstants::INTEL_CACHE_LINE_SIZE == 64);
    ASSERT_TRUE(CacheConstants::APPLE_CACHE_LINE_SIZE == 128);
    ASSERT_TRUE(CacheConstants::ARM_CACHE_LINE_SIZE == 64);
}

void test_working_sets_functionality() {
    // Test working set calculation
    CacheInfo cache = {};
    cache.l1_data_size = 32 * 1024;     // 32KB
    cache.l2_size = 256 * 1024;         // 256KB  
    cache.l3_size = 8 * 1024 * 1024;    // 8MB
    cache.l1_line_size = 64;
    
    auto [sizes, descriptions] = WorkingSetSizes::get_thread_aware_sizes(cache, 4);
    
    // Should have multiple working set sizes
    ASSERT_TRUE(sizes.size() > 0);
    ASSERT_TRUE(descriptions.size() == sizes.size());
    
    // Sizes should be reasonable (in bytes)
    for (size_t size : sizes) {
        ASSERT_TRUE(size > 1024);  // At least 1KB
        ASSERT_TRUE(size < 1024 * 1024 * 1024); // Less than 1GB
    }
}

void test_cpu_affinity_types() {
    // Test that CPU affinity enum values are distinct
    ASSERT_FALSE(CPUAffinityType::DEFAULT == CPUAffinityType::P_CORES);
    ASSERT_FALSE(CPUAffinityType::DEFAULT == CPUAffinityType::E_CORES);
    ASSERT_FALSE(CPUAffinityType::P_CORES == CPUAffinityType::E_CORES);
}

void test_output_format_types() {
    // Test output format enum values
    ASSERT_FALSE(OutputFormat::MARKDOWN == OutputFormat::JSON);
    ASSERT_FALSE(OutputFormat::JSON == OutputFormat::CSV);
    ASSERT_FALSE(OutputFormat::MARKDOWN == OutputFormat::CSV);
}

void test_test_pattern_types() {
    // Test test pattern enum values
    ASSERT_FALSE(TestPattern::SEQUENTIAL_READ == TestPattern::SEQUENTIAL_WRITE);
    ASSERT_FALSE(TestPattern::RANDOM_READ == TestPattern::RANDOM_WRITE);
    ASSERT_FALSE(TestPattern::COPY == TestPattern::TRIAD);
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Memory types structures", test_memory_types_structures);
    TEST_CASE("Memory specs structure", test_memory_specs_structure);
    TEST_CASE("System info structure", test_system_info_structure);
    TEST_CASE("Benchmark constants", test_benchmark_constants);
    TEST_CASE("Cache constants", test_cache_constants);
    TEST_CASE("Working sets functionality", test_working_sets_functionality);
    TEST_CASE("CPU affinity types", test_cpu_affinity_types);
    TEST_CASE("Output format types", test_output_format_types);
    TEST_CASE("Test pattern types", test_test_pattern_types);
    
    return framework.run_all();
}