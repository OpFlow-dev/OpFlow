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

#ifndef OPFLOW_RANGEFOR_HPP
#define OPFLOW_RANGEFOR_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Index/RangedIndex.hpp"
#include "DataStructures/Range/Ranges.hpp"
#include <omp.h>

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
            } else {
#ifdef OPFLOW_WITH_OPENMP
                auto numCore = omp_get_max_threads();
#else
                auto numCore = 1;
#endif
                auto workSize = total_count / numCore / line_size * line_size;
#pragma omp parallel
                {
                    auto thread_id = omp_get_thread_num();
                    typename R::index_type idx(range);
                    idx += workSize * thread_id;
                    auto idx0 = idx;
                    for (auto i = 0; i < workSize; i += line_size) {
                        for (auto j = 0; j < line_size; ++j) {
                            idx[0] = idx0[0] + j;
                            func(static_cast<typename R::base_index_type&>(idx));
                        }
                        idx++;
                    }
                };
                if (total_count > workSize * numCore) {
                    typename R::index_type idx(range);
                    idx += workSize * numCore;
                    auto rest = total_count - workSize * numCore;
                    auto idx0 = idx;
                    for (auto i = 0; i < rest; i += line_size) {
                        for (auto j = 0; j < line_size; ++j) {
                            idx[0] = idx0[0] + j;
                            func((static_cast<typename R::base_index_type&>(idx)));
                        }
                        idx++;
                    }
                }
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
        auto total_count = range.count();
        auto line_size = range.end[0] - range.start[0];
        if (line_size <= 0) return resultType();
        if (range.stride[0] == 1) {
            if constexpr (dim == 1) {
                auto numCore = omp_get_max_threads();
                std::vector<resultType> result(numCore);
#pragma omp parallel
                {
                    auto thread_id = omp_get_thread_num();
                    auto& tmp = result[thread_id];
                    auto workSize = total_count / numCore;
                    if (workSize * numCore < total_count) workSize++;
                    auto start = range.start[0] + workSize * thread_id;
                    auto end = std::min(start + workSize, range.end[0]);
                    tmp = func(typename R::base_index_type(start));
                    for (auto i = start + 1; i < end; ++i) {
                        tmp = op(tmp, func(typename R::base_index_type(i)));
                    }
                };
                for (auto i = 1; i < numCore; ++i) { result[0] = op(result[0], result[i]); }
                return result[0];
            } else {
                auto numCore = omp_get_max_threads();
                std::vector<resultType> result(numCore);
                auto workSize = total_count / numCore / line_size * line_size;
                if (workSize == 0) {
                    auto line_count = total_count / line_size;
                    omp_set_num_threads(line_count);
                    workSize = line_size;
                }
#pragma omp parallel
                {
                    auto thread_id = omp_get_thread_num();
                    typename R::index_type idx(range);
                    idx += workSize * thread_id;
                    auto idx0 = idx;
                    result[thread_id] = func(static_cast<typename R::base_index_type&>(idx));
                    for (auto j = 1; j < line_size; ++j) {
                        idx[0] = idx0[0] + j;
                        result[thread_id]
                                = op(result[thread_id], func(static_cast<typename R::base_index_type&>(idx)));
                    }
                    idx++;
                    for (auto i = line_size; i < workSize; i += line_size) {
                        for (auto j = 0; j < line_size; ++j) {
                            idx[0] = idx0[0] + j;
                            result[thread_id] = op(result[thread_id],
                                                   func(static_cast<typename R::base_index_type&>(idx)));
                        }
                        idx++;
                    }
                };
                omp_set_num_threads(numCore);
                if (total_count > workSize * numCore) {
                    typename R::index_type idx(range);
                    idx += workSize * numCore;
                    auto rest = total_count - workSize * numCore;
                    auto idx0 = idx;
                    for (auto i = 0; i < rest; i += line_size) {
                        for (auto j = 0; j < line_size; ++j) {
                            idx[0] = idx0[0] + j;
                            result[0] = op(result[0], func(static_cast<typename R::base_index_type&>(idx)));
                        }
                        idx++;
                    }
                }
                for (auto i = 1; i < numCore; ++i) { result[0] = op(result[0], result[i]); }
                return result[0];
            }
        } else {
            OP_NOT_IMPLEMENTED;
            std::abort();
        }
        return resultType();
    }
}// namespace OpFlow
#endif//OPFLOW_RANGEFOR_HPP
