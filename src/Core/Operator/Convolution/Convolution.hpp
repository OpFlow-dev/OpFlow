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

#ifndef OPFLOW_CONVOLUTION_HPP
#define OPFLOW_CONVOLUTION_HPP

#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Arrays/Tensor/FixedSizeTensor.hpp"

namespace OpFlow {
    template <std::integral auto... ns>
    struct Convolution {
        constexpr static int d = sizeof...(ns);
        constexpr static auto bc_width = std::max({ns...}) / 2;
        constexpr static std::array dims {ns...};

        template <StructuredFieldExprType E, typename T, typename I>
        OPFLOW_STRONG_INLINE static auto
        couldSafeEval(const E& e, const ScalarExpr<DS::FixedSizeTensor<T, ns...>>& kernel, I&& i) {
            auto ret = true;
            for (auto j = 0; j < d; ++j) {
                ret &= (i[j] - dims[j] / 2 >= e.arg1.accessibleRange.start[j])
                       && (i[j] + dims[j] / 2 < e.arg1.accessibleRange.end[j]);
            }
            return ret;
        }

        template <CartAMRFieldExprType E, typename T, typename I>
        OPFLOW_STRONG_INLINE static auto
        couldSafeEval(const E& e, const ScalarExpr<DS::FixedSizeTensor<T, ns...>>& kernel, I&& i) {
            auto ret = true;
            for (auto j = 0; j < d; ++j) {
                ret &= (i[j] - dims[j] / 2 >= e.arg1.accessibleRanges[i.l][i.p].start[j])
                       && (i[j] + dims[j] / 2 < e.arg1.accessibleRanges[i.l][i.p].end[j]);
            }
            return ret;
        }

        template <StructuredFieldExprType E, typename T, typename I>
        requires(internal::ExprTrait<E>::dim == d) OPFLOW_STRONG_INLINE
                static auto eval(const E& e, const ScalarExpr<DS::FixedSizeTensor<T, ns...>>& kernel, I&& i) {
            DS::Range<d> range;
            for (auto j = 0; j < d; ++j) {
                range.start[j] = i[j] - dims[j] / 2;
                range.end[j] = i[j] + dims[j] / 2 + 1;
                range.stride[j] = 1;
            }
            return rangeReduce_s(
                    range, [](auto&& a, auto&& b) { return a + b; },
                    [&](auto&& idx) {
                        auto _ker_idx = idx;
                        for (auto j = 0; j < d; ++j) { _ker_idx[j] = _ker_idx[j] - i[j] + dims[j] / 2; }
                        return kernel.val[_ker_idx] * e.evalAt(idx);
                    });
        }

        template <CartAMRFieldExprType E, typename T, typename I>
        requires(internal::ExprTrait<E>::dim == d) OPFLOW_STRONG_INLINE
                static auto eval(const E& e, const ScalarExpr<DS::FixedSizeTensor<T, ns...>>& kernel, I&& i) {
            DS::LevelRange<d> range;
            for (auto j = 0; j < d; ++j) {
                range.start[j] = i[j] - dims[j] / 2;
                range.end[j] = i[j] + dims[j] / 2 + 1;
                range.stride[j] = 1;
            }
            range.level = i.l;
            range.part = i.p;
            return rangeReduce_s(
                    range, [](auto&& a, auto&& b) { return a + b; },
                    [&](auto&& idx) {
                        auto _ker_idx = (DS::MDIndex<d>) idx;
                        for (auto j = 0; j < d; ++j) { _ker_idx[j] = _ker_idx[j] - i[j] + dims[j] / 2; }
                        return kernel.val[_ker_idx] * e.evalAt(idx);
                    });
        }

        template <StructuredFieldExprType E, typename T, typename I>
        requires(internal::ExprTrait<E>::dim == d) OPFLOW_STRONG_INLINE
                static auto eval_safe(const E& e, const ScalarExpr<DS::FixedSizeTensor<T, ns...>>& kernel,
                                      I&& i) {
            DS::Range<d> range;
            for (auto j = 0; j < d; ++j) {
                range.start[j] = i[j] - dims[j] / 2;
                range.end[j] = i[j] + dims[j] / 2 + 1;
                range.stride[j] = 1;
            }
            return rangeReduce_s(
                    range, [](auto&& a, auto&& b) { return a + b; },
                    [&](auto&& idx) {
                        auto _ker_idx = idx;
                        for (auto j = 0; j < d; ++j) { _ker_idx[j] = _ker_idx[j] - i[j] + dims[j] / 2; }
                        return kernel.val[_ker_idx] * e.evalSafeAt(idx);
                    });
        }

