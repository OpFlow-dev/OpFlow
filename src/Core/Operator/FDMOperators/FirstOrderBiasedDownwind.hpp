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

#ifndef OPFLOW_FIRSTORDERBIASEDDOWNWIND_HPP
#define OPFLOW_FIRSTORDERBIASEDDOWNWIND_HPP

#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "DataStructures/Range/Ranges.hpp"

namespace OpFlow {
    template <std::size_t d>
    struct D1FirstOrderBiasedDownwind {
        constexpr static auto bc_width = 1;

        template <CartesianFieldExprType E>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const E& e, auto&& i) {
            auto cond0 = e.accessibleRange.start[d] <= i[d] - 1 && i[d] < e.accessibleRange.end[d];
            auto cond1 = e.bc[d].start && e.loc[d] == LocOnMesh::Center && i[d] == e.accessibleRange.start[d]
                         && (e.bc[d].start->getBCType() == BCType::Neum
                             || e.bc[d].start->getBCType() == BCType::Dirc);
            auto cond2 = e.bc[d].end && e.loc[d] == LocOnMesh::Center && i[d] == e.accessibleRange.end[d]
                         && (e.bc[d].end->getBCType() == BCType::Neum
                             || e.bc[d].end->getBCType() == BCType::Dirc);
            return cond0 || cond1 || cond2;
        }

        template <CartAMRFieldExprType E>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const E& e, auto&& i) {
            auto cond0 = e.accessibleRanges[i.l][i.p].start[d] <= i[d] - 1
                         && i[d] < e.accessibleRanges[i.l][i.p].end[d];
            auto cond1 = e.bc[d].start && e.loc[d] == LocOnMesh::Center
                         && i[d] == e.maxLogicalRanges[i.l].start[d]
                         && (e.bc[d].start->getBCType() == BCType::Neum
                             || e.bc[d].start->getBCType() == BCType::Dirc);
            auto cond2 = e.bc[d].end && e.loc[d] == LocOnMesh::Center
                         && i[d] == e.maxLogicalRanges[i.l].end[d]
                         && (e.bc[d].end->getBCType() == BCType::Neum
                             || e.bc[d].end->getBCType() == BCType::Dirc);
            return cond0 || cond1 || cond2;
        }

        template <CartesianFieldExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto eval(const E& e, I i) {
            return (e.evalAt(i) - e.evalAt(i.template prev<d>()))
                   / (e.loc[d] == LocOnMesh::Corner ? e.mesh.dx(d, i[d] - 1)
                                                    : (e.mesh.dx(d, i[d] - 1) + e.mesh.dx(d, i[d])) * 0.5);
        }

        template <CartAMRFieldExprType E>
        OPFLOW_STRONG_INLINE static auto eval(const E& e, auto&& i) {
            return (e.evalAt(i) - e.evalAt(i.template prev<d>()))
                   / (e.loc[d] == LocOnMesh::Corner
                              ? e.mesh.dx(d, i.l, i[d] - 1)
                              : (e.mesh.dx(d, i.l, i[d] - 1) + e.mesh.dx(d, i.l, i[d])) * 0.5);
        }

        template <CartesianFieldExprType E>
        static inline void prepare(Expression<D1FirstOrderBiasedDownwind, E>& expr) {
            expr.initPropsFrom(expr.arg1);

            // name
            expr.name = fmt::format("d1<D1FirstOrderBiasedDownwind<{}>>({})", d, expr.arg1.name);

            // mesh
            expr.mesh = expr.arg1.mesh.getView();
            expr.loc = expr.arg1.loc;

            // ranges
            expr.accessibleRange.start[d]++;
            expr.logicalRange.start[d]++;
            expr.localRange.start[d]++;
            // make the result expr read-only
            expr.assignableRange.setEmpty();
        }

        template <CartAMRFieldExprType E>
        static void prepare(Expression<D1FirstOrderBiasedDownwind, E>& expr) {
            constexpr auto dim = internal::CartAMRFieldExprTrait<E>::dim;
            expr.initPropsFrom(expr.arg1);

            // name
            expr.name = fmt::format("d1<D1FirstOrderBiasedDownwind<{}>>({})", d, expr.arg1.name);

            // ranges
            if (expr.arg1.loc[d] == LocOnMesh::Corner) {
                // nodal case
                for (auto& l : expr.accessibleRanges)
                    for (auto& r : l) r.start[d]++;
            } else {
                // center case
                if (!expr.arg1.bc[d].start) {
                    for (auto& l : expr.accessibleRanges) {
                        for (auto& r : l) {
                            if (r.start[d] / Math::int_pow(expr.mesh.refinementRatio, r.level)
                                == expr.maxLogicalRanges[0].start[d])
                                r.start[d]++;
                        }
                    }
                }
                if (expr.arg1.bc[d].end) {
                    for (auto& l : expr.accessibleRanges) {
                        for (auto& r : l) {
                            if (r.end[d] / Math::int_pow(expr.mesh.refinementRatio, r.level)
                                == expr.maxLogicalRanges[0].end[d])
                                r.end[d]++;
                        }
                    }
                }
            }
            for (auto& l : expr.assignableRanges)
                for (auto& r : l) r.setEmpty();
        }
    };

    template <std::size_t d, CartesianFieldExprType T>
    struct ResultType<D1FirstOrderBiasedDownwind<d>, T> {
        using type = CartesianFieldExpr<Expression<D1FirstOrderBiasedDownwind<d>, T>>;
        using core_type = Expression<D1FirstOrderBiasedDownwind<d>, T>;
    };

    template <std::size_t d, CartAMRFieldExprType T>
    struct ResultType<D1FirstOrderBiasedDownwind<d>, T> {
        using type = CartAMRFieldExpr<Expression<D1FirstOrderBiasedDownwind<d>, T>>;
        using core_type = Expression<D1FirstOrderBiasedDownwind<d>, T>;
    };

    namespace internal {
        template <std::size_t d, CartesianFieldExprType T>
        struct ExprTrait<Expression<D1FirstOrderBiasedDownwind<d>, T>> {
            static constexpr int dim = CartesianFieldExprTrait<T>::dim;
            static constexpr int bc_width
                    = D1FirstOrderBiasedDownwind<d>::bc_width + CartesianFieldExprTrait<T>::bc_width;
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

        template <std::size_t d, CartAMRFieldExprType T>
        struct ExprTrait<Expression<D1FirstOrderBiasedDownwind<d>, T>> : ExprTrait<T> {
            static constexpr int bc_width
                    = D1FirstOrderBiasedDownwind<d>::bc_width + CartAMRFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using mesh_type
                    = decltype(std::declval<typename CartAMRFieldExprTrait<T>::mesh_type&>().getView());
        };
    }// namespace internal
}// namespace OpFlow

#endif//OPFLOW_FIRSTORDERBIASEDDOWNWIND_HPP
