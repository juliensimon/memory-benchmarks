#include "test_framework.h"
#include "../common/aligned_buffer.h"
#include "../common/errors.h"
#include <cstring>

void test_aligned_buffer_creation() {
    AlignedBuffer buffer(1024, 64);
    ASSERT_NOT_NULL(buffer.data());
    TestAssert::assert_equal_int(1024, buffer.size());
    TestAssert::assert_equal_int(64, buffer.alignment());
}

void test_aligned_buffer_alignment() {
    AlignedBuffer buffer(1024, 128);
    ASSERT_TRUE(buffer.is_aligned());
    
    // Check that pointer is aligned to 128 bytes
    uintptr_t addr = reinterpret_cast<uintptr_t>(buffer.data());
    TestAssert::assert_equal_uintptr(0, addr % 128);
}

void test_aligned_buffer_initialization() {
    AlignedBuffer buffer(256, 64);
    uint8_t* data = buffer.data();
    
    // Check that buffer is initialized with pattern
    for (size_t i = 0; i < 256; ++i) {
        TestAssert::assert_equal(static_cast<uint8_t>(i & 0xFF), data[i]);
    }
}

void test_aligned_buffer_move_constructor() {
    AlignedBuffer buffer1(512, 64);
    uint8_t* original_ptr = buffer1.data();
    
    AlignedBuffer buffer2(std::move(buffer1));
    TestAssert::assert_equal(original_ptr, buffer2.data());
    TestAssert::assert_equal_int(512, buffer2.size());
    TestAssert::assert_equal_int(64, buffer2.alignment());
    
    // Original buffer should be invalidated
    TestAssert::assert_equal(static_cast<uint8_t*>(nullptr), buffer1.data());
    TestAssert::assert_equal_size_t(0, buffer1.size());
}

void test_aligned_buffer_move_assignment() {
    AlignedBuffer buffer1(256, 128);
    AlignedBuffer buffer2(512, 64);
    
    uint8_t* original_ptr = buffer1.data();
    buffer2 = std::move(buffer1);
    
    TestAssert::assert_equal(original_ptr, buffer2.data());
    TestAssert::assert_equal_int(256, buffer2.size());
    TestAssert::assert_equal_int(128, buffer2.alignment());
}

void test_aligned_buffer_invalid_params() {
    try {
        AlignedBuffer buffer(0, 64);  // Zero size should throw
        ASSERT_TRUE(false);  // Should not reach here
    } catch (const MemoryError&) {
        // Expected
    }
    
    try {
        AlignedBuffer buffer(1024, 63);  // Non-power-of-2 alignment should throw
        ASSERT_TRUE(false);  // Should not reach here
    } catch (const MemoryError&) {
        // Expected
    }
}

int main() {
    TestFramework framework;
    
    TEST_CASE("AlignedBuffer creation", test_aligned_buffer_creation);
    TEST_CASE("AlignedBuffer alignment", test_aligned_buffer_alignment);
    TEST_CASE("AlignedBuffer initialization", test_aligned_buffer_initialization);
    TEST_CASE("AlignedBuffer move constructor", test_aligned_buffer_move_constructor);
    TEST_CASE("AlignedBuffer move assignment", test_aligned_buffer_move_assignment);
    TEST_CASE("AlignedBuffer invalid parameters", test_aligned_buffer_invalid_params);
    
    return framework.run_all();
}