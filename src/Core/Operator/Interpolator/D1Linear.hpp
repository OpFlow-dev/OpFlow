//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_D1LINEAR_HPP
#define OPFLOW_D1LINEAR_HPP

#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Operator/Interpolator/IntpInterface.hpp"
#include "Math/Interpolator/Interpolator.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <std::size_t d, IntpDirection dir>
    struct D1Linear {
        constexpr static auto bc_width = 1;

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const T& e, auto&& i) {
            if constexpr (dir == IntpDirection::Cen2Cor)
                return e.couldEvalAt(i) && e.couldEvalAt(i.template prev<d>());
            else
                return e.couldEvalAt(i) && e.couldEvalAt(i.template next<d>());
        }

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto eval(const T& e, auto&& i) {
            if constexpr (dir == IntpDirection::Cen2Cor) {
                auto x1 = e.mesh.x(d, i[d] - 1) + 0.5 * e.mesh.dx(d, i[d] - 1);
                auto x2 = e.mesh.x(d, i[d]) + 0.5 * e.mesh.dx(d, i[d]);
                auto y1 = e.evalAt(i.template prev<d>());
                auto y2 = e.evalAt(i);
                auto x = e.mesh.x(d, i[d]);
                return Math::Interpolator1D::intp(x1, y1, x2, y2, x);
            } else {
                return Math::mid(e.evalAt(i), e.evalAt(i.template next<d>()));
            }
        }

        template <CartesianFieldExprType T>
        static void prepare(const Expression<D1Linear, T>& expr) {
            if constexpr (dir == IntpDirection::Cen2Cor) {
                expr.initPropsFrom(expr.arg1);
                expr.name = std::format("D1Intp<D1Linear, {}, Cen2Cor>({})", d, expr.arg1.name);
                expr.loc = expr.arg1.loc;
                expr.loc[d] = LocOnMesh::Corner;
                expr.mesh = expr.arg1.mesh.getView();
                expr.accessibleRange.start[d]++;
                expr.localRange.start[d]++;
                expr.logicalRange.start[d]++;
                expr.assignableRange.setEmpty();
            } else {
                expr.initPropsFrom(expr.arg1);
                expr.name = std::format("D1Intp<D1Linear, {}, Cor2Cen>({})", d, expr.arg1.name);
                expr.loc = expr.arg1.loc;
                expr.loc[d] = LocOnMesh::Center;
                expr.mesh = expr.arg1.mesh.getView();
                expr.accessibleRange.end[d]--;
                expr.logicalRange.end[d]--;
                expr.localRange.end[d]--;
                expr.assignableRange.setEmpty();
            }
        }
    };

    template <std::size_t d, IntpDirection dir, CartesianFieldExprType T>
    struct ResultType<D1Linear<d, dir>, T> {
        using type = typename internal::CartesianFieldExprTrait<T>::template twin_type<
                Expression<D1Linear<d, dir>, T>>;
    };

    namespace internal {
        template <std::size_t d, IntpDirection dir, CartesianFieldExprType T>
        struct ExprTrait<Expression<D1Linear<d, dir>, T>> : ExprTrait<T> {
            static constexpr int bc_width = D1Linear<d, dir>::bc_width + CartesianFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using mesh_type
                    = decltype(std::declval<typename CartesianFieldExprTrait<T>::mesh_type&>().getView());
        };
    }// namespace internal
}// namespace OpFlow

#endif//OPFLOW_D1LINEAR_HPP
