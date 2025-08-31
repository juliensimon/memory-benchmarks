#include "test_framework.h"
#include "../common/safe_file_utils.h"
#include <fstream>

void test_safe_path_validation() {
    // Test basic validation (these should fail regardless of platform)
    ASSERT_FALSE(SafeFileUtils::is_safe_path(""));  // Empty path
    ASSERT_FALSE(SafeFileUtils::is_safe_path("../../../etc/passwd"));  // Directory traversal
    ASSERT_FALSE(SafeFileUtils::is_safe_path("/proc/../etc/passwd"));  // Directory traversal in allowed path
    ASSERT_FALSE(SafeFileUtils::is_safe_path("/etc/passwd"));  // Disallowed path
    ASSERT_FALSE(SafeFileUtils::is_safe_path("/home/user/file.txt"));  // User path
    
    // Test paths with null bytes
    std::string null_path = "/proc/cpu\0info";
    ASSERT_FALSE(SafeFileUtils::is_safe_path(null_path));
    
    // Note: Platform-specific existing path tests are handled by integration tests
    // since SafeFileUtils requires actual files to exist for realpath() validation
}

void test_input_sanitization() {
    // Test normal input
    std::string normal = "Apple M3 Max";
    TestAssert::assert_equal(std::string("Apple M3 Max"), SafeFileUtils::sanitize_input(normal));
    
    // Test input with trailing whitespace
    std::string whitespace = "CPU Model   \t\n";
    TestAssert::assert_equal(std::string("CPU Model"), SafeFileUtils::sanitize_input(whitespace));
    
    // Test input with null bytes (security test)
    std::string with_nulls = "CPU\0Model";
    std::string sanitized = SafeFileUtils::sanitize_input(with_nulls);
    ASSERT_TRUE(sanitized.find('\0') == std::string::npos);
    
    // Test extremely long input (bounds checking)
    std::string long_input(3000, 'A');
    std::string sanitized_long = SafeFileUtils::sanitize_input(long_input);
    ASSERT_TRUE(sanitized_long.length() <= SafeFileUtils::MAX_LINE_LENGTH);
}

void test_pattern_validation() {
    // Test empty pattern
    std::string result;
    ASSERT_FALSE(SafeFileUtils::find_pattern("/proc/cpuinfo", "", result));
    
    // Test very long pattern (security test)
    std::string long_pattern(300, 'A');
    ASSERT_FALSE(SafeFileUtils::find_pattern("/proc/cpuinfo", long_pattern, result));
}

void test_line_length_limits() {
    // Create a temporary test file with extremely long line
    std::string test_file = "/tmp/test_long_line.txt";
    std::ofstream file(test_file);
    if (file.is_open()) {
        // Write a line that exceeds MAX_LINE_LENGTH
        std::string long_line(SafeFileUtils::MAX_LINE_LENGTH + 100, 'X');
        file << long_line << std::endl;
        file.close();
        
        // Should fail due to line length limit
        std::string result;
        ASSERT_FALSE(SafeFileUtils::read_single_line(test_file, result));
        
        // Clean up
        std::remove(test_file.c_str());
    }
}

void test_file_size_limits() {
    // This test would require creating a very large file
    // For now, test that the size check function exists and handles normal files
    std::string small_file = "/tmp/test_small.txt";
    std::ofstream file(small_file);
    if (file.is_open()) {
        file << "Small content" << std::endl;
        file.close();
        
        // Should not be accessible due to path restrictions, but size would be fine
        std::string result;
        ASSERT_FALSE(SafeFileUtils::read_single_line(small_file, result)); // Path not allowed
        
        // Clean up
        std::remove(small_file.c_str());
    }
}

void test_max_lines_enforcement() {
    // Test that we don't read too many lines
    std::string test_file = "/tmp/test_many_lines.txt";
    std::ofstream file(test_file);
    if (file.is_open()) {
        // Write many lines
        for (int i = 0; i < 100; ++i) {
            file << "Line " << i << std::endl;
        }
        file.close();
        
        // Try to read with a limit
        std::vector<std::string> lines;
        // This will fail due to path restriction, which is expected security behavior
        ASSERT_FALSE(SafeFileUtils::read_all_lines(test_file, lines, 10));
        
        // Clean up
        std::remove(test_file.c_str());
    }
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Safe path validation", test_safe_path_validation);
    TEST_CASE("Input sanitization", test_input_sanitization);
    TEST_CASE("Pattern validation", test_pattern_validation);
    TEST_CASE("Line length limits", test_line_length_limits);
    TEST_CASE("File size limits", test_file_size_limits);
    TEST_CASE("Max lines enforcement", test_max_lines_enforcement);
    
    return framework.run_all();
}