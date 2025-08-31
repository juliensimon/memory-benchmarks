#pragma once

#include <string>
#include <fstream>
#include <vector>

/**
 * @brief Safe file utilities for secure system file reading
 * 
 * Provides bounds checking and input validation for reading system files
 * to prevent security vulnerabilities from malformed or malicious content.
 */
class SafeFileUtils {
public:
    /**
     * @brief Maximum allowed line length for system files
     */
    static constexpr size_t MAX_LINE_LENGTH = 2048;
    
    /**
     * @brief Maximum allowed file size for system files
     */
    static constexpr size_t MAX_FILE_SIZE = 1024 * 1024; // 1MB
    
    /**
     * @brief Maximum number of lines to process
     */
    static constexpr size_t MAX_LINES = 10000;
    
    /**
     * @brief Safely read a single line from a system file
     * 
     * @param file_path Path to the system file
     * @param line Output string for the line content
     * @return true if line was read successfully, false otherwise
     */
    static bool read_single_line(const std::string& file_path, std::string& line);
    
    /**
     * @brief Safely read all lines from a system file
     * 
     * @param file_path Path to the system file
     * @param lines Output vector for all lines
     * @param max_lines Maximum number of lines to read (default: MAX_LINES)
     * @return true if file was read successfully, false otherwise
     */
    static bool read_all_lines(const std::string& file_path, std::vector<std::string>& lines, 
                              size_t max_lines = MAX_LINES);
    
    /**
     * @brief Safely search for a pattern in a system file
     * 
     * @param file_path Path to the system file
     * @param pattern Pattern to search for
     * @param result Output string for the matching line
     * @return true if pattern was found, false otherwise
     */
    static bool find_pattern(const std::string& file_path, const std::string& pattern, 
                           std::string& result);
    
    /**
     * @brief Validate that a file path is safe to read
     * 
     * @param file_path Path to validate
     * @return true if path is safe, false otherwise
     */
    static bool is_safe_path(const std::string& file_path);
    
    /**
     * @brief Sanitize input string to prevent injection attacks
     * 
     * @param input Input string to sanitize
     * @return Sanitized string
     */
    static std::string sanitize_input(const std::string& input);

private:
    /**
     * @brief Check if file size is within safe limits
     */
    static bool check_file_size(const std::string& file_path);
    
    /**
     * @brief List of allowed system file paths for reading
     */
    static const std::vector<std::string> ALLOWED_SYSTEM_PATHS;
};