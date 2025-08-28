#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <cstddef>
#include <utility>
#include "memory_types.h"

/**
 * @brief Utility functions for memory operations and alignment
 *
 * This module provides common utility functions used across the memory
 * benchmark suite, including alignment calculations and buffer validation.
 */
namespace MemoryUtils {

    /**
     * @brief Align memory address to cache line boundary
     *
     * This function performs cache line alignment to optimize memory access patterns.
     * It aligns the start address UP to the next cache line boundary and the end
     * address DOWN to the previous cache line boundary.
     *
     * @param start_offset Starting offset to align
     * @param end_offset Ending offset to align  
     * @param cache_line_size Cache line size in bytes
     * @return Pair of (aligned_start, aligned_end) offsets
     *
     * @note If aligned_end <= aligned_start, the range is too small for alignment
     */
    std::pair<size_t, size_t> align_to_cache_lines(size_t start_offset, 
                                                   size_t end_offset,
                                                   size_t cache_line_size);

    /**
     * @brief Calculate working set size from aligned boundaries
     *
     * @param aligned_start Aligned start offset
     * @param aligned_end Aligned end offset
     * @return Working set size in bytes
     */
    size_t calculate_working_set_size(size_t aligned_start, size_t aligned_end);

    /**
     * @brief Validate that a buffer range is suitable for testing
     *
     * @param start_offset Starting offset
     * @param end_offset Ending offset
     * @param buffer_size Total buffer size
     * @param min_size Minimum required size
     * @return true if range is valid, false otherwise
     */
    bool validate_buffer_range(size_t start_offset, 
                              size_t end_offset,
                              size_t buffer_size,
                              size_t min_size);

    /**
     * @brief Calculate optimal buffer size for given parameters
     *
     * @param total_size Total memory to allocate
     * @param num_buffers Number of buffers
     * @param cache_line_size Cache line size for alignment
     * @return Buffer size per buffer, or 0 if invalid
     */
    size_t calculate_buffer_size(size_t total_size,
                                size_t num_buffers, 
                                size_t cache_line_size);

    /**
     * @brief Check if a size is properly aligned to cache line boundary
     *
     * @param size Size to check
     * @param cache_line_size Cache line size
     * @return true if aligned, false otherwise
     */
    bool is_cache_line_aligned(size_t size, size_t cache_line_size);

    /**
     * @brief Scale iterations based on working set size
     *
     * Smaller working sets (that fit in cache) need more iterations
     * to get accurate timing measurements.
     *
     * @param base_iterations Base number of iterations
     * @param working_set_size Size of working set in bytes
     * @return Scaled iteration count
     */
    size_t scale_iterations(size_t base_iterations, size_t working_set_size);

} // namespace MemoryUtils

#endif // MEMORY_UTILS_H