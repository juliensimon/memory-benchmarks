#include "output_formatter_utils.h"

namespace OutputFormatterUtils {

std::string format_basic_system_info(const SystemInfo& sys_info) {
    std::stringstream ss;
    ss << "# System Information\n\n";
    ss << "- **CPU:** " << sys_info.cpu_name << " ✓\n";
    ss << "- **Total RAM:** " << sys_info.total_ram_gb << " GB ✓\n";
    ss << "- **Available RAM:** " << sys_info.available_ram_gb << " GB ✓\n";
    ss << "- **Physical CPU Cores:** " << sys_info.cpu_cores << " ✓\n";
    ss << "- **Logical CPU Threads:** " << sys_info.cpu_threads << " ✓\n\n";
    return ss.str();
}

std::string format_memory_specifications(const MemorySpecs& mem_specs) {
    std::stringstream ss;
    ss << "## Memory Specifications\n\n";

    // Memory architecture - show if unified memory
    if(mem_specs.is_unified_memory) {
        ss << "- **Architecture:** " << mem_specs.architecture << " ✓\n";
    }

    // Memory type - detected for Apple Silicon, estimated for others
    bool type_detected = is_memory_type_detected(mem_specs.type);
    ss << "- **Type:** " << mem_specs.type;
    if(type_detected)
        ss << " ✓";
    ss << "\n";

    // Handle speed display
    if(mem_specs.speed_mtps > 0) {
        ss << "- **Speed:** " << mem_specs.speed_mtps << " MT/s ✓\n";
    } else {
        ss << "- **Speed:** Not available from system APIs\n";
    }

    // Data width - detected from dmidecode or Apple Silicon specifications
    ss << "- **Data Width:** " << mem_specs.data_width_bits << " bits";
    if(mem_specs.data_width_detected)
        ss << " ✓";
    ss << "\n";

    // Total width - detected from dmidecode or estimated
    ss << "- **Total Width:** " << mem_specs.total_width_bits << " bits";
    if(mem_specs.total_width_detected)
        ss << " ✓";
    ss << "\n";

    // Channels - show detection status
    ss << "- **Channels:** " << mem_specs.num_channels;
    if(mem_specs.is_virtualized) {
        if(mem_specs.num_channels == 0) {
            ss << " (cannot detect - virtualized environment)";
        } else {
            ss << " (estimated - virtualized environment)";
        }
    } else if(!mem_specs.num_channels_detected) {
        ss << " (not detected from system)";
    }
    // Note: No checkmark for hardcoded values, even if they're correct
    ss << "\n";

    // Handle theoretical bandwidth display
    if(mem_specs.theoretical_bandwidth_gbps < 0) {
        ss << "- **Theoretical Bandwidth:** N/A (virtualized environment - channels not accessible)\n\n";
    } else if(mem_specs.theoretical_bandwidth_gbps > 0) {
        ss << "- **Theoretical Bandwidth:** " << std::fixed << std::setprecision(1)
           << mem_specs.theoretical_bandwidth_gbps << " GB/s ("
           << (mem_specs.theoretical_bandwidth_gbps * 8.0) << " Gb/s)";
        if(mem_specs.speed_mtps > 0 && mem_specs.data_width_bits > 0) {
            ss << " ✓";
        }
        ss << "\n\n";
    } else {
        ss << "- **Theoretical Bandwidth:** Not calculated (speed unknown)\n\n";
    }

    return ss.str();
}

std::string format_cache_information(const CacheInfo& cache_info, const MemorySpecs& mem_specs) {
    std::stringstream ss;
    ss << "## Cache Information\n\n";
    ss << "- **L1 Data Cache:** " << (cache_info.l1_data_size / 1024)
       << " KB per core ✓\n";
    ss << "- **L1 Instruction Cache:** " << (cache_info.l1_instruction_size / 1024)
       << " KB per core ✓\n";
    
    // Handle L2 cache display based on architecture
    if(mem_specs.is_unified_memory) {
        ss << "- **L2 Cache:** " << (cache_info.l2_size / 1024) << " KB shared ✓\n";
        ss << "- **System Level Cache (SLC):** " << (cache_info.l3_size / (1024 * 1024)) << " MB shared ✓\n";
    } else {
        ss << "- **L2 Cache:** " << (cache_info.l2_size / 1024) << " KB per core ✓\n";
        ss << "- **L3 Cache:** " << (cache_info.l3_size / (1024 * 1024)) << " MB shared ✓\n";
    }
    
    ss << "- **Cache Line Size:** " << cache_info.l1_line_size << " bytes ✓\n\n";

    return ss.str();
}

std::string format_efficiency_display(double efficiency, double theoretical_bandwidth) {
    if(efficiency < 0) {
        return "N/A";
    } else if(theoretical_bandwidth > 0) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << efficiency;
        return ss.str();
    } else {
        return "N/A";
    }
}

bool is_memory_type_detected(const std::string& memory_type) {
    return (memory_type.find("LPDDR") != std::string::npos ||
            memory_type.find("DDR") != std::string::npos);
}

} // namespace OutputFormatterUtils