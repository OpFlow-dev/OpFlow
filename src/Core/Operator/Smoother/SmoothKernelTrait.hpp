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

#ifndef OPFLOW_SMOOTHKERNELTRAIT_HPP
#define OPFLOW_SMOOTHKERNELTRAIT_HPP

#include "Core/Meta.hpp"
#include "SmoothKernel.hpp"
#include "StencilKernelTrait.hpp"

namespace OpFlow::Core::Operator {

    using OpFlow::Meta::int_;

    template <typename SmoothKernel>
    struct SmoothKernelTrait;

    template <typename Derived>
    struct SmoothKernelTrait<SmoothKernelBase<Derived>> : SmoothKernelTrait<Derived> {};

    template <>
    struct SmoothKernelTrait<IdenticalSmoothKernel> {
        using width = int_<0>;
        using dim = int_<0>;
    };

    template <typename StencilKernel>
    struct SmoothKernelTrait<StencilSmoothKernel<StencilKernel>> {
        using width = typename StencilKernelTrait<StencilKernel>::width;
        using dim = typename StencilKernelTrait<StencilKernel>::dim;
    };
}// namespace OpFlow::Core::Operator
#endif//OPFLOW_SMOOTHKERNELTRAIT_HPP
