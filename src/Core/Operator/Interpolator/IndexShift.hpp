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

#ifndef OPFLOW_INDEXSHIFT_HPP
#define OPFLOW_INDEXSHIFT_HPP

#include "Core/Expr/Expr.hpp"
#include "fmt/format.h"
#include "fmt/ranges.h"
#include <array>

namespace OpFlow {
    template <std::size_t n, std::array<int, n> direction>
    struct IndexShifter {
        constexpr static auto dir = direction;

        constexpr static auto bc_width = 0;

        template <StructuredFieldExprType T, typename I>
        static OPFLOW_STRONG_INLINE auto eval_safe(const T& t, I i) {
            return t.evalSafeAt(i - I(dir));
        }

        template <StructuredFieldExprType T, typename I>
        static OPFLOW_STRONG_INLINE auto eval(const T& t, I i) {
            return t.evalAt(i - I(dir));
        }

        template <StructuredFieldExprType T>
        static inline void prepare(Expression<IndexShifter, T>& expr) {
            constexpr auto dim = internal::StructuredFieldExprTrait<T>::dim;
            static_assert(dim == n, "Dim of expr & shift vector mismatch");
            expr.name = fmt::format("IShift<{}>({})", dir, expr.arg1.name);
            expr.mesh = typename internal::StructuredFieldExprTrait<T>::mesh_type::Builder()
                                .newMesh(expr.arg1.mesh)// todo: constrain the prototype of structural mesh
                                .appendOffset(dir)
                                .build();
            using index_type = typename internal::StructuredFieldExprTrait<T>::index_type;
            for (auto i = 0; i < dim; ++i) {
                expr.bc[i].start = expr.arg1.bc[i].start->getCopy();
                expr.bc[i].start->appendOffset(index_type(dir));
                expr.bc[i].end = expr.arg1.bc[i].end->getCopy();
                expr.bc[i].end->appendOffset(index_type(dir));
            }
            expr.accessibleRange = expr.arg1.accessibleRange;
            expr.assignableRange.setEmpty();
            expr.localRange = expr.arg1.localRange;
            for (auto i = 0; i < dim; ++i) {
                expr.accessibleRange.start[i] += dir[i];
                expr.accessibleRange.end[i] += dir[i];
                expr.localRange.start[i] += dir[i];
                expr.localRange.end[i] += dir[i];
            }
        }
    };

    template <std::size_t n, std::array<int, n> direction, StructuredFieldExprType T>
    struct ResultType<IndexShifter<n, direction>, T> {
        using type = typename internal::StructuredFieldExprTrait<T>::template twin_type<
                Expression<IndexShifter<n, direction>, T>>;
    };

    namespace internal {
        template <std::size_t n, std::array<int, n> direction, StructuredFieldExprType T>
        struct ExprTrait<Expression<IndexShifter<n, direction>, T>> {
            static constexpr int dim = StructuredFieldExprTrait<T>::dim;
            static constexpr int bc_width = StructuredFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using type = typename StructuredFieldExprTrait<T>::type;
            template <typename Other>
            using other_type = typename StructuredFieldExprTrait<T>::template other_type<Other>;
            template <typename Other>
            using twin_type = typename StructuredFieldExprTrait<T>::template twin_type<Other>;
            using elem_type = typename StructuredFieldExprTrait<T>::elem_type;
            using mesh_type = typename StructuredFieldExprTrait<T>::mesh_type;
            using range_type = typename StructuredFieldExprTrait<T>::range_type;
            using index_type = typename StructuredFieldExprTrait<T>::index_type;
            using patch_field_type = typename StructuredFieldExprTrait<T>::patch_field_type;
        };
    }// namespace internal

    template <std::size_t n, std::array<int, n> dir>
    struct ArgChecker<IndexShifter<n, dir>> {
        template <CartesianFieldExprType T>
        static OPFLOW_STRONG_INLINE void check(const T& t) {
            return;
        }
    };

    template <std::size_t d, int off>
    struct IndexShifter1D {

        constexpr static auto bc_width = 0;

        template <StructuredFieldExprType T>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const T& t, auto&& i) {
            return t.accessibleRange.start[d] <= i[d] - off && i[d] - off < t.accessibleRange.end[d];
        }

