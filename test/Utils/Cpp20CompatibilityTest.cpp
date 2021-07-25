// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021  by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------
//
// This file serves as a compiler feature check test. The compiler must be able
// to compile this file to compile OpFlow.
//
// ----------------------------------------------------------------------------

#include <concepts>
#include <array>

template <auto>
struct any{};

int main() {
    any<1.0> a;
}