#include "macos_matrix_multiplier.h"
#include <Accelerate/Accelerate.h>
#include <sys/sysctl.h>
#include <chrono>
#include <cstring>

namespace MatrixMultiply {

MacOSMatrixMultiplier::MacOSMatrixMultiplier() : accelerate_available_(true) {
    detect_chip_info();
}

void MacOSMatrixMultiplier::detect_chip_info() {
    size_t size = 256;
    char brand_string[256];
    
    // Try to get the brand string
    if (sysctlbyname("machdep.cpu.brand_string", brand_string, &size, nullptr, 0) == 0) {
        chip_name_ = std::string(brand_string);
    } else {
        // Fallback: check for Apple Silicon
        size_t len = sizeof(int);
        int has_arm64 = 0;
        if (sysctlbyname("hw.optional.arm64", &has_arm64, &len, nullptr, 0) == 0 && has_arm64) {
            chip_name_ = "Apple Silicon";
        } else {
            chip_name_ = "Intel";
        }
    }
}

MatrixPerformanceStats MacOSMatrixMultiplier::multiply_float(
    float* C, const float* A, const float* B,
    const MatrixConfig& config,
    const std::atomic<bool>& stop_flag) {
    
    const size_t M = config.M;
    const size_t K = config.K;
    const size_t N = config.N;
    
    // Clear result matrix
    memset(C, 0, M * N * sizeof(float));
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t iter = 0; iter < config.iterations && !stop_flag; ++iter) {
        // Use Accelerate's optimized SGEMM (automatically uses AMX when available)
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                   static_cast<int>(M), static_cast<int>(N), static_cast<int>(K),
                   1.0f,        // alpha
                   A, static_cast<int>(K),        // Matrix A, leading dimension
                   B, static_cast<int>(N),        // Matrix B, leading dimension  
                   1.0f,        // beta (accumulate)
                   C, static_cast<int>(N));       // Matrix C, leading dimension
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double time_seconds = std::chrono::duration<double>(end_time - start_time).count();
    
    // Calculate performance metrics
    size_t operations = 2 * M * N * K * config.iterations; // 2 ops per multiply-add
    size_t bytes_processed = (M * K + K * N + M * N) * sizeof(float) * config.iterations;
    
    return calculate_matrix_stats(bytes_processed, time_seconds, operations, get_acceleration_name());
}

MatrixPerformanceStats MacOSMatrixMultiplier::multiply_double(
    double* C, const double* A, const double* B,
    const MatrixConfig& config,
    const std::atomic<bool>& stop_flag) {
    
    const size_t M = config.M;
    const size_t K = config.K;
    const size_t N = config.N;
    
    memset(C, 0, M * N * sizeof(double));
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t iter = 0; iter < config.iterations && !stop_flag; ++iter) {
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                   static_cast<int>(M), static_cast<int>(N), static_cast<int>(K),
                   1.0,         // alpha
                   A, static_cast<int>(K),        // Matrix A
                   B, static_cast<int>(N),        // Matrix B
                   1.0,         // beta
                   C, static_cast<int>(N));       // Matrix C
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double time_seconds = std::chrono::duration<double>(end_time - start_time).count();
    
    size_t operations = 2 * M * N * K * config.iterations;
    size_t bytes_processed = (M * K + K * N + M * N) * sizeof(double) * config.iterations;
    
    return calculate_matrix_stats(bytes_processed, time_seconds, operations, get_acceleration_name());
}

std::string MacOSMatrixMultiplier::get_acceleration_name() const {
    if (chip_name_.find("Apple") != std::string::npos || chip_name_.find("M1") != std::string::npos || 
        chip_name_.find("M2") != std::string::npos || chip_name_.find("M3") != std::string::npos) {
        return "Apple AMX (via Accelerate)";
    }
    return "Accelerate Framework";
}

bool MacOSMatrixMultiplier::is_available() const {
    return accelerate_available_;
}

} // namespace MatrixMultiply