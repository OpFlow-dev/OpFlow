#include <gmock/gmock.h>
#include <print>
#include <OpFlow>

TEST(MPITest, RankTest) {
    auto rank = OpFlow::getWorkerId();
    std::print("My ID = {}", rank);
    ASSERT_TRUE(true);
}