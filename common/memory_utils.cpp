#include "memory_utils.h"
#include "constants.h"
#include <algorithm>

using namespace BenchmarkConstants;

namespace MemoryUtils {

std::pair<size_t, size_t> align_to_cache_lines(size_t start_offset, 
                                               size_t end_offset,
                                               size_t cache_line_size) {
    /**
     * Memory Alignment Algorithm for Optimal Cache Performance
     * 
     * This alignment logic ensures memory accesses are optimally positioned relative
     * to cache line boundaries to maximize memory throughput and minimize cache misses.
     * 
     * Step 1: Align start_offset UP to the next cache line boundary
     *   - Formula: (offset + cache_line_size - 1) & ~(cache_line_size - 1)
     *   - Example: offset=10, cache_line=64 → (10+63) & ~63 → 73 & 0xFFC0 → 64
     *   - This ensures we start reading at the beginning of a cache line
     * 
     * Step 2: Align end_offset DOWN to the previous cache line boundary  
     *   - Formula: offset & ~(cache_line_size - 1)
     *   - Example: offset=200, cache_line=64 → 200 & 0xFFC0 → 192
     *   - This ensures we end reading at the end of a complete cache line
     * 
     * Why this matters:
     * - Prevents partial cache line reads that waste memory bandwidth
     * - Enables hardware prefetchers to work optimally
     * - Aligns with CPU memory controller natural access patterns
     * - Reduces memory controller overhead from unaligned accesses
     */
    size_t aligned_start = (start_offset + cache_line_size - 1) & ~(cache_line_size - 1);
    size_t aligned_end = end_offset & ~(cache_line_size - 1);
    
    return std::make_pair(aligned_start, aligned_end);
}

size_t calculate_working_set_size(size_t aligned_start, size_t aligned_end) {
    if (aligned_end <= aligned_start) {
        return 0;
    }
    return aligned_end - aligned_start;
}

bool validate_buffer_range(size_t start_offset, 
                          size_t end_offset,
                          size_t buffer_size,
                          size_t min_size) {
    // Basic range validation
    if (start_offset >= end_offset) {
        return false;
    }
    
    if (end_offset > buffer_size) {
        return false;
    }
    
    // Check minimum size requirement
    if ((end_offset - start_offset) < min_size) {
        return false;
    }
    
    return true;
}

size_t calculate_buffer_size(size_t total_size,
                            size_t num_buffers, 
                            size_t cache_line_size) {
    if (total_size == 0 || num_buffers == 0) {
        return 0;
    }
    
    size_t buffer_size = total_size / num_buffers;
    
    // Validate minimum requirements
    if (buffer_size < MIN_BUFFER_SIZE || buffer_size < cache_line_size) {
        return 0;
    }
    
    return buffer_size;
}

bool is_cache_line_aligned(size_t size, size_t cache_line_size) {
    return (size & (cache_line_size - 1)) == 0;
}

size_t scale_iterations(size_t base_iterations, size_t working_set_size) {
    if (working_set_size <= SMALL_CACHE_THRESHOLD) {
        return base_iterations * SMALL_CACHE_ITER_MULTIPLIER;
    } else if (working_set_size <= MEDIUM_CACHE_THRESHOLD) {
        return base_iterations * MEDIUM_CACHE_ITER_MULTIPLIER;
    } else if (working_set_size <= LARGE_CACHE_THRESHOLD) {
        return base_iterations * LARGE_CACHE_ITER_MULTIPLIER;
    }
    
    // For large working sets, use base iterations
    return base_iterations;
}

} // namespace MemoryUtils