#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstddef>
#include <cstdint>

/**
 * @brief Application-wide constants for memory benchmarking
 *
 * This header centralizes all magic numbers and configuration constants
 * to improve maintainability and reduce code duplication.
 */
namespace BenchmarkConstants {
    // Buffer size constants
    constexpr size_t MIN_BUFFER_SIZE = 4 * 1024;              // 4KB minimum buffer size
    constexpr size_t MIN_WORKING_SET_SIZE = 4 * 1024;         // 4KB minimum working set size
    constexpr size_t MAX_WORKING_SET_SIZE = 4ULL * 1024 * 1024 * 1024;  // 4GB maximum working set
    
    // Memory size constants
    constexpr size_t KB = 1024;
    constexpr size_t MB = 1024 * KB;
    constexpr size_t GB = 1024 * MB;
    
    // Configuration limits
    constexpr size_t MAX_MEMORY_SIZE_GB = 1024;               // 1TB reasonable upper limit
    constexpr double DEFAULT_MEMORY_SIZE_GB = 6.0;            // Default test memory size
    constexpr size_t DEFAULT_ITERATIONS = 10;                 // Default number of test iterations
    
    // Thread limits
    constexpr size_t MAX_THREAD_OVERSUBSCRIPTION = 2;         // Allow 2x oversubscription
    
    // Cache alignment constants
    constexpr size_t CACHE_LINE_ELEMENTS_UINT64 = 8;          // 8 uint64_t = 64 bytes cache line
    constexpr size_t CACHE_LINE_ELEMENTS_DOUBLE = 8;          // 8 double = 64 bytes cache line
    
    // Working set size multipliers
    constexpr size_t WORKING_SET_FRACTIONS[] = {8, 4, 2, 1};  // 1/8, 1/4, 1/2, full
    constexpr size_t WORKING_SET_MULTIPLIERS[] = {2, 4, 8};   // 2x, 4x, 8x cache
    
    // Standard working set sizes beyond cache
    constexpr size_t STANDARD_WORKING_SETS[] = {
        64 * MB,    // 64MB
        128 * MB,   // 128MB
        256 * MB,   // 256MB
        512 * MB,   // 512MB
        1 * GB,     // 1GB
        2 * GB,     // 2GB
        4 * GB      // 4GB
    };
    
    // Iteration scaling factors for different working set sizes
    constexpr size_t SMALL_CACHE_ITER_MULTIPLIER = 100000;    // For <= 64KB
    constexpr size_t MEDIUM_CACHE_ITER_MULTIPLIER = 100000;   // For <= 4MB
    constexpr size_t LARGE_CACHE_ITER_MULTIPLIER = 1000;      // For <= 8MB
    
    // Cache size thresholds for iteration scaling
    constexpr size_t SMALL_CACHE_THRESHOLD = 64 * KB;
    constexpr size_t MEDIUM_CACHE_THRESHOLD = 4 * MB;
    constexpr size_t LARGE_CACHE_THRESHOLD = 8 * MB;
    
    // Test pattern constants
    constexpr uint64_t TEST_PATTERN_BASE = 0x0123456789ABCDEF;
    constexpr double TRIAD_SCALAR = 3.14159;
    
    // Buffer allocation alignment
    constexpr size_t MAX_ALIGNMENT_SIZE = 1024;               // Maximum reasonable alignment
    
    // Validation constants
    constexpr double MIN_LATENCY_NS = 0.1;                    // Minimum realistic latency
    constexpr double MAX_EFFICIENCY_VIRTUALIZED = 50.0;       // Max efficiency in VMs
}

#endif  // CONSTANTS_H