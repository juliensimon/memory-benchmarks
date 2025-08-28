#include "macos_platform.h"
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/mach_types.h>
#include <mach/vm_statistics.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <pthread.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>

// Use centralized cache line size constants
using CacheConstants::APPLE_CACHE_LINE_SIZE;

std::pair<std::string, std::string> MacOSPlatform::detect_processor_info() {
    std::string arch = "";
    std::string model = "";
    
    // macOS processor detection
    FILE* chip_pipe = popen("sysctl -n machdep.cpu.brand_string 2>/dev/null", "r");
    if (chip_pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), chip_pipe) != nullptr) {
            model = std::string(buffer);
            // Remove newline
            if (!model.empty() && model.back() == '\n') {
                model.pop_back();
            }
        }
        pclose(chip_pipe);
    }
    
    return std::make_pair(arch, model);
}

size_t MacOSPlatform::detect_cache_line_size() {
    size_t cache_line_size = 0;
    size_t size = sizeof(cache_line_size);
    
    // Try to get cache line size from sysctl
    if (sysctlbyname("hw.cachelinesize", &cache_line_size, &size, nullptr, 0) == 0) {
        if (cache_line_size > 0 && cache_line_size <= 1024) {
            return cache_line_size;
        }
    }
    
    // Fallback to Apple Silicon default
    return APPLE_CACHE_LINE_SIZE;
}

CacheInfo MacOSPlatform::detect_cache_info() {
    // Detect cache line size first
    size_t cache_line_size = detect_cache_line_size();

    // Initialize with Apple Silicon defaults
    CacheInfo info = {
        64 * 1024,        // 64KB L1 data cache (E-core default)
        128 * 1024,       // 128KB L1 instruction cache (E-core default)
        4 * 1024 * 1024,  // 4MB L2 cache (E-core default)
        28 * 1024 * 1024, // 28MB System Level Cache
        8, 8, 8, 16,      // Associativity defaults
        cache_line_size,
        cache_line_size,
        cache_line_size
    };

    // macOS sysctl-based detection
    size_t size;
    uint64_t cache_size;

    // Try to get L1 data cache size
    size = sizeof(cache_size);
    if (sysctlbyname("hw.l1dcachesize", &cache_size, &size, nullptr, 0) == 0) {
        info.l1_data_size = cache_size;
    }

    // Try to get L1 instruction cache size
    size = sizeof(cache_size);
    if (sysctlbyname("hw.l1icachesize", &cache_size, &size, nullptr, 0) == 0) {
        info.l1_instruction_size = cache_size;
    }

    // Try to get L2 cache size
    size = sizeof(cache_size);
    if (sysctlbyname("hw.l2cachesize", &cache_size, &size, nullptr, 0) == 0) {
        info.l2_size = cache_size;
    }

    // Apple Silicon has System Level Cache (SLC) instead of traditional L3
    // Detect chip model to set appropriate SLC size
    std::string chip_model = "";
    FILE* chip_pipe = popen("sysctl -n machdep.cpu.brand_string 2>/dev/null", "r");
    if (chip_pipe) {
        char chip_buffer[256];
        if (fgets(chip_buffer, sizeof(chip_buffer), chip_pipe) != nullptr) {
            chip_model = std::string(chip_buffer);
            if (!chip_model.empty() && chip_model.back() == '\n') {
                chip_model.pop_back();
            }
        }
        pclose(chip_pipe);
    }

    // Set System Level Cache (SLC) size based on chip model
    if (chip_model.find("Apple") != std::string::npos) {
        if (chip_model.find("M3") != std::string::npos && chip_model.find("Max") != std::string::npos) {
            info.l3_size = 28 * 1024 * 1024;  // 28MB SLC
        } else if (chip_model.find("M3") != std::string::npos && chip_model.find("Pro") != std::string::npos) {
            info.l3_size = 20 * 1024 * 1024;  // 20MB SLC
        } else if (chip_model.find("M3") != std::string::npos) {
            info.l3_size = 14 * 1024 * 1024;  // 14MB SLC
        }
    }

    return info;
}

void MacOSPlatform::get_macos_core_counts(size_t& p_core_count, size_t& e_core_count) {
    uint32_t p_cores = 0, e_cores = 0;
    size_t size;
    
    // Get performance core count (perflevel0)
    size = sizeof(p_cores);
    if (sysctlbyname("hw.perflevel0.physicalcpu", &p_cores, &size, nullptr, 0) == 0) {
        p_core_count = p_cores;
    } else {
        p_core_count = 8;  // Fallback default for M-series chips
    }
    
    // Get efficiency core count (perflevel1)  
    size = sizeof(e_cores);
    if (sysctlbyname("hw.perflevel1.physicalcpu", &e_cores, &size, nullptr, 0) == 0) {
        e_core_count = e_cores;
    } else {
        e_core_count = 4;  // Fallback default for M-series chips
    }
}

