#include <format>
#include <gmock/gmock.h>
#include <iostream>
#ifdef OPFLOW_USE_MODULE
import opflow;
#else
#include <OpFlow>
#endif

TEST(MPITest, RankTest) {
    auto rank = OpFlow::getWorkerId();
    std::cout << std::format("My ID = {}", rank);
    ASSERT_TRUE(true);
}