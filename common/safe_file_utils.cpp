#include "safe_file_utils.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <cstdlib>

const std::vector<std::string> SafeFileUtils::ALLOWED_SYSTEM_PATHS = {
    "/proc/cpuinfo",
    "/proc/meminfo", 
    "/sys/devices/system/cpu/",
    "/sys/class/dmi/id/",
    "/sys/fs/cgroup/"
};

bool SafeFileUtils::is_safe_path(const std::string& file_path) {
    // Reject empty paths or paths that are too long
    if (file_path.empty() || file_path.length() > PATH_MAX) {
        return false;
    }
    
    // Reject paths containing directory traversal sequences
    if (file_path.find("..") != std::string::npos) {
        return false;
    }
    
    // Reject paths containing null bytes
    if (file_path.find('\0') != std::string::npos) {
        return false;
    }
    
    // Resolve the canonical path to prevent symlink attacks
    char* resolved_path = realpath(file_path.c_str(), nullptr);
    if (resolved_path == nullptr) {
        return false; // Path doesn't exist or can't be resolved
    }
    
    std::string canonical_path(resolved_path);
    free(resolved_path);
    
    // Check if canonical path starts with any allowed system path
    for (const auto& allowed_path : ALLOWED_SYSTEM_PATHS) {
        if (canonical_path.find(allowed_path) == 0) {
            // Ensure it's either an exact match or the next character is '/'
            if (canonical_path.length() == allowed_path.length() || 
                canonical_path[allowed_path.length()] == '/') {
                return true;
            }
        }
    }
    
    return false;
}

bool SafeFileUtils::check_file_size(const std::string& file_path) {
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) != 0) {
        return false; // File doesn't exist or can't be accessed
    }
    
    // Check if file size is within limits
    if (file_stat.st_size > static_cast<off_t>(MAX_FILE_SIZE)) {
        return false;
    }
    
    return true;
}

std::string SafeFileUtils::sanitize_input(const std::string& input) {
    std::string sanitized = input;
    
    // Remove null bytes
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\0'), sanitized.end());
    
    // Truncate to maximum line length
    if (sanitized.length() > MAX_LINE_LENGTH) {
        sanitized = sanitized.substr(0, MAX_LINE_LENGTH);
    }
    
    // Remove trailing whitespace and newlines
    sanitized.erase(sanitized.find_last_not_of(" \t\r\n") + 1);
    
    return sanitized;
}

bool SafeFileUtils::read_single_line(const std::string& file_path, std::string& line) {
    // Validate path safety
    if (!is_safe_path(file_path)) {
        return false;
    }
    
    // Check file size
    if (!check_file_size(file_path)) {
        return false;
    }
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string raw_line;
    if (std::getline(file, raw_line)) {
        // Bounds check - ensure line isn't too long
        if (raw_line.length() > MAX_LINE_LENGTH) {
            return false;
        }
        
        line = sanitize_input(raw_line);
        file.close();
        return true;
    }
    
    file.close();
    return false;
}

bool SafeFileUtils::read_all_lines(const std::string& file_path, std::vector<std::string>& lines, size_t max_lines) {
    // Validate path safety
    if (!is_safe_path(file_path)) {
        return false;
    }
    
    // Check file size
    if (!check_file_size(file_path)) {
        return false;
    }
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    
    lines.clear();
    std::string raw_line;
    size_t line_count = 0;
    
    while (std::getline(file, raw_line) && line_count < max_lines) {
        // Bounds check - ensure line isn't too long
        if (raw_line.length() > MAX_LINE_LENGTH) {
            file.close();
            return false;
        }
        
        lines.push_back(sanitize_input(raw_line));
        ++line_count;
    }
    
    file.close();
    return true;
}

bool SafeFileUtils::find_pattern(const std::string& file_path, const std::string& pattern, std::string& result) {
    // Validate path safety
    if (!is_safe_path(file_path)) {
        return false;
    }
    
    // Check file size
    if (!check_file_size(file_path)) {
        return false;
    }
    
    // Validate pattern (prevent injection)
    if (pattern.empty() || pattern.length() > 256) {
        return false;
    }
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string raw_line;
    size_t line_count = 0;
    
    while (std::getline(file, raw_line) && line_count < MAX_LINES) {
        // Bounds check
        if (raw_line.length() > MAX_LINE_LENGTH) {
            file.close();
            return false;
        }
        
        if (raw_line.find(pattern) != std::string::npos) {
            result = sanitize_input(raw_line);
            file.close();
            return true;
        }
        
        ++line_count;
    }
    
    file.close();
    return false;
}