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

#ifndef OPFLOW_RANGEFOR_HPP
#define OPFLOW_RANGEFOR_HPP

#include "Core/Environment.hpp"
#include "Core/Expr/Expr.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Index/RangedIndex.hpp"
#include "DataStructures/Range/Ranges.hpp"
#include <omp.h>
#include <tbb/tbb.h>
#define TBB_PREVIEW_BLOCKED_RANGE_ND 1
#include <oneapi/tbb/blocked_rangeNd.h>
#include <oneapi/tbb/detail/_range_common.h>

namespace OpFlow {
    /// \brief Serial version of range for
    /// \tparam dim Range dim
    /// \tparam F Functor type
    /// \param range Looped range
    /// \param func Applied functor
    /// \return The input functor
    template <typename R, typename F>
    F rangeFor_s(const R& range, F&& func) {
        typename R::index_type i(range);
        auto total_count = range.count();
        for (auto count = 0; count < total_count; ++count, ++i) {
            func(static_cast<typename R::base_index_type&>(i));
        }
        return std::forward<F>(func);
    }

    template <typename R, typename ReOp, typename F>
    auto rangeReduce_s(const R& range, ReOp&& op, F&& func) {
        //OP_INFO("Called on {}, size = {}", range.toString(), range.count());
        typename R::index_type idx(range);
        auto result = func(static_cast<typename R::base_index_type&>(idx));
        ++idx;
        auto total_count = range.count();
        for (auto count = 1; count < total_count; ++count, ++idx) {
            result = op(result, func(static_cast<typename R::base_index_type&>(idx)));
        }
        return result;
    }

    /// \brief Parallel version of range for
    /// \tparam dim Range dim
    /// \tparam F Functor type
    /// \param range Looped range
    /// \param func Applied functor
    /// \return The input functor
    template <typename R, typename F>
    F rangeFor(const R& range, F&& func) {
        constexpr static auto dim = R::dim;
        auto total_count = range.count();
        auto line_size = range.end[0] - range.start[0];
        if (line_size <= 0) return std::forward<F>(func);
        if (range.stride[0] == 1) {
            if constexpr (dim == 1) {
#pragma omp parallel for
                for (auto i = range.start[0]; i < range.end[0]; ++i) { func(typename R::base_index_type(i)); }
            } else if constexpr (dim == 2) {
                constexpr static int block_x = 8, block_y = 8;
#pragma omp parallel for schedule(dynamic)
#pragma omp tile sizes(block_y, block_x)
                for (int j = range.start[1]; j < range.end[1]; ++j)
                    for (int i = range.start[0]; i < range.end[0]; ++i) {
                        typename R::index_type idx(range);
                        idx[0] = i;
                        idx[1] = j;
                        func(static_cast<typename R::base_index_type&>(idx));
                    }
            } else if constexpr (dim == 3) {
                constexpr static int block_x = 8, block_y = 4, block_z = 4;
#pragma omp parallel for schedule(dynamic)
#pragma omp tile sizes(block_z, block_y, block_x)
                for (int k = range.start[2]; k < range.end[2]; ++k)
                    for (int j = range.start[1]; j < range.end[1]; ++j)
                        for (int i = range.start[0]; i < range.end[0]; ++i) {
                            typename R::index_type idx(range);
                            idx[0] = i;
                            idx[1] = j;
                            idx[2] = k;
                            func(static_cast<typename R::base_index_type&>(idx));
                        }
            } else {
#ifdef OPFLOW_WITH_OPENMP
                tbb::task_arena arena(getGlobalParallelPlan().shared_memory_workers_count);
#else
                tbb::task_arena arena(1);
#endif
                arena.template execute([&]() {
                    tbb::parallel_for(range, [&](const R& r) { rangeFor_s(r, OP_PERFECT_FOWD(func)); });
                });
            }
        } else {
            OP_NOT_IMPLEMENTED;
            std::abort();
        }
        return std::forward<F>(func);
    }

    template <typename R, typename ReOp, typename F>
    auto rangeReduce(const R& range, ReOp&& op, F&& func) {
        using resultType = Meta::RealType<decltype(func(std::declval<typename R::base_index_type&>()))>;
        constexpr static auto dim = R::dim;
        auto line_size = range.end[0] - range.start[0];
        if (line_size <= 0) return resultType();

        struct Reducer {
            resultType result {};
            const ReOp& _op;
            const F& _func;
            void operator()(const R& _range) { result = _op(result, rangeReduce_s(_range, _op, _func)); }

            Reducer(Reducer& _reducer, tbb::detail::split)
                : _op(_reducer._op), _func(_reducer._func), result() {
                if constexpr (Meta::Numerical<resultType>) result = 0;
            }

            void join(const Reducer& _reducer) { result = _op(result, _reducer.result); }

            Reducer(const ReOp& _op, const F& _func) : _op(_op), _func(_func), result() {
                // make sure for numerical type the reduction base is 0
                if constexpr (Meta::Numerical<resultType>) result = 0;
                // otherwise, we assume the default ctor of resultType provides an adequate base
            }
        } reducer {op, func};

        if (range.stride[0] == 1) {
            tbb::task_arena arena(getGlobalParallelPlan().shared_memory_workers_count);
            arena.template execute([&]() { tbb::parallel_reduce(range, reducer); });
            return reducer.result;
        } else {
            OP_NOT_IMPLEMENTED;
            std::abort();
        }
    }
}// namespace OpFlow
#endif//OPFLOW_RANGEFOR_HPP
