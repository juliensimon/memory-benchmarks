#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

// Common includes
#include "common/memory_types.h"
#include "common/platform_interface.h"
#include "common/working_sets.h"
#include "common/output_formatter.h"
#include "common/standard_tests.h"
#include "common/matrix_multiply_interface.h"
#include "common/test_patterns.h"
#include "common/argument_parser.h"
#include "common/system_info_display.h"
#include "common/memory_utils.h"
#include "common/constants.h"
#include "common/errors.h"
#include "common/aligned_buffer.h"

using namespace BenchmarkConstants;

/**
 * @brief Format memory size without trailing zeros
 */
std::string format_memory_size(double size_gb) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(6) << size_gb;
    std::string result = ss.str();

    size_t decimal_pos = result.find('.');
    if(decimal_pos != std::string::npos) {
        size_t last_non_zero = result.length() - 1;
        while(last_non_zero > decimal_pos && result[last_non_zero] == '0') {
            last_non_zero--;
        }
        if(last_non_zero == decimal_pos) {
            result = result.substr(0, decimal_pos);
        } else {
            result = result.substr(0, last_non_zero + 1);
        }
    }
    return result + "GB";
}

/**
 * @brief Memory Bandwidth Tester class
 */
class MemoryBandwidthTester {
private:
    std::unique_ptr<PlatformInterface> platform;
    CacheInfo cache_info;
    WorkingSetSizes working_sets;
    std::vector<AlignedBuffer> buffers;
    std::vector<uint8_t*> aligned_buffers; // Keep for compatibility with existing test functions
    size_t current_buffer_size;
    std::atomic<bool> stop_flag;
    OutputFormatter formatter;
    size_t cache_line_size;
    SystemInfo cached_system_info;
    CPUAffinityType cpu_affinity;

public:
    MemoryBandwidthTester(OutputFormat output_format = OutputFormat::MARKDOWN, 
                         CPUAffinityType affinity_type = CPUAffinityType::DEFAULT)
        : platform(create_platform_interface()),
          cache_info(platform->get_core_specific_cache_info(affinity_type)),
          working_sets(cache_info),
          stop_flag(false),
          formatter(output_format),
          cache_line_size(platform->detect_cache_line_size()),
          cached_system_info(platform->get_system_info()),
          cpu_affinity(affinity_type) {}

    ~MemoryBandwidthTester() {
        cleanup_buffers();
    }

    bool allocate_buffers(size_t total_size, size_t num_buffers) {
        if(total_size == 0 || num_buffers == 0) {
            throw MemoryError("Invalid buffer allocation parameters: total_size=" + 
                            std::to_string(total_size) + ", num_buffers=" + std::to_string(num_buffers));
        }

        size_t buffer_size = MemoryUtils::calculate_buffer_size(total_size, num_buffers, cache_line_size);
        if(buffer_size == 0) {
            throw MemoryError("Buffer size too small: calculated size=" + std::to_string(buffer_size) + 
                            " bytes, minimum required=" + std::to_string(MIN_BUFFER_SIZE) + " bytes");
        }

        cleanup_buffers();
        current_buffer_size = buffer_size;

        try {
            buffers.reserve(num_buffers);
            aligned_buffers.reserve(num_buffers);
            
            for(size_t i = 0; i < num_buffers; ++i) {
                // Create aligned buffer using RAII - automatically handles alignment and initialization
                buffers.emplace_back(buffer_size, cache_line_size);
                
                // Verify alignment was achieved
                if (!buffers.back().is_aligned()) {
                    throw MemoryError("Failed to achieve cache line alignment for buffer " + std::to_string(i));
                }
                
                // Store pointer for compatibility with existing test functions
                aligned_buffers.push_back(buffers.back().data());
            }
        } catch (const std::bad_alloc& e) {
            cleanup_buffers();
            throw MemoryError("Failed to allocate buffer of size " + std::to_string(buffer_size) + " bytes: " + e.what());
        } catch (const std::invalid_argument& e) {
            cleanup_buffers();
            throw MemoryError("Invalid buffer parameters: " + std::string(e.what()));
        }
        return true;
    }

