#include "output_formatter.h"
#include "output_formatter_utils.h"
#include "constants.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

OutputFormatter::OutputFormatter(OutputFormat format) : format_(format) {}

std::string OutputFormatter::format_system_info(const SystemInfo& sys_info) {
    switch(format_) {
        case OutputFormat::MARKDOWN:
            return format_markdown_system_info(sys_info);
        case OutputFormat::JSON:
            return format_json_system_info(sys_info);
        case OutputFormat::CSV:
            return format_csv_system_info(sys_info);
        default:
            return format_markdown_system_info(sys_info);
    }
}

std::string OutputFormatter::format_system_info_without_cache(const SystemInfo& sys_info) {
    switch(format_) {
        case OutputFormat::MARKDOWN:
            return format_markdown_system_info_without_cache(sys_info);
        case OutputFormat::JSON:
            return format_json_system_info(sys_info);  // Use full system info for JSON
        case OutputFormat::CSV:
            return format_csv_system_info(sys_info);   // Use full system info for CSV
        default:
            return format_markdown_system_info_without_cache(sys_info);
    }
}

std::string OutputFormatter::format_header() {
    switch(format_) {
        case OutputFormat::MARKDOWN:
            return format_markdown_header();
        case OutputFormat::JSON:
            return format_json_header();
        case OutputFormat::CSV:
            return format_csv_header();
        default:
            return format_markdown_header();
    }
}

std::string OutputFormatter::format_test_result(const TestResult& result,
                                                const MemorySpecs& mem_specs) {
    switch(format_) {
        case OutputFormat::MARKDOWN:
            return format_markdown_test_result(result, mem_specs);
        case OutputFormat::JSON:
            return format_json_test_result(result, mem_specs);
        case OutputFormat::CSV:
            return format_csv_test_result(result, mem_specs);
        default:
            return format_markdown_test_result(result, mem_specs);
    }
}

std::string OutputFormatter::format_test_results(const std::vector<TestResult>& results,
                                                 const MemorySpecs& mem_specs) {
    std::stringstream ss;

    switch(format_) {
        case OutputFormat::MARKDOWN:
            for(const auto& result : results) {
                ss << format_markdown_test_result(result, mem_specs);
            }
            break;
        case OutputFormat::JSON:
            ss << "[\n";
            for(size_t i = 0; i < results.size(); ++i) {
                ss << format_json_test_result(results[i], mem_specs);
                if(i < results.size() - 1)
                    ss << ",";
                ss << "\n";
            }
            ss << "]";
            break;
        case OutputFormat::CSV:
            for(const auto& result : results) {
                ss << format_csv_test_result(result, mem_specs);
            }
            break;
    }

    return ss.str();
}

std::string OutputFormatter::format_cache_aware_results(const std::string& pattern_name,
                                                        const std::vector<TestResult>& results,
                                                        const MemorySpecs& mem_specs) {
    switch(format_) {
        case OutputFormat::MARKDOWN:
            return format_markdown_cache_aware_results(pattern_name, results, mem_specs);
        case OutputFormat::JSON:
            return format_json_cache_aware_results(pattern_name, results, mem_specs);
        case OutputFormat::CSV:
            return format_csv_cache_aware_results(pattern_name, results, mem_specs);
        default:
            return format_markdown_cache_aware_results(pattern_name, results, mem_specs);
    }
}

std::string OutputFormatter::format_completion_message() {
    switch(format_) {
        case OutputFormat::MARKDOWN:
            return "\n## Test Complete\n\nAll memory bandwidth tests have been completed "
                   "successfully.\n";
        case OutputFormat::JSON:
            return "\n{\n  \"status\": \"complete\",\n  \"message\": \"All memory bandwidth tests "
                   "have been completed successfully.\"\n}\n";
        case OutputFormat::CSV:
            return "\n# Test Complete\n# All memory bandwidth tests have been completed "
                   "successfully.\n";
        default:
            return "\n## Test Complete\n\nAll memory bandwidth tests have been completed "
                   "successfully.\n";
    }
}

