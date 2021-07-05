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

#ifndef OPFLOW_STENCILKERNELTRAIT_HPP
#define OPFLOW_STENCILKERNELTRAIT_HPP

#include "Core/Meta.hpp"
#include "StencilKernel.hpp"

namespace OpFlow::Core::Operator {

    using OpFlow::Meta::int_;

    template <typename StencilKernel>
    struct StencilKernelTrait;

    template <>
    struct StencilKernelTrait<StencilCube22Uniform> {
        using width = int_<2>;
        using dim = int_<2>;
    };

    template <>
    struct StencilKernelTrait<StencilCube24Uniform> {
        using width = int_<4>;
        using dim = int_<2>;
    };

    template <>
    struct StencilKernelTrait<StencilCube32Uniform> {
        using width = int_<2>;
        using dim = int_<3>;
    };

    template <>
    struct StencilKernelTrait<StencilCube22ShareWeighted> {
        using width = int_<2>;
        using dim = int_<2>;
    };

    template <>
    struct StencilKernelTrait<StencilCube32ShareWeighted> {
        using width = int_<2>;
        using dim = int_<3>;
    };
}// namespace OpFlow::Core::Operator
#endif//OPFLOW_STENCILKERNELTRAIT_HPP
