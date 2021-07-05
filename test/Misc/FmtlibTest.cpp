#include "fmt/include/fmt/core.h"
#include <gmock/gmock.h>

using namespace testing;

// Test to ensure fmt lib is linked correctly
TEST(FMTTest, FMTTest) {
    fmt::print("Hello {}\n", "world");
    ASSERT_TRUE(true);
}