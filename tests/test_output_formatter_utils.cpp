#include "test_framework.h"
#include "../common/output_formatter_utils.h"
#include "../common/memory_types.h"
#include <sstream>

using namespace OutputFormatterUtils;

void test_format_basic_system_info() {
    SystemInfo sys_info = {};
    sys_info.cpu_name = "Test CPU";
    sys_info.total_ram_gb = 16;
    sys_info.available_ram_gb = 12;
    sys_info.cpu_cores = 8;
    sys_info.cpu_threads = 16;
    
    std::string result = format_basic_system_info(sys_info);
    
    // Check that all required information is present
    ASSERT_TRUE(result.find("# System Information") != std::string::npos);
    ASSERT_TRUE(result.find("Test CPU") != std::string::npos);
    ASSERT_TRUE(result.find("16 GB") != std::string::npos);
    ASSERT_TRUE(result.find("12 GB") != std::string::npos);
    ASSERT_TRUE(result.find("8 ✓") != std::string::npos);
    ASSERT_TRUE(result.find("16 ✓") != std::string::npos);
    
    // Check markdown formatting
    ASSERT_TRUE(result.find("**CPU:**") != std::string::npos);
    ASSERT_TRUE(result.find("**Total RAM:**") != std::string::npos);
}

void test_format_basic_system_info_edge_cases() {
    // Test with empty CPU name
    SystemInfo sys_info1 = {};
    sys_info1.cpu_name = "";
    sys_info1.total_ram_gb = 0;
    sys_info1.available_ram_gb = 0;
    sys_info1.cpu_cores = 1;
    sys_info1.cpu_threads = 1;
    
    std::string result1 = format_basic_system_info(sys_info1);
    ASSERT_TRUE(result1.find("0 GB") != std::string::npos);
    ASSERT_TRUE(result1.find("1 ✓") != std::string::npos);
    
    // Test with large values
    SystemInfo sys_info2 = {};
    sys_info2.cpu_name = "Very Long CPU Name With Many Words";
    sys_info2.total_ram_gb = 1024;
    sys_info2.available_ram_gb = 900;
    sys_info2.cpu_cores = 64;
    sys_info2.cpu_threads = 128;
    
    std::string result2 = format_basic_system_info(sys_info2);
    ASSERT_TRUE(result2.find("1024 GB") != std::string::npos);
    ASSERT_TRUE(result2.find("64 ✓") != std::string::npos);
    ASSERT_TRUE(result2.find("128 ✓") != std::string::npos);
}

void test_format_memory_specifications_basic() {
    MemorySpecs mem_specs = {};
    mem_specs.architecture = "Apple Silicon";
    mem_specs.type = "LPDDR5";
    mem_specs.speed_mtps = 6400;
    mem_specs.data_width_bits = 64;
    mem_specs.total_width_bits = 64;
    mem_specs.num_channels = 8;
    mem_specs.theoretical_bandwidth_gbps = 51.2;
    mem_specs.is_unified_memory = true;
    mem_specs.data_width_detected = true;
    mem_specs.total_width_detected = true;
    mem_specs.num_channels_detected = true;
    
    std::string result = format_memory_specifications(mem_specs);
    
    ASSERT_TRUE(result.find("## Memory Specifications") != std::string::npos);
    ASSERT_TRUE(result.find("Apple Silicon") != std::string::npos);
    ASSERT_TRUE(result.find("LPDDR5") != std::string::npos);
    ASSERT_TRUE(result.find("6400 MT/s") != std::string::npos);
    ASSERT_TRUE(result.find("64 bits") != std::string::npos);
    ASSERT_TRUE(result.find("51.2 GB/s") != std::string::npos);
    ASSERT_TRUE(result.find("✓") != std::string::npos);
}

