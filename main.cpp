#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>
#include <vector>
#include <memory>

// Common includes
#include "common/memory_types.h"
#include "common/platform_interface.h"
#include "common/working_sets.h"
#include "common/output_formatter.h"
#include "common/standard_tests.h"
#include "common/test_patterns.h"
#include "common/argument_parser.h"
#include "common/system_info_display.h"
#include "common/memory_utils.h"
#include "common/constants.h"
#include "common/errors.h"

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
    std::vector<uint8_t*> raw_buffers;
    std::vector<uint8_t*> aligned_buffers;
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

    bool allocate_buffers(size_t total_size, size_t num_buffers, bool silent = false) {
        (void)silent;
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

        for(size_t i = 0; i < num_buffers; ++i) {
            // Allocate extra space to ensure we can achieve cache line alignment
            uint8_t* raw_buffer = nullptr;
            try {
                raw_buffer = new uint8_t[buffer_size + cache_line_size];
            } catch (const std::bad_alloc& e) {
                cleanup_buffers();
                throw MemoryError("Failed to allocate buffer " + std::to_string(i) + 
                                " of size " + std::to_string(buffer_size + cache_line_size) + " bytes: " + e.what());
            }
            if(!raw_buffer) {
                cleanup_buffers();
                throw MemoryError("Memory allocation returned nullptr for buffer " + std::to_string(i));
            }

            /**
             * Cache Line Aligned Buffer Allocation
             * 
             * Problem: new[] doesn't guarantee cache line alignment, which can hurt performance
             * Solution: Allocate extra space and manually align the buffer
             * 
             * Step 1: Get raw pointer address as integer
             * Step 2: Round UP to next cache line boundary using bit masking
             *   - Formula: (addr + cache_line_size - 1) & ~(cache_line_size - 1)
             *   - The mask ~(cache_line_size - 1) clears the lower bits to align
             *   - Example: addr=0x1009, cache_line=64 (0x40)
             *     → (0x1009 + 0x3F) & ~0x3F → 0x1048 & 0xFFC0 → 0x1040
             * Step 3: Convert back to pointer for aligned memory access
             * 
             * This ensures all buffer accesses are cache line aligned for optimal performance.
             */
            uintptr_t addr = reinterpret_cast<uintptr_t>(raw_buffer);
            uintptr_t aligned_addr = (addr + cache_line_size - 1) & ~(cache_line_size - 1);
            uint8_t* aligned_buffer = reinterpret_cast<uint8_t*>(aligned_addr);

            raw_buffers.push_back(raw_buffer);
            aligned_buffers.push_back(aligned_buffer);

            for(size_t j = 0; j < buffer_size; ++j) {
                aligned_buffer[j] = static_cast<uint8_t>(j & 0xFF);
            }
        }
        return true;
    }

    void cleanup_buffers() {
        for(auto buffer : raw_buffers) {
            delete[] buffer;
        }
        raw_buffers.clear();
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
            size_t cache_line_size = 64;
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
                    TestPattern::COPY, TestPattern::TRIAD};
    } else {
        if(pattern_str == "sequential_read") patterns.push_back(TestPattern::SEQUENTIAL_READ);
        else if(pattern_str == "sequential_write") patterns.push_back(TestPattern::SEQUENTIAL_WRITE);
        else if(pattern_str == "random_read") patterns.push_back(TestPattern::RANDOM_READ);
        else if(pattern_str == "random_write") patterns.push_back(TestPattern::RANDOM_WRITE);
        else if(pattern_str == "copy") patterns.push_back(TestPattern::COPY);
        else if(pattern_str == "triad") patterns.push_back(TestPattern::TRIAD);
        else {
            std::cerr << "Error: Unknown pattern '" << pattern_str << "'" << std::endl;
            exit(1);
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