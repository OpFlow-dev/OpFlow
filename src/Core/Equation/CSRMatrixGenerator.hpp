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

#ifndef OPFLOW_CSRMATRIXGENERATOR_HPP
#define OPFLOW_CSRMATRIXGENERATOR_HPP

#include "Core/Equation/StencilHolder.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Index/LinearMapper/MDRangeMapper.hpp"
#include "DataStructures/Matrix/CSRMatrix.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <vector>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {
    struct CSRMatrixGenerator {
        template <typename S, typename M>
        static auto generate(S& s, M&& mapper, const std::vector<bool>& pin_flags) {
            DS::CSRMatrix csr;

            if (getGlobalParallelPlan().shared_memory_workers_count == 1)
                Meta::static_for<S::size>([&]<int i>(Meta::int_<i>) {
                    DS::CSRMatrix m = generate_s<i>(s, mapper, pin_flags[i]);
                    csr.append_s(m);
                });
            else
                Meta::static_for<S::size>([&]<int i>(Meta::int_<i>) {
                    DS::CSRMatrix m = generate<i>(s, mapper, pin_flags[i]);
                    csr.append(m);
                });

            return csr;
        }

        template <typename S, typename M>
        static void generate_rhs(S& s, M&& mapper, const std::vector<bool>& pin_flags,
                                 DS::CSRMatrix& origin) {
            int from = 0;
            Meta::static_for<S::size>([&]<int i>(Meta::int_<i>) {
                std::vector<Real> rhs = generate_rhs<i>(s, mapper, pin_flags[i]);
                origin.update_rhs(from, rhs);
                from += (int) rhs.size();
            });
        }

        template <std::size_t iTarget, typename S>
        static auto generate(S& s, auto&& mapper, bool pinValue) {
            DS::CSRMatrix mat;
            auto target = s.template getTargetPtr<iTarget>();
            auto commStencil = s.comm_stencils[iTarget];
            auto& uniEqn = s.template getEqnExpr<iTarget>();
            auto local_range = target->getLocalWritableRange();
            // shortcut for empty range case
            if (local_range.empty()) return mat;
            DS::MDRangeMapper local_mapper(local_range);
            // prepare: evaluate the common stencil & pre-fill the arrays
            int stencil_size = commStencil.pad.size() * 1.5;

            struct m_tuple {
                int r, c;
                Real v;
                bool operator<(const m_tuple& other) const {
                    return r < other.r || (r == other.r && c < other.c);
                }
            };
            DS::DenseVector<m_tuple> coo;
            coo.resize(local_range.count() * stencil_size);
            mat.resize(local_range.count(), stencil_size);
            auto r_last = mapper(target->getGlobalWritableRange().last(), iTarget);
            rangeFor(local_range, [&](auto&& i) {
                auto r = mapper(i, iTarget);   // r is the rank of i in the target scope
                auto r_local = local_mapper(i);// r_local is the rank of i in the block scope
                auto currentStencil = uniEqn.evalAt(i);
                int count = 0;
                if (pinValue && r == r_last) {
                    coo[r_local * stencil_size] = m_tuple {
                            r_last,
                            mapper(DS::ColoredIndex<typename decltype(local_range)::base_index_type> {
                                    i, iTarget}),
                            1};
                    mat.rhs[r_local] = 0.;
                    count++;
                } else {
                    for (const auto& [key, v] : currentStencil.pad) {
                        auto idx = mapper(key);
                        OP_ASSERT_MSG(!std::isnan(v),
                                      "CSRMatrixGenerator: {}'s stencil pad at {} of {}'s value is nan",
                                      target->getName(), i, key);
                        coo[r_local * stencil_size + count++] = m_tuple {r, idx, v};
                    }
                    mat.rhs[r_local] = -currentStencil.bias;
                }
                for (; count < stencil_size; ++count)
                    coo[r_local * stencil_size + count]
                            = m_tuple {std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), 0.};
            });
            tbb::global_control globalControl(tbb::detail::d1::global_control::max_allowed_parallelism,
                                              getGlobalParallelPlan().shared_memory_workers_count);
            oneapi::tbb::parallel_sort(coo.begin(), coo.end());
            auto iter = std::lower_bound(
                    coo.begin(), coo.end(),
                    m_tuple {std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), 0.});
            coo.resize(iter - coo.begin());
            std::vector<std::atomic_int> nnz_counts(local_range.count());
            int common_base = coo.front().r;
            oneapi::tbb::parallel_for_each(coo.begin(), coo.end(),
                                           [&](const m_tuple& t) { nnz_counts[t.r - common_base]++; });
            DS::DenseVector<int> nnz_prefix(local_range.count() + 1);
            nnz_prefix[0] = 0;
            oneapi::tbb::parallel_scan(
                    oneapi::tbb::blocked_range<int>(0, nnz_counts.size()), 0,
                    [&](const oneapi::tbb::blocked_range<int>& r, int sum, bool is_final) {
                        int temp = sum;
                        for (int i = r.begin(); i < r.end(); ++i) {
                            temp += nnz_counts[i];
                            if (is_final) nnz_prefix[i + 1] = temp;
                        }
                        return temp;
                    },
                    [](int l, int r) { return l + r; });
            // copy to the global array
            oneapi::tbb::parallel_for(0, local_range.count(), [&](int i) { mat.row[i] = nnz_prefix[i]; });
            mat.row.back() = coo.size();
            oneapi::tbb::parallel_for(0, (int) coo.size(), [&](int i) { mat.col[i] = coo[i].c; });
            oneapi::tbb::parallel_for(0, (int) coo.size(), [&](int i) { mat.val[i] = coo[i].v; });
            mat.trim(coo.size());

            return mat;
        }

        template <std::size_t iTarget, typename S>
        static auto generate_s(S& s, auto&& mapper, bool pinValue) {
            DS::CSRMatrix mat;
            auto target = s.template getTargetPtr<iTarget>();
            auto commStencil = s.comm_stencils[iTarget];
            auto& uniEqn = s.template getEqnExpr<iTarget>();
            auto local_range = target->getLocalWritableRange();
            // shortcut for empty range case
            if (local_range.empty()) return mat;
            DS::MDRangeMapper local_mapper(local_range);
            // prepare: evaluate the common stencil & pre-fill the arrays
            int stencil_size = commStencil.pad.size() * 1.5;

            std::vector<ptrdiff_t> row, col;
            std::vector<Real> val;
            row.push_back(0);

            auto r_last = mapper(target->getGlobalWritableRange().last(), iTarget);
            rangeFor_s(local_range, [&](auto&& i) {
                auto r = mapper(i, iTarget);   // r is the rank of i in the target scope
                auto r_local = local_mapper(i);// r_local is the rank of i in the block scope
                auto currentStencil = uniEqn.evalAt(i);
                if (pinValue && r == r_last) {
                    row.push_back(row.back() + 1);
                    col.push_back(mapper(
                            DS::ColoredIndex<typename decltype(local_range)::base_index_type> {i, iTarget}));
                    val.push_back(1.);
                    mat.rhs.push_back(0);
                } else {
                    row.push_back(currentStencil.pad.size() + row.back());
                    // presort pad
                    std::vector<std::pair<int, Real>> pad;
                    for (const auto& [key, v] : currentStencil.pad) {
                        auto idx = mapper(key);
                        OP_ASSERT_MSG(!std::isnan(v),
                                      "CSRMatrixGenerator: {}'s stencil pad at {} of {}'s value is nan",
                                      target->getName(), i, key);
                        pad.push_back({idx, v});
                    }
                    std::sort(pad.begin(), pad.end(),
                              [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
                    for (const auto& [key, v] : pad) {
                        col.push_back(key);
                        val.push_back(v);
                    }
                    mat.rhs.push_back(-currentStencil.bias);
                }
            });
            mat.row.resize(row.size());
            mat.col.resize(col.size());
            mat.val.resize(val.size());
            std::copy(row.begin(), row.end(), mat.row.begin());
            std::copy(col.begin(), col.end(), mat.col.begin());
            std::copy(val.begin(), val.end(), mat.val.begin());

            return mat;
        }

        template <std::size_t iTarget, typename S>
        static auto generate_rhs(S& s, auto&& mapper, bool pinValue) {
            auto target = s.template getTargetPtr<iTarget>();
            auto commStencil = s.comm_stencils[iTarget];
            auto& uniEqn = s.template getEqnExpr<iTarget>();
            auto local_range = target->getLocalWritableRange();
            DS::MDRangeMapper local_mapper(local_range);

            std::vector<Real> rhs(local_range.count());
            auto r_last = mapper(local_range.last(), iTarget);
            rangeFor(local_range, [&](auto&& i) {
                auto r = mapper(i, iTarget);   // r is the rank of i in the target scope
                auto r_local = local_mapper(i);// r_local is the rank of i in the block scope
                auto currentStencil = uniEqn.evalAt(i);
                if (pinValue && r == r_last) {
                    rhs[r_local] = 0.;
                } else {
                    rhs[r_local] = -currentStencil.bias;
                }
            });

            return rhs;
        }
    };
}// namespace OpFlow

#endif//OPFLOW_CSRMATRIXGENERATOR_HPP
