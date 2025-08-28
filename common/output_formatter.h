#ifndef OUTPUT_FORMATTER_H
#define OUTPUT_FORMATTER_H

#include <string>
#include <vector>

#include "memory_types.h"
#include "test_patterns.h"

/**
 * @brief Output format enumeration
 *
 * Defines the available output formats for test results.
 */
enum class OutputFormat {
    MARKDOWN,  ///< Markdown format (default)
    JSON,      ///< JSON format
    CSV        ///< CSV format
};

/**
 * @brief Test result structure for output formatting
 *
 * Contains all the information needed to format a test result,
 * including performance metrics and metadata.
 */
struct TestResult {
    std::string test_name;         ///< Name of the test
    std::string working_set_desc;  ///< Working set description (for cache-aware tests)
    PerformanceStats stats;        ///< Performance statistics
    size_t num_threads;            ///< Number of threads used
    std::string pattern_name;      ///< Pattern name
};

/**
 * @brief Output formatter class
 *
 * Handles formatting of test results in different output formats
 * including markdown, JSON, and CSV.
 */
class OutputFormatter {
  public:
    /**
     * @brief Constructor
     * @param format The output format to use
     */
    explicit OutputFormatter(OutputFormat format = OutputFormat::MARKDOWN);

    /**
     * @brief Formats system information
     * @param sys_info System information to format
     * @return Formatted system information string
     */
    std::string format_system_info(const SystemInfo& sys_info);
    std::string format_system_info_without_cache(const SystemInfo& sys_info);

    /**
     * @brief Formats test results header
     * @return Formatted header string
     */
    std::string format_header();

    /**
     * @brief Formats a single test result
     * @param result Test result to format
     * @param mem_specs Memory specifications for efficiency calculation
     * @return Formatted test result string
     */
    std::string format_test_result(const TestResult& result, const MemorySpecs& mem_specs);

    /**
     * @brief Formats multiple test results
     * @param results Vector of test results
     * @param mem_specs Memory specifications for efficiency calculation
     * @return Formatted test results string
     */
    std::string format_test_results(const std::vector<TestResult>& results,
                                    const MemorySpecs& mem_specs);

    /**
     * @brief Formats cache-aware test results
     * @param pattern_name Name of the test pattern
     * @param results Vector of test results for different working sets
     * @param mem_specs Memory specifications for efficiency calculation
     * @return Formatted cache-aware test results string
     */
    std::string format_cache_aware_results(const std::string& pattern_name,
                                           const std::vector<TestResult>& results,
                                           const MemorySpecs& mem_specs);

    /**
     * @brief Formats test completion message
     * @return Formatted completion message
     */
    std::string format_completion_message();

  private:
    OutputFormat format_;  ///< Current output format

    // Helper methods for different formats
    std::string format_markdown_system_info(const SystemInfo& sys_info);
    std::string format_markdown_system_info_without_cache(const SystemInfo& sys_info);
    std::string format_json_system_info(const SystemInfo& sys_info);
    std::string format_csv_system_info(const SystemInfo& sys_info);

    std::string format_markdown_header();
    std::string format_json_header();
    std::string format_csv_header();

    std::string format_markdown_test_result(const TestResult& result, const MemorySpecs& mem_specs);
    std::string format_json_test_result(const TestResult& result, const MemorySpecs& mem_specs);
    std::string format_csv_test_result(const TestResult& result, const MemorySpecs& mem_specs);

    std::string format_markdown_cache_aware_results(const std::string& pattern_name,
                                                    const std::vector<TestResult>& results,
                                                    const MemorySpecs& mem_specs);
    std::string format_json_cache_aware_results(const std::string& pattern_name,
                                                const std::vector<TestResult>& results,
                                                const MemorySpecs& mem_specs);
    std::string format_csv_cache_aware_results(const std::string& pattern_name,
                                               const std::vector<TestResult>& results,
                                               const MemorySpecs& mem_specs);

    /**
     * @brief Calculates efficiency percentage
     * @param bandwidth_gbps Achieved bandwidth in GB/s
     * @param theoretical_bandwidth_gbps Theoretical bandwidth in GB/s
     * @return Efficiency percentage
     */
    double calculate_efficiency(double bandwidth_gbps, double theoretical_bandwidth_gbps) const;
    bool validate_test_result(const TestResult& result, const MemorySpecs& mem_specs) const;
};

// Utility functions
std::string format_to_string(OutputFormat format);
OutputFormat string_to_format(const std::string& str);

#endif  // OUTPUT_FORMATTER_H
