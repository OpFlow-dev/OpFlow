#include <gmock/gmock.h>
#include <print>
import opflow;

TEST(MPITest, RankTest) {
    auto rank = OpFlow::getWorkerId();
    std::print("My ID = {}", rank);
    ASSERT_TRUE(true);
}