// Markdown formatting methods
std::string OutputFormatter::format_markdown_system_info(const SystemInfo& sys_info) {
    std::string result;
    result += OutputFormatterUtils::format_basic_system_info(sys_info);
    result += OutputFormatterUtils::format_memory_specifications(sys_info.memory_specs);
    result += OutputFormatterUtils::format_cache_information(sys_info.cache_info, sys_info.memory_specs);
    return result;
}

std::string OutputFormatter::format_markdown_system_info_without_cache(const SystemInfo& sys_info) {
    std::string result;
    result += OutputFormatterUtils::format_basic_system_info(sys_info);
    result += OutputFormatterUtils::format_memory_specifications(sys_info.memory_specs);
    return result;
}

std::string OutputFormatter::format_markdown_header() {
    return "## Test Results\n\n"
           "| Test | Working Set | Threads | Bandwidth (Gb/s) | Latency (ns) | Efficiency (%) |\n"
           "|------|-------------|---------|------------------|--------------|----------------|\n";
}

std::string OutputFormatter::format_markdown_test_result(const TestResult& result,
                                                         const MemorySpecs& mem_specs) {
    double efficiency =
        calculate_efficiency(result.stats.bandwidth_gbps, mem_specs.theoretical_bandwidth_gbps);
    bool suspicious = validate_test_result(result, mem_specs);

    std::stringstream ss;
    ss << "| " << result.test_name << " | " << result.working_set_desc << " | "
       << result.num_threads << " | " << std::fixed << std::setprecision(2)
       << (result.stats.bandwidth_gbps * 8.0);

    // Add warning indicator for suspicious results
    if(suspicious) {
        ss << " ⚠️";
    }

    ss << " | " << std::fixed << std::setprecision(1) << result.stats.latency_ns << " | ";

    // Handle efficiency display
    ss << OutputFormatterUtils::format_efficiency_display(efficiency, mem_specs.theoretical_bandwidth_gbps);
    ss << " |\n";

    return ss.str();
}

std::string OutputFormatter::format_markdown_cache_aware_results(
    const std::string& pattern_name, const std::vector<TestResult>& results,
    const MemorySpecs& mem_specs) {
    std::stringstream ss;
    ss << "### " << pattern_name << " (Cache-Aware)\n\n";
    ss << "| Working Set | Threads | Bandwidth (Gb/s) | Latency (ns) | Efficiency (%) |\n";
    ss << "|-------------|---------|------------------|--------------|----------------|\n";

    for(const auto& result : results) {
        double efficiency =
            calculate_efficiency(result.stats.bandwidth_gbps, mem_specs.theoretical_bandwidth_gbps);

        ss << "| " << result.working_set_desc << " | " << result.num_threads << " | " << std::fixed
           << std::setprecision(2) << (result.stats.bandwidth_gbps * 8.0) << " | " << std::fixed
           << std::setprecision(1) << result.stats.latency_ns << " | ";

        // Handle efficiency display
        ss << OutputFormatterUtils::format_efficiency_display(efficiency, mem_specs.theoretical_bandwidth_gbps);
        ss << " |\n";
    }
    ss << "\n";

    return ss.str();
}