    void cleanup_buffers() {
        // RAII: AlignedBuffer destructors automatically handle memory cleanup
        buffers.clear();
        aligned_buffers.clear();
    }

    PerformanceStats run_test(TestPattern pattern, size_t iterations, size_t num_threads, bool cache_aware = false) {
        if(aligned_buffers.empty()) return {0.0, 0.0, 0, 0.0};

        size_t buffer_size = current_buffer_size;
        size_t bytes_per_thread = buffer_size / num_threads;

        std::vector<std::thread> threads;
        std::vector<PerformanceStats> thread_results(num_threads);

        auto start_time = std::chrono::high_resolution_clock::now();

        for(size_t i = 0; i < num_threads; ++i) {
            size_t start_offset = i * bytes_per_thread;
            size_t end_offset = (i == num_threads - 1) ? buffer_size : (i + 1) * bytes_per_thread;

            threads.emplace_back([this, pattern, start_offset, end_offset, iterations, i,
                                  &thread_results, buffer_size, cache_aware, num_threads]() {
                // Set thread affinity
                platform->set_thread_affinity(i, cpu_affinity, num_threads);

                switch(pattern) {
                    case TestPattern::SEQUENTIAL_READ:
                        thread_results[i] = StandardTests::sequential_read_test(
                            aligned_buffers[0], buffer_size, start_offset, end_offset, iterations,
                            stop_flag, cache_aware);
                        break;
                    case TestPattern::SEQUENTIAL_WRITE:
                        thread_results[i] = StandardTests::sequential_write_test(
                            aligned_buffers[0], buffer_size, start_offset, end_offset, iterations,
                            stop_flag);
                        break;
                    case TestPattern::RANDOM_READ:
                        thread_results[i] = StandardTests::random_access_test(
                            aligned_buffers[0], buffer_size, start_offset, end_offset, iterations,
                            false, stop_flag);
                        break;
                    case TestPattern::RANDOM_WRITE:
                        thread_results[i] = StandardTests::random_access_test(
                            aligned_buffers[0], buffer_size, start_offset, end_offset, iterations,
                            true, stop_flag);
                        break;
                    case TestPattern::COPY:
                        if(aligned_buffers.size() >= 2) {
                            thread_results[i] = StandardTests::copy_test(
                                aligned_buffers[0], aligned_buffers[1], buffer_size, start_offset,
                                end_offset, iterations, stop_flag);
                        }
                        break;
                    case TestPattern::TRIAD:
                        if(aligned_buffers.size() >= 4) {
                            thread_results[i] = StandardTests::triad_test(
                                aligned_buffers[0], aligned_buffers[1], aligned_buffers[2],
                                aligned_buffers[3], buffer_size, start_offset, end_offset,
                                iterations, stop_flag);
                        }
                        break;
                    case TestPattern::MATRIX_MULTIPLY: {
                        // Matrix multiplication uses different parameters
                        size_t matrix_size = 1024;  // Default matrix size
                        MatrixMultiply::MatrixConfig matrix_config = 
                            MatrixMultiply::create_matrix_config(matrix_size, iterations, false);
                        
                        auto matrix_stats = StandardTests::matrix_multiply_test(matrix_config, stop_flag);
                        
                        // Convert matrix stats to PerformanceStats for compatibility
                        PerformanceStats stats;
                        stats.bandwidth_gbps = matrix_stats.bandwidth_gbps;
                        stats.latency_ns = matrix_stats.latency_ns;
                        stats.bytes_processed = matrix_stats.bytes_processed;
                        stats.time_seconds = matrix_stats.time_seconds;
                        thread_results[i] = stats;
                        break;
                    }
                }
            });
        }

        for(auto& thread : threads) {
            thread.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        double total_time = std::chrono::duration<double>(end_time - start_time).count();

        return aggregate_stats(thread_results, total_time);
    }

    std::vector<TestResult> run_cache_aware_test(TestPattern pattern, size_t iterations, size_t num_threads) {
        std::vector<TestResult> results;
        auto [sizes, descriptions] = WorkingSetSizes::get_thread_aware_sizes(cache_info, num_threads);

        for(size_t i = 0; i < sizes.size(); ++i) {
            size_t working_set_size = sizes[i];
            if(working_set_size < MIN_WORKING_SET_SIZE) continue;

            try {
                if(!allocate_buffers(working_set_size, 4)) continue;
            } catch (const MemoryError& e) {
                // Skip this working set size if allocation fails
                std::cerr << "Warning: " << e.what() << ". Skipping working set size." << std::endl;
                continue;
            }

            size_t scaled_iterations;
            scaled_iterations = MemoryUtils::scale_iterations(iterations, working_set_size);

            PerformanceStats stats = run_test(pattern, scaled_iterations, num_threads, true);

            TestResult result;
            result.test_name = get_pattern_name(pattern);
            result.working_set_desc = descriptions[i];
            result.stats = stats;
            result.num_threads = num_threads;
            result.pattern_name = get_pattern_name(pattern);

            results.push_back(result);
        }
        return results;
    }


    void print_cache_results(const std::string& pattern_name, const std::vector<TestResult>& results) {
        std::cout << formatter.format_cache_aware_results(pattern_name, results, cached_system_info.memory_specs);
    }

    const SystemInfo& get_cached_system_info() const {
        return cached_system_info;
    }

private:
    /**
     * @brief Aggregates performance statistics from multiple threads
     * 
     * Combines thread-level performance statistics into a single aggregate result.
     * Calculates total bytes processed across all threads and computes aggregate
     * bandwidth based on total time. Latency is calculated based on cache line
     * accesses and total execution time.
     * 
     * @param thread_results Vector of performance statistics from individual threads
     * @param total_time Total execution time in seconds for the entire test
     * @return Aggregated performance statistics with combined metrics
     */
    PerformanceStats aggregate_stats(const std::vector<PerformanceStats>& thread_results, double total_time) {
        PerformanceStats aggregated{};
        aggregated.time_seconds = total_time;

        for(const auto& result : thread_results) {
            aggregated.bytes_processed += result.bytes_processed;
        }

        if(total_time > 0.0) {
            aggregated.bandwidth_gbps = aggregated.bytes_processed / (total_time * 1e9);
        }

        if(aggregated.bytes_processed > 0) {
            size_t cache_line_size = CacheConstants::DEFAULT_CACHE_LINE_SIZE;
            size_t accesses = aggregated.bytes_processed / cache_line_size;
            if(accesses > 0) {
                aggregated.latency_ns = (total_time * 1e9) / accesses;
            } else {
                aggregated.latency_ns = 0.0;
            }
        } else {
            aggregated.latency_ns = 0.0;
        }

        return aggregated;
    }
};


std::vector<TestPattern> parse_patterns(const std::string& pattern_str) {
    std::vector<TestPattern> patterns;
    
    if(pattern_str == "all") {
        patterns = {TestPattern::SEQUENTIAL_READ, TestPattern::SEQUENTIAL_WRITE,
                    TestPattern::RANDOM_READ, TestPattern::RANDOM_WRITE,
                    TestPattern::COPY, TestPattern::TRIAD, TestPattern::MATRIX_MULTIPLY};
    } else {
        static const std::map<std::string, TestPattern> pattern_map = {
            {"sequential_read", TestPattern::SEQUENTIAL_READ},
            {"sequential_write", TestPattern::SEQUENTIAL_WRITE},
            {"random_read", TestPattern::RANDOM_READ},
            {"random_write", TestPattern::RANDOM_WRITE},
            {"copy", TestPattern::COPY},
            {"triad", TestPattern::TRIAD},
            {"matrix_multiply", TestPattern::MATRIX_MULTIPLY}
        };
        
        auto it = pattern_map.find(pattern_str);
        if (it != pattern_map.end()) {
            patterns.push_back(it->second);
        } else {
            throw ArgumentError("Unknown pattern '" + pattern_str + "'");
        }
    }
    return patterns;
}

int main(int argc, char* argv[]) {
    ArgumentParser parser(argv[0], "Comprehensive memory bandwidth benchmark tool with platform-specific optimizations");
    
    try {
        BenchmarkConfig config = parser.parse(argc, argv);
        
        // Handle special flags
        if (config.help_requested) {
            parser.print_help();
            return 0;
        }
        
        if (config.show_info) {
            std::cout << "Memory Bandwidth Test Tool - System Information\n\n";
            auto platform = create_platform_interface();
            OutputFormat output_format = string_to_format(config.format_str);
            SystemInfoDisplay::print_system_info(platform, output_format, true);
            return 0;
        }

        // Platform-specific CPU affinity validation
        if (config.cpu_affinity == CPUAffinityType::P_CORES && config.num_threads > 12) {
            std::cerr << "Error: P-cores are limited to 12 threads (requested: " << config.num_threads << ")" << std::endl;
            return 1;
        }
        if (config.cpu_affinity == CPUAffinityType::E_CORES && config.num_threads > 4) {
            std::cerr << "Error: E-cores are limited to 4 threads (requested: " << config.num_threads << ")" << std::endl;
            return 1;
        }

        OutputFormat output_format = string_to_format(config.format_str);
        MemoryBandwidthTester tester(output_format, config.cpu_affinity);
        auto platform = create_platform_interface();

        SystemInfoDisplay::print_cached_system_info(
            tester.get_cached_system_info(), platform, output_format, config.cpu_affinity);

        std::vector<TestPattern> patterns = parse_patterns(config.pattern_str);
        OutputFormatter formatter(output_format);

        if(config.cache_hierarchy) {
            std::cout << "\n=== CACHE HIERARCHY MODE ===\n";
            std::cout << "Testing with working sets sized for L1, L2, L3 caches\n";
            std::cout << "No cache interference - demonstrating peak cache performance\n\n";
            
            for(TestPattern pattern : patterns) {
                std::vector<TestResult> results = tester.run_cache_aware_test(pattern, config.iterations, config.num_threads);
                tester.print_cache_results(get_pattern_name(pattern), results);
            }
        } else {
            std::cout << "\n=== LARGE MEMORY MODE ===\n";
            std::cout << "Testing with large working sets (>4GB) - Natural system performance\n";
            std::cout << "No cache interference - let hardware prefetchers and memory controllers work naturally\n\n";
            
            std::cout << formatter.format_header();
            std::vector<TestResult> results;

            for(double memory_size_gb : config.memory_sizes_gb) {
                size_t total_size = static_cast<size_t>(memory_size_gb * 1024 * 1024 * 1024);
                size_t num_buffers = 4;

                try {
                    if(!tester.allocate_buffers(total_size, num_buffers)) {
                        throw MemoryError("Failed to allocate memory buffers for large-memory test with size " +
                                        std::to_string(memory_size_gb) + "GB");
                    }
                } catch (const MemoryError& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                    return 1;
                }

                for(TestPattern pattern : patterns) {
                    PerformanceStats stats = tester.run_test(pattern, config.iterations, config.num_threads, false);

                    TestResult result;
                    result.test_name = get_pattern_name(pattern);
                    result.working_set_desc = format_memory_size(memory_size_gb);
                    result.stats = stats;
                    result.num_threads = config.num_threads;
                    result.pattern_name = get_pattern_name(pattern);

                    results.push_back(result);
                }
            }

            std::cout << formatter.format_test_results(results, tester.get_cached_system_info().memory_specs);
        }

        std::cout << formatter.format_completion_message();
        return 0;
        
    } catch (const ArgumentError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Use --help for usage information." << std::endl;
        return 1;
    } catch (const MemoryError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const PlatformError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const TestError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const ConfigurationError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const BenchmarkError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}