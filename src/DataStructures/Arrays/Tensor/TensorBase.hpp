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

#ifndef OPFLOW_TENSORBASE_HPP
#define OPFLOW_TENSORBASE_HPP

#include "TensorTrait.hpp"

namespace OpFlow::DS {
    template <typename Derived>
    struct Tensor {
        using scalar_type = typename internal::TensorTrait<Derived>::scalar_type;
    };
}// namespace OpFlow::DS
#endif//OPFLOW_TENSORBASE_HPP
