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

#ifndef OPFLOW_D1INTPCENTERTOCORNER_HPP
#define OPFLOW_D1INTPCENTERTOCORNER_HPP

#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Math/Interpolator/Interpolator.hpp"

namespace OpFlow {
    template <std::size_t d>
    struct D1IntpCenterToCorner {
        constexpr static auto bc_width = 1;

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const T& e, auto&& i) {
            auto cond0 = e.accessibleRange.start[d] < i[d] && i[d] < e.accessibleRange.end[d];
            auto cond1 = i[d] == e.accessibleRange.start[d] && e.bc[d].start
                         && e.bc[d].start->getBCType() == BCType::Dirc;
            auto cond2 = i[d] == e.accessibleRange.end[d] && e.bc[d].end
                         && e.bc[d].end->getBCType() == BCType::Dirc;
            return cond0 || cond1 || cond2;
        }

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto eval_safe(const T& e, auto&& i) {
            if (e.accessibleRange.start[d] < i[d] && i[d] < e.accessibleRange.end[d]) {
                auto x1 = e.mesh.x(d, i[d] - 1) + 0.5 * e.mesh.dx(d, i[d] - 1);
                auto x2 = e.mesh.x(d, i[d]) + 0.5 * e.mesh.dx(d, i[d]);
                auto y1 = e.evalSafeAt(i.template prev<d>());
                auto y2 = e.evalSafeAt(i);
                auto x = e.mesh.x(d, i[d]);
                return Math::Interpolator1D::intp(x1, y1, x2, y2, x);
            } else if (i[d] == e.accessibleRange.start[d] && e.bc[d].start) {
                // left bc case
                if (e.bc[d].start->getBCType() == BCType::Dirc) {
                    return e.bc[d].start->evalAt(i);
                } else if (e.bc[d].start->getBCType() == BCType::Neum) {
                    return e.evalSafeAt(i) - e.bc[d].start->evalAt(i) * e.mesh.dx(d, i[d]) / 2;
                } else
                    goto error_case;
            } else if (i[d] == e.accessibleRange.end[d] && e.bc[d].end) {
                // right bc case
                if (e.bc[d].end->getBCType() == BCType::Dirc) {
                    return e.bc[d].end->evalAt(i);
                } else if (e.bc[d].end->getBCType() == BCType::Neum) {
                    return e.evalSafeAt(i.template prev<d>())
                           + e.bc[d].end->evalAt(i) * e.mesh.dx(d, i[d] - 1) / 2;
                } else
                    goto error_case;
            } else {
            error_case:
                OP_ERROR("Cannot eval D1IntpCenterToCorner<{}>({}) at {}", d, e.name, i.toString());
                OP_ABORT;
            }
        }

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto eval(const T& e, auto&& i) {
            auto x1 = e.mesh.x(d, i[d] - 1) + 0.5 * e.mesh.dx(d, i[d] - 1);
            auto x2 = e.mesh.x(d, i[d]) + 0.5 * e.mesh.dx(d, i[d]);
            auto y1 = e.evalAt(i.template prev<d>());
            auto y2 = e.evalAt(i);
            auto x = e.mesh.x(d, i[d]);
            return Math::Interpolator1D::intp(x1, y1, x2, y2, x);
        }

        template <CartesianFieldExprType T>
        static void prepare(Expression<D1IntpCenterToCorner, T>& expr) {
            expr.name = fmt::format("D1IntpCenterToCorner<{}>({})", d, expr.arg1.name);
            expr.loc = expr.arg1.loc;
            expr.loc[d] = LocOnMesh::Corner;
            expr.mesh = expr.arg1.mesh.getView();
            expr.accessibleRange = expr.arg1.accessibleRange;
            expr.localRange = expr.arg1.localRange;
            if (!(expr.arg1.bc[d].start
                  && (expr.arg1.bc[d].start->getBCType() == BCType::Dirc
                      || expr.arg1.bc[d].start->getBCType() == BCType::Neum))) {
                expr.accessibleRange.start[d]++;
                expr.localRange.start[d]++;
            }
            if (expr.arg1.bc[d].end
                && (expr.arg1.bc[d].end->getBCType() == BCType::Dirc
                    || expr.arg1.bc[d].end->getBCType() == BCType::Neum)) {
                expr.accessibleRange.end[d]++;
                expr.localRange.end[d]++;
            }
            expr.assignableRange.setEmpty();
            for (auto i = 0; i < internal::CartesianFieldExprTrait<T>::dim; ++i) {
                // todo: introduce bc type detection and correct the impl here

                expr.bc[i].start = expr.arg1.bc[i].start ? expr.arg1.bc[i].start->getCopy() : nullptr;
                expr.bc[i].end = expr.arg1.bc[i].end ? expr.arg1.bc[i].end->getCopy() : nullptr;
            }
        }
    };

    template <std::size_t d, CartesianFieldExprType T>
    struct ResultType<D1IntpCenterToCorner<d>, T> {
        using type = typename internal::CartesianFieldExprTrait<T>::template twin_type<
                Expression<D1IntpCenterToCorner<d>, T>>;
    };

    namespace internal {
        template <std::size_t d, CartesianFieldExprType T>
        struct ExprTrait<Expression<D1IntpCenterToCorner<d>, T>> : ExprTrait<T> {
            static constexpr int bc_width
                    = D1IntpCenterToCorner<d>::bc_width + CartesianFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using mesh_type
                    = decltype(std::declval<typename CartesianFieldExprTrait<T>::mesh_type&>().getView());
        };
    }// namespace internal

    template <std::size_t d, CartesianFieldExprType T>
    auto d1IntpCenterToCorner(T&& expr) {
        return makeExpression<D1IntpCenterToCorner<d>>(OP_PERFECT_FOWD(expr));
    }
}// namespace OpFlow
#endif//OPFLOW_D1INTPCENTERTOCORNER_HPP
