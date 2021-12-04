#include <OpFlow>
#include <gmock/gmock.h>

TEST(MPITest, RankTest) {
    auto rank = OpFlow::getWorkerId();
    OP_INFO("My ID = {}", rank);
    ASSERT_TRUE(true);
}