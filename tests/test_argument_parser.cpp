#include "test_framework.h"
#include "../common/argument_parser.h"

void test_help_argument() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--help"};
    BenchmarkConfig config = parser.parse(2, const_cast<char**>(argv));
    
    ASSERT_TRUE(config.help_requested);
}

void test_info_argument() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--info"};
    BenchmarkConfig config = parser.parse(2, const_cast<char**>(argv));
    
    ASSERT_TRUE(config.show_info);
}

void test_size_argument() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--size", "8"};
    BenchmarkConfig config = parser.parse(3, const_cast<char**>(argv));
    
    TestAssert::assert_equal_size_t(1, config.memory_sizes_gb.size());
    TestAssert::assert_equal(8.0, config.memory_sizes_gb[0]);
}

void test_multiple_sizes() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--size", "4,8,16"};
    BenchmarkConfig config = parser.parse(3, const_cast<char**>(argv));
    
    TestAssert::assert_equal_size_t(3, config.memory_sizes_gb.size());
    TestAssert::assert_equal(4.0, config.memory_sizes_gb[0]);
    TestAssert::assert_equal(8.0, config.memory_sizes_gb[1]);
    TestAssert::assert_equal(16.0, config.memory_sizes_gb[2]);
}

void test_iterations_argument() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--iterations", "20"};
    BenchmarkConfig config = parser.parse(3, const_cast<char**>(argv));
    
    TestAssert::assert_equal_size_t(20, config.iterations);
}

void test_threads_argument() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--threads", "8"};
    BenchmarkConfig config = parser.parse(3, const_cast<char**>(argv));
    
    TestAssert::assert_equal_size_t(8, config.num_threads);
}

void test_pattern_argument() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--pattern", "sequential_read"};
    BenchmarkConfig config = parser.parse(3, const_cast<char**>(argv));
    
    TestAssert::assert_equal(std::string("sequential_read"), config.pattern_str);
}

void test_format_argument() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--format", "json"};
    BenchmarkConfig config = parser.parse(3, const_cast<char**>(argv));
    
    TestAssert::assert_equal(std::string("json"), config.format_str);
}

void test_cache_hierarchy_mode() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--cache-hierarchy"};
    BenchmarkConfig config = parser.parse(2, const_cast<char**>(argv));
    
    ASSERT_TRUE(config.cache_hierarchy);
    TestAssert::assert_equal_size_t(0, config.memory_sizes_gb.size());  // Should clear sizes
}

void test_invalid_argument() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--invalid"};
    
    try {
        parser.parse(2, const_cast<char**>(argv));
        ASSERT_TRUE(false);  // Should throw
    } catch (const ArgumentError& e) {
        // Expected
        std::string error_msg = e.what();
        ASSERT_TRUE(error_msg.find("Unknown argument") != std::string::npos);
    }
}

void test_missing_value() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--size"};
    
    try {
        parser.parse(2, const_cast<char**>(argv));
        ASSERT_TRUE(false);  // Should throw
    } catch (const ArgumentError& e) {
        // Expected
        std::string error_msg = e.what();
        ASSERT_TRUE(error_msg.find("requires a value") != std::string::npos);
    }
}

void test_invalid_pattern() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--pattern", "invalid_pattern"};
    
    try {
        parser.parse(3, const_cast<char**>(argv));
        ASSERT_TRUE(false);  // Should throw
    } catch (const ArgumentError& e) {
        // Expected
        std::string error_msg = e.what();
        ASSERT_TRUE(error_msg.find("Invalid pattern") != std::string::npos);
    }
}

void test_mutual_exclusion() {
    ArgumentParser parser("test", "Test program");
    
    const char* argv[] = {"test", "--cache-hierarchy", "--pattern", "sequential_read"};
    
    try {
        parser.parse(4, const_cast<char**>(argv));
        ASSERT_TRUE(false);  // Should throw
    } catch (const ArgumentError& e) {
        // Expected
        std::string error_msg = e.what();
        ASSERT_TRUE(error_msg.find("mutually exclusive") != std::string::npos);
    }
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Help argument", test_help_argument);
    TEST_CASE("Info argument", test_info_argument);
    TEST_CASE("Size argument", test_size_argument);
    TEST_CASE("Multiple sizes", test_multiple_sizes);
    TEST_CASE("Iterations argument", test_iterations_argument);
    TEST_CASE("Threads argument", test_threads_argument);
    TEST_CASE("Pattern argument", test_pattern_argument);
    TEST_CASE("Format argument", test_format_argument);
    TEST_CASE("Cache hierarchy mode", test_cache_hierarchy_mode);
    TEST_CASE("Invalid argument", test_invalid_argument);
    TEST_CASE("Missing value", test_missing_value);
    TEST_CASE("Invalid pattern", test_invalid_pattern);
    TEST_CASE("Mutual exclusion", test_mutual_exclusion);
    
    return framework.run_all();
}