#!/usr/bin/env bash

# Memory Bandwidth Benchmark - Test Suite Runner
echo "Memory Bandwidth Benchmark - Test Suite"
echo "========================================"
echo ""

total_failures=0

# Run AlignedBuffer tests
echo "Running AlignedBuffer tests:"
./tests/test_aligned_buffer
aligned_result=$?
total_failures=$((total_failures + aligned_result))
echo ""

# Run ArgumentParser tests  
echo "Running ArgumentParser tests:"
./tests/test_argument_parser
parser_result=$?
total_failures=$((total_failures + parser_result))
echo ""

# Run PlatformFactory tests
echo "Running PlatformFactory tests:"
./tests/test_platform_factory
platform_result=$?
total_failures=$((total_failures + platform_result))
echo ""

# Run SafeFileUtils tests
echo "Running SafeFileUtils tests:"
./tests/test_safe_file_utils
safe_file_result=$?
total_failures=$((total_failures + safe_file_result))
echo ""

# Run OutputFormatter tests
echo "Running OutputFormatter tests:"
./tests/test_output_formatter
output_formatter_result=$?
total_failures=$((total_failures + output_formatter_result))
echo ""

# Run MatrixMultiplyUtils tests
echo "Running MatrixMultiplyUtils tests:"
./tests/test_matrix_multiply_utils
matrix_utils_result=$?
total_failures=$((total_failures + matrix_utils_result))
echo ""

# Run MemoryUtils tests
echo "Running MemoryUtils tests:"
./tests/test_memory_utils
memory_utils_result=$?
total_failures=$((total_failures + memory_utils_result))
echo ""

# Run OutputFormatterUtils tests
echo "Running OutputFormatterUtils tests:"
./tests/test_output_formatter_utils
output_formatter_utils_result=$?
total_failures=$((total_failures + output_formatter_utils_result))
echo ""

# Run SystemInfoDisplay tests
echo "Running SystemInfoDisplay tests:"
./tests/test_system_info_display_simple
system_info_result=$?
total_failures=$((total_failures + system_info_result))
echo ""

# Run TestPatterns tests
echo "Running TestPatterns tests:"
./tests/test_test_patterns
test_patterns_result=$?
total_failures=$((total_failures + test_patterns_result))
echo ""

# Run WorkingSets tests
echo "Running WorkingSets tests:"
./tests/test_working_sets
working_sets_result=$?
total_failures=$((total_failures + working_sets_result))
echo ""

if [ $total_failures -eq 0 ]; then
    echo "All tests passed! ✅"
else
    echo "Total failures: $total_failures ❌"
fi

exit $total_failures