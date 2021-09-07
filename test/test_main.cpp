#include <gmock/gmock.h>
#include <OpFlow>

int main(int argc, char* argv[]) {
    OpFlow::InitEnvironment(&argc, &argv);
    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();
    OpFlow::FinalizeEnvironment();
    return ret;
}
