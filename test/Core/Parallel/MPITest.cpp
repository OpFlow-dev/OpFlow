#include <OpFlow>
#include <gmock/gmock.h>
#include <print>

TEST(MPITest, RankTest) {
    auto rank = OpFlow::getWorkerId();
    std::print("My ID = {}", rank);
    ASSERT_TRUE(true);
}