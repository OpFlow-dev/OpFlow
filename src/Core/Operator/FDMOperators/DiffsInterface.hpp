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

#ifndef OPFLOW_DIFFSINTERFACE_HPP
#define OPFLOW_DIFFSINTERFACE_HPP

#include "Core/Expr/Expression.hpp"
#include "Core/Meta.hpp"

namespace OpFlow {

#define UNARY_OP_FUNC_DEF(func_name)                                                                         \
    template <typename Kernel>                                                                               \
    auto func_name(auto&& expr) {                                                                            \
        return makeExpression<Kernel>(std::forward<decltype(expr)>(expr));                                   \
    }

    UNARY_OP_FUNC_DEF(d1)

#define THREED_UNARY_OP_DEF(x_name, y_name, z_name, u_name)                                                  \
    template <template <std::size_t> typename Kernel>                                                        \
    auto x_name(auto&& expr) {                                                                               \
        return u_name<Kernel<0>>(std::forward<decltype(expr)>(expr));                                        \
    }                                                                                                        \
    template <template <std::size_t> typename Kernel>                                                        \
    auto y_name(auto&& expr) {                                                                               \
        return u_name<Kernel<1>>(std::forward<decltype(expr)>(expr));                                        \
    }                                                                                                        \
    template <template <std::size_t> typename Kernel>                                                        \
    auto z_name(auto&& expr) {                                                                               \
        return u_name<Kernel<2>>(std::forward<decltype(expr)>(expr));                                        \
    }

    THREED_UNARY_OP_DEF(dx, dy, dz, d1)

    UNARY_OP_FUNC_DEF(d2)

    THREED_UNARY_OP_DEF(d2x, d2y, d2z, d2)

#undef UNARY_OP_FUNC_DEF
#undef THREED_UNARY_OP_DEF
}// namespace OpFlow

#endif//OPFLOW_DIFFSINTERFACE_HPP