void test_format_memory_specifications_virtualized() {
    MemorySpecs mem_specs = {};
    mem_specs.type = "DDR4";
    mem_specs.speed_mtps = 0;
    mem_specs.data_width_bits = 64;
    mem_specs.num_channels = 0;
    mem_specs.theoretical_bandwidth_gbps = -1.0;
    mem_specs.is_unified_memory = false;
    mem_specs.is_virtualized = true;
    mem_specs.data_width_detected = false;
    
    std::string result = format_memory_specifications(mem_specs);
    
    ASSERT_TRUE(result.find("DDR4") != std::string::npos);
    ASSERT_TRUE(result.find("Not available from system APIs") != std::string::npos);
    ASSERT_TRUE(result.find("N/A (virtualized environment") != std::string::npos);
    ASSERT_TRUE(result.find("cannot detect - virtualized environment") != std::string::npos);
}

void test_format_memory_specifications_unknown_speed() {
    MemorySpecs mem_specs = {};
    mem_specs.type = "Unknown";
    mem_specs.speed_mtps = 0;
    mem_specs.theoretical_bandwidth_gbps = 0.0;
    
    std::string result = format_memory_specifications(mem_specs);
    
    ASSERT_TRUE(result.find("Not calculated (speed unknown)") != std::string::npos);
    ASSERT_TRUE(result.find("Not available from system APIs") != std::string::npos);
}

void test_format_cache_information_unified() {
    CacheInfo cache_info = {};
    cache_info.l1_data_size = 65536;      // 64KB
    cache_info.l1_instruction_size = 131072; // 128KB  
    cache_info.l2_size = 4194304;         // 4MB
    cache_info.l3_size = 33554432;        // 32MB
    cache_info.l1_line_size = 128;
    
    MemorySpecs mem_specs = {};
    mem_specs.is_unified_memory = true;
    
    std::string result = format_cache_information(cache_info, mem_specs);
    
    ASSERT_TRUE(result.find("## Cache Information") != std::string::npos);
    ASSERT_TRUE(result.find("64 KB per core") != std::string::npos);
    ASSERT_TRUE(result.find("128 KB per core") != std::string::npos);
    ASSERT_TRUE(result.find("4096 KB shared") != std::string::npos); // L2 shared
    ASSERT_TRUE(result.find("32 MB shared") != std::string::npos);   // SLC
    ASSERT_TRUE(result.find("System Level Cache") != std::string::npos);
    ASSERT_TRUE(result.find("128 bytes") != std::string::npos);
}

void test_format_cache_information_traditional() {
    CacheInfo cache_info = {};
    cache_info.l1_data_size = 32768;      // 32KB
    cache_info.l1_instruction_size = 32768; // 32KB
    cache_info.l2_size = 262144;          // 256KB
    cache_info.l3_size = 8388608;         // 8MB
    cache_info.l1_line_size = 64;
    
    MemorySpecs mem_specs = {};
    mem_specs.is_unified_memory = false;
    
    std::string result = format_cache_information(cache_info, mem_specs);
    
    ASSERT_TRUE(result.find("32 KB per core") != std::string::npos);
    ASSERT_TRUE(result.find("256 KB per core") != std::string::npos); // L2 per core
    ASSERT_TRUE(result.find("8 MB shared") != std::string::npos);      // L3 shared
    ASSERT_TRUE(result.find("L3 Cache") != std::string::npos);
    ASSERT_TRUE(result.find("64 bytes") != std::string::npos);
    
    // Should NOT have System Level Cache
    ASSERT_FALSE(result.find("System Level Cache") != std::string::npos);
}

void test_format_efficiency_display() {
    // Valid efficiency with theoretical bandwidth
    std::string result1 = format_efficiency_display(85.7, 100.0);
    ASSERT_TRUE(result1 == "85.7");
    
    // Efficiency with zero theoretical bandwidth
    std::string result2 = format_efficiency_display(75.0, 0.0);
    ASSERT_TRUE(result2 == "N/A");
    
    // Negative efficiency
    std::string result3 = format_efficiency_display(-1.0, 50.0);
    ASSERT_TRUE(result3 == "N/A");
    
    // Zero efficiency
    std::string result4 = format_efficiency_display(0.0, 100.0);
    ASSERT_TRUE(result4 == "0.0");
    
    // High precision values
    std::string result5 = format_efficiency_display(99.999, 200.0);
    ASSERT_TRUE(result5 == "100.0");
}

