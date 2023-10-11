// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2023 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_FIRSTORDERCENTEREDSTAGGERED_HPP
#define OPFLOW_FIRSTORDERCENTEREDSTAGGERED_HPP

#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "DataStructures/Range/Ranges.hpp"

namespace OpFlow {
    template <std::size_t d>
    struct D1FirstOrderCentered {
        constexpr static auto bc_width = 1;

        template <CartesianFieldExprType E>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const E& e, auto&& i) {
            return e.loc[d] == LocOnMesh::Center ? e.couldEvalAt(i) && e.couldEvalAt(i.template prev<d>())
                                                 : e.couldEvalAt(i) && e.couldEvalAt(i.template next<d>());
        }

        template <CartesianFieldExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto eval(const E& e, I i) {
            return e.loc[d] == LocOnMesh::Center
                           ? (e.evalAt(i) - e.evalAt(i.template prev<d>()))
                                     / (e.mesh.dx(d, i[d] - 1) + e.mesh.dx(d, i[d])) * 2
                           : (e.evalAt(i.template next<d>()) - e.evalAt(i)) / (e.mesh.dx(d, i[d]));
        }

        template <CartesianFieldExprType E>
        static inline void prepare(const Expression<D1FirstOrderCentered, E>& expr) {
            // name
            expr.name = fmt::format("d1<D1FirstOrderCentered<{}>>(", d) + expr.arg1.name + ")";

            // mesh
            expr.mesh = expr.arg1.mesh.getView();
            expr.loc = expr.arg1.loc;
            expr.loc[d] = expr.arg1.loc[d] == LocOnMesh::Center ? LocOnMesh::Corner : LocOnMesh::Center;

            // ranges
            expr.accessibleRange = expr.arg1.accessibleRange;
            expr.localRange = expr.arg1.localRange;
            expr.logicalRange = expr.arg1.logicalRange;
            if (expr.arg1.loc[d] == LocOnMesh::Center) {
                // nodal case
                expr.accessibleRange.start[d]++;
                expr.localRange.start[d]++;
                expr.logicalRange.start[d]++;
            } else {
                // corner case
                expr.accessibleRange.end[d]--;
                expr.localRange.end[d]--;
                expr.logicalRange.end[d]--;
            }
            // make the result expr read-only
            expr.assignableRange.setEmpty();
        }
    };

    template <std::size_t d, CartesianFieldExprType T>
    struct ResultType<D1FirstOrderCentered<d>, T> {
        using type = CartesianFieldExpr<Expression<D1FirstOrderCentered<d>, T>>;
        using core_type = Expression<D1FirstOrderCentered<d>, T>;
    };

    namespace internal {
        template <std::size_t d, CartesianFieldExprType T>
        struct ExprTrait<Expression<D1FirstOrderCentered<d>, T>> {
            static constexpr int dim = CartesianFieldExprTrait<T>::dim;
            static constexpr int bc_width
                    = D1FirstOrderCentered<d>::bc_width + CartesianFieldExprTrait<T>::bc_width;
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
#endif//OPFLOW_FIRSTORDERCENTEREDSTAGGERED_HPP
