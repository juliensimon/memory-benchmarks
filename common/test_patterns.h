#ifndef TEST_PATTERNS_H
#define TEST_PATTERNS_H

#include <cstddef>
#include <string>

/**
 * @brief Enumeration of available memory bandwidth test patterns
 *
 * Defines all the different types of memory access patterns that can be tested,
 * including standard memory operations for benchmarking system performance.
 */
enum class TestPattern {
    SEQUENTIAL_READ,   ///< Sequential read access pattern
    SEQUENTIAL_WRITE,  ///< Sequential write access pattern
    RANDOM_READ,       ///< Random read access pattern
    RANDOM_WRITE,      ///< Random write access pattern
    COPY,              ///< Memory copy operation (read from one buffer, write to another)
    TRIAD              ///< STREAM Triad operation (A[i] = B[i] + C[i] * D[i])
};

/**
 * @brief Performance statistics structure for test results
 *
 * Contains all the performance metrics collected during memory bandwidth tests,
 * including bandwidth, latency, and efficiency calculations.
 */
struct PerformanceStats {
    double bandwidth_gbps;   ///< Memory bandwidth in GB/s
    double latency_ns;       ///< Memory access latency in nanoseconds
    size_t bytes_processed;  ///< Total bytes processed during test
    double time_seconds;     ///< Total time taken for test in seconds
};

// Function declarations
std::string get_pattern_name(TestPattern pattern);
PerformanceStats calculate_stats(size_t bytes_processed, double time_seconds, size_t operations);

#endif  // TEST_PATTERNS_H
