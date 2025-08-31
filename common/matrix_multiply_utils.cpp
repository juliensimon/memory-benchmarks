#include "matrix_multiply_interface.h"
#include <random>
#include <cmath>
#include <algorithm>

namespace MatrixMultiply {

MatrixConfig create_matrix_config(size_t size, size_t iterations, bool use_double) {
    return MatrixConfig{size, size, size, iterations, use_double, false};
}

size_t calculate_matrix_memory_footprint(const MatrixConfig& config) {
    size_t element_size = config.use_double ? sizeof(double) : sizeof(float);
    return (config.M * config.K + config.K * config.N + config.M * config.N) * element_size;
}

void initialize_matrix_random(float* matrix, size_t rows, size_t cols, float scale) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-scale, scale);
    
    for (size_t i = 0; i < rows * cols; ++i) {
        matrix[i] = dis(gen);
    }
}

void initialize_matrix_random(double* matrix, size_t rows, size_t cols, double scale) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-scale, scale);
    
    for (size_t i = 0; i < rows * cols; ++i) {
        matrix[i] = dis(gen);
    }
}

bool validate_matrix_result(const float* C_test, const float* C_reference, 
                           size_t rows, size_t cols, float tolerance) {
    for (size_t i = 0; i < rows * cols; ++i) {
        float diff = std::abs(C_test[i] - C_reference[i]);
        if (diff > tolerance) {
            return false;
        }
    }
    return true;
}

MatrixPerformanceStats calculate_matrix_stats(size_t bytes_processed, double time_seconds, 
                                             size_t operations, const std::string& acceleration) {
    MatrixPerformanceStats stats;
    stats.time_seconds = time_seconds;
    stats.operations = operations;
    stats.bytes_processed = bytes_processed;
    stats.gflops = (time_seconds > 0) ? operations / (time_seconds * 1e9) : 0.0;
    stats.bandwidth_gbps = (time_seconds > 0) ? bytes_processed / (time_seconds * 1e9) : 0.0;
    stats.latency_ns = (operations > 0) ? (time_seconds * 1e9) / operations : 0.0;
    stats.acceleration = acceleration;
    return stats;
}

} // namespace MatrixMultiply