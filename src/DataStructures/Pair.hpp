// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2023 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_PAIR_HPP
#define OPFLOW_PAIR_HPP

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {
    template <typename T>
    struct Pair {
        T start, end;
        Pair() = default;
        Pair(const Pair&) = default;
        Pair(Pair&&) noexcept = default;

        Pair& operator=(const Pair&) = default;
        Pair& operator=(Pair&&) noexcept = default;
    };
}// namespace OpFlow::DS
#endif//OPFLOW_PAIR_HPP
