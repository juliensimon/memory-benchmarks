#ifndef MATRIX_MULTIPLY_INTERFACE_H
#define MATRIX_MULTIPLY_INTERFACE_H

#include <cstddef>
#include <atomic>
#include <string>

/**
 * @brief Platform-agnostic matrix multiplication interface
 * 
 * This interface allows each platform to implement its own optimized
 * matrix multiplication using available hardware acceleration.
 */

namespace MatrixMultiply {

/**
 * @brief Matrix multiplication performance statistics
 */
struct MatrixPerformanceStats {
    double gflops;              ///< Performance in GFLOPS (billion floating-point ops/sec)
    double bandwidth_gbps;      ///< Memory bandwidth in GB/s
    double latency_ns;          ///< Average operation latency in nanoseconds
    size_t bytes_processed;     ///< Total bytes processed during test
    double time_seconds;        ///< Total time taken for test in seconds
    size_t operations;          ///< Total number of operations performed
    std::string acceleration;   ///< Hardware acceleration used ("AMX", "NEON", "AVX512", etc.)
};

/**
 * @brief Matrix dimensions and configuration
 */
struct MatrixConfig {
    size_t M;           ///< Matrix A rows (and result C rows)
    size_t K;           ///< Matrix A cols / Matrix B rows
    size_t N;           ///< Matrix B cols (and result C cols)
    size_t iterations;  ///< Number of iterations to run
    bool use_double;    ///< Use double precision (false = single precision)
    bool transpose_b;   ///< Transpose B matrix for better cache locality
};

/**
 * @brief Abstract base class for platform-specific matrix multiplication
 */
class MatrixMultiplier {
public:
    virtual ~MatrixMultiplier() = default;
    
    // Single precision matrix multiplication
    virtual MatrixPerformanceStats multiply_float(
        float* C, const float* A, const float* B,
        const MatrixConfig& config,
        const std::atomic<bool>& stop_flag) = 0;
    
    // Double precision matrix multiplication
    virtual MatrixPerformanceStats multiply_double(
        double* C, const double* A, const double* B,
        const MatrixConfig& config,
        const std::atomic<bool>& stop_flag) = 0;
    
    // Get acceleration type name
    virtual std::string get_acceleration_name() const = 0;
    
    // Check if this multiplier is available on current hardware
    virtual bool is_available() const = 0;
};

// Utility functions
MatrixConfig create_matrix_config(size_t size, size_t iterations = 10, bool use_double = false);
size_t calculate_matrix_memory_footprint(const MatrixConfig& config);

// Matrix initialization and validation
void initialize_matrix_random(float* matrix, size_t rows, size_t cols, float scale = 1.0f);
void initialize_matrix_random(double* matrix, size_t rows, size_t cols, double scale = 1.0);
bool validate_matrix_result(const float* C_test, const float* C_reference, 
                           size_t rows, size_t cols, float tolerance = 1e-4f);

MatrixPerformanceStats calculate_matrix_stats(size_t bytes_processed, double time_seconds, 
                                             size_t operations, const std::string& acceleration);

} // namespace MatrixMultiply

#endif // MATRIX_MULTIPLY_INTERFACE_H