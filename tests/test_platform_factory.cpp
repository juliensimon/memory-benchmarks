#include "test_framework.h"
#include "../common/platform_interface.h"
#include <thread>
std::unique_ptr<PlatformInterface> create_platform_interface();

void test_platform_creation() {
    auto platform = create_platform_interface();
    ASSERT_NOT_NULL(platform.get());
}

void test_platform_basic_functionality() {
    auto platform = create_platform_interface();
    
    // Test basic interface methods
    std::string platform_name = platform->get_platform_name();
    ASSERT_FALSE(platform_name.empty());
    
    // Test processor info detection (allow missing info on some platforms)
    auto [arch, model] = platform->detect_processor_info();
    // At least one should be available, but both may not always be detectable
    ASSERT_TRUE(!arch.empty() || !model.empty());
    
    // Test cache line size detection
    size_t cache_line_size = platform->detect_cache_line_size();
    ASSERT_TRUE(cache_line_size > 0);
    ASSERT_TRUE(cache_line_size <= 1024);  // Reasonable upper bound
    
    // Test cache info detection
    CacheInfo cache_info = platform->detect_cache_info();
    ASSERT_TRUE(cache_info.l1_data_size > 0);
    ASSERT_TRUE(cache_info.l1_instruction_size > 0);
    ASSERT_TRUE(cache_info.l2_size > 0);
}

void test_memory_specs() {
    auto platform = create_platform_interface();
    
    MemorySpecs specs = platform->get_memory_specs();
    ASSERT_TRUE(specs.total_size_gb > 0);
    ASSERT_FALSE(specs.type.empty());
    ASSERT_TRUE(specs.speed_mtps > 0);
    ASSERT_TRUE(specs.data_width_bits > 0);
}

void test_system_info() {
    auto platform = create_platform_interface();
    
    SystemInfo info = platform->get_system_info();
    ASSERT_TRUE(info.total_ram_gb > 0);
    ASSERT_TRUE(info.cpu_cores > 0);
    ASSERT_TRUE(info.cpu_threads > 0);
    ASSERT_FALSE(info.cpu_name.empty());
    ASSERT_TRUE(info.cache_line_size > 0);
}

void test_matrix_multiplier_creation() {
    auto platform = create_platform_interface();
    
    auto multiplier = platform->create_matrix_multiplier();
    ASSERT_NOT_NULL(multiplier.get());
    ASSERT_TRUE(multiplier->is_available());
    ASSERT_FALSE(multiplier->get_acceleration_name().empty());
}

void test_thread_validation() {
    auto platform = create_platform_interface();
    
    std::string error_msg;
    
    // Test valid thread counts for DEFAULT (should always pass)
    ASSERT_TRUE(platform->validate_thread_count(1, CPUAffinityType::DEFAULT, error_msg));
    ASSERT_TRUE(platform->validate_thread_count(4, CPUAffinityType::DEFAULT, error_msg));
    ASSERT_TRUE(platform->validate_thread_count(100, CPUAffinityType::DEFAULT, error_msg));
    
    // Test platform-specific affinity validation (machine-dependent behavior)
    // P-cores should have some reasonable limit
    size_t max_p_threads = platform->get_max_threads_for_affinity(CPUAffinityType::P_CORES);
    if (max_p_threads > 0) {
        ASSERT_TRUE(platform->validate_thread_count(1, CPUAffinityType::P_CORES, error_msg));
        ASSERT_FALSE(platform->validate_thread_count(max_p_threads + 10, CPUAffinityType::P_CORES, error_msg));
        ASSERT_FALSE(error_msg.empty());
    }
}

void test_cache_line_alignment() {
    auto platform = create_platform_interface();
    
    size_t cache_line_size = platform->detect_cache_line_size();
    
    // Test that cache line size is a power of 2
    ASSERT_TRUE((cache_line_size & (cache_line_size - 1)) == 0);
    
    // Test reasonable range
    ASSERT_TRUE(cache_line_size >= 16);  // Minimum reasonable cache line size
    ASSERT_TRUE(cache_line_size <= 1024);  // Maximum reasonable cache line size
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Platform creation", test_platform_creation);
    TEST_CASE("Platform basic functionality", test_platform_basic_functionality);
    TEST_CASE("Memory specs", test_memory_specs);
    TEST_CASE("System info", test_system_info);
    TEST_CASE("Matrix multiplier creation", test_matrix_multiplier_creation);
    TEST_CASE("Thread validation", test_thread_validation);
    TEST_CASE("Cache line alignment", test_cache_line_alignment);
    
    return framework.run_all();
}