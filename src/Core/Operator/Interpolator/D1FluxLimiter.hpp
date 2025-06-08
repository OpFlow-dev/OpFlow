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

#ifndef OPFLOW_D1FLUXLIMITER_HPP
#define OPFLOW_D1FLUXLIMITER_HPP

#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Operator/Conditional.hpp"
#include "DataStructures/StencilPad.hpp"
#include "IntpInterface.hpp"
#include "Math/Interpolator/Interpolator.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <typename K>
    concept FluxLimiterKernel = requires(double r) {
        { K::eval(r) } -> std::same_as<double>;
    };

    template <typename K>
    concept LinearFluxLimiterKernel
            = FluxLimiterKernel<K> && requires(DS::StencilPad<DS::MDIndex<2>> pad, double r) {
                  // should take slop_u & slop_f in a type only support + and * to a number
                  { K::eval(pad, pad) } -> std::same_as<DS::StencilPad<DS::MDIndex<2>>>;
                  // should take normal separate slops instead of ratio
                  { K::eval(r, r) } -> std::same_as<double>;
              };

    namespace internal {
        template <FluxLimiterKernel Kernel, std::size_t d, IntpDirection dir>
        struct D1FluxLimiterUpwindImpl;

        template <FluxLimiterKernel Kernel, std::size_t d>
        struct D1FluxLimiterUpwindImpl<Kernel, d, IntpDirection::Cen2Cor> {
            template <CartesianFieldExprType T>
            OPFLOW_STRONG_INLINE static auto eval(const T& e, auto&& i) {
                auto x1 = e.mesh.x(d, i[d] - 2) + 0.5 * e.mesh.dx(d, i[d] - 2);
                auto x2 = e.mesh.x(d, i[d] - 1) + 0.5 * e.mesh.dx(d, i[d] - 1);
                auto x3 = e.mesh.x(d, i[d]) + 0.5 * e.mesh.dx(d, i[d]);
                auto y1 = e.evalAt(i.template prev<d>().template prev<d>());
                auto y2 = e.evalAt(i.template prev<d>());
                auto y3 = e.evalAt(i);
                auto slop_u = (y2 - y1) / (x2 - x1);
                auto slop_f = (y3 - y2) / (x3 - x2);
                // use the linear form as much as possible
                if constexpr (LinearFluxLimiterKernel<Kernel>)
                    return y2 + e.mesh.dx(d, i[d] - 1) * 0.5 * Kernel::eval(slop_u, slop_f);
                else {
                    auto r = slop_f / (slop_u + 1e-16);
                    return y2 + e.mesh.dx(d, i[d] - 1) * 0.5 * Kernel::eval(r) * slop_u;
                }
            }
        };

        template <FluxLimiterKernel Kernel, std::size_t d>
        struct D1FluxLimiterUpwindImpl<Kernel, d, IntpDirection::Cor2Cen> {
            template <CartesianFieldExprType T>
            OPFLOW_STRONG_INLINE static auto eval(const T& e, auto&& i) {
                auto x1 = e.mesh.x(d, i[d] - 1);
                auto x2 = e.mesh.x(d, i[d]);
                auto x3 = e.mesh.x(d, i[d] + 1);
                auto y1 = e.evalAt(i.template prev<d>());
                auto y2 = e.evalAt(i);
                auto y3 = e.evalAt(i.template next<d>());
                auto slop_u = (y2 - y1) / (x2 - x1);
                auto slop_f = (y3 - y2) / (x3 - x2);
                if constexpr (LinearFluxLimiterKernel<Kernel>)
                    return y2 + e.mesh.dx(d, i[d]) * 0.5 * Kernel::eval(slop_u, slop_f);
                else {
                    auto r = slop_f / (slop_u + 1e-16);
                    return y2 + e.mesh.dx(d, i[d]) * 0.5 * Kernel::eval(r) * slop_u;
                }
            }
        };

        template <FluxLimiterKernel Kernel, std::size_t d, IntpDirection dir>
        struct D1FluxLimiterDownwindImpl;

        template <FluxLimiterKernel Kernel, std::size_t d>
        struct D1FluxLimiterDownwindImpl<Kernel, d, IntpDirection::Cen2Cor> {
            template <CartesianFieldExprType T>
            OPFLOW_STRONG_INLINE static auto eval(const T& e, auto&& i) {
                auto x1 = e.mesh.x(d, i[d] - 1) + e.mesh.dx(d, i[d] - 1) * 0.5;
                auto x2 = e.mesh.x(d, i[d]) + e.mesh.dx(d, i[d]) * 0.5;
                auto x3 = e.mesh.x(d, i[d] + 1) + e.mesh.dx(d, i[d] + 1) * 0.5;
                auto y1 = e.evalAt(i.template prev<d>());
                auto y2 = e.evalAt(i);
                auto y3 = e.evalAt(i.template next<d>());
                auto slop_u = (y3 - y2) / (x3 - x2);
                auto slop_f = (y2 - y1) / (x2 - x1);
                if constexpr (LinearFluxLimiterKernel<Kernel>)
                    return y2 - e.mesh.dx(d, i[d]) * 0.5 * Kernel::eval(slop_u, slop_f);
                else {
                    auto r = slop_f / (slop_u + 1e-16);
                    return y2 - e.mesh.dx(d, i[d]) * 0.5 * Kernel::eval(r) * slop_u;
                }
            }
        };

        template <FluxLimiterKernel Kernel, std::size_t d>
        struct D1FluxLimiterDownwindImpl<Kernel, d, IntpDirection::Cor2Cen> {
            template <CartesianFieldExprType T>
            OPFLOW_STRONG_INLINE static auto eval(const T& e, auto&& i) {
                auto x1 = e.mesh.x(d, i[d]);
                auto x2 = e.mesh.x(d, i[d] + 1);
                auto x3 = e.mesh.x(d, i[d] + 2);
                auto y1 = e.evalAt(i);
                auto y2 = e.evalAt(i.template next<d>());
                auto y3 = e.evalAt(i.template next<d>().template next<d>());
                auto slop_u = (y3 - y2) / (x3 - x2);
                auto slop_f = (y2 - y1) / (x2 - x1);
                if constexpr (LinearFluxLimiterKernel<Kernel>)
                    return y2 - e.mesh.dx(d, i[d]) * 0.5 * Kernel::eval(slop_u, slop_f);
                else {
                    auto r = slop_f / (slop_u + 1e-16);
                    return y2 - e.mesh.dx(d, i[d]) * 0.5 * Kernel::eval(r) * slop_u;
                }
            }
        };

        template <FluxLimiterKernel Kernel, std::size_t d, IntpDirection dir>
        struct D1FluxLimiterImpl;

        template <FluxLimiterKernel Kernel, std::size_t d>
        struct D1FluxLimiterImpl<Kernel, d, IntpDirection::Cen2Cor> {
            constexpr static int bc_width = 2;

            template <CartesianFieldExprType T, ExprType U>
            OPFLOW_STRONG_INLINE static auto couldSafeEval(const U& u, const T& e, auto&& i) {
                return e.couldEvalAt(i.template prev<d>().template prev<d>())
                       && e.couldEvalAt(i.template next<d>()) && u.couldEvalAt(i);
            }

            template <CartesianFieldExprType T, ExprType U>
            OPFLOW_STRONG_INLINE static auto eval(const U& u, const T& e, auto&& i) {
                return u[i] > 0. ? D1FluxLimiterUpwindImpl<Kernel, d, IntpDirection::Cen2Cor>::eval(e, i)
                                 : D1FluxLimiterDownwindImpl<Kernel, d, IntpDirection::Cen2Cor>::eval(e, i);
            }

            template <CartesianFieldExprType T, ExprType U>
            static void prepare(const Expression<D1FluxLimiterImpl, U, T>& expr) {
                OP_ASSERT_MSG(expr.arg2.loc[d] == LocOnMesh::Center,
                              "D1FluxLimiterIntp error: Expression {} located in corner in dimension = {}",
                              expr.arg2.getName(), d);
                expr.initPropsFrom(expr.arg2);
                expr.name = std::format("D1Intp<D1FluxLimiter, {}, Cen2Cor>({})", d, expr.arg2.name);
                expr.loc[d] = LocOnMesh ::Corner;
                expr.accessibleRange.start[d] += 2;
                expr.accessibleRange.end[d] -= 1;
                expr.localRange.start[d] += 2;
                expr.localRange.end[d] -= 1;
                expr.logicalRange.start[d] += 2;
                expr.logicalRange.end[d] -= 1;
                expr.assignableRange.setEmpty();
            }
        };

        template <FluxLimiterKernel Kernel, std::size_t d>
        struct D1FluxLimiterImpl<Kernel, d, IntpDirection::Cor2Cen> {
            constexpr static int bc_width = 2;

            template <CartesianFieldExprType T, ExprType U>
            OPFLOW_STRONG_INLINE static auto couldSafeEval(const U& u, const T& e, auto&& i) {
                return e.couldEvalAt(i.template prev<d>())
                       && e.couldEvalAt(i.template next<d>().template next<d>()) && u.couldEvalAt(i);
            }

            template <CartesianFieldExprType T, ExprType U>
            OPFLOW_STRONG_INLINE static auto eval(const U& u, const T& e, auto&& i) {
                return u[i] > 0. ? D1FluxLimiterUpwindImpl<Kernel, d, IntpDirection::Cor2Cen>::eval(e, i)
                                 : D1FluxLimiterDownwindImpl<Kernel, d, IntpDirection::Cor2Cen>::eval(e, i);
            }

            template <CartesianFieldExprType T, ExprType U>
            static void prepare(const Expression<D1FluxLimiterImpl, U, T>& expr) {
                OP_ASSERT_MSG(expr.arg2.loc[d] == LocOnMesh::Corner,
                              "D1FluxLimiterIntp error: Expression {} located in center in dimension = {}",
                              expr.arg2.getName(), d);
                expr.initPropsFrom(expr.arg2);
                expr.name = std::format("D1Intp<D1FluxLimiter, {}, Cor2Cen>({})", d, expr.arg2.name);
                expr.loc[d] = LocOnMesh ::Center;
                expr.accessibleRange.start[d] += 1;
                expr.accessibleRange.end[d] -= 2;
                expr.localRange.start[d] += 1;
                expr.localRange.end[d] -= 2;
                expr.logicalRange.start[d] += 1;
                expr.logicalRange.end[d] -= 2;
                expr.assignableRange.setEmpty();
            }
        };
    }// namespace internal

    template <FluxLimiterKernel Kernel>
    struct D1FluxLimiterGen {
        template <std::size_t d, IntpDirection dir>
        using Op = internal::D1FluxLimiterImpl<Kernel, d, dir>;
    };

    template <std::size_t d, IntpDirection dir, CartesianFieldExprType T, ExprType U, FluxLimiterKernel K>
    struct ResultType<internal::D1FluxLimiterImpl<K, d, dir>, U, T> {
        using type = typename internal::CartesianFieldExprTrait<T>::template twin_type<
                Expression<internal::D1FluxLimiterImpl<K, d, dir>, U, T>>;
    };

    namespace internal {
        template <std::size_t d, IntpDirection dir, CartesianFieldExprType T, ExprType U, FluxLimiterKernel K>
        struct ExprTrait<Expression<D1FluxLimiterImpl<K, d, dir>, U, T>> : ExprTrait<T> {
            static constexpr int bc_width
                    = D1FluxLimiterImpl<K, d, dir>::bc_width + CartesianFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using mesh_type
                    = decltype(std::declval<typename CartesianFieldExprTrait<T>::mesh_type&>().getView());
        };
    }// namespace internal
}// namespace OpFlow

#endif//OPFLOW_D1FLUXLIMITER_HPP
