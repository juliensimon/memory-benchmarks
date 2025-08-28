#include "argument_parser.h"
#include "test_patterns.h"
#include "output_formatter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>

ArgumentParser::ArgumentParser(const std::string& program_name, const std::string& description)
    : program_name_(program_name), description_(description), platform_(create_platform_interface()) {
    
    // Define all supported arguments
    add_argument("--help", "-h", "Show this help message", false,
        [](BenchmarkConfig& config, const std::string&) {
            config.help_requested = true;
        });
    
    add_argument("--info", "", "Show build and platform information", false,
        [](BenchmarkConfig& config, const std::string&) {
            config.show_info = true;
        });
    
    add_argument("--size", "", "Memory size in GB for large-memory mode (default: 6)", true,
        [this](BenchmarkConfig& config, const std::string& value) {
            config.memory_sizes_gb = parse_memory_sizes(value);
        });
    
    add_argument("--iterations", "", "Number of iterations (default: 10)", true,
        [](BenchmarkConfig& config, const std::string& value) {
            try {
                config.iterations = std::stoull(value);
                if (config.iterations == 0) {
                    throw ArgumentError("Iterations must be greater than 0");
                }
            } catch (const std::exception&) {
                throw ArgumentError("Invalid iterations value: " + value);
            }
        });
    
    add_argument("--threads", "", "Number of threads (default: auto-detect)", true,
        [](BenchmarkConfig& config, const std::string& value) {
            try {
                config.num_threads = std::stoull(value);
                if (config.num_threads == 0) {
                    throw ArgumentError("Thread count must be greater than 0");
                }
            } catch (const std::exception&) {
                throw ArgumentError("Invalid thread count: " + value);
            }
        });
    
    add_argument("--pattern", "", "Test pattern: sequential_read, sequential_write, random_read, random_write, copy, triad (default: all)", true,
        [](BenchmarkConfig& config, const std::string& value) {
            config.pattern_str = value;
        });
    
    add_argument("--format", "", "Output format: markdown, json, csv (default: markdown)", true,
        [](BenchmarkConfig& config, const std::string& value) {
            config.format_str = value;
        });
    
    add_argument("--cache-hierarchy", "", "Cache-sized working sets (L1/L2/L3) - Peak cache performance", false,
        [](BenchmarkConfig& config, const std::string&) {
            config.cache_hierarchy = true;
            config.memory_sizes_gb.clear();  // Clear sizes for cache mode
        });
    
    add_argument("--large-memory", "", "Large working sets (>4GB) - Natural system performance", false,
        [](BenchmarkConfig& config, const std::string&) {
            config.cache_hierarchy = false;
        });
    
    // Platform-specific arguments
    if (platform_ && platform_->supports_cpu_affinity()) {
        if (platform_->get_platform_name() == "macOS") {
            add_argument("--p-cores", "", "Run only on Performance cores (Apple Silicon)", false,
                [](BenchmarkConfig& config, const std::string&) {
                    config.cpu_affinity = CPUAffinityType::P_CORES;
                });
            
            add_argument("--e-cores", "", "Run only on Efficiency cores (Apple Silicon)", false,
                [](BenchmarkConfig& config, const std::string&) {
                    config.cpu_affinity = CPUAffinityType::E_CORES;
                });
        }
    }
}

void ArgumentParser::add_argument(const std::string& long_name, 
                                 const std::string& short_name,
                                 const std::string& help,
                                 bool requires_value,
                                 std::function<void(BenchmarkConfig&, const std::string&)> handler) {
    arguments_.push_back({long_name, short_name, help, requires_value, handler});
}

BenchmarkConfig ArgumentParser::parse(int argc, char* argv[]) {
    BenchmarkConfig config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        bool found = false;
        
        // Find matching argument definition
        for (const auto& arg_def : arguments_) {
            if (arg == arg_def.long_name || (!arg_def.short_name.empty() && arg == arg_def.short_name)) {
                found = true;
                
                if (arg_def.requires_value) {
                    if (i + 1 >= argc) {
                        throw ArgumentError("Argument " + arg + " requires a value");
                    }
                    std::string value = argv[++i];
                    if (arg_def.handler) {
                        arg_def.handler(config, value);
                    }
                } else {
                    if (arg_def.handler) {
                        arg_def.handler(config, "");
                    }
                }
                break;
            }
        }
        
        if (!found) {
            throw ArgumentError("Unknown argument: " + arg);
        }
        
        // Early return for help
        if (config.help_requested) {
            return config;
        }
    }
    
    // Set default thread count if not specified
    if (config.num_threads == 0) {
        config.num_threads = std::thread::hardware_concurrency();
    }
    
    // Validate the complete configuration
    validate_config(config);
    
    return config;
}

void ArgumentParser::validate_config(BenchmarkConfig& config) {
    validate_thread_count(config);
    validate_memory_sizes(config);
    validate_pattern(config);
    validate_format(config);
}

