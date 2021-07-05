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

#ifndef OPFLOW_SECONDORDERCENTERED_HPP
#define OPFLOW_SECONDORDERCENTERED_HPP

#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "DataStructures/Range/Ranges.hpp"

namespace OpFlow {
    template <std::size_t d>
    struct D2SecondOrderCentered {
        constexpr static auto bc_width = 1;

        template <CartesianFieldExprType E>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const E& e, auto&& i) {
            auto cond0 = e.accessibleRange.start[d] <= i[d] - 1 && i[d] + 1 < e.accessibleRange.end[d];
            auto cond1 = e.bc[d].start && e.loc[d] == LocOnMesh::Center && i[d] == e.accessibleRange.start[d]
                         && (e.bc[d].start->getBCType() == BCType::Neum
                             || e.bc[d].start->getBCType() == BCType::Dirc);
            auto cond2 = e.bc[d].end && e.loc[d] == LocOnMesh::Center && i[d] + 1 == e.accessibleRange.end[d]
                         && (e.bc[d].end->getBCType() == BCType::Neum
                             || e.bc[d].end->getBCType() == BCType::Dirc);
            return cond0 || cond1 || cond2;
        }
        template <CartAMRFieldExprType E>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const E& e, auto&& i) {
            auto cond0 = e.accessibleRanges[i.l][i.p].start[d] <= i[d] - 1
                         && i[d] + 1 < e.accessibleRanges[i.l][i.p].end[d];
            auto cond1 = e.bc[d].start && e.loc[d] == LocOnMesh::Center
                         && i[d] == e.maxLogicalRanges[i.l].start[d]
                         && (e.bc[d].start->getBCType() == BCType::Neum
                             || e.bc[d].start->getBCType() == BCType::Dirc);
            auto cond2 = e.bc[d].end && e.loc[d] == LocOnMesh::Center
                         && i[d] + 1 == e.maxLogicalRanges[i.l].end[d]
                         && (e.bc[d].end->getBCType() == BCType::Neum
                             || e.bc[d].end->getBCType() == BCType::Dirc);
            return cond0 || cond1 || cond2;
        }

        template <CartesianFieldExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto eval_safe(const E& e, I i) {
            if (e.accessibleRange.start[d] <= i[d] - 1 && i[d] + 1 < e.accessibleRange.end[d]) {
                auto _l = e.evalSafeAt(i.template prev<d>());
                auto _c = e.evalSafeAt(i);
                auto _r = e.evalSafeAt(i.template next<d>());
                auto _dx_l = e.loc[d] == LocOnMesh::Corner
                                     ? e.mesh.dx(d, i[d] - 1)
                                     : (e.mesh.dx(d, i[d] - 1) + e.mesh.dx(d, i[d])) * 0.5;
                auto _dx_r = e.loc[d] == LocOnMesh::Corner
                                     ? e.mesh.dx(d, i[d])
                                     : (e.mesh.dx(d, i[d]) + e.mesh.dx(d, i[d] + 1)) * 0.5;
                auto _dx_c = (_dx_l + _dx_r) * 0.5;
                return ((_r - _c) / _dx_r - (_c - _l) / _dx_l) / _dx_c;
            }
            // left bc case
            if (e.bc[d].start && e.loc[d] == LocOnMesh::Center && i[d] == e.accessibleRange.start[d]) {
                if (e.bc[d].start->getBCType() == BCType::Neum) {
                    auto _dx_r = (e.mesh.dx(d, i[d]) + e.mesh.dx(d, i[d] + 1)) * 0.5;
                    auto _r = e.evalSafeAt(i.template next<d>());
                    auto _c = e.evalSafeAt(i);
                    return ((_r - _c) / _dx_r - e.bc[d].start->evalAt(i)) / (e.mesh.dx(d, i[d]) + _dx_r)
                           * 2.0;
                } else if (e.bc[d].start->getBCType() == BCType::Dirc) {
                    auto _dx_r = (e.mesh.dx(d, i[d]) + e.mesh.dx(d, i[d] + 1)) * 0.5;
                    auto _r = e.evalSafeAt(i.template next<d>());
                    auto _c = e.evalSafeAt(i);
                    auto _l = e.bc[d].start->evalAt(i);
                    auto _dx_l = e.mesh.dx(d, i[d]) * 0.5;
                    return ((_r - _c) / _dx_r - (_c - _l) / _dx_l) / (_dx_r + _dx_l) * 2.0;
                }
            }
            // right bc case
            if (e.bc[d].end && e.loc[d] == LocOnMesh::Center && i[d] + 1 == e.accessibleRange.end[d]) {
                if (e.bc[d].end->getBCType() == BCType::Neum) {
                    auto _dx_l = (e.mesh.dx(d, i[d] - 1) + e.mesh.dx(d, i[d])) * 0.5;
                    auto _l = e.evalSafeAt(i.template prev<d>());
                    auto _c = e.evalSafeAt(i);
                    return (e.bc[d].end->evalAt(i) - (_c - _l) / _dx_l) / (e.mesh.dx(d, i[d]) + _dx_l) * 2.0;
                } else if (e.bc[d].end->getBCType() == BCType::Dirc) {
                    auto _dx_l = (e.mesh.dx(d, i[d] - 1) + e.mesh.dx(d, i[d])) * 0.5;
                    auto _l = e.evalSafeAt(i.template prev<d>());
                    auto _c = e.evalSafeAt(i);
                    auto _r = e.bc[d].end->evalAt(i);
                    auto _dx_r = e.mesh.dx(d, i[d]) * 0.5;
                    return ((_r - _c) / _dx_r - (_c - _l) / _dx_l) / (_dx_r + _dx_l) * 2.0;
                }
            }
            // not handled case
            OP_ERROR("Cannot handle current case.");
            //OP_ERROR("Expr and index are: \n{}\nIndex = {}", e.toString(), i.toString());
            OP_ABORT;
        }
        template <CartAMRFieldExprType E>
        OPFLOW_STRONG_INLINE static auto eval_safe(const E& e, auto&& i) {
            if (e.accessibleRanges[i.l][i.p].start[d] <= i[d] - 1
                && i[d] + 1 < e.accessibleRanges[i.l][i.p].end[d]) {
                auto _l = e.evalSafeAt(i.template prev<d>());
                auto _c = e.evalSafeAt(i);
                auto _r = e.evalSafeAt(i.template next<d>());
                auto _dx_l = e.loc[d] == LocOnMesh::Corner
                                     ? e.mesh.dx(d, i.l, i[d] - 1)
                                     : (e.mesh.dx(d, i.l, i[d] - 1) + e.mesh.dx(d, i.l, i[d])) * 0.5;
                auto _dx_r = e.loc[d] == LocOnMesh::Corner
                                     ? e.mesh.dx(d, i.l, i[d])
                                     : (e.mesh.dx(d, i.l, i[d]) + e.mesh.dx(d, i.l, i[d] + 1)) * 0.5;
                auto _dx_c = (_dx_l + _dx_r) * 0.5;
                return ((_r - _c) / _dx_r - (_c - _l) / _dx_l) / _dx_c;
            }
            // left bc case
            if (e.bc[d].start && e.loc[d] == LocOnMesh::Center && i[d] == e.maxLogicalRanges[i.l].start[d]) {
                if (e.bc[d].start->getBCType() == BCType::Neum) {
                    auto _dx_r = (e.mesh.dx(d, i.l, i[d]) + e.mesh.dx(d, i.l, i[d] + 1)) * 0.5;
                    auto _r = e.evalSafeAt(i.template next<d>());
                    auto _c = e.evalSafeAt(i);
                    return ((_r - _c) / _dx_r - e.bc[d].start->evalAt(i)) / (e.mesh.dx(d, i.l, i[d]) + _dx_r)
                           * 2.0;
                } else if (e.bc[d].start->getBCType() == BCType::Dirc) {
                    auto _dx_r = (e.mesh.dx(d, i.l, i[d]) + e.mesh.dx(d, i.l, i[d] + 1)) * 0.5;
                    auto _r = e.evalSafeAt(i.template next<d>());
                    auto _c = e.evalSafeAt(i);
                    auto _l = e.bc[d].start->evalAt(i);
                    auto _dx_l = e.mesh.dx(d, i.l, i[d]) * 0.5;
                    return ((_r - _c) / _dx_r - (_c - _l) / _dx_l) / (_dx_r + _dx_l) * 2.0;
                }
            }
            // right bc case
            if (e.bc[d].end && e.loc[d] == LocOnMesh::Center && i[d] + 1 == e.maxLogicalRanges[i.l].end[d]) {
                if (e.bc[d].end->getBCType() == BCType::Neum) {
                    auto _dx_l = (e.mesh.dx(d, i.l, i[d] - 1) + e.mesh.dx(d, i.l, i[d])) * 0.5;
                    auto _l = e.evalAt(i.template prev<d>());
                    auto _c = e.evalAt(i);
                    return (e.bc[d].end->evalAt(i) - (_c - _l) / _dx_l) / (e.mesh.dx(d, i.l, i[d]) + _dx_l)
                           * 2.0;
                } else if (e.bc[d].end->getBCType() == BCType::Dirc) {
                    auto _dx_l = (e.mesh.dx(d, i.l, i[d] - 1) + e.mesh.dx(d, i.l, i[d])) * 0.5;
                    auto _l = e.evalAt(i.template prev<d>());
                    auto _c = e.evalAt(i);
                    auto _r = e.bc[d].end->evalAt(i);
                    auto _dx_r = e.mesh.dx(d, i.l, i[d]) * 0.5;
                    return ((_r - _c) / _dx_r - (_c - _l) / _dx_l) / (_dx_r + _dx_l) * 2.0;
                }
            }
            // not handled case
            OP_ERROR("Cannot handle current case.");
            //OP_ERROR("Expr and index are: \n{}\nIndex = {}", e.toString(), i.toString());
            OP_ABORT;
        }
        template <CartesianFieldExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto eval(const E& e, I i) {
            auto _l = e.evalAt(i.template prev<d>());
            auto _c = e.evalAt(i);
            auto _r = e.evalAt(i.template next<d>());
            auto _dx_l = e.loc[d] == LocOnMesh::Corner ? e.mesh.dx(d, i[d] - 1)
                                                       : (e.mesh.dx(d, i[d] - 1) + e.mesh.dx(d, i[d])) * 0.5;
            auto _dx_r = e.loc[d] == LocOnMesh::Corner ? e.mesh.dx(d, i[d])
                                                       : (e.mesh.dx(d, i[d]) + e.mesh.dx(d, i[d] + 1)) * 0.5;
            auto _dx_c = (_dx_l + _dx_r) * 0.5;
            return ((_r - _c) / _dx_r - (_c - _l) / _dx_l) / _dx_c;
        }
        template <CartAMRFieldExprType E>
        OPFLOW_STRONG_INLINE static auto eval(const E& e, auto&& i) {
            auto _l = e.evalAt(i.template prev<d>());
            auto _c = e.evalAt(i);
            auto _r = e.evalAt(i.template next<d>());
            auto _dx_l = e.loc[d] == LocOnMesh::Corner
                                 ? e.mesh.dx(d, i.l, i[d] - 1)
                                 : (e.mesh.dx(d, i.l, i[d] - 1) + e.mesh.dx(d, i.l, i[d])) * 0.5;
            auto _dx_r = e.loc[d] == LocOnMesh::Corner
                                 ? e.mesh.dx(d, i.l, i[d])
                                 : (e.mesh.dx(d, i.l, i[d]) + e.mesh.dx(d, i.l, i[d] + 1)) * 0.5;
            auto _dx_c = (_dx_l + _dx_r) * 0.5;
            return ((_r - _c) / _dx_r - (_c - _l) / _dx_l) / _dx_c;
        }
        template <CartesianFieldExprType E>
        OPFLOW_STRONG_INLINE static void prepare(Expression<D2SecondOrderCentered, E>& expr) {
            constexpr auto dim = internal::CartesianFieldExprTrait<E>::dim;

            // name
            expr.name = fmt::format("d2<D2SecondOrderCentered<{}>>({})", d, expr.arg1.name);

            // mesh
            expr.mesh = expr.arg1.mesh.getView();
            expr.loc = expr.arg1.loc;

            // bc
            expr.bc[d].start = nullptr;
            expr.bc[d].end = nullptr;
            for (auto i = 0; i < dim; ++i) {
                if (i != d) {
                    expr.bc[i].start = expr.arg1.bc[i].start ? expr.arg1.bc[i].start->getCopy() : nullptr;
                    expr.bc[i].end = expr.arg1.bc[i].end ? expr.arg1.bc[i].end->getCopy() : nullptr;
                    OP_WARN("BC for result expr not calculated.");
                }
            }

            // ranges
            expr.accessibleRange = expr.arg1.accessibleRange;
            if (expr.arg1.loc[d] == LocOnMesh::Corner) {
                // nodal case
                if (!expr.arg1.bc[d].start || expr.arg1.bc[d].start->getBCType() == BCType::Dirc)
                    expr.accessibleRange.start[d]++;
                if (!expr.arg1.bc[d].end || expr.arg1.bc[d].end->getBCType() == BCType::Dirc)
                    expr.accessibleRange.end[d]--;
            } else {
                // center case
                if (!expr.arg1.bc[d].start) { expr.accessibleRange.start[d]++; }
                if (!expr.arg1.bc[d].end) expr.accessibleRange.end[d]--;
            }
            expr.localRange = expr.accessibleRange;
            expr.assignableRange.setEmpty();
        }

        template <CartAMRFieldExprType E>
        OPFLOW_STRONG_INLINE static void prepare(Expression<D2SecondOrderCentered, E>& expr) {
            constexpr auto dim = internal::CartAMRFieldExprTrait<E>::dim;
            expr.initPropsFrom(expr.arg1);

            // name
            expr.name = fmt::format("d2<D2SecondOrderCentered<{}>>({})", d, expr.arg1.name);

            // ranges
            if (expr.arg1.loc[d] == LocOnMesh::Corner) {
                // nodal case
                if (!expr.arg1.bc[d].start || expr.arg1.bc[d].start->getBCType() == BCType::Dirc) {
                    // todo: the finer patches attached to the boundary should also be modified
                    expr.accessibleRanges[0][0].start[d]++;
                    expr.localRanges[0][0].start[d]++;
                }
                if (!expr.arg1.bc[d].end || expr.arg1.bc[d].end->getBCType() == BCType::Dirc) {
                    expr.accessibleRanges[0][0].end[d]--;
                    expr.localRanges[0][0].end[d]--;
                }
            } else {
                // center case
                if (!expr.arg1.bc[d].start) {
                    expr.accessibleRanges[0][0].start[d]++;
                    expr.localRanges[0][0].start[d]++;
                }
                if (!expr.arg1.bc[d].end) {
                    expr.accessibleRanges[0][0].end[d]--;
                    expr.localRanges[0][0].end[d]--;
                }
            }
            for (auto& l : expr.assignableRanges)
                for (auto& p : l) p.setEmpty();
        }
    };

    template <std::size_t d, CartesianFieldExprType T>
    struct ResultType<D2SecondOrderCentered<d>, T> {
        using type = CartesianFieldExpr<Expression<D2SecondOrderCentered<d>, T>>;
        using core_type = Expression<D2SecondOrderCentered<d>, T>;
    };

    template <std::size_t d, CartAMRFieldExprType T>
    struct ResultType<D2SecondOrderCentered<d>, T> {
        using type = CartAMRFieldExpr<Expression<D2SecondOrderCentered<d>, T>>;
        using core_type = Expression<D2SecondOrderCentered<d>, T>;
    };

    namespace internal {
        template <std::size_t d, CartesianFieldExprType T>
        struct ExprTrait<Expression<D2SecondOrderCentered<d>, T>> : ExprTrait<T> {
            static constexpr int access_flag = 0;
            static constexpr int bc_width
                    = D2SecondOrderCentered<d>::bc_width + CartesianFieldExprTrait<T>::bc_width;
            using mesh_type
                    = decltype(std::declval<typename CartesianFieldExprTrait<T>::mesh_type&>().getView());
        };

        template <std::size_t d, CartAMRFieldExprType T>
        struct ExprTrait<Expression<D2SecondOrderCentered<d>, T>> : ExprTrait<T> {
            static constexpr int access_flag = 0;
            static constexpr int bc_width
                    = D2SecondOrderCentered<d>::bc_width + CartAMRFieldExprTrait<T>::bc_width;
            using mesh_type
                    = decltype(std::declval<typename CartAMRFieldExprTrait<T>::mesh_type&>().getView());
        };
    }// namespace internal

}// namespace OpFlow
#endif//OPFLOW_SECONDORDERCENTERED_HPP
