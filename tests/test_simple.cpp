#include "test_framework.h"

void test_basic_functionality() {
    ASSERT_TRUE(true);
    TestAssert::assert_equal(1, 1);
    TestAssert::assert_equal(std::string("hello"), std::string("hello"));
}

int main() {
    TestFramework framework;
    
    TEST_CASE("Basic functionality", test_basic_functionality);
    
    return framework.run_all();
}