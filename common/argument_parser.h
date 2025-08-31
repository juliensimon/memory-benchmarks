#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include "memory_types.h"
#include "platform_interface.h"
#include "errors.h"
#include "constants.h"

/**
 * @brief Configuration structure for parsed arguments
 */
struct BenchmarkConfig {
    // Test configuration
    std::vector<double> memory_sizes_gb;
    size_t iterations;
    size_t num_threads;
    std::string pattern_str;
    bool cache_hierarchy;
    std::string format_str;
    CPUAffinityType cpu_affinity;
    
    // Flags
    bool help_requested;
    bool show_info;
    
    // Default constructor
    BenchmarkConfig() 
        : memory_sizes_gb({BenchmarkConstants::DEFAULT_MEMORY_SIZE_GB})
        , iterations(BenchmarkConstants::DEFAULT_ITERATIONS)
        , num_threads(0)  // Will be set to hardware_concurrency if 0
        , pattern_str("all")
        , cache_hierarchy(false)
        , format_str("markdown")
        , cpu_affinity(CPUAffinityType::DEFAULT)
        , help_requested(false)
        , show_info(false) {}
};

// ArgumentError is now defined in errors.h

/**
 * @brief Robust argument parser with validation
 */
class ArgumentParser {
private:
    std::string program_name_;
    std::string description_;
    std::unique_ptr<PlatformInterface> platform_;
    
    struct ArgumentDefinition {
        std::string long_name;
        std::string short_name;
        std::string help;
        bool requires_value;
        std::function<void(BenchmarkConfig&, const std::string&)> handler;
    };
    
    std::vector<ArgumentDefinition> arguments_;
    std::vector<std::string> positional_args_;
    
public:
    ArgumentParser(const std::string& program_name, const std::string& description = "");
    
    // Add argument definitions
    void add_argument(const std::string& long_name, 
                     const std::string& short_name,
                     const std::string& help,
                     bool requires_value = false,
                     std::function<void(BenchmarkConfig&, const std::string&)> handler = nullptr);
    
    // Parse arguments and return config
    BenchmarkConfig parse(int argc, char* argv[]);
    
    // Print usage information
    void print_usage() const;
    void print_help() const;
    
private:
    // Validation methods
    void validate_config(BenchmarkConfig& config);
    void validate_thread_count(const BenchmarkConfig& config);
    void validate_memory_sizes(const BenchmarkConfig& config);
    void validate_pattern(const BenchmarkConfig& config);
    void validate_format(const BenchmarkConfig& config);
    void validate_mode_compatibility(const BenchmarkConfig& config);
    
    // Helper methods
    std::vector<double> parse_memory_sizes(const std::string& size_str);
    std::vector<std::string> get_supported_patterns() const;
    std::vector<std::string> get_supported_formats() const;
};

#endif  // ARGUMENT_PARSER_H