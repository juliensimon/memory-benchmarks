#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <exception>

class TestFramework {
private:
    struct TestCase {
        std::string name;
        std::function<void()> test_func;
    };
    
    std::vector<TestCase> tests;
    int passed = 0;
    int failed = 0;
    
public:
    void add_test(const std::string& name, std::function<void()> test_func) {
        tests.push_back({name, test_func});
    }
    
    int run_all() {
        std::cout << "Running " << tests.size() << " test(s)...\n\n";
        
        for (const auto& test : tests) {
            try {
                test.test_func();
                std::cout << "✓ " << test.name << "\n";
                passed++;
            } catch (const std::exception& e) {
                std::cout << "✗ " << test.name << " - " << e.what() << "\n";
                failed++;
            } catch (...) {
                std::cout << "✗ " << test.name << " - Unknown exception\n";
                failed++;
            }
        }
        
        std::cout << "\nResults: " << passed << " passed, " << failed << " failed\n";
        return failed;
    }
};

class TestAssert {
public:
    static void assert_true(bool condition, const std::string& message = "") {
        if (!condition) {
            throw std::runtime_error("Assertion failed" + (message.empty() ? "" : ": " + message));
        }
    }
    
    static void assert_false(bool condition, const std::string& message = "") {
        assert_true(!condition, message);
    }
    
    static void assert_equal(const std::string& expected, const std::string& actual, const std::string& message = "") {
        if (expected != actual) {
            throw std::runtime_error("Expected '" + expected + "' but got '" + actual + "'" + 
                                   (message.empty() ? "" : " (" + message + ")"));
        }
    }
    
    template<typename T>
    static void assert_equal(const T& expected, const T& actual, const std::string& message = "") {
        if (expected != actual) {
            throw std::runtime_error("Values not equal" + (message.empty() ? "" : ": " + message));
        }
    }
    
    static void assert_equal_size_t(size_t expected, size_t actual, const std::string& message = "") {
        if (expected != actual) {
            throw std::runtime_error("Expected " + std::to_string(expected) + " but got " + std::to_string(actual) +
                                   (message.empty() ? "" : " (" + message + ")"));
        }
    }
    
    static void assert_equal_int(int expected, size_t actual, const std::string& message = "") {
        if (static_cast<size_t>(expected) != actual) {
            throw std::runtime_error("Expected " + std::to_string(expected) + " but got " + std::to_string(actual) +
                                   (message.empty() ? "" : " (" + message + ")"));
        }
    }
    
    static void assert_equal_uintptr(uintptr_t expected, uintptr_t actual, const std::string& message = "") {
        if (expected != actual) {
            throw std::runtime_error("Expected " + std::to_string(expected) + " but got " + std::to_string(actual) +
                                   (message.empty() ? "" : " (" + message + ")"));
        }
    }
    
    static void assert_not_null(void* ptr, const std::string& message = "") {
        if (ptr == nullptr) {
            throw std::runtime_error("Pointer is null" + (message.empty() ? "" : ": " + message));
        }
    }
};

#define TEST_CASE(name, test_func) framework.add_test(name, test_func)
#define ASSERT_TRUE(condition) TestAssert::assert_true(condition, #condition)
#define ASSERT_FALSE(condition) TestAssert::assert_false(condition, #condition)
#define ASSERT_EQ(expected, actual) TestAssert::assert_equal(expected, actual, #expected " == " #actual)
#define ASSERT_NOT_NULL(ptr) TestAssert::assert_not_null(ptr, #ptr)