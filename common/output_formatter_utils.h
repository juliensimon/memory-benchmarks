#ifndef OUTPUT_FORMATTER_UTILS_H
#define OUTPUT_FORMATTER_UTILS_H

#include <string>
#include <sstream>
#include <iomanip>
#include "memory_types.h"

/**
 * @brief Utility functions for output formatting
 *
 * This module contains shared formatting logic used by the OutputFormatter
 * to reduce code duplication between different system info formatting methods.
 */
namespace OutputFormatterUtils {

    /**
     * @brief Format basic system information (CPU, RAM, cores)
     *
     * @param sys_info System information structure
     * @return Formatted basic system information string
     */
    std::string format_basic_system_info(const SystemInfo& sys_info);

    /**
     * @brief Format memory specifications section
     *
     * @param mem_specs Memory specifications structure
     * @return Formatted memory specifications string
     */
    std::string format_memory_specifications(const MemorySpecs& mem_specs);

    /**
     * @brief Format cache information section
     *
     * @param cache_info Cache information structure  
     * @param mem_specs Memory specifications for architecture detection
     * @return Formatted cache information string
     */
    std::string format_cache_information(const CacheInfo& cache_info, const MemorySpecs& mem_specs);

    /**
     * @brief Format efficiency display with proper handling of edge cases
     *
     * @param efficiency Calculated efficiency percentage
     * @param theoretical_bandwidth Theoretical bandwidth for validation
     * @return Formatted efficiency string
     */
    std::string format_efficiency_display(double efficiency, double theoretical_bandwidth);

    /**
     * @brief Check if memory type appears to be detected vs estimated
     *
     * @param memory_type Memory type string
     * @return true if type appears detected, false if estimated
     */
    bool is_memory_type_detected(const std::string& memory_type);

} // namespace OutputFormatterUtils

#endif // OUTPUT_FORMATTER_UTILS_H