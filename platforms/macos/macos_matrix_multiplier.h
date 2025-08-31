#ifndef MACOS_MATRIX_MULTIPLIER_H
#define MACOS_MATRIX_MULTIPLIER_H

#include "../../common/matrix_multiply_interface.h"

namespace MatrixMultiply {

/**
 * @brief macOS-specific matrix multiplier using Accelerate framework
 * 
 * Uses Apple's optimized BLAS routines which automatically dispatch to
 * Apple AMX (Matrix coprocessor) on M1/M2/M3 chips when beneficial.
 */
class MacOSMatrixMultiplier : public MatrixMultiplier {
public:
    MacOSMatrixMultiplier();
    ~MacOSMatrixMultiplier() override = default;
    
    MatrixPerformanceStats multiply_float(
        float* C, const float* A, const float* B,
        const MatrixConfig& config,
        const std::atomic<bool>& stop_flag) override;
    
    MatrixPerformanceStats multiply_double(
        double* C, const double* A, const double* B,
        const MatrixConfig& config,
        const std::atomic<bool>& stop_flag) override;
    
    std::string get_acceleration_name() const override;
    bool is_available() const override;

private:
    bool accelerate_available_;
    std::string chip_name_;
    
    void detect_chip_info();
};

} // namespace MatrixMultiply

#endif // MACOS_MATRIX_MULTIPLIER_H