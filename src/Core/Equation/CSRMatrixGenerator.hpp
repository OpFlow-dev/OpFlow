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

#ifndef OPFLOW_CSRMATRIXGENERATOR_HPP
#define OPFLOW_CSRMATRIXGENERATOR_HPP

#include "Core/Equation/StencilHolder.hpp"
#include "Core/Meta.hpp"
#include <vector>

namespace OpFlow {
    struct CSRMatrixGenerator {
        template <typename S>
        static auto generate(const S& s) {}
    };
}// namespace OpFlow

#endif//OPFLOW_CSRMATRIXGENERATOR_HPP