        template <StructuredFieldExprType T, typename I>
        static OPFLOW_STRONG_INLINE auto eval_safe(const T& t, I i) {
            constexpr auto dim = internal::StructuredFieldExprTrait<T>::dim;
            constexpr auto offset = getOffsetArray<dim>();
            return t.evalSafeAt(i - I(offset));
        }

        template <StructuredFieldExprType T, typename I>
        static OPFLOW_STRONG_INLINE auto eval(const T& t, I i) {
            constexpr auto dim = internal::StructuredFieldExprTrait<T>::dim;
            constexpr auto offset = getOffsetArray<dim>();
            return t.evalAt(i - I(offset));
        }

        template <StructuredFieldExprType T>
        static OPFLOW_STRONG_INLINE void prepare(Expression<IndexShifter1D, T>& expr) {
            using index_type = typename internal::StructuredFieldExprTrait<T>::index_type;
            constexpr auto dim = internal::StructuredFieldExprTrait<T>::dim;
            constexpr auto dir = getOffsetArray<dim>();
            expr.initPropsFrom(expr.arg1);
            expr.name = fmt::format("IShift<{}>({})", dir, expr.arg1.name);
            expr.mesh.appendOffset(index_type(dir));
            for (auto i = 0; i < dim; ++i) {
                if (expr.bc[i].start) expr.bc[i].start->appendOffset(index_type(dir));
                if (expr.bc[i].end) expr.bc[i].end->appendOffset(index_type(dir));
            }
            expr.assignableRange.setEmpty();
            for (auto i = 0; i < dim; ++i) {
                expr.accessibleRange.start[i] += dir[i];
                expr.accessibleRange.end[i] += dir[i];
                expr.localRange.start[i] += dir[i];
                expr.localRange.end[i] += dir[i];
            }
        }

    private:
        template <std::size_t dim>
        constexpr static auto getOffsetArray() {
            std::array<int, dim> ret;
            for (auto i = 0; i < dim; ++i) ret[i] = 0;
            ret[d] = off;
            return ret;
        }
    };

    template <std::size_t d, int off, StructuredFieldExprType T>
    struct ResultType<IndexShifter1D<d, off>, T> {
        using type = typename internal::StructuredFieldExprTrait<T>::template twin_type<
                Expression<IndexShifter1D<d, off>, T>>;
        using core_type = Expression<IndexShifter1D<d, off>, T>;
    };

    namespace internal {
        template <std::size_t d, int off, StructuredFieldExprType T>
        struct ExprTrait<Expression<IndexShifter1D<d, off>, T>> {
            static constexpr int dim = StructuredFieldExprTrait<T>::dim;
            static constexpr int bc_width = StructuredFieldExprTrait<T>::bc_width;
            static constexpr int access_flag = 0;
            using type = typename StructuredFieldExprTrait<T>::type;
            template <typename Other>
            using other_type = typename StructuredFieldExprTrait<T>::template other_type<Other>;
            template <typename Other>
            using twin_type = typename StructuredFieldExprTrait<T>::template twin_type<Other>;
            using elem_type = typename StructuredFieldExprTrait<T>::elem_type;
            using mesh_type
                    = decltype(std::declval<typename StructuredFieldExprTrait<T>::mesh_type>().getView());
            using range_type = typename StructuredFieldExprTrait<T>::range_type;
            using index_type = typename StructuredFieldExprTrait<T>::index_type;
        };
    }// namespace internal

    template <std::size_t d, int off>
    struct ArgChecker<IndexShifter1D<d, off>> {
        template <CartesianFieldExprType T>
        static OPFLOW_STRONG_INLINE void check(const T& t) {
            return;
        }
    };

    template <auto... dir, FieldExprType E>
    auto indexShift(E&& e) {
        constexpr auto _dir = std::array {dir...};
        constexpr auto n = _dir.size();
        return makeExpression<IndexShifter<n, _dir>>(e);
    }

    template <std::size_t d, int off, FieldExprType E>
    auto indexShift1d(E&& e) {
        return makeExpression<IndexShifter1D<d, off>>(e);
    }
}// namespace OpFlow
#endif//OPFLOW_INDEXSHIFT_HPP