        template <CartAMRFieldExprType E, typename T, typename I>
        requires(internal::ExprTrait<E>::dim == d) OPFLOW_STRONG_INLINE
                static auto eval_safe(const E& e, const ScalarExpr<DS::FixedSizeTensor<T, ns...>>& kernel,
                                      I&& i) {
            DS::LevelRange<d> range;
            for (auto j = 0; j < d; ++j) {
                range.start[j] = i[j] - dims[j] / 2;
                range.end[j] = i[j] + dims[j] / 2 + 1;
                range.stride[j] = 1;
            }
            range.level = i.l;
            range.part = i.p;
            return rangeReduce_s(
                    range, [](auto&& a, auto&& b) { return a + b; },
                    [&](auto&& idx) {
                        auto _ker_idx = (DS::MDIndex<d>) idx;
                        for (auto j = 0; j < d; ++j) { _ker_idx[j] = _ker_idx[j] - i[j] + dims[j] / 2; }
                        return kernel.val[_ker_idx] * e.evalSafeAt(idx);
                    });
        }

        template <StructuredFieldExprType E, typename T>
        requires(internal::ExprTrait<E>::dim == d) static void prepare(
                Expression<Convolution, E, ScalarExpr<DS::FixedSizeTensor<T, ns...>>>& expr) {
            expr.initPropsFrom(expr.arg1);

            // name
            expr.name = fmt::format("Convolution<{}>({})", d, expr.arg1.name);

            // ranges
            for (auto i = 0; i < d; ++i) {
                expr.accessibleRange.start[i] += dims[i] / 2;
                expr.accessibleRange.end[i] -= dims[i] / 2;
                expr.localRange.start[i] += dims[i] / 2;
                expr.localRange.end[i] -= dims[i] / 2;
            }
            expr.assignableRange.setEmpty();
        }

        template <CartAMRFieldExprType E, typename T>
        requires(internal::ExprTrait<E>::dim == d) static void prepare(
                Expression<Convolution, E, ScalarExpr<DS::FixedSizeTensor<T, ns...>>>& expr) {
            expr.initPropsFrom(expr.arg1);

            // name
            expr.name = fmt::format("Convolution<{}>({})", d, expr.arg1.name);

            // bc
            for (auto i = 0; i < d; ++i) {
                expr.bc[i].start = nullptr;
                expr.bc[i].end = nullptr;
            }

            // ranges
            for (auto& l : expr.accessibleRanges)
                for (auto& r : l)
                    for (auto i = 0; i < d; ++i) {
                        r.start[i] += dims[i] / 2;
                        r.end[i] -= dims[i] / 2;
                    }
            for (auto& l : expr.localRanges)
                for (auto& r : l)
                    for (auto i = 0; i < d; ++i) {
                        r.start[i] += dims[i] / 2;
                        r.end[i] -= dims[i] / 2;
                    }
            for (auto& l : expr.assignableRanges)
                for (auto& r : l) r.setEmpty();
        }
    };

    template <std::integral auto... ns, CartesianFieldExprType T, typename D>
    struct ResultType<Convolution<ns...>, T, ScalarExpr<DS::FixedSizeTensor<D, ns...>>> {
        using type = CartesianFieldExpr<
                Expression<Convolution<ns...>, T, ScalarExpr<DS::FixedSizeTensor<D, ns...>>>>;
        using core_type = Expression<Convolution<ns...>, T, ScalarExpr<DS::FixedSizeTensor<D, ns...>>>;
    };

    template <std::integral auto... ns, CartAMRFieldExprType T, typename D>
    struct ResultType<Convolution<ns...>, T, ScalarExpr<DS::FixedSizeTensor<D, ns...>>> {
        using type = CartAMRFieldExpr<
                Expression<Convolution<ns...>, T, ScalarExpr<DS::FixedSizeTensor<D, ns...>>>>;
        using core_type = Expression<Convolution<ns...>, T, ScalarExpr<DS::FixedSizeTensor<D, ns...>>>;
    };

    namespace internal {
        template <std::integral auto... ns, CartesianFieldExprType T, typename D>
        struct ExprTrait<Expression<Convolution<ns...>, T, ScalarExpr<DS::FixedSizeTensor<D, ns...>>>>
            : ExprTrait<T> {
            static constexpr int access_flag = 0;
        };

        template <std::integral auto... ns, CartAMRFieldExprType T, typename D>
        struct ExprTrait<Expression<Convolution<ns...>, T, ScalarExpr<DS::FixedSizeTensor<D, ns...>>>>
            : ExprTrait<T> {
            static constexpr int access_flag = 0;
        };
    }// namespace internal

    template <std::integral auto... ns, typename E, typename D>
            requires StructuredFieldExprType<E> || CartAMRFieldExprType<E> auto conv(E&& expr, const DS::FixedSizeTensor<D, ns...>& kernel) {
        static_assert(((ns % 2 == 1) && ...), "Only odd sized conv kernel is allowed.");
        return makeExpression<Convolution<ns...>>(expr, ScalarExpr(kernel));
    }
}// namespace OpFlow
#endif//OPFLOW_CONVOLUTION_HPP
