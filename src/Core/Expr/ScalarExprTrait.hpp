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

#ifndef OPFLOW_SCALAREXPRTRAIT_HPP
#define OPFLOW_SCALAREXPRTRAIT_HPP

#include "Core/Expr/ExprTrait.hpp"

namespace OpFlow {
    template <typename T>
    struct ScalarExpr;

    namespace internal {
        template <typename T>
        struct ExprTrait<ScalarExpr<T>> {
            using type = std::decay_t<T>;
            using elem_type = std::decay_t<T>;
            static constexpr int access_flag = HasWriteAccess | HasDirectAccess;
        };
        template <typename T>
        struct ExprProxy<ScalarExpr<T>> {
            using type = ScalarExpr<T>;
        };
    }// namespace internal

    template <typename T>
    concept ScalarExprType = Meta::isTemplateInstance<ScalarExpr, T>::value;
}// namespace OpFlow

#endif//OPFLOW_SCALAREXPRTRAIT_HPP
