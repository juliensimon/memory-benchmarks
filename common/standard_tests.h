#ifndef STANDARD_TESTS_H
#define STANDARD_TESTS_H

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "test_patterns.h"
#include "matrix_multiply_interface.h"

/**
 * @brief Standard memory bandwidth test routines
 *
 * This module contains implementations of standard memory bandwidth tests
 * including sequential read/write, random access, copy, and STREAM triad operations.
 * These tests provide baseline memory performance measurements.
 */
namespace StandardTests {

/**
 * @brief Sequential read test implementation
 *
 * Performs sequential read access to memory buffers. This test measures
 * the maximum read bandwidth achievable with optimal memory access patterns.
 *
 * @param buffer Pointer to the memory buffer to read from
 * @param buffer_size Size of the buffer in bytes
 * @param start_offset Starting offset within the buffer
 * @param end_offset Ending offset within the buffer
 * @param iterations Number of iterations to perform
 * @param stop_flag Atomic flag to signal test termination
 * @return PerformanceStats containing test results
 */
PerformanceStats sequential_read_test(const uint8_t* buffer, size_t buffer_size,
                                      size_t start_offset, size_t end_offset, size_t iterations,
                                      const std::atomic<bool>& stop_flag, bool cache_aware = false);

/**
 * @brief Sequential write test implementation
 *
 * Performs sequential write access to memory buffers. This test measures
 * the maximum write bandwidth achievable with optimal memory access patterns.
 *
 * @param buffer Pointer to the memory buffer to write to
 * @param buffer_size Size of the buffer in bytes
 * @param start_offset Starting offset within the buffer
 * @param end_offset Ending offset within the buffer
 * @param iterations Number of iterations to perform
 * @param stop_flag Atomic flag to signal test termination
 * @return PerformanceStats containing test results
 */
PerformanceStats sequential_write_test(uint8_t* buffer, size_t buffer_size, size_t start_offset,
                                       size_t end_offset, size_t iterations,
                                       const std::atomic<bool>& stop_flag);

/**
 * @brief Random access test implementation
 *
 * Performs random read or write access to memory buffers. This test measures
 * memory performance under poor locality conditions, which is important for
 * understanding cache miss behavior.
 *
 * @param buffer Pointer to the memory buffer
 * @param buffer_size Size of the buffer in bytes
 * @param start_offset Starting offset within the buffer
 * @param end_offset Ending offset within the buffer
 * @param iterations Number of iterations to perform
 * @param is_write Whether to perform write (true) or read (false) operations
 * @param stop_flag Atomic flag to signal test termination
 * @return PerformanceStats containing test results
 */
PerformanceStats random_access_test(uint8_t* buffer, size_t buffer_size, size_t start_offset,
                                    size_t end_offset, size_t iterations, bool is_write,
                                    const std::atomic<bool>& stop_flag);

/**
 * @brief Memory copy test implementation
 *
 * Performs memory copy operations from source to destination buffer.
 * This test measures the efficiency of memory-to-memory transfers,
 * which is important for many real-world applications.
 *
 * @param src_buffer Pointer to the source buffer
 * @param dst_buffer Pointer to the destination buffer
 * @param buffer_size Size of the buffers in bytes
 * @param start_offset Starting offset within the buffers
 * @param end_offset Ending offset within the buffers
 * @param iterations Number of iterations to perform
 * @param stop_flag Atomic flag to signal test termination
 * @return PerformanceStats containing test results
 */
PerformanceStats copy_test(const uint8_t* src_buffer, uint8_t* dst_buffer, size_t buffer_size,
                           size_t start_offset, size_t end_offset, size_t iterations,
                           const std::atomic<bool>& stop_flag);

/**
 * @brief STREAM Triad test implementation
 *
 * Performs STREAM Triad operation: A[i] = B[i] + C[i] * D[i].
 * This is a standard memory bandwidth benchmark that measures
 * the performance of mixed read/write operations with arithmetic.
 *
 * @param a_buffer Pointer to buffer A (destination)
 * @param b_buffer Pointer to buffer B (source)
 * @param c_buffer Pointer to buffer C (source)
 * @param d_buffer Pointer to buffer D (source)
 * @param buffer_size Size of the buffers in bytes
 * @param start_offset Starting offset within the buffers
 * @param end_offset Ending offset within the buffers
 * @param iterations Number of iterations to perform
 * @param stop_flag Atomic flag to signal test termination
 * @return PerformanceStats containing test results
 */
PerformanceStats triad_test(uint8_t* a_buffer, const uint8_t* b_buffer, const uint8_t* c_buffer,
                            const uint8_t* d_buffer, size_t buffer_size, size_t start_offset,
                            size_t end_offset, size_t iterations,
                            const std::atomic<bool>& stop_flag);

/**
 * @brief Matrix multiplication test using hardware acceleration
 * 
 * Performs C = A * B matrix multiplication using platform-specific
 * hardware acceleration (Apple AMX, ARM Neoverse, Intel AMX).
 * This test measures GFLOPS and memory bandwidth for compute-intensive workloads.
 * 
 * @param matrix_config Matrix dimensions and configuration
 * @param iterations Number of iterations to perform
 * @param stop_flag Atomic flag to signal test termination
 * @return MatrixPerformanceStats containing specialized matrix test results
 */
MatrixMultiply::MatrixPerformanceStats matrix_multiply_test(
    const MatrixMultiply::MatrixConfig& matrix_config,
    const std::atomic<bool>& stop_flag);

}  // namespace StandardTests

#endif  // STANDARD_TESTS_H