// JSON formatting methods
std::string OutputFormatter::format_json_system_info(const SystemInfo& sys_info) {
    std::stringstream ss;
    ss << "{\n"
       << "  \"system_info\": {\n"
       << "    \"cpu_name\": \"" << sys_info.cpu_name << "\",\n"
       << "    \"total_ram_gb\": " << sys_info.total_ram_gb << ",\n"
       << "    \"available_ram_gb\": " << sys_info.available_ram_gb << ",\n"
       << "    \"cpu_cores\": " << sys_info.cpu_cores << ",\n"
       << "    \"cpu_threads\": " << sys_info.cpu_threads << ",\n"
       << "    \"cache_line_size\": " << sys_info.cache_line_size << ",\n"
       << "    \"memory_specs\": {\n"
       << "      \"type\": \"" << sys_info.memory_specs.type << "\",\n"
       << "      \"speed_mtps\": " << sys_info.memory_specs.speed_mtps << ",\n"
       << "      \"data_width_bits\": " << sys_info.memory_specs.data_width_bits << ",\n"
       << "      \"total_width_bits\": " << sys_info.memory_specs.total_width_bits << ",\n"
       << "      \"num_channels\": " << sys_info.memory_specs.num_channels << ",\n"
       << "      \"num_channels_detected\": " << (sys_info.memory_specs.num_channels_detected ? "true" : "false") << ",\n"
       << "      \"theoretical_bandwidth_gbps\": " << std::fixed << std::setprecision(1)
       << sys_info.memory_specs.theoretical_bandwidth_gbps << "\n"
       << "    },\n"
       << "    \"cache_info\": {\n"
       << "      \"l1_data_size\": " << sys_info.cache_info.l1_data_size << ",\n"
       << "      \"l1_instruction_size\": " << sys_info.cache_info.l1_instruction_size << ",\n"
       << "      \"l2_size\": " << sys_info.cache_info.l2_size << ",\n"
       << "      \"l3_size\": " << sys_info.cache_info.l3_size << ",\n"
       << "      \"l1_line_size\": " << sys_info.cache_info.l1_line_size << "\n"
       << "    }\n"
       << "  }\n"
       << "}\n";

    return ss.str();
}

std::string OutputFormatter::format_json_header() {
    return "{\n  \"test_results\": [\n";
}

std::string OutputFormatter::format_json_test_result(const TestResult& result,
                                                     const MemorySpecs& mem_specs) {
    double efficiency =
        calculate_efficiency(result.stats.bandwidth_gbps, mem_specs.theoretical_bandwidth_gbps);

    std::stringstream ss;
    ss << "    {\n"
       << "      \"test_name\": \"" << result.test_name << "\",\n"
       << "      \"working_set_desc\": \"" << result.working_set_desc << "\",\n"
       << "      \"bandwidth_gbps\": " << std::fixed << std::setprecision(2)
       << result.stats.bandwidth_gbps << ",\n"
       << "      \"bandwidth_gb_s\": " << std::fixed << std::setprecision(2)
       << (result.stats.bandwidth_gbps * 8.0) << ",\n"
       << "      \"latency_ns\": " << std::fixed << std::setprecision(1) << result.stats.latency_ns
       << ",\n"
       << "      \"efficiency_percent\": " << std::fixed << std::setprecision(1) << efficiency
       << ",\n"
       << "      \"num_threads\": " << result.num_threads << ",\n"
       << "      \"pattern_name\": \"" << result.pattern_name << "\"\n"
       << "    }";

    return ss.str();
}

std::string OutputFormatter::format_json_cache_aware_results(const std::string& pattern_name,
                                                             const std::vector<TestResult>& results,
                                                             const MemorySpecs& mem_specs) {
    std::stringstream ss;
    ss << "  {\n"
       << "    \"pattern_name\": \"" << pattern_name << "\",\n"
       << "    \"cache_aware\": true,\n"
       << "    \"results\": [\n";

    for(size_t i = 0; i < results.size(); ++i) {
        double efficiency = calculate_efficiency(results[i].stats.bandwidth_gbps,
                                                 mem_specs.theoretical_bandwidth_gbps);

        ss << "      {\n"
           << "        \"working_set_desc\": \"" << results[i].working_set_desc << "\",\n"
           << "        \"bandwidth_gbps\": " << std::fixed << std::setprecision(2)
           << results[i].stats.bandwidth_gbps << ",\n"
           << "        \"bandwidth_gb_s\": " << std::fixed << std::setprecision(2)
           << (results[i].stats.bandwidth_gbps * 8.0) << ",\n"
           << "        \"latency_ns\": " << std::fixed << std::setprecision(1)
           << results[i].stats.latency_ns << ",\n"
           << "        \"efficiency_percent\": " << std::fixed << std::setprecision(1) << efficiency
           << "\n"
           << "      }";

        if(i < results.size() - 1)
            ss << ",";
        ss << "\n";
    }

    ss << "    ]\n"
       << "  }";

    return ss.str();
}

