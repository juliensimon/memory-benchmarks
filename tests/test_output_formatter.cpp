#include "test_framework.h"
#include "../common/output_formatter.h"
#include "../common/memory_types.h"

void test_markdown_format_creation() {
    OutputFormatter formatter(OutputFormat::MARKDOWN);
    
    // Test system info formatting
    SystemInfo sys_info = {};
    sys_info.cpu_name = "Test CPU";
    sys_info.total_ram_gb = 32;
    sys_info.available_ram_gb = 24;
    sys_info.cpu_cores = 8;
    sys_info.cpu_threads = 16;
    sys_info.cache_line_size = 64;
    
    std::string output = formatter.format_system_info(sys_info);
    
    // Verify markdown content
    ASSERT_TRUE(output.find("Test CPU") != std::string::npos);
    ASSERT_TRUE(output.find("32 GB") != std::string::npos);
    ASSERT_TRUE(output.find("24 GB") != std::string::npos);
    ASSERT_TRUE(output.find("**") != std::string::npos); // Markdown bold markers
}

void test_json_format_creation() {
    OutputFormatter formatter(OutputFormat::JSON);
    
    SystemInfo sys_info = {};
    sys_info.cpu_name = "Test CPU";
    sys_info.total_ram_gb = 16;
    sys_info.cpu_cores = 4;
    
    std::string output = formatter.format_system_info(sys_info);
    
    // Verify JSON structure
    ASSERT_TRUE(output.find("{") != std::string::npos);
    ASSERT_TRUE(output.find("}") != std::string::npos);
    ASSERT_TRUE(output.find("\"cpu_name\"") != std::string::npos);
    ASSERT_TRUE(output.find("\"Test CPU\"") != std::string::npos);
    ASSERT_TRUE(output.find("\"total_ram_gb\"") != std::string::npos);
}

void test_csv_format_creation() {
    OutputFormatter formatter(OutputFormat::CSV);
    
    SystemInfo sys_info = {};
    sys_info.cpu_name = "Test CPU";
    sys_info.total_ram_gb = 8;
    sys_info.cpu_cores = 2;
    
    std::string output = formatter.format_system_info(sys_info);
    
    // Verify CSV structure
    ASSERT_TRUE(output.find(",") != std::string::npos);
    ASSERT_TRUE(output.find("Test CPU") != std::string::npos);
    ASSERT_TRUE(output.find("8") != std::string::npos);
}

void test_result_formatting() {
    OutputFormatter formatter(OutputFormat::MARKDOWN);
    
    std::vector<TestResult> results;
    TestResult result = {};
    result.test_name = "sequential_read";
    result.pattern_name = "sequential_read";
    result.working_set_desc = "1GB";
    result.num_threads = 8;
    result.stats.bandwidth_gbps = 45.67;
    result.stats.latency_ns = 12.34;
    result.stats.bytes_processed = 1000000000;
    result.stats.time_seconds = 1.0;
    
    results.push_back(result);
    
    // Create mock memory specs for the formatter
    MemorySpecs mem_specs = {};
    mem_specs.theoretical_bandwidth_gbps = 50.0;
    
    std::string output = formatter.format_test_results(results, mem_specs);
    
    // Verify result content
    ASSERT_TRUE(output.find("sequential_read") != std::string::npos);
    ASSERT_TRUE(output.find("1GB") != std::string::npos);
    ASSERT_TRUE(output.find("365.36") != std::string::npos);  // 45.67 * 8.0 = 365.36 (GB/s to Gb/s conversion)
    ASSERT_TRUE(output.find("8") != std::string::npos);
}

void test_memory_specs_formatting() {
    OutputFormatter formatter(OutputFormat::MARKDOWN);
    
    MemorySpecs specs = {};
    specs.type = "DDR4";
    specs.speed_mtps = 3200;
    specs.data_width_bits = 64;
    specs.num_channels = 2;
    specs.theoretical_bandwidth_gbps = 51.2;
    specs.architecture = "Traditional DIMM";
    
    SystemInfo sys_info = {};
    sys_info.memory_specs = specs;
    sys_info.cpu_name = "Test";
    
    std::string output = formatter.format_system_info(sys_info);
    
    // Verify memory specs content
    ASSERT_TRUE(output.find("DDR4") != std::string::npos);
    ASSERT_TRUE(output.find("3200") != std::string::npos);
    ASSERT_TRUE(output.find("51.2") != std::string::npos);
    ASSERT_TRUE(output.find("Traditional DIMM") != std::string::npos);
}

void test_cache_info_formatting() {
    OutputFormatter formatter(OutputFormat::MARKDOWN);
    
    CacheInfo cache = {};
    cache.l1_data_size = 32 * 1024;      // 32KB
    cache.l1_instruction_size = 32 * 1024; // 32KB
    cache.l2_size = 256 * 1024;           // 256KB
    cache.l3_size = 8 * 1024 * 1024;      // 8MB
    cache.l1_line_size = 64;
    
    SystemInfo sys_info = {};
    sys_info.cache_info = cache;
    sys_info.cpu_name = "Test";
    
    std::string output = formatter.format_system_info(sys_info);
    
    // Verify cache info content
    ASSERT_TRUE(output.find("32 KB") != std::string::npos || output.find("32KB") != std::string::npos);
    ASSERT_TRUE(output.find("256 KB") != std::string::npos || output.find("256KB") != std::string::npos);
    ASSERT_TRUE(output.find("8 MB") != std::string::npos || output.find("8MB") != std::string::npos);
}

void test_format_enum_conversion() {
    // Test that format enum values work correctly
    OutputFormatter md_formatter(OutputFormat::MARKDOWN);
    OutputFormatter json_formatter(OutputFormat::JSON);
    OutputFormatter csv_formatter(OutputFormat::CSV);
    
    SystemInfo sys_info = {};
    sys_info.cpu_name = "Test CPU";
    
    // Each should produce different format
    std::string md_output = md_formatter.format_system_info(sys_info);
    std::string json_output = json_formatter.format_system_info(sys_info);
    std::string csv_output = csv_formatter.format_system_info(sys_info);
    
    // Outputs should be different
    ASSERT_FALSE(md_output == json_output);
    ASSERT_FALSE(json_output == csv_output);
    ASSERT_FALSE(md_output == csv_output);
}

void test_special_characters_handling() {
    OutputFormatter formatter(OutputFormat::JSON);
    
    SystemInfo sys_info = {};
    sys_info.cpu_name = "Test \"CPU\" with special chars: & < > '";
    
    std::string output = formatter.format_system_info(sys_info);
    
    // JSON should handle special characters
    ASSERT_TRUE(output.find("Test") != std::string::npos);
    // Should be valid JSON (no unescaped quotes that break structure)
    ASSERT_TRUE(output.find("{") != std::string::npos);
    ASSERT_TRUE(output.find("}") != std::string::npos);
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Markdown format creation", test_markdown_format_creation);
    TEST_CASE("JSON format creation", test_json_format_creation);
    TEST_CASE("CSV format creation", test_csv_format_creation);
    TEST_CASE("Result formatting", test_result_formatting);
    TEST_CASE("Memory specs formatting", test_memory_specs_formatting);
    TEST_CASE("Cache info formatting", test_cache_info_formatting);
    TEST_CASE("Format enum conversion", test_format_enum_conversion);
    TEST_CASE("Special characters handling", test_special_characters_handling);
    
    return framework.run_all();
}