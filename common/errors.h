#ifndef ERRORS_H
#define ERRORS_H

#include <stdexcept>
#include <string>

/**
 * @brief Custom exception types for memory benchmark errors
 *
 * This header provides standardized exception classes for consistent
 * error handling throughout the memory benchmark application.
 */

/**
 * @brief Base class for all memory benchmark exceptions
 */
class BenchmarkError : public std::runtime_error {
public:
    explicit BenchmarkError(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Exception for argument parsing and validation errors
 */
class ArgumentError : public BenchmarkError {
public:
    explicit ArgumentError(const std::string& message) 
        : BenchmarkError("Argument error: " + message) {}
};

/**
 * @brief Exception for memory allocation and buffer errors
 */
class MemoryError : public BenchmarkError {
public:
    explicit MemoryError(const std::string& message) 
        : BenchmarkError("Memory error: " + message) {}
};

/**
 * @brief Exception for platform-specific detection errors
 */
class PlatformError : public BenchmarkError {
public:
    explicit PlatformError(const std::string& message) 
        : BenchmarkError("Platform error: " + message) {}
};

/**
 * @brief Exception for test execution errors
 */
class TestError : public BenchmarkError {
public:
    explicit TestError(const std::string& message) 
        : BenchmarkError("Test error: " + message) {}
};

/**
 * @brief Exception for configuration validation errors
 */
class ConfigurationError : public BenchmarkError {
public:
    explicit ConfigurationError(const std::string& message) 
        : BenchmarkError("Configuration error: " + message) {}
};

#endif // ERRORS_H