// CSV formatting methods
std::string OutputFormatter::format_csv_system_info(const SystemInfo& sys_info) {
    std::stringstream ss;
    ss << "# System Information\n"
       << "CPU," << sys_info.cpu_name << "\n"
       << "Total RAM (GB)," << sys_info.total_ram_gb << "\n"
       << "Available RAM (GB)," << sys_info.available_ram_gb << "\n"
       << "Physical CPU Cores," << sys_info.cpu_cores << "\n"
       << "Logical CPU Threads," << sys_info.cpu_threads << "\n"
       << "Cache Line Size (bytes)," << sys_info.cache_line_size << "\n"
       << "Memory Type," << sys_info.memory_specs.type << "\n"
       << "Memory Speed (MT/s)," << sys_info.memory_specs.speed_mtps << "\n"
       << "Data Width (bits)," << sys_info.memory_specs.data_width_bits << "\n"
       << "Total Width (bits)," << sys_info.memory_specs.total_width_bits << "\n"
       << "Channels," << sys_info.memory_specs.num_channels;
    if(sys_info.memory_specs.is_virtualized) {
        if(sys_info.memory_specs.num_channels == 0) {
            ss << " (cannot detect)";
        } else {
            ss << " (estimated)";
        }
    } else if(!sys_info.memory_specs.num_channels_detected) {
        ss << " (not detected)";
    }
    ss << "\n"
       << "Theoretical Bandwidth (GB/s)," << std::fixed << std::setprecision(1)
       << sys_info.memory_specs.theoretical_bandwidth_gbps << "\n"
       << "Theoretical Bandwidth (Gb/s)," << std::fixed << std::setprecision(1)
       << (sys_info.memory_specs.theoretical_bandwidth_gbps * 8.0) << "\n"
       << "L1 Data Cache (KB)," << (sys_info.cache_info.l1_data_size / 1024) << "\n"
       << "L1 Instruction Cache (KB)," << (sys_info.cache_info.l1_instruction_size / 1024) << "\n"
       << "L2 Cache (KB)," << (sys_info.cache_info.l2_size / 1024) << "\n"
       << "L3 Cache (MB)," << (sys_info.cache_info.l3_size / (1024 * 1024)) << "\n"
       << "Cache Line Size (bytes)," << sys_info.cache_info.l1_line_size << "\n\n";

    return ss.str();
}

std::string OutputFormatter::format_csv_header() {
    return "# Test Results\n"
           "Test,Working Set,Threads,Bandwidth (GB/s),Bandwidth (Gb/s),Latency (ns),Efficiency "
           "(%)\n";
}

std::string OutputFormatter::format_csv_test_result(const TestResult& result,
                                                    const MemorySpecs& mem_specs) {
    double efficiency =
        calculate_efficiency(result.stats.bandwidth_gbps, mem_specs.theoretical_bandwidth_gbps);

    std::stringstream ss;
    ss << "\"" << result.test_name << "\","
       << "\"" << result.working_set_desc << "\"," << result.num_threads << "," << std::fixed
       << std::setprecision(2) << result.stats.bandwidth_gbps << "," << std::fixed
       << std::setprecision(2) << (result.stats.bandwidth_gbps * 8.0) << "," << std::fixed
       << std::setprecision(1) << result.stats.latency_ns << ",";
    
    // Handle efficiency display
    ss << OutputFormatterUtils::format_efficiency_display(efficiency, mem_specs.theoretical_bandwidth_gbps);
    ss << "\n";

    return ss.str();
}

