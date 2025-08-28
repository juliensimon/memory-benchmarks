#include "system_info_display.h"
#include <iostream>

void SystemInfoDisplay::print_system_info(
    const std::unique_ptr<PlatformInterface>& platform,
    OutputFormat format,
    bool show_build_info,
    CPUAffinityType affinity_type
) {
    auto base_info = platform->get_system_info();
    OutputFormatter formatter(format);
    
    if (platform->get_platform_name() == "macOS" && affinity_type == CPUAffinityType::DEFAULT) {
        print_macos_heterogeneous_info(platform, base_info, formatter, show_build_info);
    } else {
        // Handle core-specific affinity types
        if (affinity_type != CPUAffinityType::DEFAULT) {
            SystemInfo core_specific_info = base_info;
            core_specific_info.cache_info = platform->get_core_specific_cache_info(affinity_type);
            
            // Add core-specific details to CPU name
            if (affinity_type == CPUAffinityType::P_CORES) {
                size_t p_cores = platform->get_max_threads_for_affinity(CPUAffinityType::P_CORES);
                core_specific_info.cpu_name += " (P-cores only: " + std::to_string(p_cores) + " cores)";
            } else if (affinity_type == CPUAffinityType::E_CORES) {
                size_t e_cores = platform->get_max_threads_for_affinity(CPUAffinityType::E_CORES);
                core_specific_info.cpu_name += " (E-cores only: " + std::to_string(e_cores) + " cores)";
            }
            
            std::cout << formatter.format_system_info(core_specific_info);
        } else {
            std::cout << formatter.format_system_info(base_info);
        }
    }
    
    if (show_build_info) {
        print_build_info(platform);
    }
}

void SystemInfoDisplay::print_cached_system_info(
    const SystemInfo& cached_info,
    const std::unique_ptr<PlatformInterface>& platform,
    OutputFormat format,
    CPUAffinityType affinity_type
) {
    OutputFormatter formatter(format);
    
    if (affinity_type != CPUAffinityType::DEFAULT) {
        SystemInfo core_specific_info = cached_info;
        core_specific_info.cache_info = platform->get_core_specific_cache_info(affinity_type);
        
        // Add core-specific details to CPU name
        if (affinity_type == CPUAffinityType::P_CORES) {
            size_t p_cores = platform->get_max_threads_for_affinity(CPUAffinityType::P_CORES);
            core_specific_info.cpu_name += " (P-cores only: " + std::to_string(p_cores) + " cores)";
        } else if (affinity_type == CPUAffinityType::E_CORES) {
            size_t e_cores = platform->get_max_threads_for_affinity(CPUAffinityType::E_CORES);
            core_specific_info.cpu_name += " (E-cores only: " + std::to_string(e_cores) + " cores)";
        }
        
        std::cout << formatter.format_system_info(core_specific_info);
    } else {
        // Default mode - show P/E core breakdown for Apple Silicon
        if (platform->get_platform_name() == "macOS") {
            SystemInfo enhanced_info = cached_info;
            size_t p_cores = platform->get_max_threads_for_affinity(CPUAffinityType::P_CORES);
            size_t e_cores = platform->get_max_threads_for_affinity(CPUAffinityType::E_CORES);
            enhanced_info.cpu_name += " (" + std::to_string(p_cores) + " P-cores + " + std::to_string(e_cores) + " E-cores)";
            std::cout << formatter.format_system_info(enhanced_info);
        } else {
            std::cout << formatter.format_system_info(cached_info);
        }
    }
}

void SystemInfoDisplay::print_macos_heterogeneous_info(
    const std::unique_ptr<PlatformInterface>& platform,
    const SystemInfo& base_info,
    OutputFormatter& formatter,
    bool show_build_info
) {
    size_t p_cores = platform->get_max_threads_for_affinity(CPUAffinityType::P_CORES);
    size_t e_cores = platform->get_max_threads_for_affinity(CPUAffinityType::E_CORES);
    
    SystemInfo enhanced_info = base_info;
    enhanced_info.cpu_name += " (" + std::to_string(p_cores) + " P-cores + " + std::to_string(e_cores) + " E-cores)";
    
    if (show_build_info) {
        // For --info flag, show detailed cache breakdown
        std::cout << formatter.format_system_info_without_cache(enhanced_info);
        
        // Show detailed P-core and E-core cache information
        std::cout << "## Cache Information (Heterogeneous Architecture)\n\n";
        
        auto p_cache = platform->get_core_specific_cache_info(CPUAffinityType::P_CORES);
        std::cout << "### P-cores (" << p_cores << " cores)\n";
        std::cout << "- **L1 Data Cache:** " << (p_cache.l1_data_size / 1024) << " KB per core ✓\n";
        std::cout << "- **L1 Instruction Cache:** " << (p_cache.l1_instruction_size / 1024) << " KB per core ✓\n";
        std::cout << "- **L2 Cache:** " << (p_cache.l2_size / 1024) << " KB per core ✓\n\n";
        
        auto e_cache = platform->get_core_specific_cache_info(CPUAffinityType::E_CORES);
        std::cout << "### E-cores (" << e_cores << " cores)\n";
        std::cout << "- **L1 Data Cache:** " << (e_cache.l1_data_size / 1024) << " KB per core ✓\n";
        std::cout << "- **L1 Instruction Cache:** " << (e_cache.l1_instruction_size / 1024) << " KB per core ✓\n";
        std::cout << "- **L2 Cache:** " << (e_cache.l2_size / 1024) << " KB per core ✓\n\n";
        
        std::cout << "### Shared Cache\n";
        std::cout << "- **System Level Cache (SLC):** " << (p_cache.l3_size / (1024 * 1024)) << " MB shared ✓\n";
        std::cout << "- **Cache Line Size:** " << platform->detect_cache_line_size() << " bytes ✓\n\n";
    } else {
        // For benchmark runs, show standard system info
        std::cout << formatter.format_system_info(enhanced_info);
    }
}

void SystemInfoDisplay::print_build_info(const std::unique_ptr<PlatformInterface>& platform) {
    std::cout << "## Build Information\n\n";
    std::cout << "- **Platform:** " << platform->get_platform_name() << "\n";
    std::cout << "- **CPU Affinity Support:** " << (platform->supports_cpu_affinity() ? "Yes" : "No") << "\n";
    std::cout << "- **Compiler:** g++ with C++17 support\n";
    std::cout << "- **Optimization:** -O3 -march=native -mtune=native\n";
}