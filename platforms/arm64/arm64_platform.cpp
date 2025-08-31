#include "arm64_platform.h"
#include "../common/safe_file_utils.h"
#include <thread>
#include <cstdio>
#include <fstream>

#ifdef __linux__
#include <pthread.h>
#include <sys/sysinfo.h>
#include <unistd.h>  // for sysconf
#endif

std::pair<std::string, std::string> ARM64Platform::detect_processor_info() {
    std::string arch = "aarch64";
    std::string model = "";
    
#ifdef __linux__
    // Try to get processor information from /proc/cpuinfo using safe file utilities
    std::vector<std::string> cpuinfo_lines;
    if (SafeFileUtils::read_all_lines("/proc/cpuinfo", cpuinfo_lines)) {
        for (const auto& line : cpuinfo_lines) {
            if (line.find("model name") != std::string::npos || 
                line.find("Processor") != std::string::npos ||
                line.find("cpu model") != std::string::npos) {
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos && colon_pos + 1 < line.length()) {
                    model = line.substr(colon_pos + 1);
                    model = SafeFileUtils::sanitize_input(model);
                    model.erase(0, model.find_first_not_of(" \t"));
                    if (!model.empty()) {
                        break;
                    }
                }
            }
        }
    }
    
    // If still no model name, try to identify from CPU implementer and part
    if (model.empty()) {
        std::ifstream cpuinfo2("/proc/cpuinfo");
        if (cpuinfo2.is_open()) {
            std::string line;
            int implementer = -1;
            int part = -1;
            while (std::getline(cpuinfo2, line)) {
                if (line.find("CPU implementer") != std::string::npos) {
                    size_t pos = line.find(":");
                    if (pos != std::string::npos) {
                        std::string value = line.substr(pos + 1);
                        value.erase(0, value.find_first_not_of(" \t"));
                        if (value.find("0x") == 0) {
                            implementer = std::stoi(value.substr(2), nullptr, 16);
                        }
                    }
                } else if (line.find("CPU part") != std::string::npos) {
                    size_t pos = line.find(":");
                    if (pos != std::string::npos) {
                        std::string value = line.substr(pos + 1);
                        value.erase(0, value.find_first_not_of(" \t"));
                        if (value.find("0x") == 0) {
                            part = std::stoi(value.substr(2), nullptr, 16);
                        }
                    }
                }
            }
            cpuinfo2.close();
            
            // Identify common ARM processors
            if (implementer == 0x41) { // ARM
                if (part == 0xd4f) {
                    model = "ARM Cortex-A72";
                } else if (part == 0xd0c) {
                    model = "ARM Cortex-A76";
                } else if (part == 0xd0d) {
                    model = "ARM Cortex-A77";
                } else if (part == 0xd0e) {
                    model = "ARM Cortex-A78";
                } else if (part == 0xd4a) {
                    model = "ARM Cortex-A78AE";
                } else if (part == 0xd4b) {
                    model = "ARM Cortex-X1";
                } else if (part == 0xd4c) {
                    model = "ARM Cortex-X1C";
                } else if (part == 0xd4d) {
                    model = "ARM Cortex-X2";
                } else if (part == 0xd4e) {
                    model = "ARM Cortex-A710";
                } else if (part == 0xd4f) {
                    model = "ARM Cortex-X3";
                } else if (part == 0xd50) {
                    model = "ARM Cortex-A715";
                } else if (part == 0xd51) {
                    model = "ARM Cortex-X4";
                } else if (part == 0xd52) {
                    model = "ARM Cortex-A720";
                } else if (part == 0xd53) {
                    model = "ARM Cortex-X5";
                } else if (part == 0xd54) {
                    model = "ARM Cortex-A730";
                } else if (part == 0xd55) {
                    model = "ARM Cortex-A740";
                } else if (part == 0xd56) {
                    model = "ARM Cortex-X6";
                } else if (part == 0xd57) {
                    model = "ARM Cortex-A750";
                } else if (part == 0xd58) {
                    model = "ARM Cortex-A760";
                } else if (part == 0xd59) {
                    model = "ARM Cortex-A770";
                } else if (part == 0xd5a) {
                    model = "ARM Cortex-X7";
                } else if (part == 0xd5b) {
                    model = "ARM Cortex-A780";
                } else if (part == 0xd5c) {
                    model = "ARM Cortex-X8";
                } else if (part == 0xd5d) {
                    model = "ARM Cortex-A785";
                } else if (part == 0xd5e) {
                    model = "ARM Cortex-A790";
                } else if (part == 0xd5f) {
                    model = "ARM Cortex-X9";
                } else {
                    model = "ARM Processor (implementer: 0x" + std::to_string(implementer) + 
                           ", part: 0x" + std::to_string(part) + ")";
                }
            } else if (implementer == 0x51) { // Qualcomm
                model = "Qualcomm Processor";
            } else if (implementer == 0x53) { // Samsung
                model = "Samsung Processor";
            } else if (implementer == 0x56) { // Marvell
                model = "Marvell Processor";
            } else if (implementer == 0x69) { // Intel
                model = "Intel ARM Processor";
            } else {
                model = "Unknown ARM Processor (implementer: 0x" + std::to_string(implementer) + 
                       ", part: 0x" + std::to_string(part) + ")";
            }
        }
    }
