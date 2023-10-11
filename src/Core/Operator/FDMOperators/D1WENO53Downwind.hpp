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

#ifndef OPFLOW_D1WENO53DOWNWIND_HPP
#define OPFLOW_D1WENO53DOWNWIND_HPP

#include "Core/Expr/Expression.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "DataStructures/Range/Ranges.hpp"
#include "Math/Function/Numeric.hpp"

namespace OpFlow {
    template <std ::size_t d>
    struct D1WENO53Downwind {
        constexpr static auto bc_width = 3;

        template <CartesianFieldExprType E>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const E& e, auto&& i) {
            auto cond = e.accessibleRange.start[d] <= i[d] - 3 && i[d] + 3 < e.accessibleRange.end[d];
            return cond;
        }
        template <CartAMRFieldExprType E>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const E& e, auto&& i) {
            auto r = e.accessibleRanges[i.l][i.p];
            auto cond = r.start[d] <= i[d] - 3 && i[d] + 3 < r.end[d];
            return cond;
        }

        template <CartesianFieldExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto eval_safe(const E& e, I i) {
            if (e.accessibleRange.start[d] <= i[d] - 3 && i[d] + 3 < e.accessibleRange.end[d]) {
                // inner case
                auto h = e.getMesh().dx(d, i);// uniform mesh is assumed here
                auto pm3 = e.evalSafeAt(i.template prev<d>(3));
                auto pm2 = e.evalSafeAt(i.template prev<d>(2));
                auto pm1 = e.evalSafeAt(i.template prev<d>(1));
                auto p0 = e.evalSafeAt(i);
                auto pp1 = e.evalSafeAt(i.template next<d>(1));
                auto pp2 = e.evalSafeAt(i.template next<d>(2));
                return kernel(pm3, pm2, pm1, p0, pp1, pp2, h);
            }
            // for other cases WENO53 cannot handle, the user should downgrade to lower order schemes.
            OP_ERROR("Cannot eval safe at {} for {} with WENO53.", i.toString(), e.getName());
            OP_ABORT;
        }
        template <CartAMRFieldExprType E>
        OPFLOW_STRONG_INLINE static auto eval_safe(const E& e, auto&& i) {
            auto r = e.accessibleRanges[i.l][i.p];
            if (r.start[d] <= i[d] - 3 && i[d] + 3 < r.end[d]) {
                // inner case
                auto h = e.getMesh().dx(d, i.l, i[d]);// uniform mesh is assumed here
                auto pm3 = e.evalSafeAt(i.template prev<d>(3));
                auto pm2 = e.evalSafeAt(i.template prev<d>(2));
                auto pm1 = e.evalSafeAt(i.template prev<d>(1));
                auto p0 = e.evalSafeAt(i);
                auto pp1 = e.evalSafeAt(i.template next<d>(1));
                auto pp2 = e.evalSafeAt(i.template next<d>(2));
                return kernel(pm3, pm2, pm1, p0, pp1, pp2, h);
            }
            // for other cases WENO53 cannot handle, the user should downgrade to lower order schemes.
            OP_ERROR("Cannot eval safe at {} for {} with WENO53.", i.toString(), e.getName());
            OP_ABORT;
        }

        template <CartesianFieldExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto eval(const E& e, I i) {
            auto h = e.getMesh().dx(d, i);// uniform mesh is assumed here
            auto pm3 = e.evalAt(i.template prev<d>(3));
            auto pm2 = e.evalAt(i.template prev<d>(2));
            auto pm1 = e.evalAt(i.template prev<d>(1));
            auto p0 = e.evalAt(i);
            auto pp1 = e.evalAt(i.template next<d>(1));
            auto pp2 = e.evalAt(i.template next<d>(2));
            return kernel(pm3, pm2, pm1, p0, pp1, pp2, h);
        }
        template <CartAMRFieldExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto eval(const E& e, I i) {
            auto h = e.getMesh().dx(d, i.l, i[d]);// uniform mesh is assumed here
            auto pm3 = e.evalAt(i.template prev<d>(3));
            auto pm2 = e.evalAt(i.template prev<d>(2));
            auto pm1 = e.evalAt(i.template prev<d>(1));
            auto p0 = e.evalAt(i);
            auto pp1 = e.evalAt(i.template next<d>(1));
            auto pp2 = e.evalAt(i.template next<d>(2));
            return kernel(pm3, pm2, pm1, p0, pp1, pp2, h);
        }

        template <CartesianFieldExprType E>
        static void prepare(const Expression<D1WENO53Downwind, E>& expr) {
            constexpr auto dim = internal::CartesianFieldExprTrait<E>::dim;
            expr.initPropsFrom(expr.arg1);

            // name
            expr.name = fmt::format("d1<D1WENO53Downwind<{}>>({})", d, expr.arg1.name);

            // ranges
            expr.accessibleRange.start[d] += 3;
            expr.accessibleRange.end[d] -= 3;
            expr.localRange.start[d] += 3;
            expr.localRange.end[d] -= 3;
            expr.assignableRange.setEmpty();
        }
        template <CartAMRFieldExprType E>
        static void prepare(const Expression<D1WENO53Downwind, E>& expr) {
            constexpr auto dim = internal::CartesianFieldExprTrait<E>::dim;
            expr.initPropsFrom(expr.arg1);

            // name
            expr.name = fmt::format("d1<D1WENO53Downwind<{}>>({})", d, expr.arg1.name);
            // ranges
            auto levels = expr.getLevels();
            for (auto l = 0; l < levels; ++l) {
                auto parts = expr.accessibleRanges[l].size();
                for (auto p = 0; p < parts; ++p) {
                    expr.accessibleRanges[l][p].start[d] += 3;
                    expr.accessibleRanges[l][p].end[d] -= 3;
                    expr.localRanges[l][p].start[d] += 3;
                    expr.localRanges[l][p].end[d] -= 3;
                    expr.assignableRanges[l][p].setEmpty();
                }
            }
        }

    private:
        OPFLOW_STRONG_INLINE static auto kernel(auto pm3, auto pm2, auto pm1, auto p0, auto pp1, auto pp2,
                                                auto h) {
            auto d1 = (pm2 - pm3) / h, d2 = (pm1 - pm2) / h, d3 = (p0 - pm1) / h, d4 = (pp1 - p0) / h,
                 d5 = (pp2 - pp1) / h;
            auto ddx1 = d1 / 3. - 7. * d2 / 6. + 11. * d3 / 6.;
            auto ddx2 = -d2 / 6. + 5 * d3 / 6 + d4 / 3;
            auto ddx3 = d3 / 3 + 5 * d4 / 6 - d5 / 6;
            auto s1 = 13. / 12. * Math::pow2(d1 - 2 * d2 + d3) + Math::pow2(d1 - 4 * d2 + 3 * d3) / 4;
            auto s2 = 13. / 12. * Math::pow2(d2 - 2 * d3 + d4) + Math::pow2(d2 - d4) / 4.;
            auto s3 = 13. / 12. * Math::pow2(d3 - 2 * d4 + d5) + Math::pow2(3 * d3 - 4 * d4 + d5) / 4.;
            const auto eps = 1e-6 * std::max({d1 * d1, d2 * d2, d3 * d3, d4 * d4, d5 * d5}) + 1e-99;
            auto a1 = .1 / Math::pow2(s1 + eps);
            auto a2 = .6 / Math::pow2(s2 + eps);
            auto a3 = .3 / Math::pow2(s3 + eps);
            auto w1 = a1 / (a1 + a2 + a3);
            auto w2 = a2 / (a1 + a2 + a3);
            auto w3 = a3 / (a1 + a2 + a3);
            return w1 * ddx1 + w2 * ddx2 + w3 * ddx3;
        }
    };

    template <std::size_t d, CartesianFieldExprType T>
    struct ResultType<D1WENO53Downwind<d>, T> {
        using type = CartesianFieldExpr<Expression<D1WENO53Downwind<d>, T>>;
        using core_type = Expression<D1WENO53Downwind<d>, T>;
    };
    template <std::size_t d, CartAMRFieldExprType T>
    struct ResultType<D1WENO53Downwind<d>, T> {
        using type = CartAMRFieldExpr<Expression<D1WENO53Downwind<d>, T>>;
        using core_type = Expression<D1WENO53Downwind<d>, T>;
    };

    namespace internal {
        template <std::size_t d, CartesianFieldExprType T>
        struct ExprTrait<Expression<D1WENO53Downwind<d>, T>> : ExprTrait<T> {
            static constexpr int bc_width
                    = D1WENO53Downwind<d>::bc_width + CartesianFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using mesh_type
                    = decltype(std::declval<typename CartesianFieldExprTrait<T>::mesh_type&>().getView());
        };
        template <std::size_t d, CartAMRFieldExprType T>
        struct ExprTrait<Expression<D1WENO53Downwind<d>, T>> : ExprTrait<T> {
            static constexpr int bc_width
                    = D1WENO53Downwind<d>::bc_width + CartAMRFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using mesh_type
                    = decltype(std::declval<typename CartAMRFieldExprTrait<T>::mesh_type&>().getView());
        };

    }// namespace internal
}// namespace OpFlow

#endif//OPFLOW_D1WENO53DOWNWIND_HPP
