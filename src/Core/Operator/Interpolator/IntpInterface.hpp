//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2026 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_INTPINTERFACE_HPP
#define OPFLOW_INTPINTERFACE_HPP

#include "Core/Expr/Expression.hpp"
#include "Core/Meta.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    enum class IntpDirection { Cor2Cen, Cen2Cor };

    // default linear kernel
    template <std::size_t d, IntpDirection dir>
    struct D1Linear;

    template <std::size_t dim, template <std::size_t, IntpDirection> typename Kernel = D1Linear>
    auto d1IntpCenterToCorner(auto&&... expr) {
        return makeExpression<Kernel<dim, IntpDirection::Cen2Cor>>(OP_PERFECT_FOWD(expr)...);
    }

    template <std::size_t dim, template <std::size_t, IntpDirection> typename Kernel = D1Linear>
    auto d1IntpCornerToCenter(auto&&... expr) {
        return makeExpression<Kernel<dim, IntpDirection::Cor2Cen>>(OP_PERFECT_FOWD(expr)...);
    }

    template <std::size_t dim, IntpDirection dir,
              template <std::size_t, IntpDirection> typename Kernel = D1Linear>
    auto d1Intp(auto&&... expr) {
        return makeExpression<Kernel<dim, dir>>(OP_PERFECT_FOWD(expr)...);
    }
}// namespace OpFlow

#endif//OPFLOW_INTPINTERFACE_HPP
