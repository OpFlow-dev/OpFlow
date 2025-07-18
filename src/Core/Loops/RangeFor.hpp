// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2025 by the OpFlow developers
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
#ifndef OPFLOW_INSIDE_MODULE
#include <omp.h>
#include <tbb/tbb.h>
#endif
#define TBB_PREVIEW_BLOCKED_RANGE_ND 1
#ifndef OPFLOW_INSIDE_MODULE
#include <oneapi/tbb/blocked_rangeNd.h>
#include <oneapi/tbb/detail/_range_common.h>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {
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
            tbb::task_arena arena(getGlobalParallelPlan().shared_memory_workers_count);
            arena.execute([&]() {
                tbb::parallel_for(range, [&](const R& r) { rangeFor_s(r, OP_PERFECT_FOWD(func)); });
            });
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

#ifdef OPFLOW_WITH_MPI
    template <typename R, typename ReOp, typename F>
    auto globalReduce(const R& range, ReOp&& op, F&& func) {
        auto local_result = rangeReduce(range, op, func);
        std::vector<decltype(local_result)> results(getWorkerCount());
        results[getWorkerId()] = local_result;
        static_assert(std::is_standard_layout_v<decltype(
                                      local_result)> && std::is_trivial_v<decltype(local_result)>,
                      "local_result must be pod type");
        MPI_Allgather(MPI_IN_PLACE, sizeof(decltype(local_result)), MPI_BYTE, results.data(),
                      sizeof(decltype(local_result)), MPI_BYTE, MPI_COMM_WORLD);
        return std::reduce(results.begin(), results.end(), decltype(local_result) {}, op);
    }
#endif
}// namespace OpFlow
#endif//OPFLOW_RANGEFOR_HPP
