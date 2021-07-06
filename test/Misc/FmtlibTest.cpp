// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#include "fmt/include/fmt/core.h"
#include <gmock/gmock.h>

using namespace testing;

// Test to ensure fmt lib is linked correctly
TEST(FMTTest, FMTTest) {
    fmt::print("Hello {}\n", "world");
    ASSERT_TRUE(true);
}