CacheInfo MacOSPlatform::get_core_specific_cache_info(CPUAffinityType affinity_type) {
    if (affinity_type == CPUAffinityType::DEFAULT) {
        return detect_cache_info();  // Return mixed cache info for default
    }
    
    // Start with a fresh CacheInfo structure for core-specific detection
    CacheInfo info = {};
    size_t cache_line_size = detect_cache_line_size();
    
    // Set cache line sizes
    info.l1_line_size = cache_line_size;
    info.l2_line_size = cache_line_size; 
    info.l3_line_size = cache_line_size;
    
    // Default associativity values
    info.l1d_assoc = 8;
    info.l1i_assoc = 8;
    info.l2_assoc = 8;
    info.l3_assoc = 16;
    
    // Set System Level Cache (SLC) size - same for all cores
    info.l3_size = 28 * 1024 * 1024;  // 28MB SLC for M3 Max
    
    // Get core-specific cache sizes from sysctls
    uint32_t cache_size = 0;  // Use correct data type - sysctls return 4 bytes
    
    if (affinity_type == CPUAffinityType::P_CORES) {
        // P-core cache sizes (perflevel0)
        size_t temp_size = sizeof(cache_size);
        if (sysctlbyname("hw.perflevel0.l1dcachesize", &cache_size, &temp_size, nullptr, 0) == 0) {
            info.l1_data_size = cache_size;
        } else {
            info.l1_data_size = 128 * 1024;  // 128KB L1D for P-cores
        }
        
        temp_size = sizeof(cache_size);
        if (sysctlbyname("hw.perflevel0.l1icachesize", &cache_size, &temp_size, nullptr, 0) == 0) {
            info.l1_instruction_size = cache_size;
        } else {
            info.l1_instruction_size = 192 * 1024;  // 192KB L1I for P-cores
        }
        
        temp_size = sizeof(cache_size);
        if (sysctlbyname("hw.perflevel0.l2cachesize", &cache_size, &temp_size, nullptr, 0) == 0) {
            info.l2_size = cache_size;
        } else {
            info.l2_size = 16 * 1024 * 1024;  // 16MB L2 for P-cores
        }
        
    } else if (affinity_type == CPUAffinityType::E_CORES) {
        // E-core cache sizes (perflevel1)
        size_t temp_size = sizeof(cache_size);
        if (sysctlbyname("hw.perflevel1.l1dcachesize", &cache_size, &temp_size, nullptr, 0) == 0) {
            info.l1_data_size = cache_size;
        } else {
            info.l1_data_size = 64 * 1024;  // 64KB L1D for E-cores
        }
        
        temp_size = sizeof(cache_size);
        if (sysctlbyname("hw.perflevel1.l1icachesize", &cache_size, &temp_size, nullptr, 0) == 0) {
            info.l1_instruction_size = cache_size;
        } else {
            info.l1_instruction_size = 128 * 1024;  // 128KB L1I for E-cores
        }
        
        temp_size = sizeof(cache_size);
        if (sysctlbyname("hw.perflevel1.l2cachesize", &cache_size, &temp_size, nullptr, 0) == 0) {
            info.l2_size = cache_size;
        } else {
            info.l2_size = 4 * 1024 * 1024;  // 4MB L2 for E-cores
        }
    }
    
    return info;
}

size_t MacOSPlatform::get_max_threads_for_affinity(CPUAffinityType affinity_type) {
    if (affinity_type == CPUAffinityType::DEFAULT) {
        return std::thread::hardware_concurrency();
    }
    
    size_t p_core_count, e_core_count;
    get_macos_core_counts(p_core_count, e_core_count);
    
    if (affinity_type == CPUAffinityType::P_CORES) {
        return p_core_count;
    } else if (affinity_type == CPUAffinityType::E_CORES) {
        return e_core_count;
    }
    
    return std::thread::hardware_concurrency();
}

