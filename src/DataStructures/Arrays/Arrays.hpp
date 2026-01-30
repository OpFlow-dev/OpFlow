// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_ARRAYS_HPP
#define OPFLOW_ARRAYS_HPP

#include "Core/BasicDataTypes.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Arrays/Tensor/PlainTensor.hpp"
#include "DataStructures/Index/MDIndex.hpp"
#include "DataStructures/Index/RangedIndex.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <utility>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {
    // General array
    template <typename dType, int k>
    using DenseTensor = PlainTensor<dType, k>;

    template <typename dType>
    using DenseMatrix = DenseTensor<dType, 2>;

    template <typename dType>
    using DenseVector = DenseTensor<dType, 1>;

    using Vector = DenseVector<OpFlow::Real>;

    template <int k>
    using IndexArray = MDIndex<k>;
}// namespace OpFlow::DS
#endif//OPFLOW_ARRAYS_HPP
