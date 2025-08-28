#ifndef PLATFORM_INTERFACE_H
#define PLATFORM_INTERFACE_H

#include "memory_types.h"
#include <string>
#include <utility>
#include <memory>

/**
 * @brief Abstract interface for platform-specific implementations
 *
 * This interface defines the methods that each platform must implement
 * to provide system-specific memory and CPU information.
 */
class PlatformInterface {
public:
    virtual ~PlatformInterface() = default;

    // System detection methods
    virtual std::pair<std::string, std::string> detect_processor_info() = 0;
    virtual size_t detect_cache_line_size() = 0;
    virtual CacheInfo detect_cache_info() = 0;
    virtual CacheInfo get_core_specific_cache_info(CPUAffinityType affinity_type) = 0;
    virtual MemorySpecs get_memory_specs() = 0;
    virtual SystemInfo get_system_info() = 0;

    // CPU affinity methods
    virtual size_t get_max_threads_for_affinity(CPUAffinityType affinity_type) = 0;
    virtual void set_thread_affinity(size_t thread_id, CPUAffinityType affinity_type, size_t total_threads) = 0;
    virtual bool validate_thread_count(size_t num_threads, CPUAffinityType affinity_type, std::string& error_msg) = 0;
    
    // Platform identification
    virtual std::string get_platform_name() = 0;
    virtual bool supports_cpu_affinity() = 0;
};

/**
 * @brief Factory function to create platform-specific implementation
 * @return Pointer to platform-specific implementation
 */
std::unique_ptr<PlatformInterface> create_platform_interface();

#endif  // PLATFORM_INTERFACE_H