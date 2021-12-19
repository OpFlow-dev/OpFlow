//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021  by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_D1QUICKINTPCENTERTOCORNER_HPP
#define OPFLOW_D1QUICKINTPCENTERTOCORNER_HPP

#include "Core/Field/MeshBased/Structured/CartesianFieldTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Operator/Conditional.hpp"
#include "Math/Interpolator/Interpolator.hpp"

namespace OpFlow {
    template <std::size_t d, IntpDirection dir>
    struct D1QUICKUpwind;

    template <std::size_t d>
    struct D1QUICKUpwind<d, IntpDirection::Cen2Cor> {
        constexpr static auto bc_width = 2;

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const T& e, auto&& i) {
            return e.couldEvalAt(i.template prev<d>()) && e.couldEvalAt(i.template next<d>());
        }

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto eval(const T& e, auto&& i) {
            auto x1 = e.mesh.x(d, i[d] - 1) + e.mesh.dx(d, i[d] - 1) * 0.5;
            auto x2 = e.mesh.x(d, i[d]) + e.mesh.dx(d, i[d]) * 0.5;
            auto x3 = e.mesh.x(d, i[d] + 1) + e.mesh.dx(d, i[d] + 1) * 0.5;
            auto y1 = e.evalAt(i.template prev<d>());
            auto y2 = e.evalAt(i);
            auto y3 = e.evalAt(i.template next<d>());
            return Math::Interpolator1D::intp(x1, y1, x2, y2, x3, y3, e.mesh.x(d, i));
        }

