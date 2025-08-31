#include "memory_utils.h"
#include "constants.h"
#include <algorithm>
#include <cstring>
#include <limits>

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

bool validate_memory_operation(size_t start_offset,
                               size_t end_offset,
                               size_t buffer_size,
                               size_t cache_line_size) {
    // Check for integer overflow conditions
    if (start_offset > buffer_size || end_offset > buffer_size) {
        return false;
    }
    
    // Validate range ordering
    if (start_offset >= end_offset) {
        return false;
    }
    
    // Validate cache line size is power of 2 and reasonable
    if (cache_line_size == 0 || (cache_line_size & (cache_line_size - 1)) != 0) {
        return false;
    }
    
    if (cache_line_size > 1024) { // Reasonable upper limit
        return false;
    }
    
    // Calculate aligned boundaries and check for overflow
    size_t aligned_start, aligned_end;
    
    // Check for potential overflow in alignment calculation
    if (start_offset > (std::numeric_limits<size_t>::max() - cache_line_size + 1)) {
        return false;
    }
    
    aligned_start = (start_offset + cache_line_size - 1) & ~(cache_line_size - 1);
    aligned_end = end_offset & ~(cache_line_size - 1);
    
    // Validate aligned boundaries are within buffer
    if (aligned_start >= buffer_size || aligned_end > buffer_size) {
        return false;
    }
    
    // Validate that we have a non-empty working set after alignment
    if (aligned_end <= aligned_start) {
        return false;
    }
    
    // Check for potential overflow in working set size calculation
    if (aligned_end - aligned_start > buffer_size) {
        return false;
    }
    
    return true;
}

bool safe_memory_copy(void* dst, size_t dst_size,
                      const void* src, size_t src_size,
                      size_t offset, size_t size) {
    // Validate pointers are not null
    if (!dst || !src) {
        return false;
    }
    
    // Check for zero size copy (valid but no-op)
    if (size == 0) {
        return true;
    }
    
    // Check for integer overflow in offset + size
    if (offset > std::numeric_limits<size_t>::max() - size) {
        return false;
    }
    
    // Validate bounds for source buffer
    if (offset + size > src_size) {
        return false;
    }
    
    // Validate bounds for destination buffer
    if (offset + size > dst_size) {
        return false;
    }
    
    // Perform the copy
    std::memcpy(static_cast<uint8_t*>(dst) + offset, 
                static_cast<const uint8_t*>(src) + offset, 
                size);
    
    return true;
}

bool safe_memory_set(void* ptr, size_t buffer_size, int value, size_t size) {
    // Validate pointer is not null
    if (!ptr) {
        return false;
    }
    
    // Check for zero size set (valid but no-op)
    if (size == 0) {
        return true;
    }
    
    // Validate bounds
    if (size > buffer_size) {
        return false;
    }
    
    // Perform the set
    std::memset(ptr, value, size);
    
    return true;
}

} // namespace MemoryUtils