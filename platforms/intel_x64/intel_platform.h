#ifndef INTEL_PLATFORM_H
#define INTEL_PLATFORM_H

#include "../../common/platform_interface.h"
#include "../../common/memory_types.h"

/**
 * @brief Intel x64-specific platform implementation
 *
 * Provides Intel x64-specific implementations for system detection
 * and standard CPU affinity support.
 */
class IntelPlatform : public PlatformInterface {
public:
    // System detection methods
    std::pair<std::string, std::string> detect_processor_info() override;
    size_t detect_cache_line_size() override;
    CacheInfo detect_cache_info() override;
    CacheInfo get_core_specific_cache_info(CPUAffinityType affinity_type) override;
    MemorySpecs get_memory_specs() override;
    SystemInfo get_system_info() override;

    // CPU affinity methods
    size_t get_max_threads_for_affinity(CPUAffinityType affinity_type) override;
    void set_thread_affinity(size_t thread_id, CPUAffinityType affinity_type, size_t total_threads) override;
    bool validate_thread_count(size_t num_threads, CPUAffinityType affinity_type, std::string& error_msg) override;
    
    // Platform identification
    std::string get_platform_name() override { return "Intel x64"; }
    bool supports_cpu_affinity() override;

private:
    void detect_cache_associativity(CacheInfo& info);
    bool detect_virtualization();
};

#endif  // INTEL_PLATFORM_H