// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_TENSORTRAIT_HPP
#define OPFLOW_TENSORTRAIT_HPP

#include <type_traits>

namespace OpFlow::DS {
    template <typename Derived>
    struct Tensor;

    template <typename T>
    concept TensorType = std::is_base_of_v<Tensor<T>, T>;

    namespace internal {
        template <typename T>
        struct TensorTrait;

        template <typename T>
        struct TensorTrait<const T> : TensorTrait<T> {};

        template <typename T>
        struct TensorTrait<T&> : TensorTrait<T> {};
    }// namespace internal
}// namespace OpFlow::DS
#endif//OPFLOW_TENSORTRAIT_HPP
