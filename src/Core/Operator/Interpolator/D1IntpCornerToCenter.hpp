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

#ifndef OPFLOW_D1INTPCORNERTOCENTER_HPP
#define OPFLOW_D1INTPCORNERTOCENTER_HPP

#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Math/Interpolator/Interpolator.hpp"

namespace OpFlow {
    template <std::size_t d>
    struct D1IntpCornerToCenter {
        constexpr static auto bc_width = 1;

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const T& e, auto&& i) {
            return e.accessibleRange.start[d] <= i[d] && i[d] + 1 < e.accessibleRange.end[d];
        }

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto eval_safe(const T& e, auto&& i) {
            return Math::mid(e.evalSafeAt(i), e.evalSafeAt(i.template next<d>()));
        }

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto eval(const T& e, auto&& i) {
            return Math::mid(e.evalAt(i), e.evalAt(i.template next<d>()));
        }

        template <CartesianFieldExprType T>
        static void prepare(Expression<D1IntpCornerToCenter, T>& expr) {
            expr.name = fmt::format("D1IntpCornerToCenter<{}>({})", d, expr.arg1.name);
            expr.loc = expr.arg1.loc;
            expr.loc[d] = LocOnMesh::Center;
            expr.mesh = expr.arg1.mesh.getView();
            expr.accessibleRange = expr.arg1.accessibleRange;
            expr.accessibleRange.end[d]--;
            expr.assignableRange.setEmpty();
            expr.localRange = expr.arg1.localRange;
            expr.localRange.end[d]--;
        }
    };

    template <std::size_t d, CartesianFieldExprType T>
    struct ResultType<D1IntpCornerToCenter<d>, T> {
        using type = typename internal::CartesianFieldExprTrait<T>::template twin_type<
                Expression<D1IntpCornerToCenter<d>, T>>;
    };

    namespace internal {
        template <std::size_t d, CartesianFieldExprType T>
        struct ExprTrait<Expression<D1IntpCornerToCenter<d>, T>> : ExprTrait<T> {
            static constexpr int bc_width
                    = D1IntpCornerToCenter<d>::bc_width + CartesianFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using mesh_type
                    = decltype(std::declval<typename CartesianFieldExprTrait<T>::mesh_type&>().getView());
        };
    }// namespace internal

    template <std::size_t d, CartesianFieldExprType T>
    auto d1IntpCornerToCenter(T&& expr) {
        return makeExpression<D1IntpCornerToCenter<d>>(OP_PERFECT_FOWD(expr));
    }
}// namespace OpFlow
#endif//OPFLOW_D1INTPCORNERTOCENTER_HPP