void test_is_memory_type_detected() {
    // DDR types should be detected
    ASSERT_TRUE(is_memory_type_detected("DDR4"));
    ASSERT_TRUE(is_memory_type_detected("DDR5"));
    ASSERT_TRUE(is_memory_type_detected("DDR3-1600"));
    
    // LPDDR types should be detected
    ASSERT_TRUE(is_memory_type_detected("LPDDR4"));
    ASSERT_TRUE(is_memory_type_detected("LPDDR5"));
    ASSERT_TRUE(is_memory_type_detected("LPDDR4X"));
    
    // Unknown types should not be detected
    ASSERT_FALSE(is_memory_type_detected("Unknown"));
    ASSERT_FALSE(is_memory_type_detected(""));
    ASSERT_FALSE(is_memory_type_detected("SDRAM"));
    ASSERT_FALSE(is_memory_type_detected("SRAM"));
    
    // Partial matches should work
    ASSERT_TRUE(is_memory_type_detected("High Performance DDR4"));
    ASSERT_TRUE(is_memory_type_detected("Mobile LPDDR5 Memory"));
}

void test_format_memory_specifications_edge_cases() {
    // Test with empty architecture
    MemorySpecs mem_specs1 = {};
    mem_specs1.architecture = "";
    mem_specs1.type = "DDR4";
    
    std::string result1 = format_memory_specifications(mem_specs1);
    ASSERT_FALSE(result1.find("**Architecture:**") != std::string::npos);
    
    // Test with channels not detected but not virtualized
    MemorySpecs mem_specs2 = {};
    mem_specs2.type = "DDR5";
    mem_specs2.num_channels = 4;
    mem_specs2.num_channels_detected = false;
    mem_specs2.is_virtualized = false;
    
    std::string result2 = format_memory_specifications(mem_specs2);
    ASSERT_TRUE(result2.find("not detected from system") != std::string::npos);
}

void test_cache_information_size_calculations() {
    CacheInfo cache_info = {};
    cache_info.l1_data_size = 1024;       // 1KB -> should show as 1 KB
    cache_info.l1_instruction_size = 2048; // 2KB -> should show as 2 KB
    cache_info.l2_size = 1048576;         // 1MB -> should show as 1024 KB
    cache_info.l3_size = 1048576;         // 1MB -> should show as 1 MB
    
    MemorySpecs mem_specs = {};
    mem_specs.is_unified_memory = false;
    
    std::string result = format_cache_information(cache_info, mem_specs);
    
    ASSERT_TRUE(result.find("1 KB per core") != std::string::npos);
    ASSERT_TRUE(result.find("2 KB per core") != std::string::npos);
    ASSERT_TRUE(result.find("1024 KB per core") != std::string::npos);
    ASSERT_TRUE(result.find("1 MB shared") != std::string::npos);
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Format basic system info", test_format_basic_system_info);
    TEST_CASE("Format basic system info edge cases", test_format_basic_system_info_edge_cases);
    TEST_CASE("Format memory specifications basic", test_format_memory_specifications_basic);
    TEST_CASE("Format memory specifications virtualized", test_format_memory_specifications_virtualized);
    TEST_CASE("Format memory specifications unknown speed", test_format_memory_specifications_unknown_speed);
    TEST_CASE("Format cache information unified", test_format_cache_information_unified);
    TEST_CASE("Format cache information traditional", test_format_cache_information_traditional);
    TEST_CASE("Format efficiency display", test_format_efficiency_display);
    TEST_CASE("Is memory type detected", test_is_memory_type_detected);
    TEST_CASE("Format memory specifications edge cases", test_format_memory_specifications_edge_cases);
    TEST_CASE("Cache information size calculations", test_cache_information_size_calculations);
    
    return framework.run_all();
}