#endif
    
    return std::make_pair(arch, model);
}

size_t ARM64Platform::detect_cache_line_size() {
    // ARM64 typically uses 64-byte cache lines, but some newer chips use 128 bytes
    // Try sysconf first
    long cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (cache_line_size > 0 && cache_line_size <= 1024) {
        return static_cast<size_t>(cache_line_size);
    }
    
    // Try reading from /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size using safe file utilities
    std::string cache_line_str;
    if (SafeFileUtils::read_single_line("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", cache_line_str)) {
        try {
            cache_line_size = std::stoi(cache_line_str);
            if (cache_line_size > 0 && cache_line_size <= 1024) {
                return static_cast<size_t>(cache_line_size);
            }
        } catch (const std::exception&) {
            // Fall through to default
        }
    }
    
    // Default to 64 bytes for ARM64
    return 64;
}

CacheInfo ARM64Platform::detect_cache_info() {
    size_t cache_line_size = detect_cache_line_size();
    
    // Initialize with typical ARM64 defaults (big.LITTLE architecture)
    CacheInfo info = {
        64 * 1024,        // 64KB L1 data cache (little cores)
        64 * 1024,        // 64KB L1 instruction cache
        512 * 1024,       // 512KB L2 cache
        2 * 1024 * 1024,  // 2MB L3 cache
        4, 4, 8, 16,      // Associativity
        cache_line_size,
        cache_line_size,
        cache_line_size
    };

#ifdef __linux__
    // Try to get cache info from sysfs - ARM64 systems may have different organization
    for (int cpu = 0; cpu < 8; ++cpu) {
        for (int i = 0; i < 6; ++i) {
            std::string level_path = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cache/index" + std::to_string(i) + "/level";
            std::string type_path = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cache/index" + std::to_string(i) + "/type";
            std::string size_path = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cache/index" + std::to_string(i) + "/size";
            
            std::ifstream level_file(level_path);
            std::ifstream type_file(type_path);
            std::ifstream size_file(size_path);
            
            if (level_file.is_open() && type_file.is_open() && size_file.is_open()) {
                int level;
                std::string type;
                std::string size_str;
                
                level_file >> level;
                type_file >> type;
                size_file >> size_str;
                
                size_t size_bytes = 0;
                if (size_str.find("K") != std::string::npos) {
                    size_t k_pos = size_str.find("K");
                    size_bytes = std::stoul(size_str.substr(0, k_pos)) * 1024;
                } else if (size_str.find("M") != std::string::npos) {
                    size_t m_pos = size_str.find("M");
                    size_bytes = std::stoul(size_str.substr(0, m_pos)) * 1024 * 1024;
                }
                
                if (size_bytes > 0) {
                    if (level == 1 && type == "Data") {
                        info.l1_data_size = size_bytes;
                    } else if (level == 1 && type == "Instruction") {
                        info.l1_instruction_size = size_bytes;
                    } else if (level == 2 && type == "Unified") {
                        info.l2_size = size_bytes;
                    } else if (level == 3 && type == "Unified") {
                        info.l3_size = size_bytes;
                    }
                }
            }
        }
    }
#endif

    return info;
}

