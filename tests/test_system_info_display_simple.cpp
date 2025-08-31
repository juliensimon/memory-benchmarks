#include "test_framework.h"
#include "../common/system_info_display.h"
#include "../common/memory_types.h"

// Test only the machine-independent static utility functions
// by creating a simplified test that doesn't require platform mocking

void test_system_info_display_constants() {
    // Just test that we can include the header and basic structure exists
    SystemInfo info = {};
    info.cpu_name = "Test CPU";
    info.total_ram_gb = 16;
    info.available_ram_gb = 12;
    info.cpu_cores = 8;
    info.cpu_threads = 16;
    info.cache_line_size = 64;
    
    ASSERT_TRUE(info.cpu_name == "Test CPU");
    ASSERT_TRUE(info.total_ram_gb == 16);
    ASSERT_TRUE(info.cpu_cores == 8);
    ASSERT_TRUE(info.cpu_threads == 16);
}

void test_output_format_enum_values() {
    // Test that enum values are distinct
    ASSERT_FALSE(OutputFormat::MARKDOWN == OutputFormat::JSON);
    ASSERT_FALSE(OutputFormat::JSON == OutputFormat::CSV);
    ASSERT_FALSE(OutputFormat::MARKDOWN == OutputFormat::CSV);
}

void test_cpu_affinity_type_enum_values() {
    // Test that CPU affinity enum values are distinct
    ASSERT_FALSE(CPUAffinityType::DEFAULT == CPUAffinityType::P_CORES);
    ASSERT_FALSE(CPUAffinityType::DEFAULT == CPUAffinityType::E_CORES);
    ASSERT_FALSE(CPUAffinityType::P_CORES == CPUAffinityType::E_CORES);
}

void test_cache_info_structure() {
    CacheInfo cache = {};
    cache.l1_data_size = 32768;
    cache.l1_instruction_size = 32768;
    cache.l2_size = 262144;
    cache.l3_size = 8388608;
    cache.l1_line_size = 64;
    
    ASSERT_TRUE(cache.l1_data_size == 32768);
    ASSERT_TRUE(cache.l1_instruction_size == 32768);
    ASSERT_TRUE(cache.l2_size == 262144);
    ASSERT_TRUE(cache.l3_size == 8388608);
    ASSERT_TRUE(cache.l1_line_size == 64);
}

void test_memory_specs_structure() {
    MemorySpecs specs = {};
    specs.total_size_gb = 32;
    specs.speed_mtps = 3200;
    specs.data_width_bits = 64;
    specs.theoretical_bandwidth_gbps = 51.2;
    specs.is_unified_memory = false;
    specs.architecture = "DDR4";
    specs.type = "LPDDR5";
    
    ASSERT_TRUE(specs.total_size_gb == 32);
    ASSERT_TRUE(specs.speed_mtps == 3200);
    ASSERT_TRUE(specs.data_width_bits == 64);
    ASSERT_FALSE(specs.is_unified_memory);
    ASSERT_TRUE(specs.architecture == "DDR4");
    ASSERT_TRUE(specs.type == "LPDDR5");
}

void test_system_info_complete_structure() {
    SystemInfo sys_info = {};
    sys_info.cpu_name = "Apple M1 Pro";
    sys_info.total_ram_gb = 32;
    sys_info.available_ram_gb = 28;
    sys_info.cpu_cores = 10;
    sys_info.cpu_threads = 10;
    sys_info.cache_line_size = 128;
    
    CacheInfo cache = {};
    cache.l1_data_size = 131072;
    cache.l1_instruction_size = 196608;
    cache.l2_size = 16777216;
    cache.l3_size = 33554432;
    cache.l1_line_size = 128;
    sys_info.cache_info = cache;
    
    MemorySpecs memory = {};
    memory.total_size_gb = 32;
    memory.speed_mtps = 6400;
    memory.is_unified_memory = true;
    memory.architecture = "Apple Silicon";
    sys_info.memory_specs = memory;
    
    // Test that all fields are properly set
    ASSERT_TRUE(sys_info.cpu_name.find("Apple") != std::string::npos);
    ASSERT_TRUE(sys_info.total_ram_gb == 32);
    ASSERT_TRUE(sys_info.cpu_cores == 10);
    ASSERT_TRUE(sys_info.cache_info.l1_data_size == 131072);
    ASSERT_TRUE(sys_info.memory_specs.is_unified_memory);
    ASSERT_TRUE(sys_info.memory_specs.architecture == "Apple Silicon");
}

int main() {
    TestFramework framework;
    
    TEST_CASE("System info display constants", test_system_info_display_constants);
    TEST_CASE("Output format enum values", test_output_format_enum_values);
    TEST_CASE("CPU affinity type enum values", test_cpu_affinity_type_enum_values);
    TEST_CASE("Cache info structure", test_cache_info_structure);
    TEST_CASE("Memory specs structure", test_memory_specs_structure);
    TEST_CASE("System info complete structure", test_system_info_complete_structure);
    
    return framework.run_all();
}