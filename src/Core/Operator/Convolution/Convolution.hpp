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
    template <auto kernel>
    requires DS::FixedSizeTensorType<decltype(kernel)> struct Convolution {
        constexpr static int d = DS::internal::TensorTrait<decltype(kernel)>::dim;
        constexpr static auto bc_width = kernel.max_half_width();

        template <StructuredFieldExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const E& e, I&& i) {
            auto ret = true;
            for (auto j = 0; j < d; ++j) {
                ret &= (i[j] - kernel.size_of(j) / 2 >= e.arg1.accessibleRange.start[j])
                       && (i[j] + kernel.size_of(j) / 2 < e.arg1.accessibleRange.end[j]);
            }
            return ret;
        }

        template <StructuredFieldExprType E, typename I>
        requires(internal::ExprTrait<E>::dim == d) OPFLOW_STRONG_INLINE static auto eval(const E& e, I&& i) {
            constexpr auto sizes = kernel.size();
            DS::Range<d> range;
            for (auto j = 0; j < d; ++j) {
                range.start[j] = i[j] - sizes[j] / 2;
                range.end[j] = i[j] + sizes[j] / 2;
                range.stride[j] = 1;
            }
            return rangeReduce_s(
                    range, [](auto&& a, auto&& b) { return a + b; },
                    [&](auto&& idx) {
                        auto _ker_idx = idx;
                        for (auto j = 0; j < d; ++j) { _ker_idx[j] = _ker_idx[j] - i[j] + sizes[j] / 2; }
                        return kernel[_ker_idx] * e.evalAt(idx);
                    });
        }

        template <StructuredFieldExprType E, typename I>
        requires(internal::ExprTrait<E>::dim == d) OPFLOW_STRONG_INLINE
                static auto eval_safe(const E& e, I&& i) {
            constexpr auto sizes = kernel.size();
            DS::Range<d> range;
            for (auto j = 0; j < d; ++j) {
                range.start[j] = i[j] - sizes[j] / 2;
                range.end[j] = i[j] + sizes[j] / 2;
                range.stride[j] = 1;
            }
            return rangeReduce_s(
                    range, [](auto&& a, auto&& b) { return a + b; },
                    [&](auto&& idx) {
                        auto _ker_idx = idx;
                        for (auto j = 0; j < d; ++j) { _ker_idx[j] = _ker_idx[j] - i[j] + sizes[j] / 2; }
                        return kernel[_ker_idx] * e.evalSafeAt(idx);
                    });
        }

        template <StructuredFieldExprType E>
        requires(internal::ExprTrait<E>::dim == d) static void prepare(Expression<Convolution, E>& expr) {
            expr.initPropsFrom(expr.arg1);

            // name
            expr.name = fmt::format("Convolution<{}>({})", d, expr.arg1.name);

            // bc
            for (auto i = 0; i < d; ++i) {
                expr.bc[i].start = nullptr;
                expr.bc[i].end = nullptr;
            }

            // ranges
            for (auto i = 0; i < d; ++i) {
                expr.accessibleRange.start[i] += kernel.size_of(i) / 2;
                expr.accessibleRange.end[i] -= kernel.size_of(i) / 2;
                expr.localRange.start[i] += kernel.size_of(i) / 2;
                expr.localRange.end[i] -= kernel.size_of(i) / 2;
            }
            expr.assignableRange.setEmpty();
        }
    };

    template <auto kernel, CartesianFieldExprType T>
    requires DS::FixedSizeTensorType<decltype(kernel)> struct ResultType<Convolution<kernel>, T> {
        using type = CartesianFieldExpr<Expression<Convolution<kernel>, T>>;
        using core_type = Expression<Convolution<kernel>, T>;
    };

    template <auto kernel, CartAMRFieldExprType T>
    requires DS::FixedSizeTensorType<decltype(kernel)> struct ResultType<Convolution<kernel>, T> {
        using type = CartAMRFieldExpr<Expression<Convolution<kernel>, T>>;
        using core_type = Expression<Convolution<kernel>, T>;
    };

    namespace internal {
        template <auto kernel, CartesianFieldExprType T>
        struct ExprTrait<Expression<Convolution<kernel>, T>> : ExprTrait<T> {
            static constexpr int access_flag = 0;
        };

        template <auto kernel, CartAMRFieldExprType T>
        struct ExprTrait<Expression<Convolution<kernel>, T>> : ExprTrait<T> {
            static constexpr int access_flag = 0;
        };
    }// namespace internal

    template <typename Kernel, StructuredFieldExprType E>
    auto conv(const E& expr) {
        return makeExpression<Kernel>(expr);
    }
}// namespace OpFlow
#endif//OPFLOW_CONVOLUTION_HPP