CacheInfo ARM64Platform::get_core_specific_cache_info(CPUAffinityType affinity_type) {
    // ARM64 big.LITTLE could have different cache sizes for big vs little cores
    // This would need platform-specific implementation for each SoC
    CacheInfo info = detect_cache_info();
    
    if (affinity_type == CPUAffinityType::P_CORES) {
        // Big cores typically have larger caches
        info.l1_data_size = 128 * 1024;      // 128KB L1D for big cores
        info.l1_instruction_size = 128 * 1024; // 128KB L1I for big cores
        info.l2_size = 1024 * 1024;          // 1MB L2 for big cores
    } else if (affinity_type == CPUAffinityType::E_CORES) {
        // Little cores have smaller caches
        info.l1_data_size = 64 * 1024;       // 64KB L1D for little cores
        info.l1_instruction_size = 64 * 1024; // 64KB L1I for little cores
        info.l2_size = 256 * 1024;           // 256KB L2 for little cores
    }
    
    return info;
}

size_t ARM64Platform::get_max_threads_for_affinity(CPUAffinityType affinity_type) {
    // This would need to be implemented based on the specific ARM64 SoC
    // For now, return total threads for any affinity type
    (void)affinity_type;
    return std::thread::hardware_concurrency();
}

void ARM64Platform::set_thread_affinity(size_t thread_id, CPUAffinityType affinity_type, size_t total_threads) {
    (void)affinity_type;
    (void)total_threads;
    
#ifdef __linux__
    // ARM64 big.LITTLE affinity would need platform-specific implementation
    // For now, just set standard CPU affinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id % std::thread::hardware_concurrency(), &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#else
    (void)thread_id;
#endif
}

bool ARM64Platform::validate_thread_count(size_t num_threads, CPUAffinityType affinity_type, std::string& error_msg) {
    (void)affinity_type;  // ARM64 big.LITTLE would need platform-specific detection
    
    // Basic sanity check
    size_t max_threads = std::thread::hardware_concurrency() * 2;  // Allow some oversubscription
    if (num_threads > max_threads) {
        error_msg = "Thread count (" + std::to_string(num_threads) + 
                   ") is too high (system supports max " + std::to_string(max_threads) + " threads)";
        return false;
    }
    
    return true;
}

MemorySpecs ARM64Platform::get_memory_specs() {
    MemorySpecs specs;
    
    // Default ARM64 platform specs
    specs.type = "LPDDR4";  // Common on ARM64 mobile/embedded platforms
    specs.speed_mtps = 3200;
    specs.data_width_bits = 64;
    specs.total_width_bits = 64;
    specs.num_channels = 2;  // Common default, but not detected
    specs.theoretical_bandwidth_gbps = (specs.speed_mtps * specs.data_width_bits * specs.num_channels) / 8.0 / 1000.0;
    specs.is_virtualized = false;
    specs.data_width_detected = false;
    specs.total_width_detected = false;
    specs.num_channels_detected = false;  // Not detected from system
    specs.is_unified_memory = false;
    specs.architecture = "ARM64 Architecture";
    
    return specs;
}

SystemInfo ARM64Platform::get_system_info() {
    SystemInfo sys_info;
    
    sys_info.cpu_cores = std::thread::hardware_concurrency();
    sys_info.cpu_threads = std::thread::hardware_concurrency();
    sys_info.cache_line_size = detect_cache_line_size();
    
    auto [arch, model] = detect_processor_info();
    sys_info.cpu_name = model;
    
    sys_info.memory_specs = get_memory_specs();
    sys_info.cache_info = detect_cache_info();
    
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        sys_info.total_ram_gb = si.totalram * si.mem_unit / (1024 * 1024 * 1024);
        sys_info.available_ram_gb = si.freeram * si.mem_unit / (1024 * 1024 * 1024);
    }
#endif
    
    return sys_info;
}

