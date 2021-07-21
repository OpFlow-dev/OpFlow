//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021  by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_CONVOLUTION_HPP
#define OPFLOW_CONVOLUTION_HPP

#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Arrays/Tensor/FixedSizeTensor.hpp"

namespace OpFlow {
    template <std::size_t d>
    struct Convolution {
        template <StructuredFieldExprType E, Meta::Numerical K, typename I>
        OPFLOW_STRONG_INLINE static auto eval(const E& e, const DS::FixedSizeTensor<K, d>& kernel, I&& i) {}
    };
}// namespace OpFlow
#endif//OPFLOW_CONVOLUTION_HPP
