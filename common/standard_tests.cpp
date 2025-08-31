#include "standard_tests.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <random>
#include <vector>
#ifdef __x86_64__
#include <immintrin.h>  // For SIMD operations on x86
#endif

#include "memory_types.h"
#include "test_patterns.h"
#include "memory_utils.h"
#include "constants.h"
#include "matrix_multiply_interface.h"
#include "platform_interface.h"

namespace StandardTests {

// Use centralized cache line size constant
using CacheConstants::DEFAULT_CACHE_LINE_SIZE;

/**
 * @brief Simple memory barrier - no cache interference
 */
inline void memory_barrier() {
    __sync_synchronize();
}

/**
 * @brief Natural sequential read test - let the system work as designed
 * 
 * Uses cache-line aligned array operations. No cache flushing or interference.
 * Let hardware prefetchers, cache policies, and memory controllers work naturally.
 */
PerformanceStats sequential_read_test(const uint8_t* buffer, size_t buffer_size,
                                      size_t start_offset, size_t end_offset, size_t iterations,
                                      const std::atomic<bool>& stop_flag, bool cache_aware) {
    (void)buffer_size;  // Unused
    (void)cache_aware;  // No special cache handling needed - let system work naturally
    
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
    size_t aligned_start = (start_offset + DEFAULT_CACHE_LINE_SIZE - 1) & ~(DEFAULT_CACHE_LINE_SIZE - 1);
    size_t aligned_end = end_offset & ~(DEFAULT_CACHE_LINE_SIZE - 1);
    
    if (aligned_end <= aligned_start) {
        return {0.0, 0.0, 0, 0.0};
    }
    
    size_t working_set_size = aligned_end - aligned_start;
    
    auto start_time = std::chrono::high_resolution_clock::now();

    for(size_t iter = 0; iter < iterations && !stop_flag; ++iter) {
        // Natural streaming read - process cache lines efficiently
        volatile uint64_t sum = 0;  // Prevent compiler optimization
        const uint64_t* data = reinterpret_cast<const uint64_t*>(buffer + aligned_start);
        size_t num_elements = working_set_size / sizeof(uint64_t);
        
        // Read in cache-line chunks (8 uint64_t = 64 bytes)
        // Ensure we don't read past the end of the buffer
        size_t full_chunks = num_elements / 8;
        for(size_t i = 0; i < full_chunks * 8; i += 8) {
            // Read entire cache line naturally
            sum += data[i] + data[i+1] + data[i+2] + data[i+3] +
                   data[i+4] + data[i+5] + data[i+6] + data[i+7];
        }
        
        // Handle remaining elements (if any)
        for(size_t i = full_chunks * 8; i < num_elements; i++) {
            sum += data[i];
        }
        
        // Ensure compiler doesn't optimize away the work
        __sync_synchronize();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double time_seconds = std::chrono::duration<double>(end_time - start_time).count();
    
    size_t bytes_processed = working_set_size * iterations;
    size_t operations = (working_set_size / DEFAULT_CACHE_LINE_SIZE) * iterations;

    return calculate_stats(bytes_processed, time_seconds, operations);
}

/**
 * @brief Natural sequential write test - let the system work as designed
 * 
 * Uses cache-line aligned array operations. No cache flushing or interference.
 */
PerformanceStats sequential_write_test(uint8_t* buffer, size_t buffer_size, size_t start_offset,
                                       size_t end_offset, size_t iterations,
                                       const std::atomic<bool>& stop_flag) {
    (void)buffer_size;  // Unused
    
    // Align to cache line boundaries for optimal access
    auto [aligned_start, aligned_end] = MemoryUtils::align_to_cache_lines(start_offset, end_offset, DEFAULT_CACHE_LINE_SIZE);
    
    if (aligned_end <= aligned_start) {
        return {0.0, 0.0, 0, 0.0};
    }
    
    size_t working_set_size = MemoryUtils::calculate_working_set_size(aligned_start, aligned_end);

    auto start_time = std::chrono::high_resolution_clock::now();

    for(size_t iter = 0; iter < iterations && !stop_flag; ++iter) {
        // Natural streaming write - process cache lines efficiently
        uint64_t* data = reinterpret_cast<uint64_t*>(buffer + aligned_start);
        size_t num_elements = working_set_size / sizeof(uint64_t);
        uint64_t pattern = BenchmarkConstants::TEST_PATTERN_BASE + iter;
        
        // Write in cache-line chunks
        // Ensure we don't write past the end of the buffer
        size_t full_chunks = num_elements / BenchmarkConstants::CACHE_LINE_ELEMENTS_UINT64;
        for(size_t i = 0; i < full_chunks * BenchmarkConstants::CACHE_LINE_ELEMENTS_UINT64; i += BenchmarkConstants::CACHE_LINE_ELEMENTS_UINT64) {
            // Write entire cache line naturally
            data[i] = pattern + i;
            data[i+1] = pattern + i + 1;
            data[i+2] = pattern + i + 2;
            data[i+3] = pattern + i + 3;
            data[i+4] = pattern + i + 4;
            data[i+5] = pattern + i + 5;
            data[i+6] = pattern + i + 6;
            data[i+7] = pattern + i + 7;
        }
        
        // Handle remaining elements (if any)
        for(size_t i = full_chunks * BenchmarkConstants::CACHE_LINE_ELEMENTS_UINT64; i < num_elements; i++) {
            data[i] = pattern + i;
        }
        
        __sync_synchronize();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double time_seconds = std::chrono::duration<double>(end_time - start_time).count();
    
    size_t bytes_processed = working_set_size * iterations;
    size_t operations = (working_set_size / DEFAULT_CACHE_LINE_SIZE) * iterations;

    return calculate_stats(bytes_processed, time_seconds, operations);
}

/**
 * @brief Natural random access test - realistic scatter/gather patterns
 * 
 * Uses cache-line aligned random access. No cache flushing.
 * This simulates real workloads with poor locality.
 */
PerformanceStats random_access_test(uint8_t* buffer, size_t buffer_size, size_t start_offset,
                                    size_t end_offset, size_t iterations, bool is_write,
                                    const std::atomic<bool>& stop_flag) {
    (void)buffer_size;  // Unused
    
    // Align to cache line boundaries
    auto [aligned_start, aligned_end] = MemoryUtils::align_to_cache_lines(start_offset, end_offset, DEFAULT_CACHE_LINE_SIZE);
    
    if (aligned_end <= aligned_start) {
        return {0.0, 0.0, 0, 0.0};
    }
    
    // Generate cache-line aligned random indices
    std::vector<size_t> cache_line_indices;
    for(size_t addr = aligned_start; addr < aligned_end; addr += DEFAULT_CACHE_LINE_SIZE) {
        cache_line_indices.push_back(addr);
    }

    // Shuffle for random access pattern
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(cache_line_indices.begin(), cache_line_indices.end(), gen);

    auto start_time = std::chrono::high_resolution_clock::now();

    for(size_t iter = 0; iter < iterations && !stop_flag; ++iter) {
        if (is_write) {
            // Random write - full cache lines
            uint64_t pattern = BenchmarkConstants::TEST_PATTERN_BASE + iter;
            for(size_t addr : cache_line_indices) {
                uint64_t* cache_line = reinterpret_cast<uint64_t*>(buffer + addr);
                // Write entire cache line
                for(size_t i = 0; i < BenchmarkConstants::CACHE_LINE_ELEMENTS_UINT64; ++i) {
                    cache_line[i] = pattern + addr + i;
                }
            }
        } else {
            // Random read - full cache lines
            volatile uint64_t sum = 0;
            for(size_t addr : cache_line_indices) {
                const uint64_t* cache_line = reinterpret_cast<const uint64_t*>(buffer + addr);
                // Read entire cache line
                for(size_t i = 0; i < BenchmarkConstants::CACHE_LINE_ELEMENTS_UINT64; ++i) {
                    sum += cache_line[i];
                }
            }
        }
        
        __sync_synchronize();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double time_seconds = std::chrono::duration<double>(end_time - start_time).count();
    
    size_t bytes_processed = cache_line_indices.size() * DEFAULT_CACHE_LINE_SIZE * iterations;
    size_t operations = cache_line_indices.size() * iterations;

    return calculate_stats(bytes_processed, time_seconds, operations);
}

/**
 * @brief Natural memory copy test - let memcpy work efficiently
 * 
 * Uses optimized memory copy operations aligned to cache boundaries.
 */
PerformanceStats copy_test(const uint8_t* src_buffer, uint8_t* dst_buffer, size_t buffer_size,
                           size_t start_offset, size_t end_offset, size_t iterations,
                           const std::atomic<bool>& stop_flag) {
    (void)buffer_size;  // Unused
    
    // Align to cache line boundaries
    auto [aligned_start, aligned_end] = MemoryUtils::align_to_cache_lines(start_offset, end_offset, DEFAULT_CACHE_LINE_SIZE);
    
    if (aligned_end <= aligned_start) {
        return {0.0, 0.0, 0, 0.0};
    }
    
    size_t working_set_size = MemoryUtils::calculate_working_set_size(aligned_start, aligned_end);

    auto start_time = std::chrono::high_resolution_clock::now();

    for(size_t iter = 0; iter < iterations && !stop_flag; ++iter) {
        // Use optimized memory copy - let the system work efficiently
        memcpy(dst_buffer + aligned_start, src_buffer + aligned_start, working_set_size);
        __sync_synchronize();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double time_seconds = std::chrono::duration<double>(end_time - start_time).count();
    
    size_t bytes_processed = working_set_size * iterations * 2;  // Read + Write
    size_t operations = (working_set_size / DEFAULT_CACHE_LINE_SIZE) * iterations;

    return calculate_stats(bytes_processed, time_seconds, operations);
}

/**
 * @brief Natural STREAM Triad test - realistic computational pattern
 * 
 * Performs A[i] = B[i] + C[i] * scalar using cache-aligned arrays.
 * Let the system handle memory access patterns naturally.
 */
PerformanceStats triad_test(uint8_t* a_buffer, const uint8_t* b_buffer, const uint8_t* c_buffer,
                            const uint8_t* d_buffer, size_t buffer_size, size_t start_offset,
                            size_t end_offset, size_t iterations,
                            const std::atomic<bool>& stop_flag) {
    (void)buffer_size;  // Unused
    (void)d_buffer;     // Use scalar instead
    
    // Align to cache line boundaries and work with doubles for realistic computation
    size_t aligned_start = (start_offset + sizeof(double) - 1) & ~(sizeof(double) - 1);
    size_t aligned_end = end_offset & ~(sizeof(double) - 1);
    
    if (aligned_end <= aligned_start) {
        return {0.0, 0.0, 0, 0.0};
    }
    
    size_t working_set_size = aligned_end - aligned_start;
    size_t num_elements = working_set_size / sizeof(double);
    
    // Work with double arrays for realistic STREAM triad
    double* a = reinterpret_cast<double*>(a_buffer + aligned_start);
    const double* b = reinterpret_cast<const double*>(b_buffer + aligned_start);
    const double* c = reinterpret_cast<const double*>(c_buffer + aligned_start);
    const double scalar = 3.14159;

    auto start_time = std::chrono::high_resolution_clock::now();

    for(size_t iter = 0; iter < iterations && !stop_flag; ++iter) {
        // Natural vectorized triad: A[i] = B[i] + scalar * C[i]
        // Process in chunks that fit cache lines naturally
        for(size_t i = 0; i < num_elements; i += 8) {  // 8 doubles = 64 bytes
            size_t chunk_end = std::min(i + 8, num_elements);
            for(size_t j = i; j < chunk_end; ++j) {
                a[j] = b[j] + scalar * c[j];
            }
        }
        
        __sync_synchronize();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double time_seconds = std::chrono::duration<double>(end_time - start_time).count();
    
    size_t bytes_processed = working_set_size * iterations * 3;  // Read B, Read C, Write A
    size_t operations = num_elements * iterations;

    return calculate_stats(bytes_processed, time_seconds, operations);
}

/**
 * @brief Matrix multiplication test using platform-specific hardware acceleration
 */
MatrixMultiply::MatrixPerformanceStats matrix_multiply_test(
    const MatrixMultiply::MatrixConfig& matrix_config,
    const std::atomic<bool>& stop_flag) {
    
    // Use platform-specific matrix multiplier
    auto platform = create_platform_interface();
    auto multiplier = platform->create_matrix_multiplier();
    
    if (multiplier && multiplier->is_available()) {
        const size_t M = matrix_config.M;
        const size_t K = matrix_config.K;
        const size_t N = matrix_config.N;
        
        // Allocate matrices
        std::vector<float> A(M * K);
        std::vector<float> B(K * N);
        std::vector<float> C(M * N);
        
        // Initialize matrices with random data
        MatrixMultiply::initialize_matrix_random(A.data(), M, K, 1.0f);
        MatrixMultiply::initialize_matrix_random(B.data(), K, N, 1.0f);
        
        return multiplier->multiply_float(C.data(), A.data(), B.data(), matrix_config, stop_flag);
    }
    
    // Fallback implementation for platforms without optimized matrix multiplication
    const size_t M = matrix_config.M;
    const size_t K = matrix_config.K;
    const size_t N = matrix_config.N;
    
    // Allocate matrices
    std::vector<float> A(M * K);
    std::vector<float> B(K * N);
    std::vector<float> C(M * N);
    
    // Initialize matrices with random data
    MatrixMultiply::initialize_matrix_random(A.data(), M, K, 1.0f);
    MatrixMultiply::initialize_matrix_random(B.data(), K, N, 1.0f);
    
    // Clear result matrix
    std::memset(C.data(), 0, M * N * sizeof(float));
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simple matrix multiplication fallback
    for (size_t iter = 0; iter < matrix_config.iterations && !stop_flag; ++iter) {
        for (size_t i = 0; i < M; ++i) {
            for (size_t k = 0; k < K; ++k) {
                float a_ik = A[i * K + k];
                for (size_t j = 0; j < N; ++j) {
                    C[i * N + j] += a_ik * B[k * N + j];
                }
            }
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double time_seconds = std::chrono::duration<double>(end_time - start_time).count();
    
    size_t operations = 2 * M * N * K * matrix_config.iterations;
    size_t bytes_processed = (M * K + K * N + M * N) * sizeof(float) * matrix_config.iterations;
    
    return MatrixMultiply::calculate_matrix_stats(bytes_processed, time_seconds, operations, "Scalar fallback");
}

}  // namespace StandardTests