void ArgumentParser::validate_thread_count(const BenchmarkConfig& config) {
    if (config.num_threads == 0) {
        throw ArgumentError("Thread count must be greater than 0");
    }
    
    // Platform-specific thread count validation
    if (platform_ && config.cpu_affinity != CPUAffinityType::DEFAULT) {
        size_t max_threads = platform_->get_max_threads_for_affinity(config.cpu_affinity);
        if (config.num_threads > max_threads) {
            std::string core_type;
            if (config.cpu_affinity == CPUAffinityType::P_CORES) {
                core_type = "P-cores";
            } else if (config.cpu_affinity == CPUAffinityType::E_CORES) {
                core_type = "E-cores";
            }
            
            throw ArgumentError("Thread count (" + std::to_string(config.num_threads) + 
                              ") exceeds available " + core_type + " (" + 
                              std::to_string(max_threads) + " available)");
        }
    }
    
    // General sanity check
    size_t total_threads = std::thread::hardware_concurrency();
    if (config.num_threads > total_threads * BenchmarkConstants::MAX_THREAD_OVERSUBSCRIPTION) {
        throw ArgumentError("Thread count (" + std::to_string(config.num_threads) + 
                          ") is excessively high (system has " + std::to_string(total_threads) + " threads)");
    }
}

void ArgumentParser::validate_memory_sizes(const BenchmarkConfig& config) {
    if (!config.cache_hierarchy && config.memory_sizes_gb.empty()) {
        throw ArgumentError("No memory sizes specified for large-memory mode");
    }
    
    for (double size : config.memory_sizes_gb) {
        if (size <= 0) {
            throw ArgumentError("Memory size must be greater than 0 GB");
        }
        if (size > BenchmarkConstants::MAX_MEMORY_SIZE_GB) {
            throw ArgumentError("Memory size " + std::to_string(size) + " GB is too large (max " + 
                              std::to_string(BenchmarkConstants::MAX_MEMORY_SIZE_GB) + " GB)");
        }
    }
}

void ArgumentParser::validate_pattern(const BenchmarkConfig& config) {
    auto supported = get_supported_patterns();
    if (std::find(supported.begin(), supported.end(), config.pattern_str) == supported.end()) {
        std::string valid_patterns;
        for (size_t i = 0; i < supported.size(); ++i) {
            if (i > 0) valid_patterns += ", ";
            valid_patterns += supported[i];
        }
        throw ArgumentError("Invalid pattern '" + config.pattern_str + "'. Valid patterns: " + valid_patterns);
    }
}

void ArgumentParser::validate_format(const BenchmarkConfig& config) {
    auto supported = get_supported_formats();
    if (std::find(supported.begin(), supported.end(), config.format_str) == supported.end()) {
        std::string valid_formats;
        for (size_t i = 0; i < supported.size(); ++i) {
            if (i > 0) valid_formats += ", ";
            valid_formats += supported[i];
        }
        throw ArgumentError("Invalid format '" + config.format_str + "'. Valid formats: " + valid_formats);
    }
}

std::vector<double> ArgumentParser::parse_memory_sizes(const std::string& size_str) {
    std::vector<double> sizes;
    std::stringstream ss(size_str);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        // Trim whitespace
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        
        if (!item.empty()) {
            try {
                double size = std::stod(item);
                if (size <= 0) {
                    throw ArgumentError("Memory size must be positive: " + item);
                }
                sizes.push_back(size);
            } catch (const std::invalid_argument&) {
                throw ArgumentError("Invalid memory size value: " + item);
            }
        }
    }
    
    if (sizes.empty()) {
        throw ArgumentError("No valid memory sizes provided");
    }
    
    return sizes;
}

std::vector<std::string> ArgumentParser::get_supported_patterns() const {
    return {"all", "sequential_read", "sequential_write", "random_read", "random_write", "copy", "triad"};
}

std::vector<std::string> ArgumentParser::get_supported_formats() const {
    return {"markdown", "json", "csv"};
}

void ArgumentParser::print_usage() const {
    std::cout << "Usage: " << program_name_ << " [OPTIONS]\n";
}

void ArgumentParser::print_help() const {
    std::cout << "Memory Bandwidth Benchmark - Natural System Performance\n\n";
    print_usage();
    
    if (!description_.empty()) {
        std::cout << "\n" << description_ << "\n";
    }
    
    std::cout << "\nTWO MODES:\n";
    std::cout << "  --large-memory       Large working sets (>4GB) - Natural system performance\n";
    std::cout << "  --cache-hierarchy    Cache-sized working sets (L1/L2/L3) - Peak cache performance\n";
    
    std::cout << "\nOptions:\n";
    for (const auto& arg : arguments_) {
        std::cout << "  " << arg.long_name;
        if (!arg.short_name.empty()) {
            std::cout << ", " << arg.short_name;
        }
        if (arg.requires_value) {
            std::cout << " VALUE";
        }
        std::cout << "\n        " << arg.help << "\n";
    }
    
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name_ << " --large-memory --size 8 --iterations 5\n";
    std::cout << "  " << program_name_ << " --cache-hierarchy --pattern sequential_read\n";
    
    if (platform_ && platform_->supports_cpu_affinity()) {
        if (platform_->get_platform_name() == "macOS") {
            std::cout << "  " << program_name_ << " --cache-hierarchy --p-cores\n";
            std::cout << "  " << program_name_ << " --large-memory --e-cores --threads 4\n";
        }
    }
}