std::string OutputFormatter::format_csv_cache_aware_results(const std::string& pattern_name,
                                                            const std::vector<TestResult>& results,
                                                            const MemorySpecs& mem_specs) {
    std::stringstream ss;
    ss << "# " << pattern_name << " (Cache-Aware)\n"
       << "Working Set,Threads,Bandwidth (GB/s),Bandwidth (Gb/s),Latency (ns),Efficiency (%)\n";

    for(const auto& result : results) {
        double efficiency =
            calculate_efficiency(result.stats.bandwidth_gbps, mem_specs.theoretical_bandwidth_gbps);

        ss << "\"" << result.working_set_desc << "\"," << result.num_threads << "," << std::fixed
           << std::setprecision(2) << result.stats.bandwidth_gbps << "," << std::fixed
           << std::setprecision(2) << (result.stats.bandwidth_gbps * 8.0) << "," << std::fixed
           << std::setprecision(1) << result.stats.latency_ns << ",";
        
        // Handle efficiency display
        ss << OutputFormatterUtils::format_efficiency_display(efficiency, mem_specs.theoretical_bandwidth_gbps);
        ss << "\n";
    }
    ss << "\n";

    return ss.str();
}

/**
 * @brief Calculate efficiency percentage based on achieved vs theoretical bandwidth
 *
 * @param bandwidth_gbps Achieved bandwidth in GB/s
 * @param theoretical_bandwidth_gbps Theoretical bandwidth in GB/s
 * @return Efficiency percentage (uncapped to show measurement issues)
 */
double OutputFormatter::calculate_efficiency(double bandwidth_gbps,
                                             double theoretical_bandwidth_gbps) const {
    if(theoretical_bandwidth_gbps < 0.0) {
        return -1.0;  // N/A for virtualized systems
    }
    
    if(theoretical_bandwidth_gbps <= 0.0) {
        return 0.0;
    }

    double efficiency = (bandwidth_gbps / theoretical_bandwidth_gbps) * 100.0;

    // Remove efficiency cap to reveal when measurements exceed theoretical limits
    // This helps identify when tests are measuring cache instead of main memory
    return efficiency;
}

/**
 * @brief Validate test results for suspicious patterns
 *
 * @param result Test result to validate
 * @param mem_specs Memory specifications for context
 * @return true if results are suspicious, false if they appear normal
 */
bool OutputFormatter::validate_test_result(const TestResult& result,
                                           const MemorySpecs& mem_specs) const {
    double efficiency =
        calculate_efficiency(result.stats.bandwidth_gbps, mem_specs.theoretical_bandwidth_gbps);

    // Check for suspicious patterns
    bool suspicious = false;

    // 1. Efficiency too high for virtualized environment
    if(mem_specs.is_virtualized && efficiency > BenchmarkConstants::MAX_EFFICIENCY_VIRTUALIZED) {
        suspicious = true;
    }

    // 2. Bandwidth exceeds theoretical limits
    if(result.stats.bandwidth_gbps > mem_specs.theoretical_bandwidth_gbps) {
        suspicious = true;
    }

    // 3. Unrealistically low latency
    if(result.stats.latency_ns < BenchmarkConstants::MIN_LATENCY_NS) {
        suspicious = true;
    }

    // 4. Zero or negative values
    if(result.stats.bandwidth_gbps <= 0.0 || result.stats.latency_ns <= 0.0) {
        suspicious = true;
    }

    return suspicious;
}

// Utility functions
std::string format_to_string(OutputFormat format) {
    switch(format) {
        case OutputFormat::MARKDOWN:
            return "markdown";
        case OutputFormat::JSON:
            return "json";
        case OutputFormat::CSV:
            return "csv";
        default:
            return "markdown";
    }
}

OutputFormat string_to_format(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);

    if(lower_str == "json") {
        return OutputFormat::JSON;
    } else if(lower_str == "csv") {
        return OutputFormat::CSV;
    } else {
        return OutputFormat::MARKDOWN;  // default
    }
}