bool MacOSPlatform::validate_thread_count(size_t num_threads, CPUAffinityType affinity_type, std::string& error_msg) {
    if (affinity_type == CPUAffinityType::DEFAULT) {
        return true;  // No limits for default affinity
    }
    
    size_t p_core_count, e_core_count;
    get_macos_core_counts(p_core_count, e_core_count);
    
    if (affinity_type == CPUAffinityType::P_CORES) {
        if (num_threads > p_core_count) {
            error_msg = "P-cores are limited to " + std::to_string(p_core_count) + " threads (requested: " + std::to_string(num_threads) + ")";
            return false;
        }
    } else if (affinity_type == CPUAffinityType::E_CORES) {
        if (num_threads > e_core_count) {
            error_msg = "E-cores are limited to " + std::to_string(e_core_count) + " threads (requested: " + std::to_string(num_threads) + ")";
            return false;
        }
    }
    
    return true;
}

void MacOSPlatform::set_thread_affinity(size_t thread_id, CPUAffinityType affinity_type, size_t total_threads) {
    (void)total_threads;  // Suppress unused parameter warning
    
    if (affinity_type == CPUAffinityType::DEFAULT) {
        return;  // No specific affinity needed
    }
    
    size_t p_core_count, e_core_count;
    get_macos_core_counts(p_core_count, e_core_count);
    
    mach_port_t thread_port = pthread_mach_thread_np(pthread_self());
    thread_affinity_policy_data_t policy = {0};
    
    if (affinity_type == CPUAffinityType::P_CORES) {
        // Map thread to P-cores (cores 0 to p_core_count-1)
        policy.affinity_tag = thread_id % p_core_count;
    } else if (affinity_type == CPUAffinityType::E_CORES) {
        // Map thread to E-cores (cores p_core_count to p_core_count+e_core_count-1)
        policy.affinity_tag = p_core_count + (thread_id % e_core_count);
    }
    
    thread_policy_set(thread_port, THREAD_AFFINITY_POLICY, 
                     (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);
}

MemorySpecs MacOSPlatform::get_memory_specs() {
    MemorySpecs specs;
    
    // Get total memory size
    uint64_t total_memory;
    size_t size = sizeof(total_memory);
    if (sysctlbyname("hw.memsize", &total_memory, &size, nullptr, 0) == 0) {
        specs.total_size_gb = total_memory / (1024 * 1024 * 1024);
    }
    
    // Apple Silicon unified memory defaults
    specs.type = "LPDDR5";
    specs.speed_mtps = 6400;  // M3 Max typical speed
    specs.data_width_bits = 512;  // M3 Max has wide memory interface
    specs.total_width_bits = 512;
    specs.num_channels = 32;  // M3 Max has many memory channels
    specs.theoretical_bandwidth_gbps = (specs.speed_mtps * specs.data_width_bits) / 8.0 / 1000.0;
    specs.is_virtualized = false;
    specs.data_width_detected = true;  // Apple Silicon specs are well-known
    specs.total_width_detected = true; // Apple Silicon specs are well-known
    specs.num_channels_detected = true; // Apple Silicon specs are well-known
    specs.is_unified_memory = true;
    specs.architecture = "Unified Memory Architecture (UMA) - Apple Silicon";
    
    return specs;
}

SystemInfo MacOSPlatform::get_system_info() {
    SystemInfo sys_info;
    
    // Get memory information
    uint64_t total_memory;
    size_t size = sizeof(total_memory);
    if (sysctlbyname("hw.memsize", &total_memory, &size, nullptr, 0) == 0) {
        sys_info.total_ram_gb = total_memory / (1024 * 1024 * 1024);
    }
    
    // Get available memory (approximation)
    vm_size_t page_size;
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t host_size = sizeof(vm_stat) / sizeof(natural_t);
    
    if (host_page_size(mach_host_self(), &page_size) == KERN_SUCCESS &&
        host_statistics64(mach_host_self(), HOST_VM_INFO64, 
                         reinterpret_cast<host_info64_t>(&vm_stat), &host_size) == KERN_SUCCESS) {
        uint64_t available_pages = vm_stat.free_count + vm_stat.inactive_count;
        sys_info.available_ram_gb = (available_pages * page_size) / (1024 * 1024 * 1024);
    }
    
    // Get CPU information
    sys_info.cpu_cores = std::thread::hardware_concurrency();
    sys_info.cpu_threads = std::thread::hardware_concurrency();
    sys_info.cache_line_size = detect_cache_line_size();
    
    auto [arch, model] = detect_processor_info();
    sys_info.cpu_name = model;
    
    // Get memory and cache specs
    sys_info.memory_specs = get_memory_specs();
    sys_info.cache_info = detect_cache_info();
    
    return sys_info;
}