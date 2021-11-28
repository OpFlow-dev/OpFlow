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

#ifndef OPFLOW_FIRSTORDERCENTERBIASEDUPWIND_HPP
#define OPFLOW_FIRSTORDERCENTERBIASEDUPWIND_HPP

#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "DataStructures/Range/Ranges.hpp"

namespace OpFlow {
    template <std::size_t d>
    struct D1FirstOrderCenteredUpwind {
        constexpr static auto bc_width = 1;

        template <CartesianFieldExprType E>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const E& e, auto&& i) {
            auto cond0 = e.accessibleRange.start[d] <= i[d] && i[d] + 1 < e.accessibleRange.end[d];
            auto cond1 = e.bc[d].start && e.loc[d] == LocOnMesh::Center
                         && i[d] + 1 == e.accessibleRange.start[d]
                         && (e.bc[d].start->getBCType() == BCType::Neum
                             || e.bc[d].start->getBCType() == BCType::Dirc);
            auto cond2 = e.bc[d].end && e.loc[d] == LocOnMesh::Center && i[d] + 1 == e.accessibleRange.end[d]
                         && (e.bc[d].end->getBCType() == BCType::Neum
                             || e.bc[d].end->getBCType() == BCType::Dirc);
            return cond0 || cond1 || cond2;
        }
        template <CartesianFieldExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto eval_safe(const E& e, I i) {
            if constexpr (CartesianFieldType<E>) {
                if (e.accessibleRange.start[d] <= i[d] && i[d] + 1 < e.accessibleRange.end[d])
                    return (e.evalSafeAt(i.template next<d>()) - e.evalSafeAt(i))
                           / (e.loc[d] == LocOnMesh::Corner
                                      ? e.mesh.dx(d, i[d])
                                      : (e.mesh.dx(d, i[d]) + e.mesh.dx(d, i[d] + 1)) * 0.5);
                // left bc case
                if (e.bc[d].start && e.loc[d] == LocOnMesh::Center
                    && i[d] + 1 == e.accessibleRange.start[d]) {
                    if (e.bc[d].start->getBCType() == BCType::Neum) return e.bc[d].start->evalAt(i);
                    else if (e.bc[d].start->getBCType() == BCType::Dirc) {
                        // assume asymmetric extension
                        return (e.evalAt(i.template next<d>()) - e.bc[d].start->evalAt(i))
                               * (e.mesh.idx(d, i[d] + 1)) * 2.0;
                    }
                }
                // right bc case
                if (e.bc[d].end && e.loc[d] == LocOnMesh::Center && i[d] + 1 == e.accessibleRange.end[d]) {
                    if (e.bc[d].end->getBCType() == BCType::Neum) return e.bc[d].end->evalAt(i);
                    else if (e.bc[d].end->getBCType() == BCType::Dirc) {
                        // assume asymmetric extension
                        return (e.bc[d].end->evalAt(i) - e.evalAt(i)) * e.mesh.idx(d, i[d]) * 2.0;
                    }
                }
            } else {
                return (e.evalSafeAt(i.template next<d>()) - e.evalSafeAt(i))
                       / (e.loc[d] == LocOnMesh::Corner
                                  ? e.mesh.dx(d, i[d])
                                  : (e.mesh.dx(d, i[d]) + e.mesh.dx(d, i[d] + 1)) * 0.5);
            }
            // not handled case
            OP_ERROR("Cannot handle current case.");
            OP_ERROR("Expr and index are: \n{}\nIndex = {}", e.getName(), i.toString());
            OP_ABORT;
        }

        template <CartesianFieldExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto eval(const E& e, I i) {
            return (e.evalAt(i.template next<d>()) - e.evalAt(i))
                   / (e.loc[d] == LocOnMesh::Corner ? e.mesh.dx(d, i[d])
                                                    : (e.mesh.dx(d, i[d]) + e.mesh.dx(d, i[d] + 1)) * 0.5);
        }

        template <CartesianFieldExprType E>
        static inline void prepare(Expression<D1FirstOrderCenteredUpwind, E>& expr) {
            constexpr auto dim = internal::CartesianFieldExprTrait<E>::dim;

            // name
            expr.name = fmt::format("d1<D1FirstOrderCenteredUpwind<{}>>(", d) + expr.arg1.name + ")";

            // mesh
            expr.mesh = expr.arg1.mesh.getView();
            if (expr.arg1.loc[d] == LocOnMesh::Center) expr.mesh.appendOffsetOf(d, -1);
            expr.loc = expr.arg1.loc;
            expr.loc[d] = expr.arg1.loc[d] == LocOnMesh::Center ? LocOnMesh::Corner : LocOnMesh::Center;

            // ranges
            expr.accessibleRange = expr.arg1.accessibleRange;
            if (expr.arg1.loc[d] == LocOnMesh::Corner) {
                // nodal case
                expr.accessibleRange.end[d]--;
            } else {
                // center case
                expr.accessibleRange.end[d]--;
            }
            expr.localRange = expr.accessibleRange;
            // make the result expr read-only
            expr.assignableRange.setEmpty();
        }
    };

    template <std::size_t d, CartesianFieldExprType T>
    struct ResultType<D1FirstOrderCenteredUpwind<d>, T> {
        using type = CartesianFieldExpr<Expression<D1FirstOrderCenteredUpwind<d>, T>>;
        using core_type = Expression<D1FirstOrderCenteredUpwind<d>, T>;
    };

    namespace internal {
        template <std::size_t d, CartesianFieldExprType T>
        struct ExprTrait<Expression<D1FirstOrderCenteredUpwind<d>, T>> {
            static constexpr int dim = CartesianFieldExprTrait<T>::dim;
            static constexpr int bc_width
                    = D1FirstOrderCenteredUpwind<d>::bc_width + CartesianFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using type = typename CartesianFieldExprTrait<T>::type;
            template <typename Other>
            using other_type = typename CartesianFieldExprTrait<T>::template other_type<Other>;
            template <typename Other>
            using twin_type = typename CartesianFieldExprTrait<T>::template twin_type<Other>;
            using elem_type = typename CartesianFieldExprTrait<T>::elem_type;
            using mesh_type
                    = decltype(std::declval<typename CartesianFieldExprTrait<T>::mesh_type&>().getView());
            using range_type = typename CartesianFieldExprTrait<T>::range_type;
            using index_type = typename CartesianFieldExprTrait<T>::index_type;
        };
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_FIRSTORDERCENTERBIASEDUPWIND_HPP
