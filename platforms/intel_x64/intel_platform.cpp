#include "intel_platform.h"
#include <thread>
#include <cstdio>
#include <fstream>
#include <sstream>

#ifdef __linux__
#include <pthread.h>
#include <sys/sysinfo.h>
#include <cstring>
#endif

std::pair<std::string, std::string> IntelPlatform::detect_processor_info() {
    std::string arch = "x86_64";
    std::string model = "";
    
#ifdef __linux__
    // Try to get processor information from /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    model = line.substr(pos + 1);
                    model.erase(0, model.find_first_not_of(" \t"));
                    break;
                }
            }
        }
        cpuinfo.close();
    }
#endif
    
    return std::make_pair(arch, model);
}

size_t IntelPlatform::detect_cache_line_size() {
    // Try getconf first
    FILE* pipe = popen("getconf LEVEL1_DCACHE_LINESIZE 2>/dev/null", "r");
    if (pipe) {
        char buffer[32];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            size_t cache_line_size = std::atoi(buffer);
            pclose(pipe);
            if (cache_line_size > 0 && cache_line_size <= 1024) {
                return cache_line_size;
            }
        }
        pclose(pipe);
    }
    
    // Fallback to standard Intel cache line size
    return 64;
}

CacheInfo IntelPlatform::detect_cache_info() {
    size_t cache_line_size = detect_cache_line_size();
    
    // Initialize with typical Intel defaults
    CacheInfo info = {
        32 * 1024,        // 32KB L1 data cache
        32 * 1024,        // 32KB L1 instruction cache
        256 * 1024,       // 256KB L2 cache
        8 * 1024 * 1024,  // 8MB L3 cache
        8, 8, 8, 16,      // Associativity
        cache_line_size,
        cache_line_size,
        cache_line_size
    };

#ifdef __linux__
    // Try to get cache info from sysfs
    for (int i = 0; i < 4; ++i) {
        std::string level_path = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i) + "/level";
        std::string type_path = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i) + "/type";
        std::string size_path = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i) + "/size";

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
#endif

    return info;
}

CacheInfo IntelPlatform::get_core_specific_cache_info(CPUAffinityType affinity_type) {
    (void)affinity_type;  // Intel doesn't have heterogeneous cores like Apple Silicon
    return detect_cache_info();
}

size_t IntelPlatform::get_max_threads_for_affinity(CPUAffinityType affinity_type) {
    (void)affinity_type;  // Intel doesn't have P/E core distinction
    return std::thread::hardware_concurrency();
}

void IntelPlatform::set_thread_affinity(size_t thread_id, CPUAffinityType affinity_type, size_t total_threads) {
    (void)affinity_type;
    (void)total_threads;
    
#ifdef __linux__
    // Set standard CPU affinity on Linux
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id % std::thread::hardware_concurrency(), &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#else
    (void)thread_id;  // No affinity support on other platforms
#endif
}

bool IntelPlatform::validate_thread_count(size_t num_threads, CPUAffinityType affinity_type, std::string& error_msg) {
    (void)affinity_type;  // Intel doesn't have P/E core distinction, so no affinity-specific limits
    
    // Basic sanity check
    size_t max_threads = std::thread::hardware_concurrency() * 2;  // Allow some oversubscription
    if (num_threads > max_threads) {
        error_msg = "Thread count (" + std::to_string(num_threads) + 
                   ") is too high (system supports max " + std::to_string(max_threads) + " threads)";
        return false;
    }
    
    return true;
}

bool IntelPlatform::supports_cpu_affinity() {
#ifdef __linux__
    return true;
#else
    return false;
#endif
}

bool IntelPlatform::detect_virtualization() {
#ifdef __linux__
    // Check if running in a virtualized environment
    FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo) {
        char line[256];
        while (fgets(line, sizeof(line), cpuinfo)) {
            if (strstr(line, "hypervisor") || strstr(line, "KVM") || 
                strstr(line, "VMware") || strstr(line, "VirtualBox")) {
                fclose(cpuinfo);
                return true;
            }
        }
        fclose(cpuinfo);
    }
    
    // Check dmidecode for virtualization indicators
    FILE* dmidecode = popen("sudo dmidecode -t memory 2>/dev/null | grep -c 'Memory Device'", "r");
    if (dmidecode) {
        char result[16];
        if (fgets(result, sizeof(result), dmidecode) != nullptr) {
            int device_count = atoi(result);
            pclose(dmidecode);
            // If only 1 memory device is reported, likely virtualized
            return device_count <= 1;
        }
        pclose(dmidecode);
    }
#endif
    return false;
}

MemorySpecs IntelPlatform::get_memory_specs() {
    MemorySpecs specs;
    
    // Detect virtualization
    bool is_virtualized = detect_virtualization();
    
    // Default Intel platform specs - these would need to be detected
    specs.type = "DDR4";  // Common on Intel platforms
    specs.speed_mtps = 3200;
    specs.data_width_bits = 64;
    specs.total_width_bits = 72;  // Including ECC
    specs.is_virtualized = is_virtualized;
    specs.data_width_detected = false;
    specs.total_width_detected = false;
    specs.is_unified_memory = false;
    
    // Handle channel detection based on virtualization
    if (is_virtualized) {
        specs.num_channels = 0;  // Cannot detect in virtualized environment
        specs.num_channels_detected = false;
        specs.theoretical_bandwidth_gbps = -1.0;  // N/A for virtualized systems
        specs.architecture = "Virtualized Environment - Memory channels not accessible";
    } else {
        specs.num_channels = 2;  // Common default, but not detected
        specs.num_channels_detected = false;  // Not detected from system
        specs.theoretical_bandwidth_gbps = (specs.speed_mtps * specs.data_width_bits * specs.num_channels) / 8.0 / 1000.0;
        specs.architecture = "Traditional NUMA Architecture";
    }
    
    return specs;
}

SystemInfo IntelPlatform::get_system_info() {
    SystemInfo sys_info;
    
    // Basic system info
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

void IntelPlatform::detect_cache_associativity(CacheInfo& info) {
    (void)info;  // Placeholder - would implement cache associativity detection
}