        template <CartesianFieldExprType T>
        static void prepare(Expression<D1QUICKUpwind, T>& expr) {
            expr.initPropsFrom(expr.arg1);
            expr.name = fmt::format("D1Intp<D1QUICK, {}, Cen2Cor>({})", d, expr.arg1.name);
            expr.loc[d] = LocOnMesh::Corner;
            expr.accessibleRange.start[d]++;
            expr.accessibleRange.end[d] -= 2;
            expr.localRange.start[d]++;
            expr.localRange.end[d] -= 2;
            expr.logicalRange.start[d]++;
            expr.logicalRange.end[d] -= 2;
            expr.assignableRange.setEmpty();
        }
    };

    template <std::size_t d, CartesianFieldExprType T>
    struct ResultType<D1QUICKUpwind<d, IntpDirection::Cen2Cor>, T> {
        using type = typename internal::CartesianFieldExprTrait<T>::template twin_type<
                Expression<D1QUICKUpwind<d, IntpDirection::Cen2Cor>, T>>;
    };

    namespace internal {
        template <std::size_t d, CartesianFieldExprType T>
        struct ExprTrait<Expression<D1QUICKUpwind<d, IntpDirection::Cen2Cor>, T>> : ExprTrait<T> {
            static constexpr int bc_width = D1QUICKUpwind<d, IntpDirection::Cen2Cor>::bc_width
                                            + CartesianFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using mesh_type
                    = decltype(std::declval<typename CartesianFieldExprTrait<T>::mesh_type&>().getView());
        };
    }// namespace internal

    template <std::size_t d, IntpDirection dir>
    struct D1QUICKDownwind;

    template <std::size_t d>
    struct D1QUICKDownwind<d, IntpDirection::Cen2Cor> {
        constexpr static auto bc_width = 2;

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const T& e, auto&& i) {
            return e.couldEvalAt(i.template prev<d>().template prev<d>()) && e.couldEvalAt(i);
        }

        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto eval(const T& e, auto&& i) {
            auto x1 = e.mesh.x(d, i[d] - 2) + 0.5 * e.mesh.dx(d, i[d] - 2);
            auto x2 = e.mesh.x(d, i[d] - 1) + 0.5 * e.mesh.dx(d, i[d] - 1);
            auto x3 = e.mesh.x(d, i[d]) + 0.5 * e.mesh.dx(d, i[d]);
            auto y1 = e.evalAt(i.template prev<d>().template prev<d>());
            auto y2 = e.evalAt(i.template prev<d>());
            auto y3 = e.evalAt(i);
            return Math::Interpolator1D::intp(x1, y1, x2, y2, x3, y3, e.mesh.x(d, i));
        }

        template <CartesianFieldExprType T>
        static void prepare(Expression<D1QUICKDownwind, T>& expr) {
            expr.initPropsFrom(expr.arg1);
            expr.name = fmt::format("D1Intp<D1QUICK, {}, Cen2Cor>({})", d, expr.arg1.name);
            expr.loc[d] = LocOnMesh::Corner;
            expr.accessibleRange.start[d] += 2;
            expr.accessibleRange.end[d] -= 1;
            expr.localRange.start[d] += 2;
            expr.localRange.end[d] -= 1;
            expr.logicalRange.start[d] += 2;
            expr.logicalRange.end[d] -= 1;
            expr.assignableRange.setEmpty();
        }
    };

    template <std::size_t d, CartesianFieldExprType T>
    struct ResultType<D1QUICKDownwind<d, IntpDirection::Cen2Cor>, T> {
        using type = typename internal::CartesianFieldExprTrait<T>::template twin_type<
                Expression<D1QUICKDownwind<d, IntpDirection::Cen2Cor>, T>>;
    };

    namespace internal {
        template <std::size_t d, CartesianFieldExprType T>
        struct ExprTrait<Expression<D1QUICKDownwind<d, IntpDirection::Cen2Cor>, T>> : ExprTrait<T> {
            static constexpr int bc_width = D1QUICKDownwind<d, IntpDirection::Cen2Cor>::bc_width
                                            + CartesianFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using mesh_type
                    = decltype(std::declval<typename CartesianFieldExprTrait<T>::mesh_type&>().getView());
        };
    }// namespace internal

    template <std::size_t d, IntpDirection dir>
    struct D1QUICK;

    template <std::size_t d>
    struct D1QUICK<d, IntpDirection::Cen2Cor> {
        constexpr static int bc_width = 2;

        template <CartesianFieldExprType T, ExprType U>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const U& u, const T& e, auto&& i) {
            return D1QUICKUpwind<d, IntpDirection::Cen2Cor>::couldSafeEval(e, i)
                   && D1QUICKDownwind<d, IntpDirection::Cen2Cor>::couldSafeEval(e, i) && u.couldEvalAt(i);
        }

        template <CartesianFieldExprType T, ExprType U>
        OPFLOW_STRONG_INLINE static auto eval(const U& u, const T& e, auto&& i) {
            return u.evalAt(i) > 0. ? D1QUICKDownwind<d, IntpDirection::Cen2Cor>::eval(e, i)
                                    : D1QUICKUpwind<d, IntpDirection::Cen2Cor>::eval(e, i);
        }

        template <CartesianFieldExprType T, ExprType U>
        static void prepare(Expression<D1QUICK, U, T>& expr) {
            expr.initPropsFrom(expr.arg2);
            expr.name = fmt::format("D1Intp<D1QUICK, {}, Cen2Cor>({})", d, expr.arg2.name);
            expr.loc[d] = LocOnMesh::Corner;
            expr.accessibleRange.start[d] += 2;
            expr.accessibleRange.end[d] -= 2;
            expr.localRange.start[d] += 2;
            expr.localRange.end[d] -= 2;
            expr.logicalRange.start[d] += 2;
            expr.logicalRange.end[d] -= 2;
            expr.assignableRange.setEmpty();
        }
    };

    template <std::size_t d, CartesianFieldExprType T, ExprType U>
    struct ResultType<D1QUICK<d, IntpDirection::Cen2Cor>, U, T> {
        using type = typename internal::CartesianFieldExprTrait<T>::template twin_type<
                Expression<D1QUICK<d, IntpDirection::Cen2Cor>, U, T>>;
    };

    namespace internal {
        template <std::size_t d, CartesianFieldExprType T, ExprType U>
        struct ExprTrait<Expression<D1QUICK<d, IntpDirection::Cen2Cor>, U, T>> : ExprTrait<T> {
            static constexpr int bc_width
                    = D1QUICK<d, IntpDirection::Cen2Cor>::bc_width + CartesianFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using mesh_type
                    = decltype(std::declval<typename CartesianFieldExprTrait<T>::mesh_type&>().getView());
        };
    }// namespace internal

}// namespace OpFlow

#endif//OPFLOW_D1QUICKINTPCENTERTOCORNER_HPP
